from nptdms import TdmsFile
import numpy as np
import pandas as pd
from dataclasses import dataclass
from pathlib import Path
from datetime import datetime
import warnings

"FED3 event -> the BNC event logged for the same action (both lowercase)."
"Only exact counterparts belong here. Every FED3 event missing from this map (PeriodStart,"
"PeriodEnd, Redraw, RightEarly, ...) is a special event and pairs with BNC_SPECIAL_EVENT."
"BNC-only events (TONE, LED, Dispensed, water) are ignored."
"BNC pulse counts: Right 1, Left 2, Retrieved 4, RightWithPellet 11, LeftWithPellet 12,"
"RightDuringDispense 13, LeftDuringDispense 14, RightinTimeOut 15, LeftinTimeOut 16,"
"SpecialEvent 20. RightShort/LeftShort pulse as Right/Left before FED3 relabels them."
FED3_TO_BNC = {
    'right': 'right',
    'rightshort': 'right',
    'left': 'left',
    'leftshort': 'left',
    'pellet': 'retrieved',
    'rightwithpellet': 'rightwithpellet',
    'leftwithpellet': 'leftwithpellet',
    'rightduringdispense': 'rightduringdispense',
    'leftduringdispense': 'leftduringdispense',
    'rightintimeout': 'rightintimeout',
    'leftintimeout': 'leftintimeout',
}

"BNC event (20 pulses) sent for every FED3 event outside FED3_TO_BNC."
BNC_SPECIAL_EVENT = 'specialevent'

FED3_EVENT_COL = 'Event'
FED3_MS_COL = 'time_elapsed_ms'
BNC_EVENT_COL = 'event'
BNC_MS_COL = 'millis'

"How far (ms) a FED3 event may sit from its BNC partner, after removing the constant"
"clock offset, and still count as the same event."
FED3_MATCH_TOL_MS = 1000


@dataclass
class FED3Mismatch:
    """A sample in one file with no partner of the same event type in the other."""
    event: str          # event label as written in the file holding the extra sample
    group: str          # BNC event type the sample was paired under, e.g. 'SpecialEvent'
    side: str           # 'BNC' or 'FED3' — the file that is over
    ordinal: int        # 1-based position among that file's events of this group
    row: int            # dataframe index of the extra row
    time_ms: float      # time on the FED3 clock (BNC rows shifted by the clock offset)
    at_end: bool = False
    reconciled: bool = True


def read_all(NH_p, M2P_p, FED3_p=None):
    """ Documentation
    Reads pipeline files and converts them to dataframes.
    Supports .tdms and .xlsx/.xls for M2P. FED3 is optional.

    :param NH_p:
    :param M2P_p:
    :param FED3_p: Optional path to the FED3 csv. None skips FED3 entirely.
    :return: NH_df, M2P_df, FED3_df (FED3_df is None when no path is given)
    """

    NH_df = pd.read_csv(NH_p, index_col=False)
    FED3_df = pd.read_csv(FED3_p, index_col=False) if FED3_p else None

    ext = Path(M2P_p).suffix.lower()
    if ext == '.tdms':
        M2P_df = TdmsFile.read(M2P_p).as_dataframe()
        # Rename columns to sane names
        M2P_df = M2P_df.rename(columns={
            "/'Sync'/'Own Counting'": "counter",
            "/'Sync'/'Channel'": "sync_channel",
            "/'Sync'/'Time'": "timestamp",
            "/'Sync'/'Counting Channel'": "count_channel",
            "/'Sync'/'Counting'": "count_value",
            "/'Sync'/'IoType'": "edge",
        })
    elif ext in ('.xlsx', '.xls'):
        M2P_df = pd.read_excel(M2P_p, index_col=False)
    else:
        raise ValueError(f"Unsupported M2P file format: '{ext}'. Expected .tdms or .xlsx")

    return NH_df, M2P_df, FED3_df


def drop_first_row(df):
    """
    Drops the first row of a dataframe and resets the index.

    Used to discard a leading spurious pulse in the M2P/sync file.

    :param df: Dataframe to trim
    :return: Dataframe without its first row
    """

    return df.iloc[1:].reset_index(drop=True)


