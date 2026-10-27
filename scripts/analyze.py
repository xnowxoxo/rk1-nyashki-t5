#!/usr/bin/env python3
"""Считает S(p), E(p), Карп–Флатт по логам Slurm и рисует графики.

Запуск из корня репозитория:  python3 scripts/analyze.py
Читает logs/*.out, берёт минимум из запусков для каждой (версия, p),
T1 = минимальное время seq. Пишет таблицу в results.md,
графики в plots/ (нужен matplotlib):
  speedup.png, efficiency.png — все версии с двумя и более точками;
  omp_imbalance.png           — дисбаланс нагрузки между потоками OpenMP;
  omp_chunk.png               — влияние размера порции на максимуме потоков.
"""
import glob
import os
import re
from collections import defaultdict

times = defaultdict(list)       # (version, p) -> [t, ...]
imb = {}                        # (version, p) -> дисбаланс самого быстрого запуска (OpenMP)
best = {}                       # (version, p) -> время самого быстрого запуска
bad = []
for path in sorted(glob.glob("logs/*.out")):
    last = None
    for line in open(path, encoding="utf-8", errors="replace"):
        line = line.strip()
        parts = line.split(";")
        # #detail;omp_<sched>;p;...;imbalance=X — идёт сразу за строкой своего запуска
        m = re.search(r"imbalance=([\d.]+)", line)
        if line.startswith("#detail;omp_") and m and last:
            key, t = last
            if t <= best.get(key, float("inf")):
                best[key] = t
                imb[key] = float(m.group(1))
            continue
        if len(parts) != 4 or line.startswith("#") or parts[0] == "version":
            continue
        ver, p, t, check = parts
        try:
            p, t = int(p), float(t)
        except ValueError:
            continue
        if check != "OK":
            bad.append(f"{path}: {line}")
        times[(ver, p)].append(t)
        last = ((ver, p), t)

if ("seq", 1) not in times:
    raise SystemExit("В logs/ нет строк seq — сначала запустите mpi.sbatch")

T1 = min(times[("seq", 1)])
versions = sorted({v for v, _ in times if v != "seq"})

rows = [f"T1 (seq, минимум из {len(times[('seq', 1)])}) = {T1:.3f} с", ""]
series = {}
for v in versions:
    ps = sorted(p for vv, p in times if vv == v)
    has_imb = all((v, p) in imb for p in ps)
    head = "| p | T(p), с | S | E | e (Карп–Флатт) |" + (" дисбаланс |" if has_imb else "")
    rows += [f"### {v}", "", head, "|" + "---|" * (head.count("|") - 1)]
    series[v] = []
    for p in ps:
        T = min(times[(v, p)])
        S = T1 / T
        E = S / p
        e = (1 / S - 1 / p) / (1 - 1 / p) if p > 1 else float("nan")
        series[v].append((p, S, E))
        e_txt = "—" if p == 1 else f"{e:.4f}"
        row = f"| {p} | {T:.3f} | {S:.2f} | {E:.3f} | {e_txt} |"
        if has_imb:
            row += f" {imb[(v, p)]:.3f} |"
        rows.append(row)
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
# Версии с одной точкой (серия chunk на максимуме потоков) идут только в omp_chunk.png
lines = {v: s for v, s in series.items() if len(s) >= 2}
pmax = max(p for s in lines.values() for p, _, _ in s)
color = {v: f"C{i}" for i, v in enumerate(lines)}   # одна версия — один цвет на всех графиках

plt.figure(figsize=(6, 4.5))
plt.plot([1, pmax], [1, pmax], "k--", lw=1, label="идеал S = p")
for v, s in lines.items():
    plt.plot([x[0] for x in s], [x[1] for x in s], "o-", color=color[v], label=v)
plt.xscale("log", base=2); plt.yscale("log", base=2)
plt.xlabel("p (процессы / потоки)"); plt.ylabel("S(p)")
plt.title("Мандельброт 4096², ускорение"); plt.grid(alpha=.3); plt.legend(fontsize=8)
plt.tight_layout(); plt.savefig("plots/speedup.png", dpi=150)

plt.figure(figsize=(6, 4.5))
plt.axhline(1.0, color="k", ls="--", lw=1)
for v, s in lines.items():
    plt.plot([x[0] for x in s], [x[2] for x in s], "o-", color=color[v], label=v)
plt.xscale("log", base=2); plt.ylim(0, 1.1)
plt.xlabel("p (процессы / потоки)"); plt.ylabel("E(p)")
plt.title("Мандельброт 4096², эффективность"); plt.grid(alpha=.3); plt.legend(fontsize=8)
plt.tight_layout(); plt.savefig("plots/efficiency.png", dpi=150)
print("графики: plots/speedup.png, plots/efficiency.png")

omp = [v for v in lines if v.startswith("omp_")]
if omp:
    plt.figure(figsize=(6, 4.5))
    plt.axhline(1.0, color="k", ls="--", lw=1, label="идеал")
    for v in omp:
        ps = sorted(p for vv, p in imb if vv == v)
        plt.plot(ps, [imb[(v, p)] for p in ps], "o-", color=color[v], label=v)
    plt.xscale("log", base=2)
    plt.xlabel("p (потоки)"); plt.ylabel("max работы потока / среднее")
    plt.title("OpenMP, дисбаланс нагрузки"); plt.grid(alpha=.3); plt.legend(fontsize=8)
    plt.tight_layout(); plt.savefig("plots/omp_imbalance.png", dpi=150)
    print("график: plots/omp_imbalance.png")

# Размер порции: omp_<kind>,<c> на максимальном числе потоков
chunk = defaultdict(dict)       # kind -> {c: S}
for v, s in series.items():
    m = re.fullmatch(r"omp_(static|dynamic|guided),(\d+)", v)
    if m:
        pc = max(p for p, _, _ in s)
        chunk[m.group(1)][int(m.group(2))] = (pc, dict((p, S) for p, S, _ in s)[pc])
pcs = {pc for d in chunk.values() for pc, _ in d.values()}
if len(pcs) == 1 and any(len(d) >= 2 for d in chunk.values()):
    pc = pcs.pop()
    plt.figure(figsize=(6, 4.5))
    plt.axhline(pc, color="k", ls="--", lw=1, label=f"идеал S = {pc}")
    for kind in ("static", "dynamic", "guided"):
        cs = sorted(c for c, (p, _) in chunk[kind].items() if p == pc)
        if cs:
            plt.plot(cs, [chunk[kind][c][1] for c in cs], "o-", label=f"{kind},c",
                     color=color.get(f"omp_{kind},1"))
    if ("omp_static", pc) in times:
        S0 = T1 / min(times[("omp_static", pc)])
        plt.axhline(S0, color=color.get("omp_static", "gray"), ls=":", lw=1.5, label="static без chunk")
    plt.xscale("log", base=2); plt.ylim(0, pc * 1.1)
    plt.xlabel("chunk, строк"); plt.ylabel(f"S на {pc} потоках")
    plt.title("OpenMP, влияние размера порции"); plt.grid(alpha=.3); plt.legend(fontsize=8)
    plt.tight_layout(); plt.savefig("plots/omp_chunk.png", dpi=150)
    print("график: plots/omp_chunk.png")
