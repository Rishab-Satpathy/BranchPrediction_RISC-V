# HARP: Hierarchical Adaptive RISC-V Predictor

HARP is a hybrid, context-aware branch prediction and frontend fetch architecture for RISC-V. Rather than allocating long history tables uniformly across all branches, HARP uses a two-level **Branch Context Store** and a **Shared Context Router** to assign specialized predictors only where they measurably reduce mispredictions.

All stages are built and validated in a cycle-aware C++ reference simulator before any hardware translation.

---

## Project Status

| Phase | Status | Summary |
| :--- | :--- | :--- |
| **Stage 1: Software Baseline** | Complete | C++ trace simulator with Bimodal, Gshare, direct-mapped BTB, and 6 diagnostic traces. |
| **Architecture Freeze** | Complete | Two-level context store, parallel-read/late-mux routing, and speculative recovery spec locked. |
| **Stage A: Speculative Frontend** | Next | Pipeline-aware C++ simulator with resolution delay, GHR/PHR, RAS, and checkpoint rollback. |
| **Stages B–D: Advanced Simulation** | Planned | TAGE, Local History, BCD/Router, and PNT prefetch experiments. |

---

## What Is Built (Stage 1)

The current C++ baseline compiles and runs trace-driven experiments across small, fixed-size tables to isolate core branch behaviors:

* **Bimodal Predictor:** 16-entry table of 2-bit saturating counters (handles stable and independent branches cleanly).
* **Gshare Predictor:** 16-entry table indexed by PC XORed with a 4-bit GHR (captures correlated and alternating patterns, but introduces history pollution).
* **Branch Target Buffer (BTB):** 16-entry direct-mapped target cache with valid bits, tags, and branch targets.
* **Diagnostic Suite (6 Traces):** `loop`, `independent`, `correlated`, `alternating`, `aliasing`, and `btb_conflict`—confirming that direction prediction and target prediction fail differently and must be decoupled.

---

## Locked Architecture

HARP separates **conditional direction prediction**, **target prediction**, and **non-architectural prefetch hints**, coordinating them through a shared context directory.

```text
Fetch PC + Speculative Context (GHR, Folded PHR, RAS)
   |
   +---> [Fast Path]   L0 BTB / L0 Context Cache (HBT) + Shared Bimodal (T0) + RAS
   +---> [Specialists] TAGE (T1..Tn, internal USE_ALT_ON_NA) + Selective Local History
   +---> [Targets]     L1 BTB + Indirect Target Predictor
   +---> [Context]     L1 Branch Context Directory (BCD) + Router LUT (in parallel)
                  |
                  v
         Late Selection Mux ---> Final Direction + Target ---> Next Fetch PC
                  |
         (At EX Resolution / Flush)
                  v
   Checkpoint Recovery (on mispredict) + Router Allocation Gating & Counter Training
```

### Core Subsystems

* **Two-Level Branch Context Store (L0 + L1):**
  * **L0 (Merged HBT):** Co-tagged with the L0 BTB. Holds static branch class, hotness metadata, cached routing preference, and a dedicated base counter (eliminating aliasing without creating a separate predictor family).
  * **L1 (BCD):** Tagged directory tracking dynamic branch frequency, expert utility, allocation eligibility, and conflict flags. Counters decay periodically (32,768-branch interval) so initialization phases do not permanently lock resources.
* **Conditional Direction Predictors:**
  * **Bimodal ($T_0$):** Fast Cycle-0 default, shared directly as TAGE's base table.
  * **TAGE ($T_1 \dots T_n$):** Primary multi-history tagged predictor with geometric history lengths and internal `USE_ALT_ON_NA` alternate selection.
  * **Tagged Local History:** Constrained specialist allocated only to branches flagged for persistent global-history pollution.
  * *(Long Gshare is retained strictly as a C++ comparison baseline, not a target hardware block.)*
* **Shared Context Router:** Runs in parallel with the predictors to drive a late output mux (avoiding critical-path delay), and gates new allocations into TAGE and Local History on the update path to prevent tag thrashing.
* **Target Subsystem & Speculative State:** Decoupled L0/L1 BTB (with decode back-annotated branch classes), Indirect Target Predictor, and RAS. Speculative GHR, folded PC-target PHR, and RAS updates are tracked via in-flight `PredictionRecord` checkpoints for exact rollback on misprediction.
* **Path Neighbourhood Tree (PNT):** Bounded path-footprint cache supplying non-architectural hints for L0 BTB/BCD prewarming and bounded alternate-path prefetching on low-confidence branches.

---

## Simulation Roadmap

| Stage | Focus | Deliverables | Proof Target |
| :--- | :--- | :--- | :--- |
| **A** | Speculative Frontend | Resolution delay queue, Bimodal ($T_0$), L0/L1 BTB, speculative GHR/PHR, RAS, and checkpoint rollback | Bit-exact rollback and zero-delay equivalence with Stage 1 |
| **B** | Standalone Specialists | Geometric TAGE (`USE_ALT_ON_NA`), constrained Tagged Local History, and Indirect target predictor | Standalone specialist accuracy and tag-thrashing baseline |
| **C** | HARP Control Brain | L0/L1 Context Store, periodic aging, parallel-read/late-mux Router, and allocation gating | Outperform standalone TAGE at equal total storage |
| **D** | Path Acceleration | PNT footprint descriptors, L0 prewarming, and bounded alternate-path instruction prefetching | Better frontend fetch delivery without wrong-path pollution |

### Evaluation Rules
1. **Equal-Storage Benchmarking:** Every bit (counters, tags, valid/usefulness bits, BTB targets, BCD metadata, router LUTs, RAS, and in-flight checkpoints) counts toward total capacity. HARP must beat standalone TAGE at the exact same total bit budget.
2. **Runtime Metadata Learning:** Static branch classes and direct targets are learned dynamically via decode back-annotation on first encounter rather than pre-loaded binary maps.

---

## Post-Simulation (Future Work)

Hardware sizing, bit-budget ablation (pruning low-utility counters and metadata fields), banked SRAM mapping, and synthesizable SystemVerilog integration into a five-stage RISC-V CPU are deferred until the C++ reference architecture is validated across Stages A–D.
