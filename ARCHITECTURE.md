# TokenBasement Architecture

> Universal computing from physics up.

---

## 0. Premise

Modern computer architecture fights physics. It imposes synchronous clocks on asynchronous reality. It pretends signals are perfect when they degrade. It separates compute from storage and then spends billions of transistors hiding the bottleneck. It treats failure as exceptional when failure is the default state of the physical world.

TokenBasement starts from the other direction. Accept what physics gives you. Design with it, not against it. The result is an architecture that runs on anything — from a 6502 on a breadboard to a custom ASIC — because it's built on primitives that are true at every scale.

---

## 1. The Physical Stack

### Layer 0 — Physics

Electrons move through doped silicon. Voltage crosses a threshold or it doesn't. Every signal is analog. Digital is an abstraction — a useful one, but an abstraction. Noise is not a defect. Noise is the physical reality that every layer above must accommodate.

**Principle:** The architecture never assumes a perfect signal. Every value has a confidence level. Every channel has a noise floor.

### Layer 1 — Transistors

A transistor is a switch controlled by charge. MOSFET, BJT, or future substrates — the abstraction is the same: a controllable gate between two states. ON or OFF is a simplification. The transistor occupies a continuum; we pick a threshold and call it binary.

On commodity hardware, this layer is fixed — you get what the fab produced. On custom silicon, this is where design begins.

### Layer 2 — Logic Gates

Transistors compose into logic gates: NAND, NOR, NOT, AND, OR, XOR. Any computable function can be built from NAND alone. This is the lowest layer where you "hook chips together" — discrete logic ICs (74-series), FPGA LUT cells, or standard cell libraries.

**Key property:** Logic gates are combinational. Output is a pure function of current inputs. No state. No time. No memory.

### Layer 3 — State and Sequence

Add feedback (a flip-flop, a latch) and you get memory — state that persists across time. A register is a small collection of flip-flops. A counter is a register that increments. This is where time enters the architecture.

**Two approaches to sequencing:**

- **Synchronous:** A global clock signal triggers all state transitions simultaneously. Simple to reason about. Wasteful — every component waits for the slowest one. Clock distribution burns power and silicon area. Doesn't scale across distance.

- **Asynchronous:** Each element transitions when its inputs are ready. Handshake protocols replace the clock. Faster in practice (no waiting), lower power (no idle toggling), naturally tolerant of variable timing. Harder to design. Harder to verify.

**TokenBasement chooses async.** Synchronous clocking is available as a local optimization where hardware requires it, but the architectural model is event-driven, not clock-driven.

### Layer 4 — Datapath

ALU, register file, buses, memory interface. The machinery that moves and transforms data. On commodity hardware (x86, ARM, 6502), the datapath is fixed by the chip designer. On FPGA or custom silicon, you design it.

**Key insight:** The Von Neumann architecture forces all data through a single bus between CPU and memory. This is the fundamental bottleneck of modern computing. Caches, prefetchers, and branch predictors are all workarounds for this single design decision.

**TokenBasement principle:** Compute where the data lives. Don't move data to a processor — move the operation to the data. On commodity hardware, this is approximated through locality-aware scheduling. On custom silicon, processing elements are distributed throughout memory.

### Layer 5 — Control / Instruction Decode

The layer that gives meaning to binary patterns. An opcode maps to a sequence of datapath operations.

- **6502:** Hardwired PLA. Opcode → internal bus operations across clock cycles. What you write is close to what the silicon does.
- **ARM:** Hardwired decode. Fixed-width instructions, simple pipeline.
- **x86:** Microcode ROM. Complex CISC instructions decomposed into micro-ops. A hidden interpreter.
- **FPGA:** You define the decode logic. The ISA is whatever you want it to be.

On commodity hardware, this layer is given. On custom silicon, this is where you implement native support for the architecture's primitives (confidence tracking, capability enforcement, typed values).

### Layer 6 — ISA (Instruction Set Architecture)

The contract between hardware and software. Defines opcodes, registers, addressing modes, interrupt behavior, privilege levels.

TokenBasement doesn't define its own ISA for Phase 1. It targets existing ISAs through board support packages. The universal interface sits above the ISA.

For Phase 2 (custom silicon), the ISA would natively support:
- Tagged memory (type + confidence per word)
- Capability-addressed memory (can't form an address you weren't given)
- Confidence-propagating arithmetic
- Hardware message channels with built-in encryption handshake

---

## 2. Universal Primitives

Everything a computer does reduces to three primitives:

### State
Bits that persist. Registers, memory, storage. Addressable, typed, with an associated confidence level.

Every value in the system carries:
- **The data** — the bits
- **The type** — what the bits mean (integer, pointer, capability, message, etc.)
- **The confidence** — how certain we are the bits are correct (0.0 to 1.0)

On commodity hardware, type and confidence are tracked in software (tagged structs, metadata tables). On custom silicon, they're part of the word format in hardware.

### Transformation
Operations on state. Arithmetic, logic, comparison, encryption, encoding. A transformation takes typed inputs with confidence levels and produces typed outputs with derived confidence levels.

