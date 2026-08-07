import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import numpy as np
from scipy import stats

# ── Theme definitions ──────────────────────────────────────────────
THEMES = {
    "light":        dict(bg="#ffffff", fg="#222222", bar="#4c72b0", grid="#cccccc"),
    "dark":         dict(bg="#1e1e1e", fg="#eeeeee", bar="#56b4e9", grid="#444444"),
    "gruvbox dark": dict(bg="#282828", fg="#ebdbb2", bar="#d79921", grid="#504945"),
    "solarized":    dict(bg="#002b36", fg="#839496", bar="#268bd2", grid="#073642"),
    "clean white":  dict(bg="#fafafa", fg="#111111", bar="#2ca02c", grid="#e0e0e0"),
    "publication":  dict(bg="#ffffff", fg="#1a1a1a", bar="#4a7fa8", grid="#dddddd"),
}

FONTS = {
    "default": "DejaVu Sans",
    "calibri": "Calibri",
    "georgia": "Georgia",
}


def plot_drift_scatter(csv_path,
                       theme_name="publication",
                       title="|Interval Difference| vs Interval Length — Intervals ≥ 1 min",
                       xlabel="Interval Length (log scale)",
                       ylabel="|Interval Difference| (µs)",
                       font="default",
                       fig_label="",
                       outlier_threshold_us=900_000,
                       cutoff_ms=60_000,        # 1 minute = 60,000 ms
                       log_x=True):
    """
    Scatter plot of |bnc_interval - sync4_interval| in µs (y)
    vs bnc_interval length in minutes (x), excluding intervals below cutoff_ms.

    cutoff_ms : minimum interval length to include (default 60000 ms = 1 min).
                Removes the M2P quantization noise floor which dominates at
                short intervals and suppresses the drift signal.
    log_x     : use log scale on x-axis (recommended — interval lengths
                span several orders of magnitude).
    """

    theme = THEMES.get(theme_name.lower())
    if theme is None:
        print(f"Unknown theme '{theme_name}'. Options: {', '.join(THEMES)}")
        return

    is_publication = theme_name.lower() == "publication"

    font_name = FONTS.get(font.lower(), font)
    available = [f.name for f in fm.fontManager.ttflist]
    if font_name not in available:
        font_name = "DejaVu Sans"
    plt.rcParams["font.family"] = font_name

    # ── Load data ─────────────────────────────────────────────────
    if csv_path.lower().endswith((".xlsx", ".xls")):
        df = pd.read_excel(csv_path, header=0)
    else:
        df = pd.read_csv(csv_path, header=0)

    interval_ms = pd.to_numeric(df.iloc[:, 2], errors="coerce")   # col 3: bnc_interval_ms
    diff_ms     = pd.to_numeric(df.iloc[:, 4], errors="coerce")   # col 5: diff (ms)

    data = pd.DataFrame({"interval_ms": interval_ms,
                         "diff_us": diff_ms * 1000.0}).dropna()

    # Remove diff-axis outliers
    n_raw = len(data)
    data = data[data["diff_us"].abs() < outlier_threshold_us]
    n_excluded = n_raw - len(data)
    if n_excluded:
        print(f"Note: {n_excluded} outlier(s) excluded (|diff| > {outlier_threshold_us/1000:.0f} ms).")

    # Apply interval length cutoff (quantization noise filter)
    n_before = len(data)
    data = data[data["interval_ms"] >= cutoff_ms]
    n_short = n_before - len(data)
    print(f"Note: {n_short} intervals below {cutoff_ms} ms excluded (quantization noise filter).")

    # Convert interval_ms → minutes for the x-axis
    data["interval_min"] = data["interval_ms"] / 60_000.0
    data["abs_diff_us"]  = data["diff_us"].abs()
    n = len(data)

    # ── Statistics ────────────────────────────────────────────────
    # Spearman correlation (rank-based, no distributional assumption)
    rho, p_spear = stats.spearmanr(data["interval_min"], data["abs_diff_us"])

    # Log-linear regression for the trend line and R²
    log_interval = np.log10(data["interval_min"])
    slope, intercept, r, p_reg, se = stats.linregress(log_interval, data["abs_diff_us"])
    r2 = r ** 2

    print(f"\nSpearman correlation (interval_min vs |diff_us|):")
    print(f"  ρ = {rho:.4f},  p = {p_spear:.2e}")
    print(f"\nLog-linear regression (log10(interval_min) vs |diff_us|):")
    print(f"  slope = {slope:.4f},  R² = {r2:.4f},  p = {p_reg:.2e}")

    # ── Plot ──────────────────────────────────────────────────────
    fig, ax = plt.subplots(figsize=(10, 5))
    fig.patch.set_facecolor(theme["bg"])
    ax.set_facecolor(theme["bg"])

    ax.scatter(data["interval_min"], data["abs_diff_us"],
               color=theme["bar"], alpha=0.35, s=12, zorder=2,
               edgecolors="none")

    # Log-linear regression line
    x_range = np.logspace(np.log10(data["interval_min"].min()),
                           np.log10(data["interval_min"].max()), 300)
    y_fit   = slope * np.log10(x_range) + intercept
    ax.plot(x_range, y_fit, color="#c44e52", linewidth=1.8,
            linestyle="--", zorder=4,
            label=f"Log-linear fit  R²={r2:.3f}")

    if log_x:
        ax.set_xscale("log")

        # Ticks in minutes — cutoff is 1 min so start there, cap at data max
        min_ticks = [1, 2, 5, 10, 30, 100]

        def format_tick(x, _):
            return f"{x:g} min"

        ax.set_xticks(min_ticks)
        ax.xaxis.set_major_formatter(plt.FuncFormatter(format_tick))
        ax.xaxis.set_minor_locator(plt.NullLocator())

        # Start just below the 1 min cutoff; end just past the data maximum
        ax.set_xlim(left=0.9, right=data["interval_min"].max() * 1.05)

    # ── Annotation box — Spearman only ────────────────────────────
    stats_text = (f"n = {n}\n"
                  f"Spearman ρ = {rho:.3f}\n"
                  f"p = {p_spear:.2e}")
    ax.text(0.03, 0.95, stats_text,
            transform=ax.transAxes,
            ha="left", va="top", fontsize=9,
            color=theme["fg"],
            bbox=dict(boxstyle="round,pad=0.4", facecolor=theme["bg"],
                      edgecolor=theme["grid"], alpha=0.85))

    ax.set_title(title, color=theme["fg"], fontsize=13, pad=12,
                 fontweight="bold" if is_publication else "normal")
    ax.set_xlabel(xlabel, color=theme["fg"], labelpad=8, fontweight="bold")
    ax.set_ylabel(ylabel, color=theme["fg"], labelpad=8, fontweight="bold")
    ax.tick_params(colors=theme["fg"], length=4)
    ax.yaxis.grid(True, color=theme["grid"], linewidth=0.5, linestyle=":", zorder=0)
    ax.set_axisbelow(True)

    ax.legend(framealpha=0.85, fontsize=9, labelcolor=theme["fg"],
              facecolor=theme["bg"], edgecolor=theme["grid"])

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

    if fig_label:
        fig.text(0.5, 0.01, fig_label, ha="center", fontsize=9,
                 color=theme["fg"], style="italic", transform=fig.transFigure)
        plt.tight_layout(rect=[0, 0.05, 1, 1])
    else:
        plt.tight_layout()

    plt.savefig("drift_scatter_cut_1min.png", dpi=180,
                bbox_inches="tight", facecolor=theme["bg"])
    print("\nSaved → drift_scatter_cut_1min.png")
    plt.show()


# ── Entry point ───────────────────────────────────────────────────
if __name__ == "__main__":
    csv_file  = "intervals.csv"
    # fig_label = ("Figure 10d. Absolute interval difference (|NH − M2P| in µs) vs interval length."
    #              " Excluding sub 1 minute intervals to remove M2P quantization noise.\n"
    #              "As time between events increases, clock drift accounts for more syncing discrepancy variance.")

    fig_label = ""


    plot_drift_scatter(
        csv_path             = csv_file,
        theme_name           = "publication",
        # title                = "|Interval Difference| vs Interval Length (Intervals ≥ 1 min)",
        title                = "",
        # xlabel               = "Interval Length (m, log scale)\n",
        xlabel               = "",
        # ylabel               = "|Interval Difference| (µs)",
        ylabel               = "",
        font                 = "default",
        fig_label            = fig_label,
        outlier_threshold_us = 10_000_000,
        cutoff_ms            = 60_000,   # ← 1 minute cutoff
        log_x                = True,
    )
    