def resolve_output_path(synced_p, out_dir=None):
    """
    Redirects an already-built '_synced' path into an output directory.

    When out_dir is None the path is returned unchanged, so the original
    'write next to the input file' behaviour is preserved.

    :param synced_p: Path string already ending in '_synced.csv'
    :param out_dir: Directory to write into, or None to keep the original location
    :return: Final path string to write to
    """

    if out_dir is None:
        return synced_p

    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    return str(out_dir.joinpath(Path(synced_p).name))


def build_pulse_trains(sync_df, train_gap_ms=50):
    """
    Groups sync4 pulses into trains based on inter-pulse time gaps.

    :param sync_df: Sync4 dataframe with a 'timestamp' column
    :param train_gap_ms: Gap in milliseconds used to separate trains (default: 50)
    :return: DataFrame with one row per train — train_id, received_pulses, train_start
    """

    sync = sync_df.copy()
    sync['timestamp'] = pd.to_datetime(sync['timestamp'])
    sync = sync.sort_values('timestamp').reset_index(drop=True)

    sync['gap_ms'] = sync['timestamp'].diff().dt.total_seconds().fillna(0) * 1000
    sync['train_id'] = (sync['gap_ms'] > train_gap_ms).cumsum()

    train_counts = sync.groupby('train_id').size().reset_index(name='received_pulses')
    train_times = sync.groupby('train_id')['timestamp'].first().reset_index(name='train_start')
    trains = train_counts.merge(train_times, on='train_id')

    return trains


def find_missing_pulse(bnc_df, sync_df, train_gap_ms=50):
    """
    Compares BNC pulse trains to sync4 received pulses to identify any missed pulses.

    Groups sync4 pulses into trains based on inter-pulse gaps, then matches each
    train 1-to-1 with the corresponding BNC event row and reports any mismatches.

    :param bnc_df: BNC dataframe with 'pulseCount' and 'synced_time' columns
    :param sync_df: Sync4 dataframe with 'timestamp' column
    :param train_gap_ms: Gap in milliseconds used to separate pulse trains (default: 50)
    :return: None
    """

    bnc = bnc_df.dropna(subset=['pulseCount']).reset_index(drop=True)
    trains = build_pulse_trains(sync_df, train_gap_ms)

    print(f"BNC events : {len(bnc)}")
    print(f"Sync trains: {len(trains)}")
    print(f"Total pulses received: {trains['received_pulses'].sum()}")
    print()

    if len(bnc) != len(trains):
        print(f"WARNING: BNC event count ({len(bnc)}) != sync train count ({len(trains)})")
        print("Cannot do a 1-to-1 comparison — check train_gap_ms threshold.\n")

    n = min(len(bnc), len(trains))
    missed_found = False

    for i in range(n):
        expected = int(bnc.loc[i, 'pulseCount'])
        received = int(trains.loc[i, 'received_pulses'])

        if expected != received:
            print(f"MISMATCH at BNC row {i}")
            print(f"  Event       : {bnc.loc[i, 'event']}")
            print(f"  BNC time    : {bnc.loc[i, 'synced_time']}")
            print(f"  Train start : {trains.loc[i, 'train_start']}")
            print(f"  Expected    : {expected} pulses")
            print(f"  Received    : {received} pulses")
            print(f"  Missing     : {expected - received}")
            missed_found = True

    if not missed_found:
        print("No mismatch found in the", n, "events.")


def _event_keys(events):
    """
    Normalises event labels so the two files compare equal (trimmed, lowercase).

    :param events: Series of event labels
    :return: Series of normalised labels
    """

    return events.astype(str).str.strip().str.lower()


def fed3_bnc_keys(FED3_df):
    """
    Maps every FED3 row to the BNC event it pulsed as.

    Events in FED3_TO_BNC use their mapped BNC event; everything else is a special event.

    :param FED3_df: FED3 dataframe
    :return: Series of lowercase BNC event keys, indexed like FED3_df
    """

    return _event_keys(FED3_df[FED3_EVENT_COL]).map(FED3_TO_BNC).fillna(BNC_SPECIAL_EVENT)