**Confidence propagation:** If you add two values at 99% and 95% confidence, the result's confidence is bounded by the inputs. The specific propagation model depends on the operation. Cryptographic operations require 100% input confidence or they refuse to execute.

### Events
Something changed. A signal arrived. A timer expired. A message was received. A threshold was crossed.

Events are the universal sequencing mechanism. Not clock ticks — occurrences. Each node runs at its own pace and reacts to events as they arrive.

**Event properties:**
- **Source** — which channel/node produced it
- **Causal order** — what happened before this (Lamport timestamp or vector clock)
- **Confidence** — how certain we are this event is authentic (derived from channel noise floor + encryption)
- **Payload** — the state change or message content

---

## 3. Communication Model

### Channels

Every communication between any two components — transistors on a die, chips on a bus, machines on a network, users across the internet — is modeled as a **channel**.

A channel has:
- **Noise floor** — baseline uncertainty (set by physics of the medium)
- **Bandwidth** — how much data per unit time
- **Latency** — how long between send and receive
- **Protocol** — how confidence is established (voltage thresholds, ECC, CRC, Noise encryption)

The architecture does not distinguish between:
- Two gates connected by a wire (noise floor: very low, latency: picoseconds)
- Two chips on a bus (noise floor: low, latency: nanoseconds)  
- Two boards on a backplane (noise floor: moderate, latency: microseconds)
- Two machines on a network (noise floor: variable, latency: milliseconds)
- Two nodes across the internet (noise floor: high, latency: variable, adversarial)

Same model. Same interface. Different parameters.

### Noise and Confidence

Every channel is noisy. The architecture's job is to establish sufficient confidence for the operation at hand.

| Channel | Noise source | Confidence mechanism |
|---|---|---|
| On-die wire | Thermal, crosstalk | Voltage thresholds, differential signaling |
| Chip-to-chip bus | Attenuation, EMI | Error-correcting codes (ECC, CRC) |
| Board-to-board | Signal degradation | Checksums, retransmission |
| Network | Packet loss, corruption | TCP/FEC, checksums |
| Untrusted network | Adversary, impersonation | Noise protocol encryption + authentication |
| Semantic | Version skew, misinterpretation | Types, schemas, contracts |

**It's noise all the way up.** The mechanism changes. The model doesn't.

### Message Passing

The universal communication primitive is the **message**: a typed, confidence-tagged payload sent over a channel.

No shared memory as a default. Shared memory is an optimization available when two nodes are on the same physical substrate and the coherency cost is justified. But the *model* is always message-passing. This means:

