#!/usr/bin/env python3
"""Считает S(p), E(p), Карп–Флатт по логам Slurm и рисует графики.

Запуск из корня репозитория:  python3 scripts/analyze.py
Читает logs/*.out, берёт минимум из запусков для каждой (версия, p),
T1 = минимальное время seq. Пишет таблицу в results.md,
графики в plots/speedup.png и plots/efficiency.png (нужен matplotlib).
"""
import glob
import os
from collections import defaultdict

times = defaultdict(list)       # (version, p) -> [t, ...]
bad = []
for path in sorted(glob.glob("logs/*.out")):
    for line in open(path, encoding="utf-8", errors="replace"):
        parts = line.strip().split(";")
        if len(parts) != 4 or line.startswith("#") or parts[0] == "version":
            continue
        ver, p, t, check = parts
        try:
            p, t = int(p), float(t)
        except ValueError:
            continue
        if check != "OK":
            bad.append(f"{path}: {line.strip()}")
        times[(ver, p)].append(t)

if ("seq", 1) not in times:
    raise SystemExit("В logs/ нет строк seq — сначала запустите mpi.sbatch")

T1 = min(times[("seq", 1)])
versions = sorted({v for v, _ in times if v != "seq"})

rows = [f"T1 (seq, минимум из {len(times[('seq', 1)])}) = {T1:.3f} с", ""]
series = {}
for v in versions:
    ps = sorted(p for vv, p in times if vv == v)
    rows += [f"### {v}", "", "| p | T(p), с | S | E | e (Карп–Флатт) |",
             "|---|---|---|---|---|"]
    series[v] = []
    for p in ps:
        T = min(times[(v, p)])
        S = T1 / T
        E = S / p
        e = (1 / S - 1 / p) / (1 - 1 / p) if p > 1 else float("nan")
        series[v].append((p, S, E))
        e_txt = "—" if p == 1 else f"{e:.4f}"
        rows.append(f"| {p} | {T:.3f} | {S:.2f} | {E:.3f} | {e_txt} |")
    rows.append("")

if bad:
    rows += ["### Запуски без CHECK: OK", ""] + [f"- `{b}`" for b in bad]

open("results.md", "w", encoding="utf-8").write("\n".join(rows) + "\n")
print("\n".join(rows))

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
except ImportError:
    raise SystemExit("matplotlib не установлен — таблица в results.md, графики пропущены")

os.makedirs("plots", exist_ok=True)
pmax = max(p for s in series.values() for p, _, _ in s)

plt.figure(figsize=(6, 4.5))
plt.plot([1, pmax], [1, pmax], "k--", lw=1, label="идеал S = p")
for v, s in series.items():
    plt.plot([x[0] for x in s], [x[1] for x in s], "o-", label=v)
plt.xscale("log", base=2); plt.yscale("log", base=2)
plt.xlabel("p (процессы / потоки)"); plt.ylabel("S(p)")
plt.title("Мандельброт 4096², ускорение"); plt.grid(alpha=.3); plt.legend()
plt.tight_layout(); plt.savefig("plots/speedup.png", dpi=150)

plt.figure(figsize=(6, 4.5))
plt.axhline(1.0, color="k", ls="--", lw=1)
for v, s in series.items():
    plt.plot([x[0] for x in s], [x[2] for x in s], "o-", label=v)
plt.xscale("log", base=2); plt.ylim(0, 1.1)
plt.xlabel("p (процессы / потоки)"); plt.ylabel("E(p)")
plt.title("Мандельброт 4096², эффективность"); plt.grid(alpha=.3); plt.legend()
plt.tight_layout(); plt.savefig("plots/efficiency.png", dpi=150)
print("графики: plots/speedup.png, plots/efficiency.png")