def group_fed3_events(NH_df, FED3_df):
    """
    Collects the FED3 rows and BNC rows of every BNC event type FED3 can pulse.

    Grouping is by BNC event, so FED3 labels that share a pulse (Right and RightShort,
    or every special event) are paired in log order against the same BNC rows.

    :param NH_df: NeuroHab dataframe
    :param FED3_df: FED3 dataframe
    :return: dict of BNC event key -> (FED3 row index, BNC row index), each in log order
    """

    fed3_keys = fed3_bnc_keys(FED3_df)
    bnc_keys = _event_keys(NH_df[BNC_EVENT_COL])
    bnc_keys[bnc_keys.str.startswith('*')] = BNC_SPECIAL_EVENT    # already relabelled by a previous run

    groups = {}
    for bnc_key in dict.fromkeys([*FED3_TO_BNC.values(), BNC_SPECIAL_EVENT]):
        fed3_idx = FED3_df.index[fed3_keys == bnc_key]
        bnc_idx = NH_df.index[bnc_keys == bnc_key]
        if len(fed3_idx) or len(bnc_idx):
            groups[bnc_key] = (fed3_idx, bnc_idx)

    return groups


def estimate_clock_offset(NH_df, FED3_df, groups):
    """
    Estimates the constant offset between the FED3 and BNC device clocks.

    Uses the nth-to-nth pairs of event types whose counts agree in both files, so a
    mismatch can't skew the estimate. Falls back to every type when none agree.

    :param NH_df: NeuroHab dataframe
    :param FED3_df: FED3 dataframe
    :param groups: Output of group_fed3_events
    :return: Median of (FED3 ms - BNC millis), or NaN when the files share no events
    """

    equal = [g for g in groups.values() if len(g[0]) == len(g[1])]
    diffs = []
    for fed3_idx, bnc_idx in (equal or list(groups.values())):
        n = min(len(fed3_idx), len(bnc_idx))
        fed3_ms = FED3_df.loc[fed3_idx[:n], FED3_MS_COL].to_numpy(float)
        bnc_ms = NH_df.loc[bnc_idx[:n], BNC_MS_COL].to_numpy(float)
        diffs.append(fed3_ms - bnc_ms)

    diffs = np.concatenate(diffs) if diffs else np.array([])
    return float(np.median(diffs)) if diffs.size else float('nan')


def find_over_sample(long_ms, short_ms):
    """
    Finds which sample in the longer list has no partner in the shorter one.

    Tries removing each sample in turn and keeps the removal that leaves the remaining
    nth-to-nth pairs closest together in time. Ties go to the latest sample.

    :param long_ms: Event times (FED3 clock) of the list holding one extra sample
    :param short_ms: Event times (FED3 clock) of the other list
    :return: Position of the extra sample in long_ms
    """

    if len(short_ms) == 0:
        return len(long_ms) - 1

    before = np.abs(long_ms[:-1] - short_ms)    # pair cost for pairs ahead of the extra
    after = np.abs(long_ms[1:] - short_ms)      # pair cost for pairs behind the extra
    cost = (np.concatenate(([0.0], np.cumsum(before)))
            + np.concatenate((np.cumsum(after[::-1])[::-1], [0.0])))

    return int(len(cost) - 1 - np.argmin(cost[::-1]))


