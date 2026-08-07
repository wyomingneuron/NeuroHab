import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import numpy as np

# ── Theme definitions ──────────────────────────────────────────────
THEMES = {
    "light":       dict(bg="#ffffff", fg="#222222", bar="#4c72b0", grid="#cccccc"),
    "dark":        dict(bg="#1e1e1e", fg="#eeeeee", bar="#56b4e9", grid="#444444"),
    "gruvbox dark":dict(bg="#282828", fg="#ebdbb2", bar="#d79921", grid="#504945"),
    "solarized":   dict(bg="#002b36", fg="#839496", bar="#268bd2", grid="#073642"),
    "clean white": dict(bg="#fafafa", fg="#111111", bar="#2ca02c", grid="#e0e0e0"),
    "publication": dict(bg="#ffffff", fg="#1a1a1a", bar="#4a7fa8", grid="#dddddd"),
}

# ── Font options ───────────────────────────────────────────────────
FONTS = {
    "default":  "DejaVu Sans",
    "calibri":  "Calibri",
    "georgia":  "Georgia",
}

# ── Category bar colors (used when color_mode="category") ─────────
CATEGORY_COLORS = [
    "#4c72b0", "#dd8452", "#55a868", "#c44e52",
    "#8172b2", "#937860", "#da8bc3", "#8c8c8c",
]

def plot(csv_path, theme_name="light", title="Column Means", xlabel="", ylabel="Mean Value",
         font="default", color_mode="category", fig_label="",
         show_dots=True, dot_jitter=0.08, dot_size=18, dot_alpha=0.55):
    """
    color_mode options:
      "category" — each bar a distinct color
      "value"    — lowest = steel blue, highest = amber, interpolated in between
      "single"   — use the theme's default bar color

    dot overlay options:
      show_dots   — whether to overlay individual data points (default True)
      dot_jitter  — horizontal jitter width in bar-width units (default 0.08)
      dot_size    — marker size in points² (default 18)
      dot_alpha   — opacity of dots, 0–1 (default 0.55)
    """
    theme = THEMES.get(theme_name.lower())
    if theme is None:
        print(f"Unknown theme '{theme_name}'. Options: {', '.join(THEMES)}")
        return

    is_publication = theme_name.lower() == "publication"

    # Resolve font
    font_name = FONTS.get(font.lower(), font)
    available = [f.name for f in fm.fontManager.ttflist]
    if font_name not in available:
        print(f"Font '{font_name}' not found, falling back to DejaVu Sans.")
        font_name = "DejaVu Sans"
    plt.rcParams["font.family"] = font_name

    # Read file
    if csv_path.lower().endswith((".xlsx", ".xls")):
        df = pd.read_excel(csv_path, header=0)
    else:
        df = pd.read_csv(csv_path, header=0)

    df = df.select_dtypes(include="number").dropna(axis=1, how="all")
    means = df.mean()
    print(means)
    stds = df.std()
    print(stds)
    print("n =", len(df))

    # Bar colors
    if color_mode == "value":
        ranks = means.rank(method="first") - 1
        norm  = ranks / max(ranks.max(), 1)
        cmap  = plt.matplotlib.colors.LinearSegmentedColormap.from_list(
                    "steel_amber", ["#4a7fa8", "#8fb898", "#d4943a"])
        bar_colors = [cmap(n) for n in norm]
    elif color_mode == "category":
        bar_colors = [CATEGORY_COLORS[i % len(CATEGORY_COLORS)] for i in range(len(means))]
    else:  # single
        bar_colors = [theme["bar"]] * len(means)

    # ── Plot ──────────────────────────────────────────────────────
    fig, ax = plt.subplots(figsize=(max(6, len(means) * 1.2), 5))
    fig.patch.set_facecolor(theme["bg"])
    ax.set_facecolor(theme["bg"])

    # Publication: thin dark edge on bars for definition
    edge = "#555555" if is_publication else theme["bg"]
    y_max = np.ceil(1000)
    bars = ax.bar(means.index, means.values, color=bar_colors, edgecolor=edge,
                  linewidth=0.6 if is_publication else 0, width=0.6)

    # ── Individual data point dots ─────────────────────────────────
    if show_dots:
        bar_positions = {col: bar.get_x() + bar.get_width() / 2
                         for col, bar in zip(means.index, bars)}
        rng = np.random.default_rng(seed=42)  # fixed seed → reproducible jitter

        for i, col in enumerate(means.index):
            col_data = df[col].dropna().values
            n = len(col_data)
            if n == 0:
                continue

            # Jitter x positions within ±dot_jitter (in data/bar units)
            jitter = rng.uniform(-dot_jitter, dot_jitter, size=n)
            x_pos  = bar_positions[col] + jitter

            # Dot color: slightly darker than bar for contrast
            base_color = bar_colors[i]
            ax.scatter(x_pos, col_data,
                       s=dot_size,
                       color=base_color,
                       edgecolors="white" if is_publication else theme["bg"],
                       linewidths=0.5,
                       alpha=dot_alpha,
                       zorder=3)

    # Value labels on top of bars
    for bar in bars:
        ax.text(bar.get_x() + bar.get_width() / 2,
                bar.get_height() * 1.01,
                f"{bar.get_height():.2f}",
                ha="center", va="bottom",
                color=theme["fg"], fontsize=9)

    ax.set_ylim(0, y_max)
    ax.set_title(title, color=theme["fg"], fontsize=13, pad=12,
                 fontweight="bold" if is_publication else "normal")
    ax.set_xlabel(xlabel, color=theme["fg"], labelpad=8, fontweight="bold")
    ax.set_ylabel(ylabel, color=theme["fg"], labelpad=8, fontweight="bold")
    ax.tick_params(colors=theme["fg"], length=4 if is_publication else 3)
    ax.yaxis.grid(False)

    # Publication: remove top and right spines (open box style)
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
        fig.text(0.5, 0.01, fig_label, ha="center", fontsize=9, color=theme["fg"], style="italic",
                 transform=fig.transFigure)
        plt.tight_layout(rect=[0, 0.05, 1, 1])
    else:
        plt.tight_layout()
    plt.show()

