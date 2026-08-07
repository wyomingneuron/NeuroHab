import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import numpy as np
from scipy.optimize import curve_fit
import warnings
from scipy.optimize import OptimizeWarning

# ── Theme definitions ──────────────────────────────────────────────
THEMES = {
    "light":       dict(bg="#ffffff", fg="#222222", grid="#cccccc"),
    "dark":        dict(bg="#1e1e1e", fg="#eeeeee", grid="#444444"),
    "gruvbox dark":dict(bg="#282828", fg="#ebdbb2", grid="#504945"),
    "solarized":   dict(bg="#002b36", fg="#839496", grid="#073642"),
    "clean white": dict(bg="#fafafa", fg="#111111", grid="#e0e0e0"),
    "publication": dict(bg="#ffffff", fg="#1a1a1a", grid="#dddddd"),
}

FONTS = {
    "default": "DejaVu Sans",
    "calibri": "Calibri",
    "georgia": "Georgia",
}

# ── Data ───────────────────────────────────────────────────────────
DATA = {
    "Single": {
        5000:  dict(counts=[1238, 1238, 1238], hz=[247.60, 247.60, 247.60]),
        60000: dict(counts=[3741, 3741, 3741], hz=[62.35,  62.35,  62.35]),
    },
    "Dual": {
        5000:  dict(counts=[312, 312, 312], hz=[62.40, 62.40, 62.40]),
        60000: dict(counts=[594, 594, 594], hz=[9.90,  9.90,  9.90]),
    },
    "Triple": {
        5000:  dict(counts=[139, 139, 139], hz=[27.80, 27.80, 27.80]),
        60000: dict(counts=[594, 594, 594], hz=[9.90,  9.90,  9.90]),
    },
}

GROUP_COLORS = {
    "Single": {"5000": "#4a7fa8", "60000": "#6fa3c8", "120000": "#a8cfe0"},
    "Dual":   {"5000": "#8fb898", "60000": "#b0d4b8", "120000": "#d0ece0"},
    "Triple": {"5000": "#d4943a", "60000": "#e8b86d", "120000": "#f5d4a0"},
}

def apply_spine_style(ax, theme, is_publication):
    if is_publication:
        ax.spines["top"].set_visible(False)
        ax.spines["right"].set_visible(False)
        ax.spines["left"].set_linewidth(1.2)
        ax.spines["bottom"].set_linewidth(1.2)
        for spine in ["left", "bottom"]:
            ax.spines[spine].set_edgecolor("#1a1a1a")
    else:
        for spine in ax.spines.values():
            spine.set_edgecolor(theme["grid"])

