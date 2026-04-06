/**
 * Token Confidence — Tagged values and confidence propagation.
 *
 * DRAFT — This header defines the convergence point between the
 * TokenBasement spec and the kernel implementation.
 *
 * Every value in the system carries:
 *   - The data (bits)
 *   - The type (what the bits mean)
 *   - The confidence (0.0–1.0, how certain we are the bits are correct)
 *
 * On commodity hardware (Phase 1), this is a struct wrapping the value.
 * On custom silicon (Phase 2), this metadata is part of the word format.
 *
 * The confidence model governs:
 *   1. Thermal management — heat degrades confidence, halting propagation
 *   2. Failure handling — dead channel = confidence 0, recovery = restoration
 *   3. Security — adversary = noise, encryption = confidence floor raiser
 *   4. Computation correctness — operations refuse below threshold
 *   5. Resource allocation — kernel distributes based on confidence needs
 */

#ifndef TK_CONFIDENCE_H
#define TK_CONFIDENCE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Type tags ── */

typedef enum {
    TK_TYPE_NONE        = 0x00,  /* Untyped / raw bits */
    TK_TYPE_INTEGER     = 0x01,  /* Signed integer */
    TK_TYPE_UNSIGNED    = 0x02,  /* Unsigned integer */
    TK_TYPE_FLOAT       = 0x03,  /* IEEE 754 float */
    TK_TYPE_POINTER     = 0x04,  /* Address (capability-checked) */
    TK_TYPE_CAPABILITY  = 0x05,  /* Unforgeable access token */
    TK_TYPE_MESSAGE     = 0x06,  /* Channel message payload */
    TK_TYPE_HASH        = 0x07,  /* Cryptographic hash */
    TK_TYPE_KEY         = 0x08,  /* Key material (requires conf=1.0 to read) */
    TK_TYPE_BOOLEAN     = 0x09,  /* True/false */
    TK_TYPE_TIMESTAMP   = 0x0A,  /* Causal timestamp (Lamport or vector) */
    TK_TYPE_NOISE       = 0x0B,  /* Noise measurement (from BSP sensor) */
    TK_TYPE_CONFIDENCE  = 0x0C,  /* Confidence value (meta — confidence about confidence) */
    TK_TYPE_CUSTOM      = 0xFF,  /* Host-defined type */
} tk_type_tag_t;

/* ── Confidence (fixed-point, 0–65535 maps to 0.0–1.0) ── */

/*
 * Why fixed-point and not float:
 *   - Deterministic across platforms (no IEEE rounding variance)
 *   - Maps directly to hardware registers (Phase 2)
 *   - Cheap comparison and arithmetic on embedded targets (6502)
 *   - 16-bit resolution is more than sufficient (1/65535 ≈ 0.0015%)
 *
 * TK_CONF_MAX (65535) = absolute certainty (1.0)
 * TK_CONF_MIN (0)     = no confidence (0.0)
 */

typedef uint16_t tk_confidence_t;

#define TK_CONF_MAX         65535   /* 1.0 — certain */
#define TK_CONF_MIN         0       /* 0.0 — no confidence */
#define TK_CONF_FULL        TK_CONF_MAX
#define TK_CONF_ZERO        TK_CONF_MIN

/* Common thresholds */
#define TK_CONF_CRYPTO      TK_CONF_MAX   /* Crypto ops require absolute confidence */
#define TK_CONF_HIGH        58982   /* ~0.90 */
#define TK_CONF_MEDIUM      39321   /* ~0.60 */
#define TK_CONF_LOW         19660   /* ~0.30 */

/* Convert to/from float (for display, not computation) */
#define TK_CONF_TO_FLOAT(c)    ((float)(c) / (float)TK_CONF_MAX)
#define TK_CONF_FROM_FLOAT(f)  ((tk_confidence_t)((f) * TK_CONF_MAX + 0.5f))

/* ── Tagged value ── */

/*
 * A tagged value: data + type + confidence.
 * This is the universal primitive the spec describes.
 *
 * In Phase 1 (commodity hardware), this is a struct.
 * In Phase 2 (custom silicon), the tag and confidence
 * would be part of the memory word format.
 *
 * The data field is a union — the type tag determines
 * which member is valid. On 6502 (8-bit), only the
 * smaller members are meaningful; the compiler handles layout.
 */

typedef struct {
    tk_type_tag_t    type;
    tk_confidence_t  confidence;
    union {
        int64_t      i64;
        uint64_t     u64;
        double       f64;
        uint8_t      bytes[32];     /* For hashes, keys, capabilities */
        void        *ptr;           /* For pointers (capability-checked) */
        int          boolean;
        uint64_t     timestamp;
        uint16_t     noise;         /* Noise reading (same scale as confidence) */
    } data;
} tk_tagged_value_t;

/* ── Noise floor (per-channel) ── */

