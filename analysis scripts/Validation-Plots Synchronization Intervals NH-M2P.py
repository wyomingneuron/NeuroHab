import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import numpy as np

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


def plot_distribution(csv_path,
                      theme_name="publication",
                      title="Distribution of Interval Differences (BNC vs Train)",
                      xlabel="Interval Difference (µs)",
                      ylabel="Count",
                      font="default",
                      fig_label="",
                      n_bins=80,
                      outlier_threshold_us=900_000,
                      show_mean_line=True,
                      show_std_bands=True):

    theme = THEMES.get(theme_name.lower())
    if theme is None:
        print(f"Unknown theme '{theme_name}'. Options: {', '.join(THEMES)}")
        return

    is_publication = theme_name.lower() == "publication"

    font_name = FONTS.get(font.lower(), font)
    available = [f.name for f in fm.fontManager.ttflist]
    if font_name not in available:
        print(f"Font '{font_name}' not found, falling back to DejaVu Sans.")
        font_name = "DejaVu Sans"
    plt.rcParams["font.family"] = font_name

    if csv_path.lower().endswith((".xlsx", ".xls")):
        df = pd.read_excel(csv_path, header=0)
    else:
        df = pd.read_csv(csv_path, header=0)

    # Column 4 is the raw difference in milliseconds
    diff_ms = pd.to_numeric(df.iloc[:, 4]).dropna()
    diff_us = diff_ms * 1000.0

    n_raw = len(diff_us)
    n_excluded = n_raw - len(diff_us)
    if n_excluded:
        print(f"Note: {n_excluded} outlier(s) beyond ±{outlier_threshold_us/1000:.0f} ms excluded.")

    mean_us = diff_us.mean()
    std_us  = diff_us.std()
    n       = len(diff_us)

    print(f"\nStatistics (n={n}):")
    print(f"  Mean : {mean_us:+.2f} µs")
    print(f"  Std  : {std_us:.2f} µs")
    print(f"  Min / Max : {diff_us.min():.2f} / {diff_us.max():.2f} µs")

    # ── Plot ───────────────────────────────────────────────────────
    fig, ax = plt.subplots(figsize=(10, 5))
    fig.patch.set_facecolor(theme["bg"])
    ax.set_facecolor(theme["bg"])

    bar_color = theme["bar"]

    # σ bands centred on actual mean
    if show_std_bands:
        ax.axvspan(mean_us - 2 * std_us, mean_us + 2 * std_us,
                   color=bar_color, alpha=0.10, zorder=0, label="±2σ")
        ax.axvspan(mean_us - std_us, mean_us + std_us,
                   color=bar_color, alpha=0.15, zorder=1, label="±1σ")

    # Histogram of raw differences (not centred)
    x_lim = 1500  # µs
    ax.hist(diff_us, bins=n_bins,
            color=bar_color,
            edgecolor=theme["bg"] if not is_publication else "#555555",
            linewidth=0.4 if is_publication else 0,
            zorder=2)

    ax.set_xlim(-x_lim, x_lim)
    ax.xaxis.set_major_locator(plt.MultipleLocator(250))
    ax.xaxis.set_minor_locator(plt.MultipleLocator(125))
    ax.tick_params(axis='x', which='major', length=6, width=1.2)
    ax.tick_params(axis='x', which='minor', length=3, width=0.8)

    # Mean line at actual mean value
    if show_mean_line:
        ax.axvline(mean_us, color="#c44e52", linewidth=1.6, linestyle="--",
                   zorder=5, label=f"Mean = {mean_us:+.1f} µs")

    # Annotation box
    stats_text = (f"n = {n}\n"
                  f"mean = {mean_us:+.1f} µs\n"
                  f"σ = {std_us:.1f} µs")
    ax.text(0.97, 0.95, stats_text,
            transform=ax.transAxes,
            ha="right", va="top", fontsize=9,
            color=theme["fg"],
            bbox=dict(boxstyle="round,pad=0.4", facecolor=theme["bg"],
                      edgecolor=theme["grid"], alpha=0.85))

    # ── Styling ────────────────────────────────────────────────────
    ax.set_title(title, color=theme["fg"], fontsize=13, pad=12,
                 fontweight="bold" if is_publication else "normal")
    ax.set_xlabel(xlabel, color=theme["fg"], labelpad=8, fontweight="bold")
    ax.set_ylabel(ylabel, color=theme["fg"], labelpad=8, fontweight="bold")
    ax.tick_params(colors=theme["fg"])
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

    plt.savefig("interval_distribution.png", dpi=180,
                bbox_inches="tight", facecolor=theme["bg"])
    print("\nSaved → interval_distribution.png")
    plt.show()


# ── Entry point ───────────────────────────────────────────────────
if __name__ == "__main__":
    print(f"Available themes: {', '.join(THEMES)}")

    csv_file  = "intervals.csv"

    # fig_label = ("Figure 10a. Distribution of interval differences between NeuroHab and M2P received pulses."
    #              "(NH received interval − M2P received interval) in µs.")

    fig_label = ""

    plot_distribution(
        csv_path             = csv_file,
        theme_name           = "publication",
        # title                = "Synchronisation Interval Difference Distribution (µs)",
        title                = "",
        xlabel               = "Interval Difference (µs)",
        ylabel               = "Count",
        font                 = "default",
        fig_label            = fig_label,
        n_bins               = 80,
        outlier_threshold_us = 10_000_000,
        show_mean_line       = True,
        show_std_bands       = True,
    )