def reconcile_event_type(NH_df, FED3_df, fed3_idx, bnc_idx, offset, group):
    """
    Pairs the nth FED3 event of one type with the nth BNC event of the same type.

    When the counts differ, the extra sample is located and removed, one at a time,
    until both lists are the same length. The remaining pairs are then checked: if
    they all sit within FED3_MATCH_TOL_MS of each other the type reconciled.

    :param NH_df: NeuroHab dataframe
    :param FED3_df: FED3 dataframe
    :param fed3_idx: FED3 rows of this type, in log order
    :param bnc_idx: BNC rows of the matching type, in log order
    :param offset: FED3 minus BNC clock offset in ms
    :param group: Display name of the BNC event type, e.g. 'SpecialEvent'
    :return: (list of (FED3 row, BNC row) pairs, list of FED3Mismatch)
    """

    fed3_ms = FED3_df.loc[fed3_idx, FED3_MS_COL].to_numpy(float)
    bnc_ms = NH_df.loc[bnc_idx, BNC_MS_COL].to_numpy(float) + offset

    fed3_keep = list(range(len(fed3_ms)))
    bnc_keep = list(range(len(bnc_ms)))
    mismatches = []

    while len(fed3_keep) != len(bnc_keep):
        bnc_over = len(bnc_keep) > len(fed3_keep)
        if bnc_over:
            long_keep, long_ms, long_idx, long_df, long_col = bnc_keep, bnc_ms, bnc_idx, NH_df, BNC_EVENT_COL
            short_ms = fed3_ms[fed3_keep]
        else:
            long_keep, long_ms, long_idx, long_df, long_col = fed3_keep, fed3_ms, fed3_idx, FED3_df, FED3_EVENT_COL
            short_ms = bnc_ms[bnc_keep]

        pos = long_keep.pop(find_over_sample(long_ms[long_keep], short_ms))
        row = long_idx[pos]
        mismatches.append(FED3Mismatch(
            event=str(long_df.at[row, long_col]).strip(),
            group=group,
            side='BNC' if bnc_over else 'FED3',
            ordinal=pos + 1,
            row=row,
            time_ms=float(long_ms[pos]),
        ))

    residual = np.abs(fed3_ms[fed3_keep] - bnc_ms[bnc_keep])
    reconciled = bool((residual <= FED3_MATCH_TOL_MS).all())
    for mismatch in mismatches:
        mismatch.reconciled = reconciled

    pairs = [(fed3_idx[f], bnc_idx[b]) for f, b in zip(fed3_keep, bnc_keep)]
    return pairs, mismatches


def flag_end_mismatches(mismatches, pairs, FED3_df):
    """
    Marks mismatches that fall after the last matched event of the session.

    :param mismatches: FED3Mismatch list to update in place
    :param pairs: All (FED3 row, BNC row) pairs
    :param FED3_df: FED3 dataframe
    :return: None
    """

    if not pairs:
        return

    last_ms = FED3_df.loc[[f for f, _ in pairs], FED3_MS_COL].max()
    for mismatch in mismatches:
        mismatch.at_end = mismatch.time_ms > last_ms


def assign_anchor_times(NH_df, FED3_df, pairs, mismatches):
    """
    Copies each paired BNC row's synced_time onto its FED3 row, making it an anchor.

    Adds 'synced_time' and 'sync_method' ('anchor', 'interval' or 'mismatch') columns.
    Every row starts as 'interval'; extra FED3 samples are marked 'mismatch'.

    :param NH_df: NeuroHab dataframe with 'synced_time' already assigned
    :param FED3_df: FED3 dataframe to annotate in place
    :param pairs: All (FED3 row, BNC row) pairs
    :param mismatches: All FED3Mismatch records
    :return: FED3 rows whose BNC partner has no synced_time (left as interval rows)
    """

    FED3_df['synced_time'] = pd.Series(pd.NaT, index=FED3_df.index, dtype='datetime64[ns]')
    FED3_df['sync_method'] = 'interval'
    for mismatch in mismatches:
        if mismatch.side == 'FED3':
            FED3_df.at[mismatch.row, 'sync_method'] = 'mismatch'

    unsynced = []
    for fed3_row, bnc_row in pairs:
        synced = NH_df.at[bnc_row, 'synced_time']
        if pd.isna(synced):
            unsynced.append(fed3_row)
            continue
        FED3_df.at[fed3_row, 'synced_time'] = pd.Timestamp(synced)
        FED3_df.at[fed3_row, 'sync_method'] = 'anchor'

    return unsynced


