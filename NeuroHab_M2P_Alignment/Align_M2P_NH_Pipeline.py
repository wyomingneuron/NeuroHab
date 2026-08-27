from nptdms import TdmsFile
import pandas as pd
from pathlib import Path
from datetime import datetime
import warnings


def read_all(NH_p, M2P_p):
    """ Documentation
    Reads pipeline files and converts them to dataframes.
    Supports .tdms and .xlsx/.xls for M2P.

    :param NH_p:
    :param M2P_p:
    :return:
    """

    NH_df = pd.read_csv(NH_p, index_col=False)
    # FED3_df = pd.read_csv(FED3_p, index_col=False)

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

    return NH_df, M2P_df


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


def align_to_M2P(NH_df, M2P_df, NH_p, M2P_p, out_dir=None):
    """ Documentation
    Takes the read dataframes from read_all and outputs 3 new files with timestamps that are aligned to the M2P file.

    :param NH_df:
    :param M2P_df:
    :param NH_p:
    :param M2P_p:
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

    bnc = NH_df.dropna(subset=['pulseCount']).reset_index(drop=True)

    if len(bnc) != len(trains):
        warnings.warn(
            f"BNC event count ({len(bnc)}) != pulse train count ({len(trains)}). "
            "synced_time alignment may be off — check for missed pulses first."
        )

    n = min(len(bnc), len(trains))
    NH_df['synced_time'] = pd.NaT
    NH_df.loc[bnc.index[:n], 'synced_time'] = trains['train_start'].values[:n]

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


if __name__ == "__main__":
    "Display the full DF for testing."
    # pd.set_option('display.max_rows', None)  # show all rows
    pd.set_option('display.max_columns', None)  # show all columns
    pd.set_option('display.width', None)  # no line wrapping
    # pd.set_option('display.max_colwidth', None)  # show full cell content (no ...)
    # pd.set_option('display.float_format', '{:.6f}'.format)  # optional: nicer floats

    "Set the path to the files to align."
    dir_path = Path().cwd().joinpath("to-sync/8-26-26 1")

    "Set which files you want to align. They will be aligned to the M2P_path."
    NH_path = dir_path.joinpath("BNC_1.csv")
    M2P_path = dir_path.joinpath("SignalSync_4.tdms")

    "Where to write the synced files. None = next to the input files."
    OUT_DIR = None

    NH, M2P = read_all(NH_path, M2P_path)

    "Set to False to keep the first row of the M2P/tdms file."
    DROP_FIRST_ROW = True

    if DROP_FIRST_ROW:
        M2P = drop_first_row(M2P)

    print(M2P)
    align_to_M2P(NH, M2P, NH_path, M2P_path, out_dir=OUT_DIR)