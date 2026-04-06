# TokenBasement

The foundational architecture spec for Infinite Token's universal computing platform.

Everything above this is software. This is where software meets physics.

## What This Is

A specification for a universal computing architecture that:

- Runs on any existing hardware (x86, ARM, 6502, FPGA, microcontrollers)
- Treats noise as a first-class physical reality, not a bug to suppress
- Communicates asynchronously at every layer — no global clock, no global sync
- Provides a uniform interface from transistor-level to application-level
- Leaves the door open for custom silicon that implements the primitives natively

## Documents

- [ARCHITECTURE.md](ARCHITECTURE.md) — The full spec, bottom to top
- [GLOSSARY.md](GLOSSARY.md) — Terms and definitions

## Phases

**Phase 1: Commodity Hardware** — Universal software model running on existing chips. Board support packages translate between the universal interface and platform-specific hardware.

**Phase 2: Custom Silicon** — Chips designed to implement the primitives natively. Same spec, same protocol, dramatically less overhead. Coexists with Phase 1 hardware on the same network.