def label_bnc_special_events(NH_df, FED3_df, pairs):
    """
    Renames each paired BNC SpecialEvent after the FED3 event it stands for.

    Format: '*<FED3 event> ln:<FED3 file line>', e.g. '*TrainingDraw_S1 ln:5'. The line
    counts the header as line 1. Unpaired SpecialEvent rows keep their label.

    :param NH_df: NeuroHab dataframe to relabel in place
    :param FED3_df: FED3 dataframe as read from file (default index = data row number)
    :param pairs: All (FED3 row, BNC row) pairs
    :return: None
    """

    special = fed3_bnc_keys(FED3_df) == BNC_SPECIAL_EVENT

    for fed3_row, bnc_row in pairs:
        if special[fed3_row]:
            label = str(FED3_df.at[fed3_row, FED3_EVENT_COL]).strip()
            NH_df.at[bnc_row, BNC_EVENT_COL] = f"*{label} ln:{fed3_row + 2}"


def fill_interval_times(FED3_df):
    """
    Times every non-anchor FED3 row from its closest anchor using the FED3 ms clock.

    synced_time = anchor synced_time + (row ms - anchor ms). Ties go to the earlier anchor.

    :param FED3_df: FED3 dataframe with anchors already assigned
    :return: Series mapping each filled FED3 row to the anchor row it was timed from
    """

    anchors = FED3_df[FED3_df['sync_method'] == 'anchor'].sort_values(FED3_MS_COL)
    if anchors.empty:
        return pd.Series(dtype=object)

    anchor_ms = anchors[FED3_MS_COL].to_numpy(float)
    used = {}

    for row in FED3_df.index[FED3_df['sync_method'] != 'anchor']:
        row_ms = float(FED3_df.at[row, FED3_MS_COL])
        if np.isnan(row_ms):
            continue

        pos = int(np.searchsorted(anchor_ms, row_ms))
        candidates = [p for p in (pos - 1, pos) if 0 <= p < len(anchor_ms)]
        nearest = min(candidates, key=lambda p: abs(anchor_ms[p] - row_ms))
        anchor_row = anchors.index[nearest]

        FED3_df.at[row, 'synced_time'] = (
            anchors.at[anchor_row, 'synced_time']
            + pd.to_timedelta(row_ms - anchor_ms[nearest], unit='ms')
        )
        used[row] = anchor_row

    return pd.Series(used, dtype=object)


def group_label(NH_df, bnc_key, bnc_idx):
    """
    Returns the BNC event name as written in the BNC file, for console output.

    :param NH_df: NeuroHab dataframe
    :param bnc_key: Lowercase BNC event key
    :param bnc_idx: BNC rows of that event
    :return: The first BNC row's label, or the key when BNC has none
    """

    return str(NH_df.at[bnc_idx[0], BNC_EVENT_COL]).strip() if len(bnc_idx) else bnc_key


def report_fed3_counts(groups, labels, FED3_df):
    """
    Prints the per-type event counts in each file.

    :param groups: Output of group_fed3_events
    :param labels: BNC event key -> display name
    :param FED3_df: FED3 dataframe, used to list the FED3 labels in each group
    :return: None
    """

    fed3_events = FED3_df[FED3_EVENT_COL].astype(str).str.strip()

    print("Event counts (BNC event <- FED3 events)          FED3 / BNC")
    for bnc_key, (fed3_idx, bnc_idx) in groups.items():
        fed3_labels = ', '.join(dict.fromkeys(fed3_events[fed3_idx])) or '-'
        label = f"{labels[bnc_key]} <- {fed3_labels}"
        flag = '' if len(fed3_idx) == len(bnc_idx) else '   <- mismatch'
        print(f"  {label:<46} {len(fed3_idx):>5} / {len(bnc_idx)}{flag}")


