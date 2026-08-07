"""
behavior_plot.py
────────────────
Plots nose/HB distance traces + behavioral events on a single ethogram-style
figure. Each port gets its own vertical channel (EEG-style offset). Within
each channel the distance trace sits at the channel midpoint; Near is at the
BOTTOM of each channel, Far at the TOP. Event triangles are drawn above the
trace with a 60-degree rotated timestamp label showing the event time in
seconds (based on FPS).

CONFIGURE everything in the CONFIG block below, then run:
    python behavior_plot.py
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.lines as mlines
from matplotlib.ticker import FixedLocator, FixedFormatter

# ── CONFIG ────────────────────────────────────────────────────────────────────

DLC_PATH  = "Behavior Trace Files/DLC_Test_2_filtered.csv"
BNC_PATH  = "Behavior Trace Files/BNC_1 REAL DLC TEST 2 Section.csv"
BOOK_PATH = "Behavior Trace Files/Book3.xlsx"

BODY_PART  = "Nose"      # "Nose" or "HB" etc.
PORTS      = ["RW", "RP", "PP"]      # ["RW", "RP", "PP"]
MAX_FRAMES = 18_000      # set to None for all frames

FPS = 30                 # frames per second — used to convert frame → time label

# Port → event name(s) to overlay as triangles
PORT_EVENT_MAP = {
    "RW": "RightWater",
    "LW": "LeftWater",
    "RP": ["Right", "RightDuringDispense"],
    "LP": "Left",
    "PP": "Retrieved",
}

# ── Legend label per port ─────────────────────────────────────────────────────
PORT_LEGEND_LABELS = {
    "RW": "Right Water",
    "LW": "Left Water",
    "RP": "Right Poke",
    "LP": "Left Poke",
    "PP": "Retrieved",
}

# ── Y-axis label override per port ───────────────────────────────────────────
# Override the port name shown on the y-axis tick. Leave a port out to use
# the raw port key (e.g. "RW") as the label.
PORT_Y_LABELS = {
    "RW": "Right Water Port",
    "RP": "Right Poke Port",
    "PP": "Pellet Port",
}

# ── Distance processing ───────────────────────────────────────────────────────
THRESHOLD    = 25    # pixels — used for ceiling and floor clipping

CLIP_CEILING = True  # values ABOVE threshold → clipped to THRESHOLD
CLIP_FLOOR   = False # True  → values BELOW threshold → set to 0
                     # False → raw pixel values kept as-is

BINARIZE     = True  # collapse clipped trace to 0/1 (only when both clips True)

# ── Appearance ────────────────────────────────────────────────────────────────
# FIGURE_LABEL = "Figure 11. Event Activations vs Mouse Proximity [RW:Right Water | RP:Right Pellet | PP:Pellet Port]"
FIGURE_LABEL = ""

# All distance traces are drawn in black
TRACE_COLOR = "#000000"

# Event marker colors — cycled across ports in order: RED, BLUE, GREEN, ORANGE
EVENT_COLORS = ["#E02020", "#2060D0", "#20A040", "#F07820"]

TRACE_LW        = 0.4   # distance trace line width
EVENT_MARKER_S  = 10     # event triangle marker size (points²)
TIME_LABEL_SIZE = 8     # font size for the rotated time labels above markers

FIG_WIDTH       = 8     # inches
FIG_HEIGHT_PER  = 1.4   # inches per port channel
FONT_SIZE       = 12     # base font size
DPI             = 300
OUT_FILE        = "behavior_plot.png"

# ─────────────────────────────────────────────────────────────────────────────


# ── Data loading ──────────────────────────────────────────────────────────────

def load_dlc(path: str) -> pd.DataFrame:
    df = pd.read_csv(path, header=[0, 1], index_col=0)
    df.columns = ["_".join(col).strip() for col in df.columns]
    return df


def load_frame_times(book_path: str) -> pd.Series:
    df = pd.read_excel(book_path, sheet_name="Sync")
    return pd.to_datetime(df["Time"])


def load_events(bnc_path: str, frame_times: pd.Series) -> pd.DataFrame:
    df = pd.read_csv(bnc_path)
    df["datetime"] = pd.to_datetime(
        df["date"] + " " + df["synced_time"],
        format="%m/%d/%Y %H:%M:%S.%f",
    )
    df["frame"] = frame_times.searchsorted(df["datetime"])
    return df[["event", "frame"]].reset_index(drop=True)


# ── Distance computation & processing ────────────────────────────────────────

def compute_distance(df: pd.DataFrame, body_part: str, port: str) -> np.ndarray:
    dx = df[f"{body_part}_x"].values - df[f"{port}_x"].values
    dy = df[f"{body_part}_y"].values - df[f"{port}_y"].values
    return np.sqrt(dx ** 2 + dy ** 2)


def process_distance(raw: np.ndarray) -> np.ndarray:
    dist = raw.copy()
    if CLIP_CEILING:
        dist = np.where(dist > THRESHOLD, THRESHOLD, dist)
    if CLIP_FLOOR:
        dist = np.where(dist < THRESHOLD, 0.0, dist)
    if BINARIZE and CLIP_CEILING and CLIP_FLOOR:
        dist = (dist > 0).astype(float)
    return dist


def normalise_for_channel(dist: np.ndarray) -> np.ndarray:
    if BINARIZE and CLIP_CEILING and CLIP_FLOOR:
        return dist
    vmax = THRESHOLD if CLIP_CEILING else dist.max()
    if vmax == 0:
        return dist
    return np.clip(dist / vmax, 0.0, 1.0)


# ── Label de-overlap ──────────────────────────────────────────────────────────

def cull_overlapping_labels(frames: np.ndarray, min_gap_frames: int) -> np.ndarray:
    if len(frames) == 0:
        return np.array([], dtype=bool)
    keep = np.ones(len(frames), dtype=bool)
    last_kept = frames[0]
    for idx in range(1, len(frames)):
        if frames[idx] - last_kept < min_gap_frames:
            keep[idx] = False
        else:
            last_kept = frames[idx]
    return keep


# ── Plotting ──────────────────────────────────────────────────────────────────

def plot_all_ports(df: pd.DataFrame, events: pd.DataFrame) -> None:
    n              = len(PORTS)
    slot           = 1.0 / n
    present_events = set(events["event"].unique())
    frames         = df.index.to_numpy()
    n_frames       = len(frames)

    min_gap_frames = 60

    fig, ax = plt.subplots(
        figsize=(FIG_WIDTH, FIG_HEIGHT_PER * n),
        dpi=DPI,
    )
    fig.patch.set_facecolor("#FFFFFF")
    ax.set_facecolor("#FFFFFF")

    clip_box = ax.patch

    legend_handles = []
    ytick_pos = []
    ytick_lab = []

    text_artists = []

    for i, port in enumerate(PORTS):
        ev_color = EVENT_COLORS[i % len(EVENT_COLORS)]

        top_y      = 1.0 - i * slot
        bot_y      = top_y - slot * 0.55
        mid_y      = (top_y + bot_y) / 2
        triangle_y = top_y - slot * 0.005

        # ── Distance trace ────────────────────────────────────────────────
        raw  = compute_distance(df, BODY_PART, port)
        dist = process_distance(raw)
        norm = normalise_for_channel(dist)
        y_trace = top_y - norm * (top_y - bot_y)

        line, = ax.plot(frames, y_trace, color=TRACE_COLOR, linewidth=TRACE_LW,
                        zorder=3, alpha=0.9)
        line.set_clip_on(True)
        line.set_clip_path(clip_box)

        # ── Event triangle markers + time labels ──────────────────────────
        raw_mapping    = PORT_EVENT_MAP.get(port, [])
        ev_names       = raw_mapping if isinstance(raw_mapping, list) else [raw_mapping]
        ev_names       = [e for e in ev_names if e in present_events]
        port_ev_frames = events[events["event"].isin(ev_names)]["frame"].to_numpy()
        port_ev_frames = port_ev_frames[(port_ev_frames >= frames[0]) &
                                        (port_ev_frames < n_frames)]

        if len(port_ev_frames) > 0:
            sc = ax.scatter(
                port_ev_frames,
                np.full(len(port_ev_frames), triangle_y),
                marker=(3, 0, 180),
                color=ev_color,
                s=EVENT_MARKER_S,
                zorder=4,
                linewidths=0,
            )
            sc.set_clip_on(True)
            sc.set_clip_path(clip_box)

            label_mask = cull_overlapping_labels(port_ev_frames, min_gap_frames)
            label_y    = triangle_y + slot * 0.04

            for ef, show in zip(port_ev_frames, label_mask):
                if not show:
                    continue
                t_total   = ef / FPS
                t_minutes = int(t_total // 60)
                t_sec     = t_total % 60

                frame_loc = df.index.get_loc(ef) if ef in df.index else min(ef, len(df) - 1)
                raw_dist_at_event = raw[frame_loc]

                txt = ax.text(
                    ef, label_y,
                    f"{t_minutes} min {t_sec:.2f} sec | {raw_dist_at_event:.1f} px",
                    rotation=45,
                    rotation_mode="anchor",
                    ha="left",
                    va="bottom",
                    fontsize=TIME_LABEL_SIZE,
                    color=ev_color,
                    zorder=5,
                )
                txt.set_clip_on(True)
                txt.set_clip_path(clip_box)
                text_artists.append(txt)

        # ── Channel divider ───────────────────────────────────────────────
        if i < n - 1:
            ax.axhline(top_y - slot, color="#CCCCCC", linewidth=0.4,
                       linestyle="-", zorder=1)

        ytick_pos += [top_y, mid_y, bot_y]
        # Use PORT_Y_LABELS override if provided, else fall back to raw port key
        port_display = PORT_Y_LABELS.get(port, port)
        ytick_lab += ["Near", port_display, "Far"]

        # ── Legend entries ────────────────────────────────────────────────
        if i == 0:
            legend_handles.append(
                mlines.Line2D([], [], color=TRACE_COLOR, linewidth=TRACE_LW,
                              label=f"{BODY_PART} → port distance (px)")
            )
        leg_label = PORT_LEGEND_LABELS.get(port, ", ".join(ev_names) if ev_names else port)
        legend_handles.append(
            plt.scatter([], [], marker=(3, 0, 180), color=ev_color,
                        s=EVENT_MARKER_S, label=leg_label, linewidths=0)
        )

    # ── Y-axis ticks ──────────────────────────────────────────────────────────
    ax.yaxis.set_major_locator(FixedLocator(ytick_pos))
    ax.yaxis.set_major_formatter(FixedFormatter(ytick_lab))
    ax.set_ylim(-0.02, 1.02)

    # Build a lookup from display label → port index for bold/color styling
    port_display_set = {PORT_Y_LABELS.get(p, p): idx for idx, p in enumerate(PORTS)}

    for tick_label, pos in zip(ax.get_yticklabels(), ytick_pos):
        text = tick_label.get_text()
        if text in port_display_set:
            port_idx = port_display_set[text]
            tick_label.set_fontsize(FONT_SIZE)
            tick_label.set_fontweight("bold")
            tick_label.set_color(EVENT_COLORS[port_idx % len(EVENT_COLORS)])
        else:
            tick_label.set_fontsize(FONT_SIZE - 2)
            tick_label.set_color("#888888")

    ax.set_xlabel("Frame", fontsize=FONT_SIZE, fontweight="bold")
    ax.set_ylabel("Port / Proximity", fontsize=FONT_SIZE, fontweight="bold")
    ax.set_xlim(frames[0], frames[-1])
    ax.tick_params(axis="x", labelsize=FONT_SIZE - 1)
    ax.tick_params(axis="y", length=0)

    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.grid(False)

    ax.set_clip_on(True)

    ax.set_title(
        f"{BODY_PART} Proximity to Port (px) vs Event Activation Time (min sec)",
        fontsize=FONT_SIZE + 1, pad=6, fontweight="bold",
    )

    # Legend inside the axes at top-right
    ax.legend(
        handles=legend_handles,
        loc="upper right",
        bbox_to_anchor=(1.0, 1.0),
        borderaxespad=0.4,
        fontsize=FONT_SIZE - 1,
        framealpha=0.9,
        edgecolor="#BBBBBB",
    )

    if FIGURE_LABEL:
        fig.text(
            0.98, 0.01, FIGURE_LABEL,
            ha="right", va="bottom",
            fontsize=FONT_SIZE - 1, color="#555555", style="italic",
        )

    # Rect leaves room at bottom for figure label, top for title
    fig.tight_layout(rect=[0, 0.04, 1, 0.96])
    plt.savefig(OUT_FILE, dpi=DPI, bbox_inches="tight",
                facecolor=fig.get_facecolor())
    print(f"Saved: {OUT_FILE}")
    plt.show()


# ── Entry point ───────────────────────────────────────────────────────────────

def main() -> None:
    df          = load_dlc(DLC_PATH)
    frame_times = load_frame_times(BOOK_PATH)
    events      = load_events(BNC_PATH, frame_times)

    if MAX_FRAMES is not None:
        df     = df.iloc[:MAX_FRAMES]
        events = events[events["frame"] <= MAX_FRAMES]

    plot_all_ports(df, events)


if __name__ == "__main__":
    main()