"""
BNC FED3 Data Analyzer  v3.0
4 fixed plots: Event Raster, Event Counts, Cumulative Water Intake, Pellet Retrieval Latency
13 professional themes — all UI + plots themed together
Theme persisted in config and restored on launch
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import os, json, warnings
import pandas as pd
import numpy as np
import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
from matplotlib.figure import Figure

warnings.filterwarnings("ignore", category=UserWarning, module="matplotlib")

# ══════════════════════════════════════════════════════════════════════════════
#  THEMES
# ══════════════════════════════════════════════════════════════════════════════

THEMES = {
    "Default (Dark Blue)": dict(
        ui_bg="#1e1e2e", ui_panel="#2a2a3e", ui_widget="#313244",
        ui_hover="#45475a", ui_text="#cdd6f4", ui_sub="#888ba8",
        ui_faint="#45475a", ui_bar="#252535", ui_bar_lbl="#666888",
        fig_bg="#1e1e2e", ax_bg="#181825", ax_spine="#45475a",
        ax_tick="#888ba8", ax_lbl="#888ba8", ax_title="#cdd6f4",
        ax_grid="#2a2a44", leg_bg="#313244", leg_edge="#45475a", leg_txt="#cdd6f4",
        empty="#45475a",
        d_left="#4C72B0", d_right="#DD8452", d_disp="#55A868", d_retr="#C44E52",
        d_lw="#8172B2", d_rw="#937860", d_fw="#64B5CD",
        d_water="#29b6f6", d_led="#ffd54f", d_tone="#ef5350",
        d_mean="#ffd54f", d_fill="#29b6f6", tab="tab20",
    ),
    "Clean White": dict(
        ui_bg="#f5f5f5", ui_panel="#ffffff", ui_widget="#e0e0e0",
        ui_hover="#bdbdbd", ui_text="#111111", ui_sub="#444444",
        ui_faint="#bdbdbd", ui_bar="#eeeeee", ui_bar_lbl="#888888",
        fig_bg="#ffffff", ax_bg="#fafafa", ax_spine="#cccccc",
        ax_tick="#333333", ax_lbl="#333333", ax_title="#111111",
        ax_grid="#e8e8e8", leg_bg="#ffffff", leg_edge="#cccccc", leg_txt="#111111",
        empty="#aaaaaa",
        d_left="#1565C0", d_right="#E65100", d_disp="#2E7D32", d_retr="#B71C1C",
        d_lw="#6A1B9A", d_rw="#4E342E", d_fw="#00838F",
        d_water="#0288D1", d_led="#F9A825", d_tone="#C62828",
        d_mean="#E65100", d_fill="#0288D1", tab="tab20b",
    ),
    "Pitch Black": dict(
        ui_bg="#000000", ui_panel="#0a0a0a", ui_widget="#1a1a1a",
        ui_hover="#2a2a2a", ui_text="#e0e0e0", ui_sub="#909090",
        ui_faint="#303030", ui_bar="#111111", ui_bar_lbl="#606060",
        fig_bg="#000000", ax_bg="#050505", ax_spine="#2a2a2a",
        ax_tick="#808080", ax_lbl="#808080", ax_title="#dddddd",
        ax_grid="#1a1a1a", leg_bg="#111111", leg_edge="#2a2a2a", leg_txt="#e0e0e0",
        empty="#2a2a2a",
        d_left="#5c9bd6", d_right="#f4845f", d_disp="#6bcb77", d_retr="#e05c5c",
        d_lw="#a87dc2", d_rw="#c8a882", d_fw="#7ecbcf",
        d_water="#56bfff", d_led="#ffe066", d_tone="#ff6b6b",
        d_mean="#ffe066", d_fill="#56bfff", tab="tab20",
    ),
    "Slate": dict(
        ui_bg="#0d1117", ui_panel="#161b22", ui_widget="#21262d",
        ui_hover="#30363d", ui_text="#c9d1d9", ui_sub="#8b949e",
        ui_faint="#30363d", ui_bar="#1c2128", ui_bar_lbl="#484f58",
        fig_bg="#0d1117", ax_bg="#0d1117", ax_spine="#30363d",
        ax_tick="#8b949e", ax_lbl="#8b949e", ax_title="#c9d1d9",
        ax_grid="#1c2128", leg_bg="#161b22", leg_edge="#30363d", leg_txt="#c9d1d9",
        empty="#30363d",
        d_left="#388bfd", d_right="#f78166", d_disp="#56d364", d_retr="#ff7b72",
        d_lw="#bc8cff", d_rw="#e3b341", d_fw="#39d3dd",
        d_water="#1f6feb", d_led="#e3b341", d_tone="#ff7b72",
        d_mean="#e3b341", d_fill="#1f6feb", tab="tab20",
    ),
    "Nordic": dict(
        ui_bg="#2e3440", ui_panel="#3b4252", ui_widget="#434c5e",
        ui_hover="#4c566a", ui_text="#eceff4", ui_sub="#aab0bc",
        ui_faint="#4c566a", ui_bar="#353d4e", ui_bar_lbl="#7a8498",
        fig_bg="#2e3440", ax_bg="#272c38", ax_spine="#4c566a",
        ax_tick="#aab0bc", ax_lbl="#aab0bc", ax_title="#eceff4",
        ax_grid="#3b4252", leg_bg="#3b4252", leg_edge="#4c566a", leg_txt="#eceff4",
        empty="#4c566a",
        d_left="#81a1c1", d_right="#d08770", d_disp="#a3be8c", d_retr="#bf616a",
        d_lw="#b48ead", d_rw="#ebcb8b", d_fw="#88c0d0",
        d_water="#5e81ac", d_led="#ebcb8b", d_tone="#bf616a",
        d_mean="#ebcb8b", d_fill="#5e81ac", tab="tab20c",
    ),
    "Solarized Dark": dict(
        ui_bg="#002b36", ui_panel="#073642", ui_widget="#094555",
        ui_hover="#0d5a6e", ui_text="#93a1a1", ui_sub="#839496",
        ui_faint="#094555", ui_bar="#05303d", ui_bar_lbl="#3c6472",
        fig_bg="#002b36", ax_bg="#002028", ax_spine="#094555",
        ax_tick="#839496", ax_lbl="#839496", ax_title="#93a1a1",
        ax_grid="#073642", leg_bg="#073642", leg_edge="#094555", leg_txt="#93a1a1",
        empty="#094555",
        d_left="#268bd2", d_right="#cb4b16", d_disp="#859900", d_retr="#dc322f",
        d_lw="#6c71c4", d_rw="#b58900", d_fw="#2aa198",
        d_water="#268bd2", d_led="#b58900", d_tone="#dc322f",
        d_mean="#b58900", d_fill="#268bd2", tab="tab20b",
    ),
    "Solarized Light": dict(
        ui_bg="#fdf6e3", ui_panel="#eee8d5", ui_widget="#ddd8c4",
        ui_hover="#c9c3af", ui_text="#073642", ui_sub="#586e75",
        ui_faint="#c9c3af", ui_bar="#e8e2d0", ui_bar_lbl="#7a8a8a",
        fig_bg="#fdf6e3", ax_bg="#fdf6e3", ax_spine="#c9c3af",
        ax_tick="#073642", ax_lbl="#073642", ax_title="#073642",
        ax_grid="#eee8d5", leg_bg="#eee8d5", leg_edge="#c9c3af", leg_txt="#073642",
        empty="#c9c3af",
        d_left="#268bd2", d_right="#cb4b16", d_disp="#859900", d_retr="#dc322f",
        d_lw="#6c71c4", d_rw="#b58900", d_fw="#2aa198",
        d_water="#268bd2", d_led="#b58900", d_tone="#dc322f",
        d_mean="#b58900", d_fill="#268bd2", tab="tab20b",
    ),
    "Dracula": dict(
        ui_bg="#282a36", ui_panel="#21222c", ui_widget="#313341",
        ui_hover="#414355", ui_text="#f8f8f2", ui_sub="#bdbdcf",
        ui_faint="#414355", ui_bar="#252636", ui_bar_lbl="#7070a0",
        fig_bg="#282a36", ax_bg="#1e1f29", ax_spine="#414355",
        ax_tick="#bdbdcf", ax_lbl="#bdbdcf", ax_title="#f8f8f2",
        ax_grid="#313341", leg_bg="#21222c", leg_edge="#414355", leg_txt="#f8f8f2",
        empty="#414355",
        d_left="#8be9fd", d_right="#ffb86c", d_disp="#50fa7b", d_retr="#ff5555",
        d_lw="#bd93f9", d_rw="#f1fa8c", d_fw="#6be5fd",
        d_water="#8be9fd", d_led="#f1fa8c", d_tone="#ff5555",
        d_mean="#f1fa8c", d_fill="#8be9fd", tab="tab20",
    ),
    "Monokai": dict(
        ui_bg="#272822", ui_panel="#1e1f1a", ui_widget="#3e3d32",
        ui_hover="#49483e", ui_text="#f8f8f2", ui_sub="#c8c8b8",
        ui_faint="#49483e", ui_bar="#2a2b25", ui_bar_lbl="#8a8a70",
        fig_bg="#272822", ax_bg="#1e1f1a", ax_spine="#49483e",
        ax_tick="#c8c8b8", ax_lbl="#c8c8b8", ax_title="#f8f8f2",
        ax_grid="#3e3d32", leg_bg="#1e1f1a", leg_edge="#49483e", leg_txt="#f8f8f2",
        empty="#49483e",
        d_left="#66d9e8", d_right="#fd971f", d_disp="#a6e22e", d_retr="#f92672",
        d_lw="#ae81ff", d_rw="#e6db74", d_fw="#66d9e8",
        d_water="#66d9e8", d_led="#e6db74", d_tone="#f92672",
        d_mean="#e6db74", d_fill="#66d9e8", tab="tab20",
    ),
    "Gruvbox Dark": dict(
        ui_bg="#282828", ui_panel="#1d2021", ui_widget="#3c3836",
        ui_hover="#504945", ui_text="#ebdbb2", ui_sub="#d5c4a1",
        ui_faint="#504945", ui_bar="#252321", ui_bar_lbl="#928374",
        fig_bg="#282828", ax_bg="#1d2021", ax_spine="#504945",
        ax_tick="#d5c4a1", ax_lbl="#d5c4a1", ax_title="#ebdbb2",
        ax_grid="#3c3836", leg_bg="#1d2021", leg_edge="#504945", leg_txt="#ebdbb2",
        empty="#504945",
        d_left="#83a598", d_right="#fe8019", d_disp="#b8bb26", d_retr="#fb4934",
        d_lw="#d3869b", d_rw="#fabd2f", d_fw="#8ec07c",
        d_water="#83a598", d_led="#fabd2f", d_tone="#fb4934",
        d_mean="#fabd2f", d_fill="#83a598", tab="tab20c",
    ),
    "Publication (B&W)": dict(
        ui_bg="#f0f0f0", ui_panel="#ffffff", ui_widget="#d8d8d8",
        ui_hover="#bbbbbb", ui_text="#111111", ui_sub="#333333",
        ui_faint="#bbbbbb", ui_bar="#e8e8e8", ui_bar_lbl="#666666",
        fig_bg="#ffffff", ax_bg="#ffffff", ax_spine="#000000",
        ax_tick="#000000", ax_lbl="#000000", ax_title="#000000",
        ax_grid="#e0e0e0", leg_bg="#ffffff", leg_edge="#000000", leg_txt="#000000",
        empty="#cccccc",
        d_left="#000000", d_right="#555555", d_disp="#222222", d_retr="#888888",
        d_lw="#333333", d_rw="#777777", d_fw="#444444",
        d_water="#222222", d_led="#555555", d_tone="#888888",
        d_mean="#000000", d_fill="#aaaaaa", tab="Greys",
    ),
    "Ocean": dict(
        ui_bg="#0a1628", ui_panel="#0f1f38", ui_widget="#162840",
        ui_hover="#1e3550", ui_text="#b8d4e8", ui_sub="#7aaabf",
        ui_faint="#1e3550", ui_bar="#0d1b30", ui_bar_lbl="#3a6a88",
        fig_bg="#0a1628", ax_bg="#071020", ax_spine="#1e3550",
        ax_tick="#7aaabf", ax_lbl="#7aaabf", ax_title="#b8d4e8",
        ax_grid="#0f1f38", leg_bg="#0f1f38", leg_edge="#1e3550", leg_txt="#b8d4e8",
        empty="#1e3550",
        d_left="#4fc3f7", d_right="#ff8a65", d_disp="#69f0ae", d_retr="#ef5350",
        d_lw="#ce93d8", d_rw="#ffcc02", d_fw="#80deea",
        d_water="#29b6f6", d_led="#ffcc02", d_tone="#ef5350",
        d_mean="#ffcc02", d_fill="#29b6f6", tab="tab20c",
    ),
    "Rose Pine": dict(
        ui_bg="#191724", ui_panel="#1f1d2e", ui_widget="#26233a",
        ui_hover="#403d52", ui_text="#e0def4", ui_sub="#b8b6cc",
        ui_faint="#403d52", ui_bar="#1c1a2b", ui_bar_lbl="#6e6a86",
        fig_bg="#191724", ax_bg="#13111e", ax_spine="#403d52",
        ax_tick="#b8b6cc", ax_lbl="#b8b6cc", ax_title="#e0def4",
        ax_grid="#26233a", leg_bg="#1f1d2e", leg_edge="#403d52", leg_txt="#e0def4",
        empty="#403d52",
        d_left="#31748f", d_right="#f6c177", d_disp="#9ccfd8", d_retr="#eb6f92",
        d_lw="#c4a7e7", d_rw="#ebbcba", d_fw="#9ccfd8",
        d_water="#31748f", d_led="#f6c177", d_tone="#eb6f92",
        d_mean="#f6c177", d_fill="#31748f", tab="tab20",
    ),
}

THEME_NAMES = list(THEMES.keys())
DEFAULT_THEME = "Default (Dark Blue)"

# Active theme reference — always call set_theme() before using _T
_T = THEMES[DEFAULT_THEME]

def set_theme(name):
    global _T
    _T = THEMES.get(name, THEMES[DEFAULT_THEME])

def _tab20():
    try:
        cmap = plt.get_cmap(_T["tab"])
        if hasattr(cmap, "colors"):
            return cmap.colors
        # For non-Listed colormaps (Greys), sample 20 evenly spaced
        return [cmap(i / 19) for i in range(20)]
    except Exception:
        return plt.get_cmap("tab20").colors

# ══════════════════════════════════════════════════════════════════════════════
#  CONFIG
# ══════════════════════════════════════════════════════════════════════════════

CONFIG_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "bnc_config.json")

def load_config():
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE) as f:
                return json.load(f)
        except Exception:
            pass
    return {}

def save_config(cfg):
    try:
        with open(CONFIG_FILE, "w") as f:
            json.dump(cfg, f)
    except Exception:
        pass

# ══════════════════════════════════════════════════════════════════════════════
#  DATA LOADER
# ══════════════════════════════════════════════════════════════════════════════

def load_csv(path):
    with open(path, "r", encoding="utf-8-sig") as f:
        header_line = f.readline().strip().replace("\r", "")
    col_names = [c.strip() for c in header_line.split(",")]
    df = pd.read_csv(path, skiprows=1, header=None,
                     names=col_names + ["_overflow"],
                     dtype=str, encoding="utf-8-sig", engine="python")
    df = df.drop(columns=["_overflow"], errors="ignore")
    df.columns = [c.strip().replace("\r", "") for c in df.columns]
    df = df.apply(lambda col: col.str.strip().str.replace("\r", "", regex=False)
                  if col.dtype == object else col)
    df = df.dropna(subset=["date", "time"])
    df = df[(df["date"] != "") & (df["time"] != "")]
    # Try multiple date formats to handle MM/DD/YY, M/D/YY, MM/DD/YYYY, M/D/YYYY
    datetime_str = df["date"] + " " + df["time"]
    df["datetime"] = pd.to_datetime(datetime_str, format="%m/%d/%y %H:%M:%S", errors="coerce")
    mask = df["datetime"].isna()
    if mask.any():
        df.loc[mask, "datetime"] = pd.to_datetime(datetime_str[mask], format="%m/%d/%Y %H:%M:%S", errors="coerce")
    mask = df["datetime"].isna()
    if mask.any():
        df.loc[mask, "datetime"] = pd.to_datetime(datetime_str[mask], infer_datetime_format=True, errors="coerce")
    df = df.dropna(subset=["datetime"])
    if df.empty:
        raise ValueError("No valid rows found. Could not parse date/time columns.\n"
                         "Supported formats: MM/DD/YY, M/D/YY, MM/DD/YYYY, M/D/YYYY (with HH:MM:SS time).")
    df = df.sort_values("datetime").reset_index(drop=True)
    for col in df.columns:
        if col not in {"date", "time", "event", "datetime"}:
            df[col] = pd.to_numeric(df[col], errors="coerce")
    # Use millis column for high-accuracy timing if available
    if "millis" in df.columns and df["millis"].notna().any():
        ms0 = df["millis"].iloc[0]
        df["elapsed_ms"]  = df["millis"] - ms0
        df["elapsed_min"] = df["elapsed_ms"] / 1000.0 / 60.0
    else:
        df["elapsed_ms"]  = (df["datetime"] - df["datetime"].iloc[0]).dt.total_seconds() * 1000
        df["elapsed_min"] = df["elapsed_ms"] / 1000.0 / 60.0
    return df

# ══════════════════════════════════════════════════════════════════════════════
#  LEGEND HELPER
# ══════════════════════════════════════════════════════════════════════════════

def _legend(ax, **kw):
    handles, labels = ax.get_legend_handles_labels()
    if not handles:
        return
    leg = ax.legend(handles, labels, **kw)
    leg.get_frame().set_facecolor(_T["leg_bg"])
    leg.get_frame().set_edgecolor(_T["leg_edge"])
    for t in leg.get_texts():
        t.set_color(_T["leg_txt"])

# ══════════════════════════════════════════════════════════════════════════════
#  THE 4 PLOTS
# ══════════════════════════════════════════════════════════════════════════════

# -- Event Raster -----------------------------------------------------------
# Toggle state keyed by id(ax).
# Uses button_press_event + manual hit-test so clicking works at any zoom.
_raster_expanded = {}

def _raster_style_ax(ax):
    t = _T
    ax.set_facecolor(t['ax_bg'])
    for sp in ax.spines.values():
        sp.set_edgecolor(t['ax_spine'])
    ax.tick_params(colors=t['ax_tick'], labelsize=7)
    ax.xaxis.label.set_color(t['ax_lbl'])
    ax.yaxis.label.set_color(t['ax_lbl'])
    ax.title.set_color(t['ax_title'])

def _raster_do_draw(ax):
    df     = ax._raster_df
    events = sorted(df['event'].dropna().unique(), key=lambda x: str(x))
    n      = len(events)
    if n == 0:
        ax.text(0.5, 0.5, 'No events', ha='center', va='center',
                transform=ax.transAxes, color=_T['empty'], fontsize=8)
        return
    expanded = _raster_expanded.setdefault(id(ax), set())
    colors   = _tab20()
    HALF     = 0.38

    for i, ev in enumerate(events):
        sub   = df[df['event'] == ev]['elapsed_min'].values
        color = colors[i % len(colors)]
        if len(sub) == 0:
            continue
        if i in expanded:
            ax.vlines(sub, -0.5, n - 0.5,
                      color=color, linewidth=0.7, alpha=0.35, zorder=2)
        else:
            ax.vlines(sub, i - HALF, i + HALF,
                      color=color, linewidth=0.9, alpha=0.70, zorder=2)
        ax.scatter(sub, [i] * len(sub),
                   s=10, color=color, alpha=0.90, linewidths=0, zorder=3)

    ax.set_yticks(range(n))
    ax.set_ylim(-0.55, n - 0.45)
    ax.set_yticklabels([])
    ax.tick_params(axis='y', length=0, labelsize=0)

    # Coloured bbox labels - fat hit area, colour-coded, bold when expanded
    ax._raster_labels = []
    for i, ev in enumerate(events):
        color  = colors[i % len(colors)]
        weight = 'bold'   if i in expanded else 'normal'
        fa     = 0.32     if i in expanded else 0.12
        ta     = 1.0      if i in expanded else 0.85
        bbox_p = dict(boxstyle='round,pad=0.3', facecolor=color,
                      edgecolor=color, alpha=fa)
        lbl = ax.text(-0.01, i, str(ev),
                      transform=ax.get_yaxis_transform(),
                      ha='right', va='center',
                      fontsize=7.5, color=color, fontweight=weight,
                      alpha=ta, bbox=bbox_p, clip_on=False, zorder=5)
        lbl._raster_row = i
        ax._raster_labels.append(lbl)

    for i in range(n):
        if i % 2 == 0:
            ax.axhspan(i - 0.5, i + 0.5,
                       color=_T['ax_grid'], alpha=0.18, zorder=0, linewidth=0)

    ax.set_xlabel('Elapsed Time (min)', fontsize=8)
    ax.set_title('Event Raster  —  click label to toggle full-height lines',
                 fontsize=9, fontweight='bold')
    ax.grid(axis='x', alpha=0.25, color=_T['ax_grid'], zorder=1)

def _raster_btn_press(event):
    # Display-pixel hit-test, zoom/pan/fullscreen safe.
    # Uses ax.transData to convert pixel->data so the mapping is always exact.
    fig = event.canvas.figure
    # Get a live renderer so window_extent is fresh after any resize
    try:
        renderer = fig.canvas.get_renderer()
    except Exception:
        renderer = None
    for ax_ in fig.get_axes():
        if not hasattr(ax_, '_raster_df'):
            continue
        # Fresh axes bounding box in display pixels
        bbox = ax_.get_window_extent(renderer=renderer)
        # Click must be LEFT of the axes spine and within vertical extent
        if event.x >= bbox.x0:
            continue
        if event.y < bbox.y0 or event.y > bbox.y1:
            continue
        events = sorted(ax_._raster_df['event'].dropna().unique(), key=lambda x: str(x))
        n = len(events)
        if n == 0:
            continue
        # Convert pixel y -> data y using the axes transform directly
        # This is exact regardless of zoom, pan, or window size
        inv = ax_.transData.inverted()
        _, y_data = inv.transform((event.x, event.y))
        row = int(round(y_data))
        if row < 0 or row >= n:
            continue
        exp_ = _raster_expanded.setdefault(id(ax_), set())
        if row in exp_:
            exp_.discard(row)
        else:
            exp_.add(row)
        # Preserve current pan/zoom
        xlim = ax_.get_xlim()
        ylim = ax_.get_ylim()
        ax_.clear()
        _raster_style_ax(ax_)
        _raster_do_draw(ax_)
        ax_.set_xlim(xlim)
        ax_.set_ylim(ylim)
        event.canvas.draw_idle()
        return

def _raster_connect(canvas):
    if not hasattr(canvas, '_raster_btn_cid'):
        canvas._raster_btn_cid = canvas.mpl_connect('button_press_event', _raster_btn_press)

def plot_event_raster(ax, df):
    ax._raster_df = df
    _raster_connect(ax.get_figure().canvas)
    _raster_do_draw(ax)

def plot_event_counts(ax, df):
    """Horizontal bar chart — total count per event type."""
    counts = df["event"].value_counts()
    colors = _tab20()
    bar_colors = [colors[i % len(colors)] for i in range(len(counts))]
    bars = ax.barh(counts.index[::-1], counts.values[::-1],
                   color=bar_colors[::-1],
                   edgecolor=_T["ax_bg"], linewidth=0.4)
    ax.bar_label(bars, fmt="%d", padding=3,
                 color=_T["ax_lbl"], fontsize=7)
    ax.set_xlabel("Count", fontsize=8)
    ax.set_title("Event Counts", fontsize=9, fontweight="bold")
    ax.grid(axis="x", alpha=0.3, color=_T["ax_grid"])


def plot_cumulative_water(ax, df):
    """Cumulative water volume (mL) by port.
    Uses cummax() to enforce strict monotonicity — the FED3 device occasionally
    logs a pre-dispense QTY on companion event rows that is lower than the
    true running total, which would otherwise cause a spurious dip.
    """
    plotted = False
    for label, col, color in [
        ("Left",  "leftWD_QTY",  _T["d_lw"]),
        ("Right", "rightWD_QTY", _T["d_rw"]),
        ("Front", "frontWD_QTY", _T["d_fw"]),
    ]:
        if col not in df.columns:
            continue
        sub = df[["elapsed_min", col]].dropna(subset=[col]).copy()
        if sub.empty or sub[col].max() == 0:
            continue
        sub[col] = sub[col].cummax()   # guarantee never-decreasing
        ax.plot(sub["elapsed_min"], sub[col], label=label,
                linewidth=1.8, color=color)
        plotted = True
    if not plotted:
        ax.text(0.5, 0.5, "No water intake data",
                ha="center", va="center", transform=ax.transAxes,
                color=_T["empty"], fontsize=8)
    ax.set_xlabel("Elapsed Time (min)", fontsize=8)
    ax.set_ylabel("Volume (mL)", fontsize=8)
    ax.set_title("Cumulative Water Intake (mL)", fontsize=9, fontweight="bold")
    _legend(ax, fontsize=7, framealpha=0.85)
    ax.grid(alpha=0.3, color=_T["ax_grid"])


def plot_retrieval_latency(ax, df):
    """Latency (s) from each Dispensed event to the next Retrieved event.
    X-axis = elapsed session time at the moment of retrieval (minutes),
    so you can see both WHEN in the session each retrieval occurred and
    how long the animal took to collect the pellet.
    """
    dispensed = df[df["event"] == "Dispensed"]["elapsed_min"].values
    retrieved = df[df["event"] == "Retrieved"]["elapsed_min"].values
    retr_times = []   # elapsed_min of the retrieval event
    latencies  = []   # seconds from dispense to retrieval

    for d in dispensed:
        later = retrieved[retrieved > d]
        if len(later):
            r = later[0]
            retr_times.append(r)
            latencies.append((r - d) * 60)

    if not latencies:
        ax.text(0.5, 0.5, "No Dispensed\u2192Retrieved pairs",
                ha="center", va="center", transform=ax.transAxes,
                color=_T["empty"], fontsize=8)
        ax.set_title("Pellet Retrieval Latency", fontsize=9, fontweight="bold")
        return

    retr_times = np.array(retr_times)
    latencies  = np.array(latencies)
    mean_val   = np.mean(latencies)
    median_val = np.median(latencies)

    # Scatter: x = session time of retrieval, y = latency
    ax.scatter(retr_times, latencies,
               color=_T["d_disp"], s=22, alpha=0.80,
               linewidths=0, zorder=3)

    # Global mean + median lines only
    ax.axhline(mean_val, color=_T["d_mean"], linewidth=1.2,
               linestyle="--", alpha=0.75, label=f"Mean = {mean_val:.1f}s")
    ax.axhline(median_val, color=_T["d_fill"], linewidth=1.2,
               linestyle=":", alpha=0.75, label=f"Median = {median_val:.1f}s")

    ax.set_xlabel("Session Time at Retrieval (min)", fontsize=8)
    ax.set_ylabel("Latency (s)", fontsize=8)
    ax.set_title("Pellet Retrieval Latency", fontsize=9, fontweight="bold")
    _legend(ax, fontsize=7, framealpha=0.85)
    ax.grid(alpha=0.25, color=_T["ax_grid"])


def plot_iei_distribution(ax, df):
    events = sorted(df['event'].dropna().unique(), key=lambda x: str(x))
    n = len(events)
    if n == 0:
        ax.text(0.5, 0.5, 'No events', ha='center', va='center',
                transform=ax.transAxes, color=_T['empty'], fontsize=8)
        return

    colors     = _tab20()
    all_ieis   = []
    valid_evs  = []
    valid_cols = []
    MIN_IEI    = 1.0     # ms - drop sub-millisecond same-timestamp noise

    for i, ev in enumerate(events):
        # Use elapsed_ms if available (high-accuracy from millis column)
        if 'elapsed_ms' in df.columns:
            times = np.sort(df[df['event'] == ev]['elapsed_ms'].values)
        else:
            times = np.sort(df[df['event'] == ev]['elapsed_min'].values) * 60 * 1000
        if len(times) < 2:
            continue
        ieis = np.diff(times)  # already in ms
        ieis = ieis[ieis >= MIN_IEI]
        if len(ieis) == 0:
            continue
        all_ieis.append(ieis)
        valid_evs.append(str(ev))
        valid_cols.append(colors[i % len(colors)])

    if not all_ieis:
        ax.text(0.5, 0.5, 'Not enough repeated events\n(need >= 2 occurrences each)',
                ha='center', va='center', transform=ax.transAxes,
                color=_T['empty'], fontsize=8)
        ax.set_title('Inter-Event Interval Distribution', fontsize=9, fontweight='bold')
        return

    m         = len(valid_evs)
    positions = np.arange(m)
    flat      = np.concatenate(all_ieis)
    flat_pos  = flat[flat >= MIN_IEI]

    y_min_pct = float(np.percentile(flat_pos, 1))
    y_max_pct = float(np.percentile(flat_pos, 99)) * 1.15
    if y_max_pct <= y_min_pct:
        y_max_pct = y_min_pct * 10

    use_log = (y_max_pct / max(y_min_pct, 1e-9)) > 100

    # Clip data to 99th pct so outliers don't crush the boxes
    clipped = [np.clip(arr, MIN_IEI, y_max_pct) for arr in all_ieis]  # values in ms

    bp = ax.boxplot(clipped, positions=positions, widths=0.45,
                    vert=True, patch_artist=True, showfliers=False, zorder=2,
                    medianprops=dict(color=_T['ax_title'], linewidth=1.8),
                    whiskerprops=dict(linewidth=0.9, color=_T['ax_spine']),
                    capprops=dict(linewidth=0.9, color=_T['ax_spine']),
                    boxprops=dict(linewidth=0.8))

    for patch, col in zip(bp['boxes'], valid_cols):
        patch.set_facecolor(col)
        patch.set_alpha(0.35)
        patch.set_edgecolor(col)

    rng = np.random.default_rng(42)
    for xi, (ieis, col) in enumerate(zip(clipped, valid_cols)):
        pts    = ieis if len(ieis) <= 300 else rng.choice(ieis, 300, replace=False)
        jitter = rng.uniform(-0.18, 0.18, size=len(pts))
        ax.scatter(xi + jitter, pts, s=6, color=col,
                   alpha=0.55, linewidths=0, zorder=3)

    ax.set_xticks(positions)
    ax.set_xticklabels(valid_evs, fontsize=7, rotation=30, ha='right')
    ax.set_xlim(-0.6, m - 0.4)
    ax.set_title('Inter-Event Interval Distribution', fontsize=9, fontweight='bold')
    ax.grid(axis='y', alpha=0.25, color=_T['ax_grid'])

    def _fmt(v, _):
        if v <= 0:
            return '0'
        if v >= 10000:
            return f'{v/1000:.1f}s'
        if v >= 1000:
            return f'{v:.0f}'
        if v >= 10:
            return f'{v:.0f}'
        return f'{v:.1f}'

    if use_log:
        ax.set_yscale('log')
        ax.set_ylim(max(y_min_pct * 0.5, MIN_IEI), y_max_pct)
        ax.set_ylabel('IEI (ms, log)', fontsize=8)
        ax.yaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(_fmt))
    else:
        ax.set_ylim(0, y_max_pct)
        ax.set_ylabel('IEI (ms)', fontsize=8)
        ax.yaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(_fmt))

    ax.set_autoscale_on(False)

# -- Event Frequency (Hz) ----------------------------------------------------
_hz_active = {}
_hz_bin_s  = {}

def _hz_clear_buttons(fig):
    for item in getattr(fig, '_hz_btn_patches', []):
        try: item.remove()
        except Exception: pass
    for item in getattr(fig, '_hz_btn_texts', []):
        try: item.remove()
        except Exception: pass
    fig._hz_btn_patches = []
    fig._hz_btn_texts   = []
    fig._hz_ev_btns     = []   # (Bbox, ev_name)
    fig._hz_bin_btns    = []   # (Bbox, delta)

def plot_event_frequency(ax, df):
    events_all = sorted(df['event'].dropna().unique(), key=lambda x: str(x))
    aid = id(ax)
    if aid not in _hz_active:
        _hz_active[aid] = set(str(e) for e in events_all)
    if aid not in _hz_bin_s:
        _hz_bin_s[aid] = 60.0

    active  = _hz_active[aid]
    bin_s   = _hz_bin_s[aid]
    colors  = _tab20()
    ev_list = [str(e) for e in events_all]
    t_max   = df['elapsed_min'].max() * 60

    if t_max <= 0:
        ax.text(0.5, 0.5, 'No time data', ha='center', va='center',
                transform=ax.transAxes, color=_T['empty'], fontsize=8)
        return

    bins  = np.arange(0, t_max + bin_s, bin_s)
    t_mid = (bins[:-1] + bins[1:]) / 2 / 60

    plotted = False
    for i, ev in enumerate(ev_list):
        if ev not in active:
            continue
        times_s = df[df['event'] == ev]['elapsed_min'].values * 60
        if len(times_s) == 0:
            continue
        counts, _ = np.histogram(times_s, bins=bins)
        hz = counts / bin_s
        ax.plot(t_mid, hz, color=colors[i % len(colors)],
                linewidth=1.4, alpha=0.85, label=ev, zorder=3)
        plotted = True

    if not plotted:
        ax.text(0.5, 0.5, 'No events selected\nClick a button below to add an event',
                ha='center', va='center', transform=ax.transAxes,
                color=_T['empty'], fontsize=9)

    ax.set_xlabel('Elapsed Time (min)', fontsize=8)
    ax.set_ylabel('Frequency (Hz)', fontsize=8)
    ax.set_title(f'Event Frequency  \u2014  bin = {bin_s:.0f} s',
                 fontsize=9, fontweight='bold')
    ax.grid(alpha=0.22, color=_T['ax_grid'], zorder=1)
    _legend(ax, fontsize=7, framealpha=0.85, loc='upper right')

    fig    = ax.get_figure()
    canvas = fig.canvas
    _hz_clear_buttons(fig)
    fig._hz_ax = ax
    fig._hz_df = df

    n_ev = len(ev_list)
    if n_ev == 0:
        return

    ax_pos = ax.get_position()
    BTN_H  = 0.030          # shorter buttons
    PAD    = 0.004
    FS     = 6.5
    # Push buttons well below the x-axis label — use 18% of the bottom margin
    BY     = ax_pos.y0 * 0.28
    if BY < 0.002:
        BY = 0.002

    # Bin control buttons on the right
    BIN_W = 0.058
    bx = ax_pos.x1
    for label, delta in [('bin +5s', 5), ('bin -5s', -5)]:
        bx -= BIN_W + PAD
        pat = matplotlib.patches.FancyBboxPatch(
            (bx, BY), BIN_W, BTN_H,
            boxstyle='round,pad=0.005',
            transform=fig.transFigure, clip_on=False,
            facecolor=_T['ui_widget'], edgecolor=_T['ui_faint'],
            linewidth=0.8, zorder=10)
        fig.add_artist(pat)
        txt = fig.text(bx + BIN_W/2, BY + BTN_H/2, label,
                       ha='center', va='center', fontsize=FS,
                       color=_T['ui_text'],
                       transform=fig.transFigure, clip_on=False, zorder=11)
        fig._hz_btn_patches.append(pat)
        fig._hz_btn_texts.append(txt)
        # Store Bbox in figure fraction for hit-testing
        fig._hz_bin_btns.append((matplotlib.transforms.Bbox([[bx, BY],[bx+BIN_W, BY+BTN_H]]), delta))

    # Event toggle buttons — shrink to fit all without overlap
    avail_w = ax_pos.x1 - (BIN_W + PAD) * 2 - PAD * 2 - ax_pos.x0
    BTN_W   = min(0.09, max(0.032, (avail_w - PAD*(n_ev-1)) / max(n_ev,1)))
    ex = ax_pos.x0

    for j, ev in enumerate(ev_list):
        color     = colors[j % len(colors)]
        is_active = ev in active
        face      = color           if is_active else _T['ui_bg']
        edge      = color
        tc        = _T['ax_bg']     if is_active else color
        fw        = 'bold'          if is_active else 'normal'
        pat = matplotlib.patches.FancyBboxPatch(
            (ex, BY), BTN_W, BTN_H,
            boxstyle='round,pad=0.005',
            transform=fig.transFigure, clip_on=False,
            facecolor=face, edgecolor=edge,
            alpha=0.88, linewidth=1.3, zorder=10)
        fig.add_artist(pat)
        disp = ev if len(ev) <= 13 else ev[:12] + '\u2026'
        txt = fig.text(ex + BTN_W/2, BY + BTN_H/2, disp,
                       ha='center', va='center', fontsize=FS,
                       color=tc, fontweight=fw,
                       transform=fig.transFigure, clip_on=False, zorder=11)
        fig._hz_btn_patches.append(pat)
        fig._hz_btn_texts.append(txt)
        fig._hz_ev_btns.append((matplotlib.transforms.Bbox([[ex, BY],[ex+BTN_W, BY+BTN_H]]), ev))
        ex += BTN_W + PAD

    # Connect button_press_event once per canvas
    if not hasattr(canvas, '_hz_btn_cid'):
        def _hz_press(event):
            fig_ = canvas.figure
            if not hasattr(fig_, '_hz_ev_btns'):
                return
            fw_ = fig_.get_figwidth()  * fig_.dpi
            fh_ = fig_.get_figheight() * fig_.dpi
            if fw_ == 0 or fh_ == 0:
                return
            fx = event.x / fw_
            fy = event.y / fh_
            ax_  = fig_._hz_ax
            df_  = fig_._hz_df
            for bb, ev_name in fig_._hz_ev_btns:
                if bb.contains(fx, fy):
                    act_ = _hz_active.setdefault(id(ax_), set())
                    if ev_name in act_: act_.discard(ev_name)
                    else:               act_.add(ev_name)
                    ax_.clear(); _raster_style_ax(ax_)
                    plot_event_frequency(ax_, df_)
                    canvas.draw_idle(); return
            for bb, delta in fig_._hz_bin_btns:
                if bb.contains(fx, fy):
                    _hz_bin_s[id(ax_)] = max(1.0,
                        _hz_bin_s.get(id(ax_), 60.0) + delta)
                    ax_.clear(); _raster_style_ax(ax_)
                    plot_event_frequency(ax_, df_)
                    canvas.draw_idle(); return
        canvas._hz_btn_cid = canvas.mpl_connect('button_press_event', _hz_press)

PLOT_FNS = [
    ("Event Raster",                plot_event_raster),
    ("Event Counts",                plot_event_counts),
    ("Cumulative Water Intake",     plot_cumulative_water),
    ("Pellet Retrieval Latency",    plot_retrieval_latency),
    ("IEI Distribution",            plot_iei_distribution),
    ("Event Frequency (Hz)",        plot_event_frequency),
]

# ══════════════════════════════════════════════════════════════════════════════
#  MAIN APPLICATION
# ══════════════════════════════════════════════════════════════════════════════

class BNCApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("BNC FED3 Data Analyzer  v3.0")
        self.state("zoomed")

        self.df = None
        self.current_file = None
        self.cfg = load_config()

        # Apply saved theme before anything is drawn
        saved_theme = self.cfg.get("theme", DEFAULT_THEME)
        if saved_theme not in THEMES:
            saved_theme = DEFAULT_THEME
        set_theme(saved_theme)
        self.configure(bg=_T["ui_bg"])

        # Apply ttk style once, before any widgets are created
        self._apply_ttk_style()

        self._build_ui()

        # Auto-load last file
        last = self.cfg.get("last_file")
        if last and os.path.exists(last):
            self._do_load(last)

    # ── ttk style ─────────────────────────────────────────────────────────────
    def _apply_ttk_style(self):
        """Apply ttk style for the current theme. Call before/after widget creation."""
        t = _T
        style = ttk.Style(self)
        style.theme_use("clam")

        style.configure("TCombobox",
                         relief="flat",
                         padding=4,
                         fieldbackground=t["ui_widget"],
                         background=t["ui_widget"],
                         foreground=t["ui_text"],
                         selectbackground=t["ui_widget"],
                         selectforeground=t["ui_text"],
                         arrowcolor=t["ui_text"],
                         bordercolor=t["ui_faint"],
                         lightcolor=t["ui_widget"],
                         darkcolor=t["ui_widget"])

        style.map("TCombobox",
                  fieldbackground=[("readonly", t["ui_widget"]),
                                   ("focus",    t["ui_widget"]),
                                   ("active",   t["ui_widget"])],
                  foreground=[("readonly",  t["ui_text"]),
                               ("focus",    t["ui_text"]),
                               ("active",   t["ui_text"]),
                               ("disabled", t["ui_sub"])],
                  selectbackground=[("readonly", t["ui_widget"]),
                                    ("focus",    t["ui_hover"])],
                  selectforeground=[("readonly", t["ui_text"]),
                                    ("focus",    t["ui_text"])],
                  background=[("active",   t["ui_hover"]),
                               ("pressed",  t["ui_hover"]),
                               ("readonly", t["ui_widget"])],
                  arrowcolor=[("readonly", t["ui_text"]),
                               ("active",  t["ui_text"]),
                               ("disabled", t["ui_sub"])])

        style.configure("TSeparator", background=t["ui_faint"])

        # Dropdown popup listbox — must use option_add on root
        self.option_add("*TCombobox*Listbox.background",       t["ui_widget"])
        self.option_add("*TCombobox*Listbox.foreground",       t["ui_text"])
        self.option_add("*TCombobox*Listbox.selectBackground", t["ui_hover"])
        self.option_add("*TCombobox*Listbox.selectForeground", t["ui_text"])
        self.option_add("*TCombobox*Listbox.relief",           "flat")
        self.option_add("*TCombobox*Listbox.borderWidth",      "1")

        # Notebook (tabs)
        style.configure("TNotebook",
                         background=t["ui_bg"],
                         borderwidth=0,
                         tabmargins=0)
        style.configure("TNotebook.Tab",
                         background=t["ui_widget"],
                         foreground=t["ui_text"],
                         padding=[14, 5],
                         font=("Segoe UI", 9),
                         borderwidth=0)
        style.map("TNotebook.Tab",
                  background=[("selected", t["ui_panel"]),
                               ("active",  t["ui_hover"])],
                  foreground=[("selected", t["ui_text"]),
                               ("active",  t["ui_text"])])

        # Treeview (data grid)
        style.configure("Treeview",
                         background=t["ui_bg"],
                         foreground=t["ui_text"],
                         fieldbackground=t["ui_bg"],
                         rowheight=20,
                         font=("Segoe UI", 8),
                         borderwidth=0,
                         relief="flat")
        style.configure("Treeview.Heading",
                         background=t["ui_widget"],
                         foreground=t["ui_text"],
                         font=("Segoe UI", 8, "bold"),
                         relief="flat",
                         borderwidth=1)
        style.map("Treeview",
                  background=[("selected", t["ui_hover"])],
                  foreground=[("selected", t["ui_text"])])
        style.map("Treeview.Heading",
                  background=[("active", t["ui_hover"])],
                  foreground=[("active", t["ui_text"])])

    # ── Build UI ───────────────────────────────────────────────────────────────
    def _build_ui(self):
        self.paned = tk.PanedWindow(self, orient=tk.HORIZONTAL,
                                    bg=_T["ui_bg"], sashwidth=6,
                                    sashrelief="flat", sashpad=0)
        self.paned.pack(fill=tk.BOTH, expand=True)

        # ── Left sidebar (1/4) ─────────────────────────────────────────────
        self.sidebar = tk.Frame(self.paned, bg=_T["ui_panel"], width=272)
        self.paned.add(self.sidebar, minsize=200, width=272)
        self._build_sidebar()

        # ── Right side: Notebook with Plots + Data tabs ────────────────────
        self.right_frame = tk.Frame(self.paned, bg=_T["ui_bg"])
        self.paned.add(self.right_frame, stretch="always")
        self.right_frame.rowconfigure(0, weight=1)
        self.right_frame.columnconfigure(0, weight=1)
        self._build_notebook()

    def _build_notebook(self):
        """Create (or recreate) the tabbed notebook."""
        for w in self.right_frame.winfo_children():
            w.destroy()

        t = _T
        self.notebook = ttk.Notebook(self.right_frame)
        self.notebook.grid(row=0, column=0, sticky="nsew")

        # ── Tab 1: Plots ───────────────────────────────────────────────────
        self.plot_tab = tk.Frame(self.notebook, bg=t["ui_bg"])
        self.plot_tab.rowconfigure(0, weight=1)
        self.plot_tab.rowconfigure(1, weight=1)
        self.plot_tab.columnconfigure(0, weight=1)
        self.plot_tab.columnconfigure(1, weight=1)
        self.notebook.add(self.plot_tab, text="  📊  Plots  ")

        # plot_area is the old name referenced by _build_plots
        self.plot_area = self.plot_tab
        self._build_plots()

        # ── Tab 2: Data ────────────────────────────────────────────────────
        self.data_tab = tk.Frame(self.notebook, bg=t["ui_bg"])
        self.data_tab.rowconfigure(0, weight=1)
        self.data_tab.columnconfigure(0, weight=1)
        self.notebook.add(self.data_tab, text="  📋  Data  ")
        self._build_data_tab()

    # ── Sidebar ────────────────────────────────────────────────────────────────
    def _build_sidebar(self):
        """Build (or rebuild) all sidebar content."""
        for w in self.sidebar.winfo_children():
            w.destroy()

        t = _T
        sb = self.sidebar

        # Title
        tk.Label(sb, text="BNC Analyzer", bg=t["ui_panel"], fg=t["ui_text"],
                 font=("Segoe UI", 13, "bold")).pack(pady=(16, 2))
        tk.Label(sb, text="FED3 Event Explorer  v3.0", bg=t["ui_panel"],
                 fg=t["ui_sub"], font=("Segoe UI", 8)).pack(pady=(0, 10))

        ttk.Separator(sb).pack(fill=tk.X, padx=14, pady=4)

        # ── File ──────────────────────────────────────────────────────────
        tk.Label(sb, text="DATA FILE", bg=t["ui_panel"], fg=t["ui_sub"],
                 font=("Segoe UI", 8, "bold")).pack(anchor="w", padx=16, pady=(8, 2))

        fname = os.path.basename(self.current_file) if self.current_file else "No file loaded"
        flbl_fg = t["ui_text"] if self.current_file else t["ui_sub"]
        self.file_label = tk.Label(sb, text=fname, bg=t["ui_panel"], fg=flbl_fg,
                                   font=("Segoe UI", 8), wraplength=234, justify="left")
        self.file_label.pack(anchor="w", padx=16, pady=(0, 6))

        bf = tk.Frame(sb, bg=t["ui_panel"])
        bf.pack(fill=tk.X, padx=16)
        self._btn(bf, "📂  Import CSV", self._browse_file).pack(fill=tk.X, pady=2)
        self._btn(bf, "🔄  Reload",     self._reload).pack(fill=tk.X, pady=2)

        ttk.Separator(sb).pack(fill=tk.X, padx=14, pady=10)

        # ── Session info ──────────────────────────────────────────────────
        tk.Label(sb, text="SESSION INFO", bg=t["ui_panel"], fg=t["ui_sub"],
                 font=("Segoe UI", 8, "bold")).pack(anchor="w", padx=16, pady=(0, 3))
        self.info_text = tk.Text(sb, bg=t["ui_bg"], fg=t["ui_text"],
                                  font=("Courier New", 7), height=14, width=30,
                                  relief="flat", bd=1, padx=5, pady=5,
                                  state="disabled",
                                  insertbackground=t["ui_text"],
                                  selectbackground=t["ui_hover"],
                                  selectforeground=t["ui_text"])
        self.info_text.pack(padx=16, pady=(0, 8), fill=tk.X)

        ttk.Separator(sb).pack(fill=tk.X, padx=14, pady=4)

        # ── Export ────────────────────────────────────────────────────────
        tk.Label(sb, text="EXPORT", bg=t["ui_panel"], fg=t["ui_sub"],
                 font=("Segoe UI", 8, "bold")).pack(anchor="w", padx=16, pady=(6, 3))
        self._btn(sb, "💾  Save All Plots",   self._save_all).pack(fill=tk.X, padx=16, pady=2)
        self._btn(sb, "🖼️  Save Individual…", self._save_individual).pack(fill=tk.X, padx=16, pady=2)

        ttk.Separator(sb).pack(fill=tk.X, padx=14, pady=8)

        # ── Theme ─────────────────────────────────────────────────────────
        tk.Label(sb, text="THEME", bg=t["ui_panel"], fg=t["ui_sub"],
                 font=("Segoe UI", 8, "bold")).pack(anchor="w", padx=16, pady=(2, 4))

        self.theme_var = tk.StringVar(value=self.cfg.get("theme", DEFAULT_THEME))
        self.theme_combo = ttk.Combobox(sb, textvariable=self.theme_var,
                                         values=THEME_NAMES, state="readonly",
                                         font=("Segoe UI", 9), height=14)
        self.theme_combo.pack(fill=tk.X, padx=16, pady=(0, 6))
        self.theme_combo.bind("<<ComboboxSelected>>", self._on_theme_change)

        # Spacer + version
        tk.Frame(sb, bg=t["ui_panel"]).pack(fill=tk.BOTH, expand=True)
        tk.Label(sb, text="BNC • FED3 Analyzer v3.0", bg=t["ui_panel"],
                 fg=t["ui_faint"], font=("Segoe UI", 7)).pack(pady=6)

        # Restore info if data already loaded
        if self.df is not None:
            self._update_info()

    def _btn(self, parent, text, cmd):
        t = _T
        b = tk.Button(parent, text=text, command=cmd,
                      bg=t["ui_widget"], fg=t["ui_text"],
                      font=("Segoe UI", 9), relief="flat",
                      cursor="hand2", padx=8, pady=5, bd=0,
                      activebackground=t["ui_hover"],
                      activeforeground=t["ui_text"])
        return b

    # ── Plot panels ────────────────────────────────────────────────────────────
    def _build_plots(self):
        """Build (or rebuild) the 4 matplotlib panel cells with per-panel dropdown."""
        for w in self.plot_area.winfo_children():
            w.destroy()
        self.figs       = []
        self.axes       = []
        self.canvases   = []
        self.panel_vars = []

        PLOT_NAMES = [n for n, _ in PLOT_FNS]
        saved = self.cfg.get("panel_selections", PLOT_NAMES[:])

        for idx in range(4):
            row, col = divmod(idx, 2)
            t = _T

            cell = tk.Frame(self.plot_area, bg=t["ui_bg"],
                            highlightbackground=t["ui_widget"],
                            highlightthickness=1)
            cell.grid(row=row, column=col, sticky="nsew", padx=3, pady=3)
            cell.rowconfigure(1, weight=1)
            cell.rowconfigure(2, weight=0)
            cell.columnconfigure(0, weight=1)

            # ── Header bar: "Panel N" label + plot selector dropdown + fullscreen ──
            bar = tk.Frame(cell, bg=t["ui_bar"])
            bar.grid(row=0, column=0, sticky="ew")
            bar.columnconfigure(1, weight=1)

            tk.Label(bar, text=f"Panel {idx+1}", bg=t["ui_bar"],
                     fg=t["ui_bar_lbl"], font=("Segoe UI", 7, "bold"),
                     padx=6).grid(row=0, column=0, pady=2)

            default = saved[idx] if idx < len(saved) else PLOT_NAMES[idx % len(PLOT_NAMES)]
            if default not in PLOT_NAMES:
                default = PLOT_NAMES[idx % len(PLOT_NAMES)]
            var = tk.StringVar(value=default)
            self.panel_vars.append(var)

            combo = ttk.Combobox(bar, textvariable=var, values=PLOT_NAMES,
                                  state="readonly", font=("Segoe UI", 8),
                                  height=len(PLOT_NAMES))
            combo.grid(row=0, column=1, sticky="ew", padx=4, pady=2)
            combo.bind("<<ComboboxSelected>>",
                       lambda e, i=idx: self._panel_changed(i))

            # Fullscreen button
            tk.Button(bar, text="⛶", bg=t["ui_bar"], fg=t["ui_bar_lbl"],
                      font=("Segoe UI", 10), relief="flat", bd=0,
                      cursor="hand2", padx=6, pady=1,
                      activebackground=t["ui_hover"],
                      activeforeground=t["ui_text"],
                      command=lambda i=idx: self._fullscreen_panel(i)
                      ).grid(row=0, column=2, pady=2, padx=(0, 4))

            # ── Matplotlib figure ──────────────────────────────────────────
            fig = Figure(facecolor=t["fig_bg"])
            fig.subplots_adjust(left=0.11, right=0.97, top=0.91, bottom=0.15)
            ax = fig.add_subplot(111, facecolor=t["ax_bg"])
            self._style_ax(ax)

            canvas = FigureCanvasTkAgg(fig, master=cell)
            canvas.get_tk_widget().grid(row=1, column=0, sticky="nsew")

            # Per-cell navigation toolbar (zoom / pan / home)
            toolbar_frame = tk.Frame(cell, bg=t["ui_bar"])
            toolbar_frame.grid(row=2, column=0, sticky="ew")
            toolbar = NavigationToolbar2Tk(canvas, toolbar_frame, pack_toolbar=False)
            toolbar.config(bg=t["ui_bar"])
            for child in toolbar.winfo_children():
                try:
                    child.config(bg=t["ui_bar"], activebackground=t["ui_hover"],
                                 fg=t["ui_text"], highlightbackground=t["ui_bar"],
                                 relief="flat", bd=0)
                except Exception:
                    pass
            toolbar.update()
            toolbar.pack(side=tk.LEFT, fill=tk.X)

            self.figs.append(fig)
            self.axes.append(ax)
            self.canvases.append(canvas)

        self._draw_empty_all()

    def _panel_changed(self, idx):
        """Redraw one panel after its dropdown selection changes."""
        self.cfg["panel_selections"] = [v.get() for v in self.panel_vars]
        save_config(self.cfg)
        self._draw_panel(idx)

    def _draw_panel(self, idx):
        """Draw a single panel using its currently selected plot function."""
        if self.df is None:
            return
        name   = self.panel_vars[idx].get()
        fn_map = {n: f for n, f in PLOT_FNS}
        fn     = fn_map.get(name)
        if fn is None:
            return
        fig = self.figs[idx]
        ax  = self.axes[idx]
        # Always clear Hz buttons - they belong only to the Hz plot
        _hz_clear_buttons(fig)
        fig.set_facecolor(_T["fig_bg"])
        ax.clear()
        self._style_ax(ax)
        try:
            fn(ax, self.df)
        except Exception as e:
            ax.text(0.5, 0.5, f"Error:\n{e}", ha="center", va="center",
                    transform=ax.transAxes, color="#ef5350", fontsize=8)
        self._style_legend(ax)
        self.canvases[idx].draw()

    def _fullscreen_panel(self, idx):
        if self.df is None:
            return
        t = _T
        name = self.panel_vars[idx].get()
        fn_map = {n: f for n, f in PLOT_FNS}
        fn = fn_map.get(name)
        if fn is None:
            return

        win = tk.Toplevel(self)
        win.title(name)
        win.configure(bg=t["fig_bg"])
        win.state("zoomed")
        win.focus_set()

        win.rowconfigure(0, weight=0)
        win.rowconfigure(1, weight=1)
        win.rowconfigure(2, weight=0)
        win.columnconfigure(0, weight=1)

        # Title bar
        title_bar = tk.Frame(win, bg=t["ui_bar"])
        title_bar.grid(row=0, column=0, sticky="ew")
        title_bar.columnconfigure(0, weight=1)

        tk.Label(title_bar, text=name, bg=t["ui_bar"], fg=t["ui_text"],
                 font=("Segoe UI", 11, "bold"), padx=14, pady=6
                 ).grid(row=0, column=0, sticky="w")

        tk.Button(
            title_bar, text="✕  Close",
            bg=t["ui_widget"], fg=t["ui_text"],
            font=("Segoe UI", 9), relief="flat", bd=0,
            cursor="hand2", padx=12, pady=5,
            activebackground=t["ui_hover"],
            activeforeground=t["ui_text"],
            command=win.destroy
        ).grid(row=0, column=1, padx=10, pady=4)

        win.bind("<Escape>", lambda e: win.destroy())

        # Figure with larger fonts
        fig = Figure(facecolor=t["fig_bg"])
        fig.subplots_adjust(left=0.07, right=0.97, top=0.94, bottom=0.12)
        ax = fig.add_subplot(111, facecolor=t["ax_bg"])
        self._style_ax(ax)
        try:
            fn(ax, self.df)
        except Exception as e:
            ax.text(0.5, 0.5, f"Error:\n{e}", ha="center", va="center",
                    transform=ax.transAxes, color="#ef5350", fontsize=10)
        self._style_legend(ax)

        ax.title.set_fontsize(14)
        ax.xaxis.label.set_fontsize(11)
        ax.yaxis.label.set_fontsize(11)
        ax.tick_params(labelsize=10)
        leg = ax.get_legend()
        if leg:
            for txt in leg.get_texts():
                txt.set_fontsize(10)

        canvas = FigureCanvasTkAgg(fig, master=win)
        canvas.get_tk_widget().grid(row=1, column=0, sticky="nsew")

        # Toolbar
        tb_frame = tk.Frame(win, bg=t["ui_bar"])
        tb_frame.grid(row=2, column=0, sticky="ew")
        toolbar = NavigationToolbar2Tk(canvas, tb_frame, pack_toolbar=False)
        toolbar.config(bg=t["ui_bar"])
        for child in toolbar.winfo_children():
            try:
                child.config(bg=t["ui_bar"], activebackground=t["ui_hover"],
                             fg=t["ui_text"], highlightbackground=t["ui_bar"],
                             relief="flat", bd=0)
            except Exception:
                pass
        toolbar.update()
        toolbar.pack(side=tk.LEFT, fill=tk.X)

        canvas.draw()

    # ── Data tab (canvas-based spreadsheet) ───────────────────────────────────
    #
    #  Pure tk.Canvas renderer.
    #  • Instant scroll via mouse-wheel and scrollbars
    #  • Instant zoom via buttons / Ctrl+wheel
    #  • Frozen header canvas above body canvas
    #  • Column divider lines + row band shading
    #
    _GRID_BASE_FONT = 9
    _GRID_ZOOM_MIN  = 7
    _GRID_ZOOM_MAX  = 24
    _GRID_PAD_X     = 6
    _GRID_PAD_Y     = 3

    def _build_data_tab(self):
        for w in self.data_tab.winfo_children():
            w.destroy()
        t = _T

        # Persistent state – survive theme rebuilds
        self._grid_font_size  = getattr(self, "_grid_font_size",  self._GRID_BASE_FONT)
        self._grid_scroll_x   = 0.0
        self._grid_scroll_y   = 0.0
        self._grid_rows_cache = []
        self._grid_col_names  = []
        self._grid_col_widths = []
        self._grid_row_h      = 0

        # ── data_tab must fill its notebook cell ──────────────────────────
        self.data_tab.rowconfigure(0, weight=1)
        self.data_tab.columnconfigure(0, weight=1)

        # ── Outer frame fills data_tab ────────────────────────────────────
        outer = tk.Frame(self.data_tab, bg=t["ui_bg"])
        outer.grid(row=0, column=0, sticky="nsew")
        # row 0 = toolbar  (fixed height)
        # row 1 = header   (fixed height, set dynamically)
        # row 2 = body     (expands)
        # row 3 = h-scroll (fixed height)
        outer.rowconfigure(0, weight=0)
        outer.rowconfigure(1, weight=0)
        outer.rowconfigure(2, weight=1)
        outer.rowconfigure(3, weight=0)
        outer.columnconfigure(0, weight=1)
        outer.columnconfigure(1, weight=0)

        # ── Toolbar ───────────────────────────────────────────────────────
        bar = tk.Frame(outer, bg=t["ui_bar"], pady=3)
        bar.grid(row=0, column=0, columnspan=2, sticky="ew")

        tk.Label(bar, text="Zoom:", bg=t["ui_bar"], fg=t["ui_bar_lbl"],
                 font=("Segoe UI", 8)).pack(side=tk.LEFT, padx=(10, 4))

        for txt, cmd in [("−", self._grid_zoom_out),
                          ("+", self._grid_zoom_in)]:
            tk.Button(bar, text=txt, width=2,
                      bg=t["ui_widget"], fg=t["ui_text"],
                      font=("Segoe UI", 10, "bold"), relief="flat", bd=0,
                      cursor="hand2", activebackground=t["ui_hover"],
                      activeforeground=t["ui_text"],
                      command=cmd).pack(side=tk.LEFT, padx=1)

        self._zoom_label = tk.Label(bar, text=f"{self._grid_font_size}pt",
                                     bg=t["ui_bar"], fg=t["ui_text"],
                                     font=("Segoe UI", 8), width=4)
        self._zoom_label.pack(side=tk.LEFT)

        tk.Button(bar, text="Reset",
                  bg=t["ui_widget"], fg=t["ui_text"],
                  font=("Segoe UI", 8), relief="flat", bd=0,
                  cursor="hand2", activebackground=t["ui_hover"],
                  activeforeground=t["ui_text"],
                  command=self._grid_zoom_reset).pack(side=tk.LEFT, padx=(4, 0))

        self._grid_info_lbl = tk.Label(bar, text="",
                                        bg=t["ui_bar"], fg=t["ui_bar_lbl"],
                                        font=("Segoe UI", 8))
        self._grid_info_lbl.pack(side=tk.RIGHT, padx=10)

        # ── Header canvas ─────────────────────────────────────────────────
        # height will be set dynamically in _grid_redraw
        self._hdr_canvas = tk.Canvas(outer, bg=t["ui_widget"],
                                      highlightthickness=0, height=24)
        self._hdr_canvas.grid(row=1, column=0, columnspan=1, sticky="ew")

        # ── Body canvas ───────────────────────────────────────────────────
        self._grid_canvas = tk.Canvas(outer, bg=t["ui_bg"],
                                       highlightthickness=0)
        self._grid_canvas.grid(row=2, column=0, sticky="nsew")

        # ── Scrollbars ────────────────────────────────────────────────────
        vsb = ttk.Scrollbar(outer, orient="vertical",   command=self._grid_vscroll)
        hsb = ttk.Scrollbar(outer, orient="horizontal", command=self._grid_hscroll)
        vsb.grid(row=2, column=1, sticky="ns")
        hsb.grid(row=3, column=0, sticky="ew")
        self._grid_vsb = vsb
        self._grid_hsb = hsb

        # ── Bindings ──────────────────────────────────────────────────────
        gc = self._grid_canvas
        gc.bind("<MouseWheel>",         lambda e: self._grid_wheel(e, "y"))
        gc.bind("<Shift-MouseWheel>",   lambda e: self._grid_wheel(e, "x"))
        gc.bind("<Button-4>",           lambda e: self._grid_wheel(e, "y",  120))
        gc.bind("<Button-5>",           lambda e: self._grid_wheel(e, "y", -120))
        gc.bind("<Control-MouseWheel>", self._grid_ctrl_wheel)
        gc.bind("<Configure>",          lambda e: self._grid_redraw())

        if self.df is not None:
            self._refresh_data_table()
        else:
            self._grid_draw_placeholder()

    # ── Cell formatting ───────────────────────────────────────────────────────
    def _fmt_cell(self, val, col_name):
        import math
        if val is None or (isinstance(val, float) and math.isnan(val)):
            return ""
        cl = col_name.lower()
        if cl in ("time", "date", "datetime"):
            try:
                ts = pd.Timestamp(val)
                return ts.strftime("%m/%d/%y") if cl == "date" else ts.strftime("%H:%M:%S")
            except Exception:
                return str(val)
        if isinstance(val, float):
            return str(int(val)) if val == int(val) else f"{val:.4f}"
        if isinstance(val, int):
            return str(val)
        return str(val)

    # ── Data population ───────────────────────────────────────────────────────
    def _refresh_data_table(self):
        if self.df is None:
            self._grid_draw_placeholder()
            return
        df   = self.df
        cols = [c for c in df.columns if c != "elapsed_min"]
        self._grid_col_names  = cols
        self._grid_rows_cache = [
            [self._fmt_cell(row[c], c) for c in cols]
            for _, row in df.iterrows()
        ]
        nr, nc = len(self._grid_rows_cache), len(cols)
        if hasattr(self, "_grid_info_lbl"):
            self._grid_info_lbl.config(text=f"{nr} rows × {nc} cols")
        self._grid_scroll_x = 0.0
        self._grid_scroll_y = 0.0
        self._grid_measure_cols()
        self._grid_redraw()

    def _grid_measure_cols(self):
        """Compute column widths using real font metrics at current font size."""
        import tkinter.font as tkfont
        cols = self._grid_col_names
        rows = self._grid_rows_cache
        fs   = max(self._GRID_ZOOM_MIN, min(self._GRID_ZOOM_MAX, self._grid_font_size))
        pad  = self._GRID_PAD_X * 2
        try:
            hf = tkfont.Font(family="Segoe UI", size=fs, weight="bold")
            bf = tkfont.Font(family="Segoe UI", size=fs)
            widths = []
            for i, col in enumerate(cols):
                w = hf.measure(col) + pad + 4
                for row in rows[:80]:
                    cw = bf.measure(row[i]) + pad
                    if cw > w:
                        w = cw
                widths.append(max(48, min(280, w)))
            self._grid_col_widths = widths
        except Exception:
            px = max(6, int(fs * 1.35))
            self._grid_col_widths = [
                max(48, min(280,
                    max(len(col)*px,
                        max((len(r[i])*px for r in rows[:80]), default=0)) + pad))
                for i, col in enumerate(cols)
            ]

    # ── Zoom (instant) ────────────────────────────────────────────────────────
    def _grid_zoom_in(self):
        self._grid_font_size = min(self._GRID_ZOOM_MAX, self._grid_font_size + 1)
        self._zoom_label.config(text=f"{self._grid_font_size}pt")
        self._grid_measure_cols()
        self._grid_redraw()

    def _grid_zoom_out(self):
        self._grid_font_size = max(self._GRID_ZOOM_MIN, self._grid_font_size - 1)
        self._zoom_label.config(text=f"{self._grid_font_size}pt")
        self._grid_measure_cols()
        self._grid_redraw()

    def _grid_zoom_reset(self):
        self._grid_font_size = self._GRID_BASE_FONT
        self._zoom_label.config(text=f"{self._grid_font_size}pt")
        self._grid_measure_cols()
        self._grid_redraw()

    def _grid_ctrl_wheel(self, event):
        d = getattr(event, "delta", 0)
        if d > 0:
            self._grid_zoom_in()
        elif d < 0:
            self._grid_zoom_out()

    # ── Scroll callbacks (from scrollbar widgets) ─────────────────────────────
    def _grid_total_w(self):
        return sum(self._grid_col_widths) if self._grid_col_widths else 0

    def _grid_total_h(self):
        return self._grid_row_h * len(self._grid_rows_cache)

    def _grid_vscroll(self, *args):
        total = self._grid_total_h()
        vh    = self._grid_canvas.winfo_height() or 400
        if total <= vh:
            return
        if args[0] == "moveto":
            self._grid_scroll_y = float(args[1]) * total
        elif args[0] == "scroll":
            step = self._grid_row_h if args[2] == "units" else vh
            self._grid_scroll_y += int(args[1]) * step
        self._grid_scroll_y = max(0.0, min(self._grid_scroll_y, float(total - vh)))
        self._grid_redraw()

    def _grid_hscroll(self, *args):
        total = self._grid_total_w()
        vw    = self._grid_canvas.winfo_width() or 800
        if total <= vw:
            return
        if args[0] == "moveto":
            self._grid_scroll_x = float(args[1]) * total
        elif args[0] == "scroll":
            step = 40 if args[2] == "units" else vw
            self._grid_scroll_x += int(args[1]) * step
        self._grid_scroll_x = max(0.0, min(self._grid_scroll_x, float(total - vw)))
        self._grid_redraw()

    def _grid_wheel(self, event, axis="y", delta=None):
        """Mouse-wheel: instant scroll."""
        d = delta if delta is not None else getattr(event, "delta", 0)
        # Normalise delta (Windows ±120, Linux ±1)
        pixels = -(d / 120.0) * self._grid_row_h * 3
        if axis == "y":
            total = self._grid_total_h()
            vh    = self._grid_canvas.winfo_height() or 400
            self._grid_scroll_y = max(0.0, min(float(max(0, total - vh)),
                                               self._grid_scroll_y + pixels))
        else:
            total = self._grid_total_w()
            vw    = self._grid_canvas.winfo_width() or 800
            self._grid_scroll_x = max(0.0, min(float(max(0, total - vw)),
                                               self._grid_scroll_x + pixels))
        self._grid_redraw()

    # ── Drawing ───────────────────────────────────────────────────────────────
    def _grid_draw_placeholder(self):
        if not hasattr(self, "_grid_canvas"):
            return
        c = self._grid_canvas
        c.delete("all")
        t = _T
        c.config(bg=t["ui_bg"])
        w = c.winfo_width()  or 600
        h = c.winfo_height() or 300
        c.create_text(w // 2, h // 2,
                       text="Load a CSV file to view data",
                       fill=t["empty"], font=("Segoe UI", 11))
        if hasattr(self, "_hdr_canvas"):
            self._hdr_canvas.delete("all")
            self._hdr_canvas.config(height=24)

    def _grid_redraw(self):
        """Redraw both canvases from current scroll/zoom state."""
        if not hasattr(self, "_grid_canvas") or not self._grid_col_widths:
            return

        import tkinter.font as tkfont

        t     = _T
        fs    = max(self._GRID_ZOOM_MIN,
                    min(self._GRID_ZOOM_MAX, self._grid_font_size))
        pad_x = self._GRID_PAD_X
        pad_y = self._GRID_PAD_Y
        cols  = self._grid_col_names
        rows  = self._grid_rows_cache
        cw    = self._grid_col_widths

        try:
            hdr_font  = tkfont.Font(family="Segoe UI", size=fs, weight="bold")
            body_font = tkfont.Font(family="Segoe UI", size=fs)
            rh = body_font.metrics("linespace") + pad_y * 2 + 2
        except Exception:
            hdr_font = body_font = ("Segoe UI", fs)
            rh = fs + pad_y * 2 + 6
        rh = max(rh, 18)
        self._grid_row_h = rh

        total_w = sum(cw)
        total_h = rh * len(rows)
        sx = int(self._grid_scroll_x)
        sy = int(self._grid_scroll_y)

        bc  = self._grid_canvas
        hc  = self._hdr_canvas
        vw  = bc.winfo_width()  or 800
        vh  = bc.winfo_height() or 400
        div = t["ax_spine"]

        # ── Header (frozen, X-scrolls with body) ──────────────────────────
        hc.config(bg=t["ui_widget"], height=rh)
        hc.delete("all")
        hc.create_rectangle(0, 0, max(vw, total_w), rh,
                             fill=t["ui_widget"], outline="")
        x = -sx
        for i, col in enumerate(cols):
            x0, x1 = x, x + cw[i]
            if x1 > 0 and x0 < vw:
                hc.create_text(x0 + pad_x, rh // 2,
                                text=col, anchor="w",
                                font=hdr_font, fill=t["ui_text"])
            hc.create_line(x1, 0, x1, rh, fill=div, width=1)
            x = x1
        # solid bottom border on header
        hc.create_line(0, rh - 1, vw, rh - 1, fill=div, width=2)

        # ── Body ──────────────────────────────────────────────────────────
        bc.config(bg=t["ui_bg"])
        bc.delete("all")

        first_row = max(0, sy // rh)
        last_row  = min(len(rows), first_row + (vh // rh) + 2)

        for ri in range(first_row, last_row):
            y0 = ri * rh - sy
            y1 = y0 + rh
            bg = t["ui_bg"] if ri % 2 == 0 else t["ui_widget"]
            bc.create_rectangle(0, y0, vw, y1, fill=bg, outline="")
            bc.create_line(0, y1, vw, y1, fill=div, width=1)

            x = -sx
            row_data = rows[ri]
            for ci in range(len(cols)):
                x0, x1 = x, x + cw[ci]
                if x1 > 0 and x0 < vw:
                    bc.create_text(x0 + pad_x, y0 + rh // 2,
                                    text=row_data[ci], anchor="w",
                                    font=body_font, fill=t["ui_text"])
                bc.create_line(x1, y0, x1, y1, fill=div, width=1)
                x = x1

        # ── Scrollbars ────────────────────────────────────────────────────
        if total_h > vh:
            self._grid_vsb.set(sy / total_h,
                                min(1.0, (sy + vh) / total_h))
        else:
            self._grid_vsb.set(0.0, 1.0)

        if total_w > vw:
            self._grid_hsb.set(sx / total_w,
                                min(1.0, (sx + vw) / total_w))
        else:
            self._grid_hsb.set(0.0, 1.0)

    # ── Axis styling ───────────────────────────────────────────────────────────
    def _style_ax(self, ax):
        t = _T
        ax.set_facecolor(t["ax_bg"])
        for spine in ax.spines.values():
            spine.set_edgecolor(t["ax_spine"])
        ax.tick_params(colors=t["ax_tick"], labelsize=7)
        ax.xaxis.label.set_color(t["ax_lbl"])
        ax.yaxis.label.set_color(t["ax_lbl"])
        ax.title.set_color(t["ax_title"])

    def _style_legend(self, ax):
        leg = ax.get_legend()
        if leg:
            leg.get_frame().set_facecolor(_T["leg_bg"])
            leg.get_frame().set_edgecolor(_T["leg_edge"])
            for txt in leg.get_texts():
                txt.set_color(_T["leg_txt"])

    # ── Empty / draw ───────────────────────────────────────────────────────────
    def _draw_empty_all(self):
        for i, ax in enumerate(self.axes):
            ax.clear()
            self._style_ax(ax)
            ax.text(0.5, 0.5, "Load a CSV to view plots",
                    ha="center", va="center",
                    color=_T["empty"], fontsize=9,
                    transform=ax.transAxes)
            self.canvases[i].draw()

    def _draw_all(self):
        if self.df is None:
            return
        for i in range(4):
            self._draw_panel(i)
        self.cfg["panel_selections"] = [v.get() for v in self.panel_vars]
        save_config(self.cfg)


    # ── Theme change ───────────────────────────────────────────────────────────
    def _on_theme_change(self, _event=None):
        name = self.theme_var.get()
        set_theme(name)
        self.cfg["theme"] = name
        save_config(self.cfg)
        self._apply_theme_to_ui()

    def _apply_theme_to_ui(self):
        t = _T
        self.configure(bg=t["ui_bg"])
        self.paned.configure(bg=t["ui_bg"])
        self.right_frame.configure(bg=t["ui_bg"])
        self.sidebar.configure(bg=t["ui_panel"])

        # Re-apply ttk style FIRST so new widgets get correct colours
        self._apply_ttk_style()

        # Rebuild sidebar, notebook (plots + data tab)
        self._build_sidebar()
        self._build_notebook()

        if self.df is not None:
            self._draw_all()
            self._refresh_data_table()

    # ── File ops ───────────────────────────────────────────────────────────────
    def _browse_file(self):
        initial = os.path.dirname(self.cfg.get("last_file", os.path.expanduser("~")))
        path = filedialog.askopenfilename(
            title="Select BNC CSV file",
            initialdir=initial,
            filetypes=[("CSV files", "*.csv"), ("All files", "*.*")])
        if path:
            self._do_load(path)

    def _reload(self):
        if self.current_file:
            self._do_load(self.current_file)
        else:
            messagebox.showinfo("No File", "Please import a CSV file first.")

    def _do_load(self, path):
        try:
            df = load_csv(path)
            self.df = df
            self.current_file = path
            self.cfg["last_file"] = path
            save_config(self.cfg)
            self.file_label.config(text=os.path.basename(path),
                                   fg=_T["ui_text"])
            self._update_info()
            self._draw_all()
            self._refresh_data_table()
        except Exception as e:
            messagebox.showerror("Load Error", f"Failed to load file:\n{e}")

    # ── Session info ───────────────────────────────────────────────────────────
    def _update_info(self):
        df = self.df
        if df is None:
            return
        t0, t1 = df["datetime"].iloc[0], df["datetime"].iloc[-1]
        dur = (t1 - t0).total_seconds() / 60
        lines = [
            f"Start:    {t0.strftime('%m/%d/%y %H:%M')}",
            f"End:      {t1.strftime('%m/%d/%y %H:%M')}",
            f"Duration: {dur:.1f} min",
            f"Events:   {len(df)}",
            "",
            "── Event Counts ──────",
        ]
        for ev, cnt in df["event"].value_counts().items():
            lines.append(f"  {ev:<22} {cnt:>3}")
        self.info_text.config(state="normal",
                              bg=_T["ui_bg"], fg=_T["ui_text"])
        self.info_text.delete("1.0", tk.END)
        self.info_text.insert(tk.END, "\n".join(lines))
        self.info_text.config(state="disabled")

    # ── Save ───────────────────────────────────────────────────────────────────
    def _styled_fig_for_export(self):
        """Return (fig, axes_flat) with theme styling applied, plots drawn."""
        t = _T
        fig, axes = plt.subplots(2, 2, figsize=(14, 9), facecolor=t["fig_bg"])
        fig.subplots_adjust(hspace=0.42, wspace=0.30,
                            left=0.07, right=0.97, top=0.95, bottom=0.07)
        for i, (ax, (_title, fn)) in enumerate(zip(axes.flat, PLOT_FNS)):
            ax.set_facecolor(t["ax_bg"])
            for spine in ax.spines.values():
                spine.set_edgecolor(t["ax_spine"])
            ax.tick_params(colors=t["ax_tick"], labelsize=7)
            ax.xaxis.label.set_color(t["ax_lbl"])
            ax.yaxis.label.set_color(t["ax_lbl"])
            ax.title.set_color(t["ax_title"])
            if self.df is not None:
                try:
                    fn(ax, self.df)
                except Exception:
                    pass
            leg = ax.get_legend()
            if leg:
                leg.get_frame().set_facecolor(t["leg_bg"])
                leg.get_frame().set_edgecolor(t["leg_edge"])
                for txt in leg.get_texts():
                    txt.set_color(t["leg_txt"])
                    txt.set_fontsize(7)
        return fig

    def _save_all(self):
        path = filedialog.asksaveasfilename(
            title="Save All Plots",
            defaultextension=".png",
            filetypes=[("PNG", "*.png"), ("PDF", "*.pdf"), ("SVG", "*.svg")])
        if not path:
            return
        fig = self._styled_fig_for_export()
        fig.savefig(path, dpi=150, bbox_inches="tight",
                    facecolor=_T["fig_bg"])
        plt.close(fig)
        messagebox.showinfo("Saved", f"Saved to:\n{path}")

    def _save_individual(self):
        if self.df is None:
            messagebox.showinfo("No Data", "Load a CSV first.")
            return
        folder = filedialog.askdirectory(title="Select output folder")
        if not folder:
            return
        t = _T
        saved = []
        for title, fn in PLOT_FNS:
            fig, ax = plt.subplots(figsize=(8, 4.5), facecolor=t["fig_bg"])
            ax.set_facecolor(t["ax_bg"])
            for spine in ax.spines.values():
                spine.set_edgecolor(t["ax_spine"])
            ax.tick_params(colors=t["ax_tick"], labelsize=8)
            ax.xaxis.label.set_color(t["ax_lbl"])
            ax.yaxis.label.set_color(t["ax_lbl"])
            ax.title.set_color(t["ax_title"])
            try:
                fn(ax, self.df)
            except Exception:
                pass
            leg = ax.get_legend()
            if leg:
                leg.get_frame().set_facecolor(t["leg_bg"])
                leg.get_frame().set_edgecolor(t["leg_edge"])
                for txt in leg.get_texts():
                    txt.set_color(t["leg_txt"])
            fig.tight_layout()
            safe = title.replace(" ", "_").replace("(", "").replace(")", "")
            out = os.path.join(folder, f"BNC_{safe}.png")
            fig.savefig(out, dpi=150, bbox_inches="tight", facecolor=t["fig_bg"])
            plt.close(fig)
            saved.append(os.path.basename(out))
        messagebox.showinfo("Saved",
            f"Saved {len(saved)} plots to:\n{folder}\n\n" + "\n".join(saved))


# ── Entry point ────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    app = BNCApp()
    app.mainloop()