def report_mismatches(mismatches, NH_df):
    """
    Prints every mismatch and warns about the ones that need the user's attention.

    :param mismatches: All FED3Mismatch records
    :param NH_df: NeuroHab dataframe, used to spot an unmatched final BNC Right
    :return: None
    """

    print(f"\nMismatches: {len(mismatches)}")

    for m in sorted(mismatches, key=lambda m: m.time_ms):
        other = 'FED3' if m.side == 'BNC' else 'BNC'
        name = m.event if m.event.lower() == m.group.lower() else f"{m.event} ({m.group})"
        where = f"{name} #{m.ordinal} over in {m.side} (under in {other}), {m.side} row {m.row}"

        if m.side == 'BNC' and m.event.lower() == 'right' and m.row == NH_df.index[-1]:
            warnings.warn(
                f"The final BNC event (row {m.row}) is an unmatched Right. Remove it from the "
                "sample — it was probably erroneous due to wire disconnection."
            )

        if m.at_end:
            print(f"  {where} — at end of sample, ignored.")
            continue

        print(
            f"  Mismatch {where} identified indicating wiring interference or "
            "connection issue during experiment."
        )
        if m.reconciled:
            warnings.warn(
                f"Mismatch {where} was removed and the remaining {m.group} events "
                "reconciled. Check the data."
            )
        else:
            warnings.warn(
                f"Mismatch {where} was removed but the remaining {m.group} events still "
                f"don't line up within {FED3_MATCH_TOL_MS} ms. {m.group} alignment is unreliable."
            )


def report_unsynced_anchors(unsynced, FED3_df):
    """
    Warns about FED3 rows whose BNC partner fell outside the sync file.

    :param unsynced: FED3 rows returned by assign_anchor_times
    :param FED3_df: FED3 dataframe
    :return: None
    """

    if not unsynced:
        return

    print(f"\nPaired but BNC partner has no synced_time: {len(unsynced)}")
    for row in unsynced:
        print(f"  {FED3_df.at[row, FED3_EVENT_COL]} FED3 row {row} — timed by interval instead")
    warnings.warn(
        f"{len(unsynced)} FED3 event(s) matched BNC events that have no pulse train in the "
        "sync file. They were timed by interval instead."
    )


def report_special_events(FED3_df, used, legacy):
    """
    Prints every special FED3 event and how it was timed.

    Anchored specials took their SpecialEvent's M2P time. The rest were timed by interval
    from the closest anchor.

    :param FED3_df: FED3 dataframe with synced_time assigned
    :param used: Output of fill_interval_times
    :param legacy: True when the BNC file has no SpecialEvent pulses at all
    :return: None
    """

    special = FED3_df.index[fed3_bnc_keys(FED3_df) == BNC_SPECIAL_EVENT]
    anchored = int((FED3_df.loc[special, 'sync_method'] == 'anchor').sum())

    print(f"\nSpecial events: {len(special)} — anchored {anchored}, by interval {len(special) - anchored}")
    if legacy and len(special):
        print("  No SpecialEvent pulses in the BNC file (older session) — all timed by interval.")

    for row in special:
        label = str(FED3_df.at[row, FED3_EVENT_COL]).strip()
        method = FED3_df.at[row, 'sync_method']

        if method == 'anchor':
            print(f"  {label:<20} row {row:<6} anchored to SpecialEvent")
        elif row in used.index:
            anchor_row = used[row]
            delta = FED3_df.at[row, FED3_MS_COL] - FED3_df.at[anchor_row, FED3_MS_COL]
            anchor_label = str(FED3_df.at[anchor_row, FED3_EVENT_COL]).strip()
            print(f"  {label:<20} row {row:<6} {method}: {delta:+9.0f} ms from {anchor_label} (row {anchor_row})")
        else:
            print(f"  {label:<20} row {row:<6} not synced — no anchor available")


def report_anchor_spread(FED3_df):
    """
    Prints how constant the FED3-to-sync clock offset is across all anchors.

    A correctly paired session gives a small spread. A pair that slipped by one event
    shows up as a jump of roughly one inter-event interval.

    :param FED3_df: FED3 dataframe with anchors assigned
    :return: None
    """

    anchors = FED3_df[FED3_df['sync_method'] == 'anchor']
    if anchors.empty:
        return

    offset = anchors['synced_time'] - pd.to_timedelta(anchors[FED3_MS_COL], unit='ms')
    spread = (offset.max() - offset.min()).total_seconds()
    print(f"\nAnchor offset spread (synced_time - FED3 ms): {spread:.3f} s")

    if spread * 1000 > FED3_MATCH_TOL_MS:
        warnings.warn(
            f"FED3 anchor offset varies by {spread:.3f} s across the session. "
            "Some FED3 events may be paired with the wrong BNC event."
        )


