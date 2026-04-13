# TokenBasement

The hardware architecture specification for Infinite Token.

## What this is

TokenBasement is a spec. It defines how a computer should work from transistors up, starting from one premise: noise is physics, not a bug. Every signal degrades, every channel is unreliable, every node can fail. The architecture is designed around that reality instead of pretending it doesn't exist.

The spec covers 10 layers — from electron behavior in silicon up through an instruction set, a kernel model, and a path to custom chips. It's hardware-agnostic: the same model runs on a 6502, an ARM Pi, an x86 desktop, or an FPGA.

## What it is NOT

This repo has no code. No tests. No build system. It's three documents:

- `ARCHITECTURE.md` — the full spec, 9 sections, bottom to top
- `GLOSSARY.md` — term definitions
- `PROJECT.md` — this file

The code lives in [infinite-token](https://github.com/nes4less/infinite-token).

## How it connects to Infinite Token

TokenBasement is the blueprint. Infinite Token is the building.

| TokenBasement (spec) | Infinite Token (code) |
|---|---|
| "Every operation is a typed message on a channel" | Wire protocol: `0x544B`, self-describing binary frames, encode/decode in TS and C |
| "No delete — state transitions are logged" | CRUBA engine: Create, Read, Update, **Block**, Archive. Block replaces delete. Every mutation is audit-logged. |
| "Capabilities are unforgeable access tokens" | `src/capability/`: verb-based permissions. `os/kernel/auth/`: capability verification in C. |
| "Communication is message-passing, not shared memory" | `src/transport/`: Unix socket, TCP, WebSocket transports. `os/kernel/ipc/`: C kernel IPC server. |
| "Events drive execution, not clocks" | `src/events/`: EventBus with typed events, pub/sub, replay. `os/kernel/observe/`: event drains, spans, counters. |
| "Noise is real — confidence tracks certainty" | Partially implemented: rule denials are the accountability signal. The full confidence-per-value model is Phase 2 (custom silicon). |
| "The kernel manages confidence budgets" | `os/kernel/loop/`: main event loop. `os/kernel/router/`: capability-gated routing. `os/kernel/schedule/`: time-based scheduling. |
| "Same model across all hardware" | TS in browser + Node. C kernel for bare metal. Wire protocol byte-for-byte identical across both. LFS build targets ARM, x86_64, 6502. |
| "Custom silicon implements primitives natively" | Phase 2 — not started. The spec defines what the silicon would do; the software stack proves the model works before committing to a fab. |

## The stack, mapped

```
TokenBasement Spec Layer          Infinite Token Implementation
─────────────────────────         ─────────────────────────────
Layer 0-2: Physics → Logic        os/lfs/hardware/ (board configs)
Layer 3: State + Sequence          os/kernel/ (C, 30 modules)
Layer 4: Datapath                  os/kernel/wire/ (binary encode/decode)
Layer 5: Control / Decode          os/kernel/ipc/ (frame dispatch)
Layer 6: ISA                       os/lfs/ (targets: ARM, x86_64, 6502)
Universal Primitives:
  - State                          src/operations/ (CrubaEngine, StorageAdapter)
  - Transformation                 src/rules/ (anti-spam, guardrails)
  - Events                         src/events/ (EventBus)
Communication Model:
  - Channels                       src/transport/ (socket, TCP, WS)
  - Messages                       src/wire/ (binary frames)
  - Noise / Confidence             src/diagnostics/ (accountability layer)
Failure Model                      Block replaces delete. Denials logged.
Security Model:
  - Capabilities                   src/capability/ + os/kernel/auth/
  - Noise Protocol                 src/crypto/ (4-factor encryption)
Kernel                             os/kernel/loop/ + router/ + schedule/
Phase 1: Commodity HW              Everything that exists today
Phase 2: Custom Silicon            Not started — spec only
```

## What's implemented vs what's spec-only

| Concept | Status |
|---|---|
| Binary wire protocol | **Implemented** — TS + C, byte-compatible, 2,099→1,460 tests |
| CRUBA operations | **Implemented** — engine, rules, audit log, block store |
| Capability model | **Implemented** — TS registry + C kernel auth |
| Event-driven execution | **Implemented** — EventBus, kernel event loop, diagnostic drains |
| Message-passing transport | **Implemented** — Unix socket, TCP, WebSocket, memory |
| Identity + trust | **Implemented** — registry, trust scoring, handshakes |
| Process isolation | **Partially** — containment engine exists, has a bug |
| Bare metal boot | **Structured** — LFS Makefiles, PID 1 init, toolchain configs. Not yet booted. |
| Confidence per value | **Not implemented** — spec only. Would require tagged memory (Phase 2). |
| Async logic | **Not implemented** — spec only. Software runs on synchronous hardware. |
| Custom silicon | **Not implemented** — spec only. Phase 2. |
| Confidence-propagating ALU | **Not implemented** — spec only. Phase 2. |
| Capability-addressed memory | **Not implemented** — spec only. Phase 2. |

## Reading order

1. **This file** — understand what connects to what
2. **`ARCHITECTURE.md`** — the full spec, if you want to understand the "why" behind every design decision in Infinite Token
3. **`infinite-token/PROJECT.md`** — the code-level explanation of what's actually built
4. **`GLOSSARY.md`** — reference for terms

## The point

TokenBasement answers: what would a computer look like if you started from physics instead of from conventions?

Infinite Token answers: can you actually build that, and does it work?

The answer so far: the software model works. 26,378 lines of TypeScript, 64,343 lines of C, 1,460 tests passing, zero tsc errors, 28/28 C kernel modules green, byte-compatible wire protocol across both languages, bootable OS image structure for three architectures. The Phase 1 spec is proven in code. Phase 2 (custom silicon) remains a spec.
