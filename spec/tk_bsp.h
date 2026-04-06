/**
 * Token BSP — Board Support Package interface.
 *
 * DRAFT — The contract between the universal kernel and platform-specific hardware.
 * This is the only part of the system that knows what chip it's running on.
 *
 * Each hardware platform (6502, ARM, x86, FPGA) provides an implementation
 * of this interface. Everything above it is universal.
 *
 * From the TokenBasement architecture spec:
 *   "Each hardware platform provides a BSP that implements [the platform
 *    interface]. This is what varies per chip. Everything above is universal."
 */

#ifndef TK_BSP_H
#define TK_BSP_H

#include <stdint.h>
#include <stddef.h>
#include "tk_confidence.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Error codes ── */

typedef enum {
    TK_BSP_OK                  = 0,
    TK_BSP_ERR_INVALID_ARG     = 1,
    TK_BSP_ERR_NO_CAPABILITY   = 2,   /* Caller lacks the capability for this operation */
    TK_BSP_ERR_ADDR_FAULT      = 3,   /* Invalid address */
    TK_BSP_ERR_CHANNEL_CLOSED  = 4,
    TK_BSP_ERR_CHANNEL_FULL    = 5,   /* Send buffer full */
    TK_BSP_ERR_TIMEOUT         = 6,
    TK_BSP_ERR_NOT_SUPPORTED   = 7,   /* Platform doesn't support this operation */
    TK_BSP_ERR_SENSOR_FAULT    = 8,   /* Hardware sensor read failed */
} tk_bsp_error_t;

/* ── Channel handle ── */

typedef uint32_t tk_channel_id_t;

#define TK_CHANNEL_INVALID  0

/* ── Event types ── */

typedef enum {
    TK_EVENT_MESSAGE     = 0x01,  /* Message arrived on channel */
    TK_EVENT_TIMER       = 0x02,  /* Timer expired */
    TK_EVENT_INTERRUPT   = 0x03,  /* Hardware interrupt */
    TK_EVENT_CHANNEL_UP  = 0x04,  /* Channel established */
    TK_EVENT_CHANNEL_DOWN = 0x05, /* Channel lost */
    TK_EVENT_THERMAL     = 0x06,  /* Thermal threshold crossed */
    TK_EVENT_CUSTOM      = 0xFF,
} tk_event_type_t;

typedef struct {
    tk_event_type_t     type;
    tk_channel_id_t     source;
    uint64_t            timestamp;     /* Platform-local timestamp (monotonic) */
    tk_confidence_t     confidence;    /* Event confidence (from channel noise floor) */
    const uint8_t      *payload;
    uint32_t            payload_len;
} tk_event_t;

/* Event handler callback */
typedef void (*tk_event_handler_fn)(const tk_event_t *event, void *ctx);

/* ── Platform capabilities descriptor ── */

typedef struct {
    uint8_t   word_size;          /* Bits per word: 8 (6502), 32 (ARM), 64 (x86) */
    uint32_t  addressable_memory; /* Bytes of addressable memory */
    uint16_t  max_channels;       /* Max concurrent channels */
    tk_confidence_t noise_baseline; /* Platform's inherent noise floor */
    int       has_mmu;            /* Memory management unit available */
    int       has_crypto_hw;      /* Hardware crypto acceleration */
    int       has_thermal_sensor; /* On-die temperature sensor */
    int       has_csprng;         /* Hardware random number generator */
    uint32_t  clock_hz;           /* 0 = async (no global clock) */
    const char *platform_id;      /* Human-readable: "6502", "rpi4", "x86_64", "fpga_ice40" */
} tk_platform_info_t;


/* ══════════════════════════════════════════════════════
 * BSP Interface — State (Memory)
 * ══════════════════════════════════════════════════════ */

/**
 * Read memory at address, returning data with confidence.
 *
 * On Phase 1 hardware, confidence is always TK_CONF_MAX (ECC handles bit errors).
 * On Phase 2 hardware, confidence comes from tagged memory hardware.
 *
 * @param addr       Memory address
 * @param size       Bytes to read
 * @param out_data   Output buffer
 * @param out_conf   Output confidence (per-read, not per-byte)
 */
typedef tk_bsp_error_t (*tk_bsp_mem_read_fn)(
    uint32_t addr, uint32_t size,
    uint8_t *out_data, tk_confidence_t *out_conf,
    void *bsp_ctx
);

/**
 * Write memory at address with type tag.
 *
 * On Phase 1, the type tag is stored in a software metadata table.
 * On Phase 2, the tag is written into the hardware word format.
 */
typedef tk_bsp_error_t (*tk_bsp_mem_write_fn)(
    uint32_t addr, const uint8_t *data, uint32_t size,
    tk_type_tag_t type_tag,
    void *bsp_ctx
);

/* ══════════════════════════════════════════════════════
 * BSP Interface — Events
 * ══════════════════════════════════════════════════════ */

/**
 * Register a handler for events from a source.
 * The handler is called when an event arrives on this channel/interrupt.
 */
typedef tk_bsp_error_t (*tk_bsp_event_register_fn)(
    tk_channel_id_t source,
    tk_event_handler_fn handler,
    void *handler_ctx,
    void *bsp_ctx
);

