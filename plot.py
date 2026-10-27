# S(p), E(p) and Karp-Flatt e(p) from the Slurm logs.  Usage (from this folder): python3.11 plot.py  (needs matplotlib)
# Each time is the minimum over its runs; T1 = minimum time of the seq version.
#   S = T1 / T(p),  E = S / p,  e = (1/S - 1/p) / (1 - 1/p)
import glob
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

TITLE = "Трек 5 · Мандельброт 4096², 2000 итер."
COLORS = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100"]  # categorical slots 1-4, fixed order
MARKERS = ["o", "s", "^", "D"]
INK, MUTED, GRID = "#0b0b0b", "#52514e", "#e5e4df"

best = {}  # (version, p) -> min time_s
for path in sorted(glob.glob("logs/*.out")):
    for line in open(path):
        f = line.strip().split(";")
        if line.startswith("#") or len(f) != 4 or f[0] == "version":
            continue
        if f[3] != "CHECK: OK":
            sys.exit(f"{path}: {line.strip()}")
        key = (f[0], int(f[1]))
        best[key] = min(best.get(key, float("inf")), float(f[2]))

t1 = best[("seq", 1)]
versions = list(dict.fromkeys(v for v, _ in best if v != "seq"))
ps = sorted({p for v, p in best if v != "seq"})
S = {k: t1 / t for k, t in best.items()}

rows = [f"T1 (seq) = {t1:.4f} s, each time = min of 3 runs\n",
        "| version | p | T(p), s | S = T1/T(p) | E = S/p | e (Karp-Flatt) |", "|---|---|---|---|---|---|"]
for v in versions:
    for p in ps:
        s = S[(v, p)]
        e = f"{(1 / s - 1 / p) / (1 - 1 / p):.4f}" if p > 1 else "-"
        rows.append(f"| {v} | {p} | {best[(v, p)]:.4f} | {s:.2f} | {s / p:.3f} | {e} |")
open("plots/metrics.md", "w").write("\n".join(rows) + "\n")
print("\n".join(rows))


def plot(name, ylabel, y, ideal, ideal_label):
    fig, ax = plt.subplots(figsize=(7, 4.4), dpi=150)
    ax.plot(ps, [ideal(p) for p in ps], "--", color=MUTED, lw=1.2, label=ideal_label)
    for v, c, m in zip(versions, COLORS, MARKERS):
        ax.plot(ps, [y(v, p) for p in ps], color=c, marker=m, ms=6, lw=2, label=v.removeprefix("omp-"))
    ax.set_xticks(ps)
    ax.set_xlabel("p, потоков OpenMP", color=INK)
    ax.set_ylabel(ylabel, color=INK)
    ax.set_title(TITLE, color=INK, fontsize=11, loc="left")
    ax.grid(color=GRID, lw=0.6)
    ax.tick_params(colors=MUTED)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    ax.legend(frameon=False, labelcolor=INK, loc="upper left", bbox_to_anchor=(1.01, 1))  # outside: never covers data
    fig.savefig(f"plots/{name}.png", bbox_inches="tight")


plot("speedup", "Ускорение S = T₁ / T(p)", lambda v, p: S[(v, p)], lambda p: p, "идеал S = p")
plot("efficiency", "Эффективность E = S / p", lambda v, p: S[(v, p)] / p, lambda p: 1, "идеал E = 1")
