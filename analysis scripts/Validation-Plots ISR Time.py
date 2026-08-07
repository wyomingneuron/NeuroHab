import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import numpy as np

# ── Theme definitions ──────────────────────────────────────────────
THEMES = {
    "publication": dict(bg="#ffffff", fg="#1a1a1a", bar="#4a7fa8", grid="#dddddd"),
}

# ── Data ──────────────────────────────────────────────────────────
# Timestamp pairs (t1, t2); difference = t2 - t1
pairs = [
    (62977383,62977384),(63134937,63134938),(63258258,63258259),(63403184,63403185),
    (63562571,63562572),(63634657,63634658),(63724638,63724639),(63881978,63881979),
    (63954055,63954057),(64039610,64039611),(64179532,64179533),(64251616,64251617),
    (64448581,64448583),(64520669,64520670),(64600969,64600971),(64733740,64733742),
    (64805905,64805906),(65138034,65138035),(65306621,65306622),(65470547,65470548),
    (65857893,65857893),(65942430,65942431),(66093413,66093414),(66240000,66240001),
    (66465856,66465857),(66555037,66555038),(66715206,66715207),(66872751,66872752),
    (67105039,67105040),(67321325,67321326),(67393408,67393409),(67473391,67473392),
    (67717784,67717785),(67817999,67818000),(68053931,68053932),(68149437,68149439),
    (68389280,68389281),(68862359,68862360),(69203989,69203990),(69211137,69211138),
    (69374755,69374756),(69446813,69446814),(69665284,69665285),(69762538,69762539),
    (69929059,69929060),(70001133,70001134),(70085601,70085602),(70230893,70230894),
    (70374740,70374741),(70606822,70606823),(70749509,70749510),(70821589,70821590),
    (70904286,70904288),(71095113,71095114),(71167203,71167204),(71267136,71267137),
    (71511571,71511572),(71601561,71601562),(71775473,71775474),(71847556,71847557),
    (71939373,71939374),(72096760,72096761),(72168846,72168847),(72268823,72268824),
    (72430801,72430801),(72502878,72502879),(72572786,72572787),(72791205,72791206),
    (72889321,72889322),(73141897,73141898),(73237331,73237332),(73487013,73487014),
    (73578090,73578091),(73905800,73905801),(74141701,74141702),(74242252,74242253),
    (74246336,74246337),(74423891,74423892),(74591108,74591109),(74840748,74840749),
    (74951926,74951928),(75215858,75215859),(75312012,75312013),(75494080,75494081),
    (75680922,75680923),(75947471,75947472),(76041769,76041771),(76282692,76282693),
    (76368601,76368602),(76620165,76620166),(76731355,76731356),(76991338,76991339),
    (77099547,77099548),(77361581,77361582),(77462441,77462442),(77663629,77663630),
    (77735771,77735772),(77849666,77849667),(78113994,78113995),(78215545,78215546),
    (78217588,78217589),(78403369,78403370),(78475515,78475516),(78584807,78584808),
    (78770808,78770809),(78842914,78842915),(78949334,78949335),(79212310,79212311),
    (79329952,79329953),(79574737,79574738),(79697054,79697055),(79892901,79892902),
    (79965063,79965064),(80093836,80093837),(80349895,80349896),(80459129,80459130),
    (80657558,80657559),(80842652,80842653),(81021899,81021900),(81208769,81208770),
    (81461914,81461915),(81578579,81578580),(81769259,81769260),(81958091,81958092),
    (82219965,82219966),(82329189,82329190),(82524575,82524576),(82706706,82706707),
    (82708748,82708749),(82901553,82901554),(82973678,82973679),(83081948,83081949),
    (83318224,83318225),(83442437,83442438),(83628319,83628320),(83793849,83793850),
    (83979964,83979965),(84153897,84153898),(84408067,84408068),(84539729,84539730),
    (84728368,84728369),(84800471,84800473),(84929204,84929205),(85085834,85085835),
    (85157952,85157953),(85290418,85290419),(85556027,85556028),(85656812,85656813),
    (85836829,85836830),(85908935,85908936),(86026474,86026475),(86211187,86211188),
    (86283287,86283288),(86400769,86400770),(86667397,86667398),(86775312,86775312),
    (87024756,87024758),(87150103,87150103),(87346383,87346384),(87517245,87517246),
    (87519287,87519288),(88278588,88278589),
]