/**
 * Wait for the next event (blocking).
 * Returns when an event is available. On async hardware, this returns
 * immediately if an event is pending. On sync hardware, this blocks
 * until an interrupt or message arrives.
 *
 * @param out_event  Output event (valid until next wait call)
 * @param timeout_ms Max wait time (0 = no timeout)
 */
typedef tk_bsp_error_t (*tk_bsp_event_wait_fn)(
    tk_event_t *out_event,
    uint32_t timeout_ms,
    void *bsp_ctx
);

/* ══════════════════════════════════════════════════════
 * BSP Interface — Channels
 * ══════════════════════════════════════════════════════ */

/**
 * Open a channel to a target.
 * The target address format is platform-specific:
 *   - 6502: memory-mapped I/O address
 *   - ARM: inter-core mailbox ID or peripheral address
 *   - x86: interrupt vector or socket descriptor
 *   - Network: IP:port or node ID
 */
typedef tk_bsp_error_t (*tk_bsp_channel_open_fn)(
    const uint8_t *target_addr, uint32_t addr_len,
    tk_channel_id_t *out_channel,
    void *bsp_ctx
);

/**
 * Send a message on a channel.
 * The message is a byte buffer with a type tag and confidence level.
 */
typedef tk_bsp_error_t (*tk_bsp_channel_send_fn)(
    tk_channel_id_t channel,
    const uint8_t *data, uint32_t len,
    tk_type_tag_t type_tag,
    tk_confidence_t confidence,
    void *bsp_ctx
);

/**
 * Receive a message from a channel (blocking or non-blocking).
 * Returns the message data, type tag, and confidence (degraded by channel noise).
 *
 * @param out_data       Output buffer
 * @param out_data_size  Buffer capacity
 * @param out_len        Actual message length
 * @param out_type       Message type tag
 * @param out_conf       Message confidence (after channel noise applied)
 * @param timeout_ms     Max wait (0 = non-blocking, UINT32_MAX = block forever)
 */
typedef tk_bsp_error_t (*tk_bsp_channel_recv_fn)(
    tk_channel_id_t channel,
    uint8_t *out_data, uint32_t out_data_size,
    uint32_t *out_len,
    tk_type_tag_t *out_type,
    tk_confidence_t *out_conf,
    uint32_t timeout_ms,
    void *bsp_ctx
);

/**
 * Query a channel's current noise floor.
 * Includes thermal, medium, and adversarial components.
 */
typedef tk_bsp_error_t (*tk_bsp_channel_noise_fn)(
    tk_channel_id_t channel,
    tk_noise_state_t *out_noise,
    void *bsp_ctx
);

/**
 * Close a channel.
 */
typedef tk_bsp_error_t (*tk_bsp_channel_close_fn)(
    tk_channel_id_t channel,
    void *bsp_ctx
);

/* ══════════════════════════════════════════════════════
 * BSP Interface — Thermal / Noise Sensors
 * ══════════════════════════════════════════════════════ */

/**
 * Read the current die/board temperature.
 * Returns a thermal noise value that maps to confidence degradation.
 *
 * The mapping: higher temperature → higher thermal_noise → lower confidence.
 * The kernel uses this to throttle computation naturally.
 *
 * @param out_thermal_noise  Current thermal noise (0 = cool, TK_CONF_MAX = critical)
 * @param out_temp_raw       Raw temperature value (platform-specific units, may be NULL)
 */
typedef tk_bsp_error_t (*tk_bsp_thermal_read_fn)(
    tk_confidence_t *out_thermal_noise,
    uint32_t *out_temp_raw,
    void *bsp_ctx
);

/* ══════════════════════════════════════════════════════
 * BSP Interface — Platform Info
 * ══════════════════════════════════════════════════════ */

/**
 * Get platform capabilities and parameters.
 */
typedef tk_bsp_error_t (*tk_bsp_info_fn)(
    tk_platform_info_t *out_info,
    void *bsp_ctx
);

/* ══════════════════════════════════════════════════════
 * BSP vtable — one struct, all platform functions
 * ══════════════════════════════════════════════════════ */

typedef struct {
    /* State */
    tk_bsp_mem_read_fn       mem_read;
    tk_bsp_mem_write_fn      mem_write;

    /* Events */
    tk_bsp_event_register_fn event_register;
    tk_bsp_event_wait_fn     event_wait;

    /* Channels */
    tk_bsp_channel_open_fn   channel_open;
    tk_bsp_channel_send_fn   channel_send;
    tk_bsp_channel_recv_fn   channel_recv;
    tk_bsp_channel_noise_fn  channel_noise;
    tk_bsp_channel_close_fn  channel_close;

    /* Sensors */
    tk_bsp_thermal_read_fn   thermal_read;  /* NULL if no thermal sensor */

    /* Info */
    tk_bsp_info_fn           info;

    /* Opaque context (pointer to platform-specific state) */
    void                    *ctx;
} tk_bsp_t;

#ifdef __cplusplus
}
#endif

#endif /* TK_BSP_H */