def plot(theme_name="publication", font="default",
         title_bar="Maximum Recording Frequency by Channel and Pulse Train Length",
         title_line="Maximum Recording Frequency by Channel and Pulse Train Length",
         fig_label="",
         bar_xlabel="Channel Count", bar_ylabel="Frequency (Hz)",
         line_xlabel="Window Duration (ms)", line_ylabel="Frequency (Hz)",
         data=None):

    global DATA
    if data is not None:
        DATA = data

    theme = THEMES.get(theme_name.lower())
    if theme is None:
        print(f"Unknown theme. Options: {', '.join(THEMES)}")
        return

    is_publication = theme_name.lower() == "publication"
    font_name = FONTS.get(font.lower(), font)
    available = [f.name for f in fm.fontManager.ttflist]
    if font_name not in available:
        print(f"Font '{font_name}' not found, falling back to DejaVu Sans.")
        font_name = "DejaVu Sans"
    plt.rcParams["font.family"] = font_name

    groups    = list(DATA.keys())
    durations = sorted(set(dur for g in groups for dur in DATA[g].keys()))
    x         = np.arange(len(groups))
    width     = 0.7 / len(durations)

    # ── Plot 1: Single bar chart — frequency on y, label = "hz | count" ──
    fig1, ax = plt.subplots(figsize=(9, 5))
    fig1.patch.set_facecolor(theme["bg"])
    ax.set_facecolor(theme["bg"])
    ax.set_title(title_bar, color=theme["fg"], fontsize=13, pad=12,
                 fontweight="bold" if is_publication else "normal")

    for i, dur in enumerate(durations):
        offset = (i - (len(durations) - 1) / 2) * width
        means_hz = [np.mean(DATA[g][dur]["hz"])     for g in groups]
        means_ct = [np.mean(DATA[g][dur]["counts"]) for g in groups]
        colors   = [GROUP_COLORS[g][str(dur)] for g in groups]

        bars = ax.bar(x + offset, means_hz, width,
                      label=f"{dur//1000}s window",
                      color=colors,
                      edgecolor="#555555" if is_publication else theme["bg"],
                      linewidth=0.6 if is_publication else 0)

        for bar, hz_val, ct_val in zip(bars, means_hz, means_ct):
            ax.text(bar.get_x() + bar.get_width() / 2,
                    bar.get_height() + (ax.get_ylim()[1] * 0.01),
                    f"{hz_val:.1f} Hz\n{ct_val:.0f} Pulses",
                    ha="center", va="bottom",
                    color=theme["fg"], fontsize=7.5, linespacing=1.4)

    ax.set_ylabel(bar_ylabel, color=theme["fg"], fontweight="bold", labelpad=8)
    ax.set_xlabel(bar_xlabel, color=theme["fg"], fontweight="bold", labelpad=8)
    ax.set_xticks(x)
    ax.set_xticklabels([f"{g} Channel" for g in groups])
    ax.tick_params(colors=theme["fg"])
    ax.yaxis.grid(False)

    # y headroom for labels — tighter ceiling
    all_hz = [np.mean(DATA[g][dur]["hz"]) for g in groups for dur in durations]
    ax.set_ylim(0, np.ceil(max(all_hz) / 50) * 50 * 1.20)

    # Custom legend: one patch per group+duration combo
    from matplotlib.patches import Patch
    legend_handles = [
        Patch(facecolor=GROUP_COLORS[g][str(dur)],
              edgecolor="#555555" if is_publication else "none",
              label=f"{g} Channel – {dur//1000}s window")
        for g in groups for dur in durations
    ]
    ax.legend(handles=legend_handles, fontsize=8,
              facecolor=theme["bg"], labelcolor=theme["fg"], framealpha=0.5)
    apply_spine_style(ax, theme, is_publication)

    if fig_label:
        fig1.text(0.5, 0.01, fig_label, ha="center", fontsize=9,
                  color=theme["fg"], style="italic", transform=fig1.transFigure)
        fig1.tight_layout(rect=[0, 0.05, 1, 1])
    else:
        fig1.tight_layout()

    # ── Plot 2: Line plot — time (ms) on x, frequency on y, pulse count labels ──
    fig2, ax2 = plt.subplots(figsize=(9, 5))
    fig2.patch.set_facecolor(theme["bg"])
    ax2.set_facecolor(theme["bg"])
    ax2.set_title(title_line, color=theme["fg"], fontsize=13, pad=12,
                  fontweight="bold" if is_publication else "normal")

    predict_ms = 600000  # 10 minutes in ms
    legend_handles_line = []

    def exp_decay(t, a, b, c):
        # a = amplitude, b = decay rate, c = asymptote (floor hz)
        return a * np.exp(-b * t) + c

    for group in groups:
        hz_vals = [np.mean(DATA[group][dur]["hz"])     for dur in durations]
        ct_vals = [np.mean(DATA[group][dur]["counts"]) for dur in durations]
        t_vals  = [dur for dur in durations]
        color   = GROUP_COLORS[group]["5000"]

        try:
            p0     = [max(hz_vals), 1e-5, min(hz_vals) * 0.5]
            bounds = ([0, 0, 0], [max(hz_vals)*2, 1e-2, min(hz_vals)])
            with warnings.catch_warnings():
                warnings.simplefilter("ignore", OptimizeWarning)
                popt, _ = curve_fit(exp_decay, t_vals, hz_vals, p0=p0,
                                    bounds=bounds, maxfev=10000)
            t_smooth     = np.linspace(min(t_vals), predict_ms, 400)
            hz_smooth    = exp_decay(t_smooth, *popt)
            hz_predicted = exp_decay(predict_ms, *popt)

            ax2.plot(t_smooth, hz_smooth, linestyle="--", color=color,
                     linewidth=1.2, alpha=0.6)
            ax2.scatter([predict_ms], [hz_predicted], marker="*", color=color,
                        s=120, zorder=5)

            if groups.index(group) == 0:
                pred_oy = 0 + ((len(groups) - 1 - groups.index(group)) * 5)
            elif groups.index(group) == 1:
                pred_oy = 0 + ((len(groups) - 1 - groups.index(group)) * 15)
            else:
                pred_oy = -25

            ax2.annotate(f"~{hz_predicted:.1f} Hz @ 10min",
                         xy=(predict_ms, hz_predicted),
                         xytext=(-10, pred_oy), textcoords="offset points",
                         ha="right", va="bottom", fontsize=7.5,
                         color=color,
                         bbox=dict(boxstyle="round,pad=0.2", fc=theme["bg"],
                                   ec="none", alpha=1))
        except RuntimeError:
            pass

        line, = ax2.plot(t_vals, hz_vals,
                         linestyle="-", marker="o", color=color,
                         markersize=9, linewidth=1.8)
        legend_handles_line.append(
            plt.Line2D([0], [0], color=color, marker="o", linewidth=1.8,
                       markersize=7, label=f"{group} Channel")
        )

        for t, hz, ct in zip(t_vals, hz_vals, ct_vals):
            # Scale vertical offset by group rank so Single > Dual > Triple
            group_rank = groups.index(group)  # 0=Single, 1=Dual, 2=Triple

            if group_rank == 0:
                oy = 0 + ((len(groups) - 1 - group_rank) * 5)
            elif group_rank == 1:
                oy = 0 + ((len(groups) - 1 - group_rank) * 15)
            else:
                oy = -25

            ax2.annotate(f"{hz:.1f} Hz\n{ct:.0f} Pulses",
                         xy=(t, hz),
                         xytext=(0, oy), textcoords="offset points",
                         ha="center", va="bottom", fontsize=8,
                         color=color,
                         bbox=dict(boxstyle="round,pad=0.2", fc=theme["bg"],
                                   ec="none", alpha=1))

    # Add regression note to legend
    legend_handles_line.append(
        plt.Line2D([0], [0], color=theme["fg"], linewidth=1.2, linestyle="--",
                   alpha=0.6, label="Exponential decay fit (extrapolated)")
    )
    legend_handles_line.append(
        plt.Line2D([0], [0], color=theme["fg"], marker="*", linewidth=0,
                   markersize=9, label="Predicted @ 10 min")
    )

    ax2.set_xlabel(line_xlabel, color=theme["fg"], fontweight="bold", labelpad=8)
    ax2.set_ylabel(line_ylabel, color=theme["fg"], fontweight="bold", labelpad=8)
    # Include prediction point in x ticks
    ax2.set_xticks(durations + [predict_ms])
    ax2.set_xticklabels([f"{d//1000}s" for d in durations] + ["10min"])
    ax2.tick_params(colors=theme["fg"])
    ax2.yaxis.grid(False)

    all_hz2 = [np.mean(DATA[g][dur]["hz"]) for g in groups for dur in durations]
    ax2.set_yscale("log")
    ax2.set_ylim(max(1, min(all_hz2) * 0.6), np.ceil(max(all_hz2) / 50) * 50 * 1.55)
    # Clean log axis: sparse explicit major ticks only, no minor tick labels
    from matplotlib.ticker import FixedLocator, ScalarFormatter, NullFormatter
    bottom_tick = max(1, min(all_hz2) * 0.6)
    ax2.yaxis.set_major_locator(FixedLocator([bottom_tick, 10, 20, 50, 100, 200, 300]))
    from matplotlib.ticker import FuncFormatter
    ax2.yaxis.set_major_formatter(FuncFormatter(lambda v, _: "0" if abs(v - bottom_tick) < 0.01 else f"{int(v)}"))
    ax2.yaxis.set_minor_formatter(NullFormatter())
    ax2.legend(handles=legend_handles_line, fontsize=8,
               facecolor=theme["bg"], labelcolor=theme["fg"], framealpha=0.5)
    apply_spine_style(ax2, theme, is_publication)

    if fig_label:
        fig2.text(0.5, 0.01, fig_label, ha="center", fontsize=9,
                  color=theme["fg"], style="italic", transform=fig2.transFigure)
        fig2.tight_layout(rect=[0, 0.05, 1, 1])
    else:
        fig2.tight_layout()

    plt.show()