- No cache coherency problem (each node owns its state)
- No false sharing
- Network transparency (local and remote communication use the same interface)
- Natural distribution (adding nodes doesn't require architectural changes)

---

## 4. Failure Model

Failure is not exceptional. Failure is the default state.

A channel that fails is just a channel whose noise went to 100%. A node that dies is just a node that stopped producing events. There is no special "error path" — the confidence model handles it uniformly.

**Degradation, not crash:**
- Channel noise increases → confidence in messages decreases → system routes around it or reduces fidelity
- Node goes silent → its channels time out → downstream nodes work with last-known-good state at reduced confidence
- Bit flip in memory → confidence of that value drops → operations using it know they're working with degraded input

**Recovery is just confidence restoration.** A rebooted node is a channel that went from 0% to some baseline. A repaired wire is a channel whose noise floor dropped. No special recovery protocol — the confidence model naturally absorbs the node back into the system.

---

## 5. Security Model

Security is not a layer. Security is a property of the channel.

An adversary is indistinguishable from noise at the architectural level. An attacker flipping bits, injecting packets, or impersonating a node is just another source of noise on the channel. The defense is the same: increase confidence through redundancy and verification.

### Capabilities

Access control is structural, not permissive. You can't address what you weren't given a token for. Not "you're not allowed to" — **you physically can't form the request.**

A capability is an unforgeable token that grants access to a specific resource (memory range, channel, operation). The holder can:
- Use it (invoke the operation)
- Delegate it (pass the token to another node)
- Attenuate it (create a weaker version with fewer rights)
- Never: forge it, guess it, or escalate it

On commodity hardware, capabilities are enforced in kernel software. On custom silicon, the address bus itself only routes valid capability-bearing requests.

### Noise Protocol Integration

The Noise protocol framework provides the handshake patterns for establishing encrypted, authenticated channels between nodes. It maps directly to the channel model:

- **Handshake** — two nodes exchange public keys and establish a shared secret. This raises the channel's confidence floor from "untrusted" to "authenticated + encrypted."
- **Transport** — subsequent messages on the channel are encrypted and authenticated. Noise at this layer is reduced to what an adversary can achieve against the cipher.
- **Rekeying** — forward secrecy. Compromise of current keys doesn't reveal past traffic. The confidence of historical messages is preserved.

Every channel above the physical layer can optionally negotiate a Noise handshake. The cost is paid once per channel establishment. The benefit persists for the channel's lifetime.

---

## 6. Kernel Model

The kernel is the **confidence budget allocator**. It doesn't primarily schedule time — it manages certainty.

### Responsibilities

1. **Channel management** — maintain the confidence map of all channels. Monitor noise floors. Detect degradation. Route around failure.

2. **Confidence allocation** — different operations need different confidence levels. Crypto key comparison: 100%. Audio playback: 90%. Screen brightness: 60%. The kernel allocates hardware resources (power, redundancy, verification cycles) based on what each task actually requires.

3. **Event dispatch** — route events from channels to the nodes that care about them. No polling. No wasted cycles. Something happened → the right node finds out.

4. **Capability management** — create, delegate, attenuate, and revoke access tokens. Enforce the security model.

5. **Board support abstraction** — translate between the universal interface and platform-specific hardware. This is the only part that knows what chip it's running on.

### Board Support Package

Each hardware platform provides a BSP that implements:

```
interface Platform {
  // State
  mem_read(addr, size) → {data, confidence}
  mem_write(addr, data, type_tag)
  
  // Events  
  event_register(source, handler)
  event_wait() → Event
  
  // Channels
  channel_open(target) → Channel
  channel_send(channel, message)
  channel_receive(channel) → {message, confidence}
  channel_noise_floor(channel) → float
  
  // Platform info
  capabilities() → {word_size, addressable_memory, channels, noise_baseline}
}
```

This is what varies per chip. Everything above is universal.

---

## 7. Phase 1 — Commodity Hardware

Run the universal model on existing hardware. Accept the overhead of emulating async/confidence/messages on sync/perfect/shared-memory hardware.

### Target Platforms

**6502** — Simplest target. Flat memory, memory-mapped I/O, IRQ/NMI interrupts. The bus protocol is 16-bit address, 8-bit data, R/W line, clock. BSP maps directly. Confidence tracking in software.

**ARM (Raspberry Pi, etc.)** — MMU provides memory isolation for capability emulation. GIC handles interrupt routing for event dispatch. Multiple cores communicate via the message-passing model, avoiding coherency overhead.

**x86** — Most complex BSP. APIC for interrupts, virtual memory for capability emulation. Microcode layer is opaque but doesn't affect the model. Existing OS can host the kernel as a process for development; bare-metal for production.

**FPGA** — Most flexible. Can implement custom datapaths, native message channels, async logic. The bridge between Phase 1 and Phase 2 — you can prototype custom silicon primitives in FPGA fabric before committing to a fab.

### What You Pay

- Confidence tracking in software (metadata per value, propagation logic in every operation)
- Message passing emulated over shared memory or bus transactions
- Async model emulated over synchronous clocks (interrupt-driven, cooperative)
- Capability enforcement in kernel software (MMU tricks, table lookups)

### What You Get

- Uniform programming model across all platforms
- Natural distribution — add nodes without architectural changes  
- Graceful degradation — nodes fail, system adapts
- Security by default — capability model from day one
- A proven spec ready for Phase 2

---

## 8. Phase 2 — Custom Silicon

Chips designed to implement the primitives natively. Same spec, same protocol, no emulation overhead.

### Native Primitives

**Async logic** — No global clock. Handshake-based signaling. Each gate fires when inputs are ready. The chip runs at the speed physics allows at that moment, at that temperature, at that voltage. Self-optimizing.

**Tagged memory** — Every word carries type and confidence metadata in hardware. The ALU propagates confidence automatically. No software overhead for tracking.

**Capability-addressed memory** — The address bus only routes requests bearing valid capability tokens. Can't even form an invalid address. Security enforced by circuit topology, not permission checks.

**Native message channels** — Dedicated point-to-point links between processing elements. Noise protocol handshake implemented in silicon. No shared bus bottleneck.

**Confidence-aware ALU** — Arithmetic that knows about uncertainty. Automatically tracks and propagates confidence through computation. Refuses operations when input confidence is below threshold for that operation class.

**Distributed processing** — No single CPU. Processing elements distributed throughout memory. Computation moves to data, not data to computation. Eliminates the Von Neumann bottleneck entirely.

### Coexistence

Phase 2 chips are just nodes on the network with better parameters:
- Lower noise floor (native encryption, on-die channels)
- Lower latency (no emulation overhead)
- Lower power (async logic, no clock distribution, confidence-based power scaling)
- Higher confidence (hardware-enforced types and capabilities)

They communicate with Phase 1 hardware using the same protocol. A 6502 and a custom ASIC on the same network see each other as nodes with different capabilities. The kernel adapts automatically.

---

## 9. Open Questions

- **Confidence propagation model** — Exact semantics for how confidence flows through operations. Bayesian? Interval arithmetic? Custom algebra? Needs formal definition.
- **Causal ordering at scale** — Vector clocks grow linearly with node count. What's the practical ordering mechanism for large networks?
- **Minimum viable BSP** — What's the smallest useful subset of the platform interface? Can a BSP fit in the 6502's limited memory?
- **FPGA prototype scope** — Which Phase 2 primitives to prototype first? Tagged memory and async logic are the highest-value candidates.
- **Economic model** — How does confidence allocation translate to resource billing in a multi-tenant environment?
- **Formal verification** — Can the confidence propagation model be formally verified? Important for the security claims.