diffs = [t2 - t1 for t1, t2 in pairs]
n = len(diffs)

# Count each value
counts = {0: diffs.count(0), 1: diffs.count(1), 2: diffs.count(2)}
pct    = {k: v / n * 100 for k, v in counts.items()}

print(f"n = {n}")
for k, v in counts.items():
    print(f"  diff={k}: {v} events ({pct[k]:.1f}%)")

# ── Plot ───────────────────────────────────────────────────────────
theme = THEMES["publication"]

plt.rcParams["font.family"] = "DejaVu Sans"

fig, ax = plt.subplots(figsize=(7, 4.5))
fig.patch.set_facecolor(theme["bg"])
ax.set_facecolor(theme["bg"])

x      = np.array([0, 1, 2])
values = np.array([counts[0], counts[1], counts[2]])

bars = ax.bar(x, values,
              width=0.5,
              color=theme["bar"],
              edgecolor="#2e5f82",
              linewidth=0.8,
              zorder=3)

# Annotate each bar with count and percentage
for bar, val, pv in zip(bars, values, [pct[0], pct[1], pct[2]]):
    ax.text(bar.get_x() + bar.get_width() / 2,
            bar.get_height() + 1.5,
            f"n={val}\n({pv:.1f}%)",
            ha="center", va="bottom",
            fontsize=9, color=theme["fg"])

# Stats annotation
mean_diff = np.mean(diffs)
std_diff  = np.std(diffs)
stats_text = (f"n = {n}\n"
              f"Mean = {mean_diff:.3f} µs\n"
              f"SD = {std_diff:.3f} µs")
ax.text(0.97, 0.95, stats_text,
        transform=ax.transAxes,
        ha="right", va="top", fontsize=9,
        color=theme["fg"],
        bbox=dict(boxstyle="round,pad=0.4",
                  facecolor=theme["bg"],
                  edgecolor=theme["grid"],
                  alpha=0.85))

# Axes formatting
ax.set_xticks([0, 1, 2])
ax.set_xticklabels(["0 µs", "1 µs", "2 µs"], fontsize=11)
ax.set_xlabel("ISR Duration (µs) Distribution", color=theme["fg"],
              labelpad=8, fontweight="bold")
ax.set_ylabel("ISR Duration Count", color=theme["fg"],
              labelpad=8, fontweight="bold")
# ax.set_title("ISR Execution Timestamp Difference Distribution",
#              color=theme["fg"], fontsize=13, pad=12, fontweight="bold")
ax.set_title("",
             color=theme["fg"], fontsize=13, pad=12, fontweight="bold")

ax.set_xlim(-0.5, 2.5)
ax.set_ylim(0, max(values) * 1.25)

ax.yaxis.grid(True, color=theme["grid"], linewidth=0.5, linestyle=":", zorder=0)
ax.set_axisbelow(True)
ax.tick_params(colors=theme["fg"], length=4)

# Publication spines
ax.spines["top"].set_visible(False)
ax.spines["right"].set_visible(False)
ax.spines["left"].set_linewidth(1.2)
ax.spines["bottom"].set_linewidth(1.2)
for spine in ["left", "bottom"]:
    ax.spines[spine].set_edgecolor("#1a1a1a")

# fig_label = "Figure 12. Event ISR execution time for operant events (n=163)."
fig_label = ""
fig.text(0.5, 0.01, fig_label, ha="center", fontsize=8,
         color=theme["fg"], style="italic", wrap=True,
         transform=fig.transFigure)

plt.tight_layout(rect=[0, 0.08, 1, 1])
plt.savefig("isr_timestamp_diff.png", dpi=180,
            bbox_inches="tight", facecolor=theme["bg"])
print("\nSaved → isr_timestamp_diff.png")
plt.show()
