import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import matplotlib.patches as mpatches
import numpy as np

# ── Theme definitions (inherited from Latency script) ──────────────
THEMES = {
    "light":       dict(bg="#ffffff", fg="#222222", bar="#4c72b0", grid="#cccccc"),
    "dark":        dict(bg="#1e1e1e", fg="#eeeeee", bar="#56b4e9", grid="#444444"),
    "gruvbox dark":dict(bg="#282828", fg="#ebdbb2", bar="#d79921", grid="#504945"),
    "solarized":   dict(bg="#002b36", fg="#839496", bar="#268bd2", grid="#073642"),
    "clean white": dict(bg="#fafafa", fg="#111111", bar="#2ca02c", grid="#e0e0e0"),
    "publication": dict(bg="#ffffff", fg="#1a1a1a", bar="#4a7fa8", grid="#dddddd"),
}

FONTS = {
    "default": "DejaVu Sans",
    "calibri":  "Calibri",
    "georgia":  "Georgia",
}

# ── Pass / fail colors ─────────────────────────────────────────────
SUCCESS_COLOR = "#3a9e5f"   # green
FAIL_COLOR    = "#d94f4f"   # red


def _apply_spines(ax, is_publication, theme):
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


def _resolve_font(font):
    font_name = FONTS.get(font.lower(), font)
    available = [f.name for f in fm.fontManager.ttflist]
    if font_name not in available:
        print(f"Font '{font_name}' not found, falling back to DejaVu Sans.")
        font_name = "DejaVu Sans"
    plt.rcParams["font.family"] = font_name


def plot_dot_grid(trials=15, samples_per_trial=10,
                  successes=None,
                  theme_name="publication",
                  font="default",
                  title="Lickport Sensor Activation Accuracy",
                  fig_label="Figure 2a. Activation grid across all trials."):
    """
    Dot grid: samples_per_trial rows × trials columns.
    Green = Pass, Red = Fail.
    """
    theme = THEMES.get(theme_name.lower())
    if theme is None:
        print(f"Unknown theme '{theme_name}'. Options: {', '.join(THEMES)}")
        return

    is_publication = theme_name.lower() == "publication"
    _resolve_font(font)

    if successes is None:
        successes = [samples_per_trial] * trials
    if len(successes) != trials:
        raise ValueError(f"Length of successes ({len(successes)}) must equal trials ({trials})")

    total_activations = sum(successes)
    total_possible    = trials * samples_per_trial
    overall_pct       = 100 * total_activations / total_possible

    fig, ax = plt.subplots(figsize=(2 + trials * 0.55, 2 + samples_per_trial * 0.52))
    fig.patch.set_facecolor(theme["bg"])
    ax.set_facecolor(theme["bg"])

    dot_r = 0.35
    for t_idx, n_success in enumerate(successes):
        for s_idx in range(samples_per_trial):
            color = SUCCESS_COLOR if s_idx < n_success else FAIL_COLOR
            ax.add_patch(plt.Circle((t_idx + 1, s_idx + 1), dot_r, color=color, zorder=3))

    ax.set_xlim(0.4, trials + 0.6)
    ax.set_ylim(0.4, samples_per_trial + 0.6)
    ax.set_aspect("equal")
    ax.set_xticks(range(1, trials + 1))
    ax.set_xticklabels([str(i) for i in range(1, trials + 1)], fontsize=8)
    ax.set_yticks(range(1, samples_per_trial + 1))
    ax.set_yticklabels([str(i) for i in range(1, samples_per_trial + 1)], fontsize=8)
    ax.tick_params(colors=theme["fg"], length=0)
    ax.set_xlabel("Trial", color=theme["fg"], labelpad=8, fontweight="bold")
    ax.set_ylabel("Sample", color=theme["fg"], labelpad=8, fontweight="bold")
    ax.set_title("Activation Grid", color=theme["fg"], fontsize=11, pad=10,
                 fontweight="bold" if is_publication else "normal")

    _apply_spines(ax, is_publication, theme)

    # Legend — circles, top right outside axes
    legend_handles = [
        plt.Line2D([0], [0], marker="o", color="none", markerfacecolor=SUCCESS_COLOR,
                   markersize=7, label="Pass"),
        plt.Line2D([0], [0], marker="o", color="none", markerfacecolor=FAIL_COLOR,
                   markersize=7, label="Fail"),
    ]
    ax.legend(handles=legend_handles, loc="upper left",
              bbox_to_anchor=(1.02, 1.0),
              fontsize=8, framealpha=0.0,
              labelcolor=theme["fg"])

    fig.suptitle(title, color=theme["fg"], fontsize=13,
                 fontweight="bold" if is_publication else "normal")

    summary = (f"{total_activations}/{total_possible} activations  "
               f"({overall_pct:.1f}% overall)  ·  "
               f"{trials} trials × {samples_per_trial} samples")
    caption = f"{fig_label}  |  {summary}" if fig_label else summary
    fig.text(0.5, 0.01, caption, ha="center", fontsize=8.5,
             color=theme["fg"], style="italic", transform=fig.transFigure)

    plt.tight_layout(rect=[0, 0.05, 1, 0.95])
    plt.show()


