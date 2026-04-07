# Convergence Map — Kernel ↔ Spec

> Two directions meeting in the middle. The kernel builds up from working code.
> The spec designs down from physical principles. This document tracks where they
> meet, what's missing, and what each side needs from the other.

---

## Primitive Alignment

| Spec Primitive | Kernel Implementation | Gap |
|---|---|---|
| **State** (data + type + confidence) | `wire` frames (typed payloads) | No confidence field per value. Type is frame-level, not word-level. |
| **Transformation** (confidence-propagating ops) | — | Not implemented. Need confidence algebra. |
| **Event** (source + causal order + confidence + payload) | `ipc` dispatch + `schedule` timers | No causal ordering (Lamport/vector). No per-event confidence. |

---

## Channel Model Alignment

| Spec Property | Kernel Implementation | Gap |
|---|---|---|
| Noise floor per channel | — | Not tracked. Wire has no noise concept. |
| Bandwidth / latency | — | Not tracked (runtime scheduling concern). |
| Protocol (voltage → encryption) | `auth` (HMAC-SHA256 handshake) | Maps to Noise Protocol handshake. Missing transport encryption (AES). |
| Message = typed + confidence-tagged payload | `wire` frames + `media` types | No confidence tag on messages. |

---

## Security Model Alignment

| Spec Concept | Kernel Implementation | Gap |
|---|---|---|
| Capabilities (unforgeable tokens) | `auth` cap_table (identity × frame_type → allow/deny) | ✅ Structural enforcement. |
| Delegation | — | Not implemented. |
| Attenuation (weaker derived caps) | — | Not implemented. |
| Noise Protocol handshake | `auth` challenge-response (HMAC-SHA256) | Challenge-response maps. Missing symmetric transport encryption. |
| Adversary = noise | — | Conceptual alignment, no code path yet. |

---

## Kernel Model Alignment

| Spec Responsibility | Kernel Module | Gap |
|---|---|---|
| Channel management (confidence map) | `router` (route table, health scoring, failover) | Health scoring ≈ noise floor proxy. Missing explicit confidence mapping. |
| Confidence allocation | — | Not implemented. This is the central missing primitive. |
| Event dispatch | `ipc` + `loop` | ✅ Working. |
| Capability management | `auth` cap_table | ✅ Grant/revoke/check. Missing delegate/attenuate. |
| Board support abstraction | — | Not implemented. Interface not defined in C yet. |

---

## BSP Interface (Spec → C)

The spec defines this interface. The kernel needs to implement it.

```
Platform {
  mem_read(addr, size)          → {data, confidence}
  mem_write(addr, data, type_tag)
  event_register(source, handler)
  event_wait()                  → Event
  channel_open(target)          → Channel
  channel_send(channel, message)
  channel_receive(channel)      → {message, confidence}
  channel_noise_floor(channel)  → float
  capabilities()                → {word_size, addressable_memory, channels, noise_baseline}
}
```

**Mapping to kernel modules:**

| BSP Function | Kernel Module | Status |
|---|---|---|
| `mem_read` / `mem_write` | `persist` (snapshot) | Persist does bulk state. Need per-value tagged access. |
| `event_register` / `event_wait` | `ipc` + `loop` | ✅ Working (handler registration + tick loop). |
| `channel_open` / `send` / `receive` | `wire` + `router` | Wire = framing. Router = routing. Missing channel lifecycle. |
| `channel_noise_floor` | — | Not implemented. |
| `capabilities` | — | Platform descriptor not defined. |

---

## Confidence Model (The Convergence Point)

This is where both directions meet. Neither side has it yet. It's load-bearing for:

1. **Thermal management** — confidence degrades with temperature (noise rises)
2. **Failure handling** — failed channel = confidence 0, recovery = confidence restoration
3. **Security** — adversary = noise source, encryption = confidence floor raiser
4. **Computation correctness** — operations refuse below threshold confidence
5. **Resource allocation** — kernel allocates based on confidence requirements

### What needs to be defined:

- **Tagged value type** — `{data, type_tag, confidence}` as a C struct
- **Confidence propagation algebra** — rules per operation class:
  - Arithmetic: `conf_out = min(conf_a, conf_b)` (conservative)
  - Crypto: requires `conf == 1.0` or refuses
  - I/O read: `conf = 1.0 - channel_noise_floor`
  - Aggregation: `conf_out = product(conf_inputs)` (Bayesian-ish)
- **Noise source interface** — BSP provides noise readings (thermal, channel, etc.)
- **Confidence budget allocator** — kernel function that distributes available confidence
- **Threshold enforcement** — operations check confidence before executing

---

## Heat / Thermal Loop (Physics → Software)

```
computation → heat → thermal noise rises → confidence degrades
→ drops below noise floor → propagation halts → chip cools
→ thermal noise drops → confidence recovers → propagation resumes
```

The BSP provides `thermal_noise()`. The kernel maps this to confidence penalties.
No external thermal controller needed — the confidence model IS the thermal governor.

### Implementation path:
1. BSP reports temperature as a noise source
2. Kernel adjusts channel noise floors based on thermal reading
3. Channels whose noise exceeds threshold stop propagating
4. Workload naturally reduces → heat dissipates → noise drops → channels resume

---

## Build Order (Both Directions)

### Kernel Up (sovereignty → convergence)

| # | Module | Maps To |
|---|---|---|
| 1 | ✅ Schedule | Event sequencing (spec: Events) |
| 2 | ✅ Journal | Crash recovery, causal history |
| 3 | ✅ Encrypt | Noise Protocol transport layer (spec: channel confidence floor) |
| 4 | ✅ Sync | Reconnection = confidence restoration |
| 5 | Export | Data portability |
| 6 | Migrate+ | Schema evolution |
| 7 | **Confidence** | Tagged values + propagation algebra |
| 8 | **BSP** | Platform interface |

### Spec Down (formalization → C headers)

| # | Artifact | Purpose |
|---|---|---|
| 1 | `tk_confidence.h` | Tagged value type, propagation rules, threshold enforcement |
| 2 | `tk_bsp.h` | Platform interface (BSP contract in C) |
| 3 | `tk_channel.h` | Channel lifecycle, noise floor, confidence mapping |
| 4 | Confidence algebra spec | Formal rules for propagation per operation class |
| 5 | Thermal-as-noise spec | BSP thermal sensor → confidence penalty mapping |

---

## Current State

- **Kernel:** 21/21 modules green. 4/6 sovereignty modules done.
- **Spec:** Architecture + Glossary complete. Confidence type + BSP interface drafted.
- **Gap:** Confidence model (algebra), channel lifecycle, delegation/attenuation.
- **Next:** Export module (kernel up) + confidence algebra formalization (spec down).