def report_fed3_summary(FED3_df, mismatches):
    """
    Prints the overall FED3 alignment result.

    :param FED3_df: FED3 dataframe with sync_method assigned
    :param mismatches: All FED3Mismatch records
    :return: None
    """

    methods = FED3_df['sync_method'].value_counts()
    is_special = fed3_bnc_keys(FED3_df) == BNC_SPECIAL_EVENT
    special_anchored = int((is_special & (FED3_df['sync_method'] == 'anchor')).sum())
    special_interval = int(is_special.sum()) - special_anchored

    print(
        f"\nFED3 rows: {len(FED3_df)} — anchor {methods.get('anchor', 0)}, "
        f"interval {methods.get('interval', 0)}, mismatch {methods.get('mismatch', 0)}"
    )
    if not mismatches:
        print(
            f"FED3 alignment OK: 0 mismatches, {special_anchored} special events anchored, "
            f"{special_interval} by interval."
        )
    else:
        print(f"FED3 alignment has {len(mismatches)} mismatch(es) — see above.")


def align_fed3(NH_df, FED3_df):
    """
    Assigns synced_time to FED3 rows using the already-synced NeuroHab events.

    FED3 events are grouped by the BNC event they pulse as (FED3_TO_BNC; everything else
    pulses BNC_SPECIAL_EVENT), paired nth-to-nth per group, and copy that BNC row's
    synced_time (anchors). Rows left without a partner are timed from their closest
    anchor by the interval on the FED3 ms clock. Count mismatches are located, removed
    and reported. A BNC file with no SpecialEvent rows (sessions before the special-event
    pulse existed) times every special event by interval instead.

    :param NH_df: NeuroHab dataframe with 'synced_time' already assigned
    :param FED3_df: FED3 dataframe to annotate in place
    :return: The FED3 dataframe with 'synced_time' and 'sync_method' columns
    """

    FED3_df.drop(columns=['synced_time', 'sync_method'], errors='ignore', inplace=True)

    groups = group_fed3_events(NH_df, FED3_df)
    legacy = len(groups.get(BNC_SPECIAL_EVENT, ((), ()))[1]) == 0
    if legacy:
        groups.pop(BNC_SPECIAL_EVENT, None)
    labels = {key: group_label(NH_df, key, bnc_idx) for key, (_, bnc_idx) in groups.items()}

    offset = estimate_clock_offset(NH_df, FED3_df, groups)

    if np.isnan(offset):
        FED3_df['synced_time'] = pd.NaT
        FED3_df['sync_method'] = 'interval'
        warnings.warn("No FED3 event has a BNC counterpart — the FED3 file was left unsynced.")
        return FED3_df

    print(f"FED3 clock offset (FED3 ms - BNC millis): {offset / 1000:.3f} s\n")

    pairs, mismatches = [], []
    for bnc_key, (fed3_idx, bnc_idx) in groups.items():
        type_pairs, type_mismatches = reconcile_event_type(
            NH_df, FED3_df, fed3_idx, bnc_idx, offset, labels[bnc_key]
        )
        pairs += type_pairs
        mismatches += type_mismatches
    flag_end_mismatches(mismatches, pairs, FED3_df)

    unsynced = assign_anchor_times(NH_df, FED3_df, pairs, mismatches)
    used = fill_interval_times(FED3_df)

    report_fed3_counts(groups, labels, FED3_df)
    report_mismatches(mismatches, NH_df)
    report_unsynced_anchors(unsynced, FED3_df)
    report_special_events(FED3_df, used, legacy)
    report_anchor_spread(FED3_df)
    report_fed3_summary(FED3_df, mismatches)

    label_bnc_special_events(NH_df, FED3_df, pairs)

    return FED3_df