def plot_bar_chart(trials=15, samples_per_trial=10,
                   successes=None,
                   theme_name="publication",
                   font="default",
                   title="Lickport Sensor Activation Accuracy",
                   fig_label="Figure 2b. Activations per trial.",
                   xlabel="Accuracy Trial",
                   ylabel="Successful Activations",
                   show_percent_labels=True):
    """
    Bar chart: one bar per trial, height = number of successful activations.
    """
    theme = THEMES.get(theme_name.lower())
    if theme is None:
        print(f"Unknown theme '{theme_name}'. Options: {', '.join(THEMES)}")
        return

    is_publication = theme_name.lower() == "publication"
    _resolve_font(font)

    if successes is None:
        successes = [samples_per_trial] * trials
    if len(successes) != trials:
        raise ValueError(f"Length of successes ({len(successes)}) must equal trials ({trials})")

    total_activations = sum(successes)
    total_possible    = trials * samples_per_trial
    overall_pct       = 100 * total_activations / total_possible

    fig, ax = plt.subplots(figsize=(max(6, trials * 0.75), 5))
    fig.patch.set_facecolor(theme["bg"])
    ax.set_facecolor(theme["bg"])

    trial_labels = [f"T{i+1}" for i in range(trials)]
    bar_colors   = [SUCCESS_COLOR if s == samples_per_trial else FAIL_COLOR for s in successes]

    bars = ax.bar(trial_labels, successes,
                  color=bar_colors,
                  edgecolor="#555555" if is_publication else theme["bg"],
                  linewidth=0.6 if is_publication else 0,
                  width=0.6)

    # Reference line at max possible
    ax.axhline(samples_per_trial, color=theme["fg"], linewidth=0.8,
               linestyle="--", alpha=0.4, zorder=1)

    # Labels on bars
    for bar, n_suc in zip(bars, successes):
        pct   = 100 * n_suc / samples_per_trial
        label = f"{int(pct)}%" if show_percent_labels else str(n_suc)
        ax.text(bar.get_x() + bar.get_width() / 2,
                bar.get_height() + 0.08,
                label,
                ha="center", va="bottom",
                color=theme["fg"], fontsize=8)

    ax.set_ylim(0, samples_per_trial * 1.18)
    ax.set_yticks(range(0, samples_per_trial + 1, max(1, samples_per_trial // 5)))
    ax.set_title("", color=theme["fg"], fontsize=11, pad=10,
                 fontweight="bold" if is_publication else "normal")
    ax.set_xlabel(xlabel, color=theme["fg"], labelpad=8, fontweight="bold")
    ax.set_ylabel(ylabel, color=theme["fg"], labelpad=8, fontweight="bold")
    ax.tick_params(colors=theme["fg"], length=4 if is_publication else 3, labelsize=8)
    ax.yaxis.grid(False)

    _apply_spines(ax, is_publication, theme)

    fig.suptitle(title, color=theme["fg"], fontsize=13,
                 fontweight="bold" if is_publication else "normal")

    # summary = (f"{total_activations}/{total_possible} activations  "
    #            f"({overall_pct:.1f}% overall)  ·  "
    #            f"{trials} trials × {samples_per_trial} samples")
    # caption = f"{fig_label}  |  {summary}" if fig_label else summary
    # fig.text(0.5, 0.01, caption, ha="center", fontsize=8.5,
    #          color=theme["fg"], style="italic", transform=fig.transFigure)

    plt.tight_layout(rect=[0, 0.05, 1, 0.95])
    plt.show()


# ── Entry point ───────────────────────────────────────────────────
if __name__ == "__main__":
    print(f"Available themes: {', '.join(THEMES)}")

    # ── Shared parameters ────────────────────────────────────────
    trials            = 15    # number of trials
    samples_per_trial = 10    # presses per trial

    # Per-trial success counts — None = all perfect
    # Example with some failures: [10, 10, 9, 10, 10, 8, 10, 10, 10, 10, 10, 10, 10, 10, 10]
    successes         = None

    theme_arg         = "publication"
    font              = "default"   # default, calibri, georgia
    # title             = "Lickport Sensor Activation Accuracy"
    title             = ""

    # ── Figure 1: Dot grid ───────────────────────────────────────
    # plot_dot_grid(
    #     trials=trials,
    #     samples_per_trial=samples_per_trial,
    #     successes=successes,
    #     theme_name=theme_arg,
    #     font=font,
    #     title=title,
    #     fig_label="Figure 2a. Activation grid across all validation trials.",
    # )

    # ── Figure 2: Bar chart ──────────────────────────────────────
    plot_bar_chart(
        trials=trials,
        samples_per_trial=samples_per_trial,
        successes=successes,
        theme_name=theme_arg,
        font=font,
        title=title,
        fig_label="", #"Figure 3. Activations per trial.",
        xlabel="Accuracy Trial",
        ylabel="Successful Activations",
        show_percent_labels=True,
    )