# ── Entry point ───────────────────────────────────────────────────
if __name__ == "__main__":
    # Available themes:     light, dark, gruvbox dark, solarized, clean white, publication
    # Available fonts:      default, calibri, georgia
    # Available color_mode: category, value, single
    print(f"Available themes: {', '.join(THEMES)}")

    """  // SET y_max = np.ceil(100) 
    csv_file   = "Latency Tests Right Left FED3 - Clean 3.xlsx"
    # fig_label  = "Figure 8. Mean response latency from sensor->ISR->FED3. (100 samples per bar.)"
    fig_label  = ""
    theme_arg  = "publication"
    title      = "Mean FED3 Left Right Port Response Latency (µs)"
    xlabel     = "Left vs Right Port"
    ylabel     = "Mean Response Latency (µs)"
    font       = "default"    # default, calibri, georgia
    color_mode = "value"      # category, value, single

    # Dot overlay controls
    show_dots  = True   # set False to hide dots
    dot_jitter = 0.25   # horizontal spread (in bar-width units)
    dot_size   = 20     # marker size (points²)
    dot_alpha  = 0.50   # transparency (0=invisible, 1=opaque)

    plot(csv_file, theme_arg, title, xlabel, ylabel, font, color_mode, fig_label,
         show_dots=show_dots, dot_jitter=dot_jitter, dot_size=dot_size, dot_alpha=dot_alpha)
    # """

    # """ y_max = np.ceil(1000)
    csv_file   = "Latency Tests Lickports - Clean 4.xlsx"
    # fig_label  = "Figure 2. Mean response latency from sensor update to activation."
    fig_label  = ""
    theme_arg  = "publication"
    # title      = "Mean Lickport Response Latency (µs)"
    title      = ""
    xlabel     = "Number of Sensors Active"
    ylabel     = "Mean Response Latency (µs)"
    font       = "default"    # default, calibri, georgia
    color_mode = "value"      # category, value, single

    # Dot overlay controls
    show_dots  = True   # set False to hide dots
    dot_jitter = 0.25   # horizontal spread (in bar-width units)
    dot_size   = 20     # marker size (points²)
    dot_alpha  = 0.50   # transparency (0=invisible, 1=opaque)

    plot(csv_file, theme_arg, title, xlabel, ylabel, font, color_mode, fig_label,
         show_dots=show_dots, dot_jitter=dot_jitter, dot_size=dot_size, dot_alpha=dot_alpha)
    # """