/*
 * A channel's noise floor is the baseline uncertainty.
 * Messages arriving on this channel have confidence
 * capped at (TK_CONF_MAX - noise_floor).
 *
 * Noise sources:
 *   - Physical: thermal, EMI, signal degradation
 *   - Network: packet loss, latency variance
 *   - Adversarial: tampering, injection (indistinguishable from noise)
 *   - Semantic: version skew, encoding mismatch
 *
 * After a Noise Protocol handshake, the channel's noise floor
 * drops dramatically (encryption removes adversarial noise).
 */

typedef struct {
    tk_confidence_t  noise_floor;     /* Baseline noise (higher = noisier) */
    tk_confidence_t  thermal_noise;   /* Current thermal contribution */
    tk_confidence_t  channel_noise;   /* Medium-specific noise */
    tk_confidence_t  adversarial_noise; /* Estimated adversarial noise (0 after Noise handshake) */
    tk_confidence_t  effective_floor;  /* max(noise_floor, thermal + channel + adversarial) */
} tk_noise_state_t;

/* ── Confidence propagation rules ── */

/*
 * DRAFT — The confidence algebra. This is the spec's biggest
 * open question. These rules define how confidence flows through
 * operations.
 *
 * Conservative model (proposed):
 *
 * Arithmetic (add, sub, mul, div):
 *   conf_out = min(conf_a, conf_b)
 *   Rationale: result is only as reliable as the least reliable input.
 *
 * Comparison (eq, lt, gt):
 *   conf_out = min(conf_a, conf_b)
 *   A comparison between a certain and uncertain value is uncertain.
 *
 * Bitwise (and, or, xor):
 *   conf_out = min(conf_a, conf_b)
 *   Same reasoning as arithmetic.
 *
 * Cryptographic (encrypt, decrypt, hash, sign, verify):
 *   Requires: conf_inputs == TK_CONF_MAX
 *   conf_out = TK_CONF_MAX (if inputs were certain, crypto output is certain)
 *   Refuses to execute with uncertain inputs — crypto on noisy data is meaningless.
 *
 * I/O read (from channel):
 *   conf_out = TK_CONF_MAX - channel.effective_floor
 *   Data from a noisy channel arrives with proportionally reduced confidence.
 *
 * I/O write (to channel):
 *   No confidence change — the write is as confident as the value being written.
 *   The receiver applies their channel's noise floor.
 *
 * Aggregation (mean, sum over collection):
 *   conf_out = min(conf_all_inputs) OR product-based (TBD)
 *   The min model is simpler but loses information. The product model
 *   (multiply confidences) is more Bayesian but can degrade rapidly.
 *   Research needed.
 *
 * Copy / move:
 *   conf_out = conf_in
 *   Copying doesn't change confidence.
 *
 * Type cast:
 *   conf_out = conf_in (if lossless) OR conf_in * precision_ratio (if lossy)
 *   Truncating a double to int loses precision — confidence should reflect this.
 *
 * Thermal degradation:
 *   When temperature rises, all values in affected memory region get:
 *   conf_adjusted = conf - thermal_penalty
 *   Where thermal_penalty is proportional to temperature above baseline.
 *   This is the mechanism by which heat becomes a natural throttle.
 */

/* Propagation helpers */

static inline tk_confidence_t tk_conf_min(tk_confidence_t a, tk_confidence_t b) {
    return a < b ? a : b;
}

static inline tk_confidence_t tk_conf_max(tk_confidence_t a, tk_confidence_t b) {
    return a > b ? a : b;
}

/* Confidence after I/O read from a channel */
static inline tk_confidence_t tk_conf_from_channel(const tk_noise_state_t *noise) {
    if (noise->effective_floor >= TK_CONF_MAX) return TK_CONF_MIN;
    return TK_CONF_MAX - noise->effective_floor;
}

/* Check if confidence meets a threshold */
static inline int tk_conf_meets(tk_confidence_t conf, tk_confidence_t threshold) {
    return conf >= threshold;
}

/* Apply thermal penalty */
static inline tk_confidence_t tk_conf_apply_thermal(tk_confidence_t conf,
                                                      tk_confidence_t thermal_penalty) {
    if (thermal_penalty >= conf) return TK_CONF_MIN;
    return conf - thermal_penalty;
}

/* Recompute effective noise floor from components */
static inline void tk_noise_recompute(tk_noise_state_t *ns) {
    /* Effective floor is the max of: static floor, or sum of dynamic components */
    uint32_t dynamic = (uint32_t)ns->thermal_noise +
                       (uint32_t)ns->channel_noise +
                       (uint32_t)ns->adversarial_noise;
    if (dynamic > TK_CONF_MAX) dynamic = TK_CONF_MAX;

    tk_confidence_t dynamic_floor = (tk_confidence_t)dynamic;
    ns->effective_floor = dynamic_floor > ns->noise_floor ? dynamic_floor : ns->noise_floor;
}

#ifdef __cplusplus
}
#endif

#endif /* TK_CONFIDENCE_H */