# ── Entry point ───────────────────────────────────────────────────
if __name__ == "__main__":
    # Available themes:  light, dark, gruvbox dark, solarized, clean white, publication
    # Available fonts:   default, calibri, georgia

    # ── Data (edit here to update values) ─────────────────────────
    data = {
        "Single": {
            5000:   dict(counts=[1238, 1238, 1238], hz=[247.60, 247.60, 247.60]),
            60000:  dict(counts=[3741, 3741, 3741], hz=[62.35,  62.35,  62.35]),
            120000: dict(counts=[5703, 5703, 5703], hz=[47.53,  47.53,  47.53]),
        },
        "Dual": {
            5000:   dict(counts=[312,  312,  312],  hz=[62.40, 62.40, 62.40]),
            60000:  dict(counts=[983,  983,  983],  hz=[16.38, 16.38, 16.38]),
            120000: dict(counts=[1690, 1690, 1690], hz=[14.08, 14.08, 14.08]),
        },
        "Triple": {
            5000:   dict(counts=[139,  139,  139],  hz=[27.80, 27.80, 27.80]),
            60000:  dict(counts=[845,  845,  845],  hz=[14.08, 14.08, 14.08]),
            120000: dict(counts=[1578, 1578, 1578], hz=[13.15, 13.15, 13.15]),
        },
    }

    # ── Settings ──────────────────────────────────────────────────
    theme_name   = "publication"
    font         = "calibri"
    # title_bar    = "Maximum Recording Frequency by Channel and Pulse Train Length (Synchronous)"
    title_bar    = ""
    # title_line   = "Maximum Recording Frequency by Channel and Pulse Train Length (Synchronous)"
    title_line   = ""
    # fig_label    = "Figure 4a. BNC frequency maximums for single, dual, and triple channel recording with synchronous pulse trains."# Includes the predicted maximum frequencies at 10 minutes."
    fig_label    = ""
    bar_xlabel   = "Synchronous Recording Configuration"
    bar_ylabel   = "Frequency (Hz)"
    line_xlabel  = "Pulse Train Duration"
    line_ylabel  = "Frequency (Hz) of Synchronous Pulses"

    plot(theme_name, font, title_bar, title_line, fig_label,
         bar_xlabel, bar_ylabel, line_xlabel, line_ylabel, data)