def align_to_M2P(NH_df, M2P_df, NH_p, M2P_p, FED3_df=None, FED3_p=None, out_dir=None):
    """ Documentation
    Takes the read dataframes from read_all and outputs new files with timestamps that are aligned to the M2P file.

    :param NH_df:
    :param M2P_df:
    :param NH_p:
    :param M2P_p:
    :param FED3_df: Optional FED3 dataframe. None skips FED3 alignment.
    :param FED3_p: Optional FED3 path, used to name the output file.
    :param out_dir: Directory to write the synced files into. None writes next to the input files.
    :return:
    """

    "Quality Control"
    sumPulses = NH_df['pulseCount'].sum()
    numRows = len(M2P_df)

    if sumPulses != numRows:
        print(f"sumPulses: {sumPulses}, numRows: {numRows}")
        warnings.warn("Number of pulses for NeuroHab is not the same as sync received! Could result in misaligned data!")
    else:
        print(f"PULSES ARE EQUAL: sumPulses: {sumPulses}, numRows: {numRows}")

    find_missing_pulse(NH_df, M2P_df)

    "Build pulse trains and assign train_start as synced_time for each BNC event row."
    trains = build_pulse_trains(M2P_df)

    bnc_index = NH_df.dropna(subset=['pulseCount']).index

    if len(bnc_index) != len(trains):
        warnings.warn(
            f"BNC event count ({len(bnc_index)}) != pulse train count ({len(trains)}). "
            "synced_time alignment may be off — check for missed pulses first."
        )

    n = min(len(bnc_index), len(trains))
    NH_df['synced_time'] = pd.NaT
    NH_df.loc[bnc_index[:n], 'synced_time'] = trains['train_start'].values[:n]

    "Align the FED3 log to the NeuroHab events, if a FED3 file was supplied."
    if FED3_df is not None:
        print()
        align_fed3(NH_df, FED3_df)

    """
        Write the synced dataframes with their filenames + synced back to the alignment_files.
    """
    NH_p = str(NH_p).upper().replace('.CSV', '_synced.csv')
    M2P_p = str(M2P_p).replace('.tdms', '_synced.csv').replace('.xlsx', '_synced.csv').replace('.xls', '_synced.csv')

    NH_p = resolve_output_path(NH_p, out_dir)
    M2P_p = resolve_output_path(M2P_p, out_dir)

    NH_df.to_csv(NH_p)
    # M2P_df.to_csv(M2P_p)

    print(f"Wrote: {NH_p}")

    if FED3_df is not None and FED3_p is not None:
        FED3_p = str(FED3_p).upper().replace('.CSV', '_synced.csv')
        FED3_p = resolve_output_path(FED3_p, out_dir)
        FED3_df.to_csv(FED3_p)
        print(f"Wrote: {FED3_p}")


if __name__ == "__main__":
    "Display the full DF for testing."
    # pd.set_option('display.max_rows', None)  # show all rows
    pd.set_option('display.max_columns', None)  # show all columns
    pd.set_option('display.width', None)  # no line wrapping
    # pd.set_option('display.max_colwidth', None)  # show full cell content (no ...)
    # pd.set_option('display.float_format', '{:.6f}'.format)  # optional: nicer floats

    "Set the path to the files to align."
    dir_path = Path().cwd().joinpath("to-sync/example_data_1")

    "Set which files you want to align. They will be aligned to the M2P_path."
    NH_path = dir_path.joinpath("BNC_1.csv")
    M2P_path = dir_path.joinpath("SignalSync_4.tdms")

    "Optional: set to a FED3 csv path, or None to skip FED3 alignment."
    FED3_path = None
    # FED3_path = dir_path.joinpath("FED3_1.csv")

    "Where to write the synced files. None = next to the input files."
    OUT_DIR = None

    NH, M2P, FED3 = read_all(NH_path, M2P_path, FED3_path)

    "Set to True to drop the first row of the M2P/tdms file (spurious startup pulse)."
    DROP_FIRST_ROW = False

    if DROP_FIRST_ROW:
        M2P = drop_first_row(M2P)

    print(M2P)
    align_to_M2P(NH, M2P, NH_path, M2P_path, FED3_df=FED3, FED3_p=FED3_path, out_dir=OUT_DIR)