# Glossary

**Async** — Event-driven execution without a global clock. Components fire when their inputs are ready, not on a shared tick.

**Board Support Package (BSP)** — The thin platform-specific layer that translates between the universal kernel interface and actual hardware. The only part of the system that knows what chip it's running on.

**Capability** — An unforgeable token granting access to a specific resource. Cannot be forged, guessed, or escalated. Can be delegated or attenuated.

**Channel** — Any communication path between two components. A wire, a bus, a network link, an encrypted tunnel. Characterized by noise floor, bandwidth, latency, and protocol.

**Confidence** — A measure (0.0 to 1.0) of certainty that a value or message is correct. Derived from the noise characteristics of the channel that delivered it and the operations performed on it.

**Confidence Budget** — The kernel's allocation of hardware resources (power, redundancy, verification) based on what confidence level each operation requires.

**Datapath** — The hardware components (ALU, registers, buses) that move and transform data.

**Event** — The universal sequencing primitive. Something changed — a signal arrived, a timer expired, a message was received. Replaces clock ticks as the driver of computation.

**ISA (Instruction Set Architecture)** — The contract between hardware and software. Defines what binary patterns mean as operations.

**Message** — The universal communication primitive. A typed, confidence-tagged payload sent over a channel.

**Noise** — Any source of uncertainty in a signal. Thermal noise, electromagnetic interference, packet loss, adversarial tampering, semantic misinterpretation. The architecture treats all of these uniformly.

**Noise Floor** — The baseline uncertainty of a channel. Set by the physics of the medium and the protocols in use.

**Noise Protocol** — A framework for building encrypted channels through handshake patterns. Used to raise a channel's confidence floor from untrusted to authenticated + encrypted.

**Node** — Any autonomous computing element. A CPU core, a microcontroller, a machine on a network, a processing element in custom silicon. Has its own state, its own execution pace, communicates via channels.

**Phase 1** — Running the universal model on existing commodity hardware (x86, ARM, 6502, FPGA). Primitives emulated in software.

**Phase 2** — Custom silicon implementing the primitives natively. Same spec, no emulation overhead.

**Tagged Memory** — Memory where every word carries metadata: what type the data is and how confident we are in it. Software-emulated in Phase 1, hardware-native in Phase 2.

**Von Neumann Bottleneck** — The fundamental performance limitation of architectures where a single bus connects CPU and memory. All data and instructions compete for the same path.
