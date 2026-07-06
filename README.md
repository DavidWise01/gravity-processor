# Gravity Processor

**Author:** David Wise (ROOT0) / TriPod LLC  
**License:** CC-BY-ND-4.0 · TRIPOD-IP-v1.1

Analog attractor-field processor using Ag₂S memristive nodes. 42-prim kernel — analog nearest-attractor / winner-take-all classifier. 1mm×1mm die, 96µW @ 10kHz, 100µs solution time.

---

## 42-Prim Kernel — Spec

```
Total Nodes: 48
├── Live Tensors: 4    — CPG clock + spacetime metric
├── Gravity Prims: 42  — Ag₂S mass/charge attractors, hex-packed
├── IN: 1              — Mass injector
└── OUT: 1             — Trajectory readout

Die:     1mm × 1mm
Power:   96µW @ 10kHz
Latency: 0µs startup, 100µs solution
Material: Ag₂S nodes + Ti₃SiC₂ traces on SiO₂
```

### Live Tensor Cores (CPG Clock)

| Node | Phase | Role |
|------|-------|------|
| L0 | 0° | Time axis `g_tt` |
| L1 | 90° | X-axis `g_xx` |
| L2 | 180° | Y-axis `g_yy` |
| L3 | 270° | Z-axis `g_zz` |

### Ag₂S ECM Cell

```
theta ∈ [0,1] — filament state
g(theta) = G_OFF*(1-theta) + G_ON*theta
SET via sinh ion-drift, tau_fire=15µs at V_SET
0.65 eV retention barrier (non-volatile)
```

Dynamics: overdamped (ions in ionic medium). Probe descends potential well and settles — analog attractor resolution.

---

## Files

| File | Description |
|------|-------------|
| `gravity.c` | Full 42-prim kernel implementation in C |
| `memristive_kernel_128.c` | 128-node memristive kernel |
| `memristive_seed_v2.c` | Memristive seed v2 |
| `memristive_dashboard.jsx` | React dashboard for memristive state |
| `Makefile` | Build file |
| `gravity-bom/` | Bill of materials + 4 protoboard layout diagrams |
| `boron/` | Boron single-chip design |
| `liquid-proc/` | Liquid processor variant |

## gravity-bom — Physical Build

The `gravity-bom/` folder contains:
- `42 prim.md` — complete spec in markdown
- `BOM_42prim.json` — full bill of materials
- `BOM_42prim 00.json` — BOM variant 00
- 4× protoboard layout diagram JPEGs (detailed physical build reference)

---

*"Gravity = analog ATTRACTOR FIELD, not Newtonian gravity."*  
*"A charged probe drifts through superposed wells; where it settles is the result."*
