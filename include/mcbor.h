/*
 * microcbor — Minimal CBOR encoder/decoder for embedded systems.
 *
 * Implements a subset of RFC 8949 (CBOR) sufficient for IoT telemetry:
 * integers, booleans, null, floats, strings, byte arrays, arrays, and maps.
 *
 * C99 · Zero dependencies · Zero allocations · Portable
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/microcbor
 */

#ifndef MCBOR_H
#define MCBOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mcbor_config.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ── Error codes ───────────────────────────────────────────────────────── */

typedef int32_t mcbor_err_t;

#define MCBOR_OK              ((mcbor_err_t)0)   /**< Success.                            */
#define MCBOR_ERR_NULL        ((mcbor_err_t)-1)  /**< NULL pointer argument.              */
#define MCBOR_ERR_OVERFLOW    ((mcbor_err_t)-2)  /**< Buffer too small for encode.        */
#define MCBOR_ERR_UNDERFLOW   ((mcbor_err_t)-3)  /**< Not enough data to decode.          */
#define MCBOR_ERR_TYPE        ((mcbor_err_t)-4)  /**< Unexpected CBOR type.               */
#define MCBOR_ERR_INVALID     ((mcbor_err_t)-5)  /**< Malformed CBOR data.                */
#define MCBOR_ERR_SIZE        ((mcbor_err_t)-6)  /**< String/bytes too large for output.  */
#define MCBOR_ERR_RANGE       ((mcbor_err_t)-7)  /**< Host size exceeds wire-format max.  */
#define MCBOR_ERR_UNSUPPORTED ((mcbor_err_t)-8)  /**< Feature is not supported.           */

const char *mcbor_err_str(mcbor_err_t err);

/* ── CBOR types ────────────────────────────────────────────────────────── */

typedef uint8_t mcbor_type_t;

#define MCBOR_UINT    ((mcbor_type_t)0)  /**< Unsigned integer (major 0).               */
#define MCBOR_NEGINT  ((mcbor_type_t)1)  /**< Negative integer (major 1): value = -1-n. */
#define MCBOR_BYTES   ((mcbor_type_t)2)  /**< Byte string (major 2).                    */
#define MCBOR_TEXT    ((mcbor_type_t)3)  /**< UTF-8 text string (major 3).              */
#define MCBOR_ARRAY   ((mcbor_type_t)4)  /**< Array (major 4).                          */
#define MCBOR_MAP     ((mcbor_type_t)5)  /**< Map (major 5).                            */
#define MCBOR_SIMPLE  ((mcbor_type_t)7)  /**< Simple value / float (major 7).           */

typedef uint8_t mcbor_simple_kind_t;

#define MCBOR_SIMPLE_NONE    ((mcbor_simple_kind_t)0)
#define MCBOR_SIMPLE_BOOL    ((mcbor_simple_kind_t)1)
#define MCBOR_SIMPLE_NULL    ((mcbor_simple_kind_t)2)
#define MCBOR_SIMPLE_FLOAT32 ((mcbor_simple_kind_t)3)

const char *mcbor_type_name(mcbor_type_t type);

/* ═══════════════════════════════════════════════════════════════════════════
 * Encoder
 *
 * Write CBOR items sequentially into a caller-provided buffer.
 * The encoder tracks the write position and detects overflow.
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    uint8_t  *buf;       /**< Output buffer.                             */
    size_t    capacity;  /**< Buffer capacity in bytes.                  */
    size_t    pos;       /**< Current write position.                    */
    bool      overflow;  /**< Set true on first overflow (sticky).       */
} mcbor_enc_t;

/**
 * Initialise encoder.
 *
 * @param enc  Encoder instance (caller-allocated).
 * @param buf  Output buffer.
 * @param cap  Buffer capacity in bytes.
 */
mcbor_err_t mcbor_enc_init(mcbor_enc_t *enc, uint8_t *buf, size_t cap);

/** Get number of bytes written so far. */
mcbor_err_t mcbor_enc_size(const mcbor_enc_t *enc, size_t *out_size);

/** Check if overflow occurred. */
mcbor_err_t mcbor_enc_overflow(const mcbor_enc_t *enc, bool *out_overflow);

/* ── Encode primitives ─────────────────────────────────────────────────── */

/** Encode unsigned integer (0 .. UINT32_MAX). */
mcbor_err_t mcbor_enc_uint(mcbor_enc_t *enc, uint32_t value);

/** Encode signed integer (INT32_MIN .. INT32_MAX). */
mcbor_err_t mcbor_enc_int(mcbor_enc_t *enc, int32_t value);

/** Encode boolean. */
mcbor_err_t mcbor_enc_bool(mcbor_enc_t *enc, bool value);

/** Encode null. */
mcbor_err_t mcbor_enc_null(mcbor_enc_t *enc);

/** Encode IEEE 754 single-precision float. */
mcbor_err_t mcbor_enc_float(mcbor_enc_t *enc, float value);

/** Encode UTF-8 text string. */
mcbor_err_t mcbor_enc_text(mcbor_enc_t *enc, const char *str, size_t len);

/** Encode NUL-terminated text string. */
mcbor_err_t mcbor_enc_str(mcbor_enc_t *enc, const char *str);

/** Encode byte string. */
mcbor_err_t mcbor_enc_bytes(mcbor_enc_t *enc, const uint8_t *data, size_t len);

/* ── Encode containers ─────────────────────────────────────────────────── */

/**
 * Begin a definite-length array.
 * After this, encode exactly `count` items.
 */
mcbor_err_t mcbor_enc_array(mcbor_enc_t *enc, size_t count);

/**
 * Begin a definite-length map.
 * After this, encode exactly `count` key-value pairs (2 × count items).
 */
mcbor_err_t mcbor_enc_map(mcbor_enc_t *enc, size_t count);

/* ═══════════════════════════════════════════════════════════════════════════
 * Decoder
 *
 * Read CBOR items sequentially from a buffer.
 * The decoder tracks the read position and validates structure.
 * ═══════════════════════════════════════════════════════════════════════════ */

/** Decoded value — a tagged union of all supported types. */
typedef struct {
    mcbor_type_t type;
    union {
        uint32_t       uint_val;    /**< MCBOR_UINT                      */
        int32_t        int_val;     /**< MCBOR_NEGINT (already negated)  */
        bool           bool_val;    /**< MCBOR_SIMPLE (bool)             */
        float          float_val;   /**< MCBOR_SIMPLE (float)            */
        struct {
            const uint8_t *ptr;     /**< Pointer into input buffer.      */
            size_t         len;     /**< Length in bytes.                 */
        } bytes;                    /**< MCBOR_BYTES or MCBOR_TEXT        */
        size_t         container;   /**< MCBOR_ARRAY or MCBOR_MAP: count */
    } val;
    mcbor_simple_kind_t simple_kind;/**< Explicit subtype for simple values. */
} mcbor_value_t;

typedef struct {
    const uint8_t *buf;      /**< Input buffer.                          */
    size_t         size;     /**< Total buffer size.                     */
    size_t         pos;      /**< Current read position.                 */
} mcbor_dec_t;

/**
 * Initialise decoder.
 *
 * @param dec   Decoder instance (caller-allocated).
 * @param buf   Input buffer containing CBOR data.
 * @param size  Size of input data in bytes.
 */
mcbor_err_t mcbor_dec_init(mcbor_dec_t *dec, const uint8_t *buf, size_t size);

/** Get remaining unread bytes. */
mcbor_err_t mcbor_dec_remaining(const mcbor_dec_t *dec, size_t *out_remaining);

/** Check if all data has been consumed. */
mcbor_err_t mcbor_dec_done(const mcbor_dec_t *dec, bool *out_done);

/**
 * Decode the next CBOR item.
 *
 * @param dec  Decoder instance.
 * @param val  Output value.
 * @return MCBOR_OK on success, error code on failure.
 *
 * For containers (ARRAY, MAP), this returns the container header.
 * The caller then decodes the contained items one by one.
 */
mcbor_err_t mcbor_dec_next(mcbor_dec_t *dec, mcbor_value_t *val);

/* ── Convenience typed decoders ────────────────────────────────────────── */

/** Decode next item as unsigned integer. Returns MCBOR_ERR_TYPE if wrong. */
mcbor_err_t mcbor_dec_uint(mcbor_dec_t *dec, uint32_t *out);

/** Decode next item as signed integer (handles both uint and negint). */
mcbor_err_t mcbor_dec_int(mcbor_dec_t *dec, int32_t *out);

/** Decode next item as boolean. */
mcbor_err_t mcbor_dec_bool(mcbor_dec_t *dec, bool *out);

/** Decode next item as float. */
mcbor_err_t mcbor_dec_float(mcbor_dec_t *dec, float *out);

/**
 * Decode next item as text string.
 * Copies into caller's buffer with NUL termination.
 */
mcbor_err_t mcbor_dec_str(mcbor_dec_t *dec, char *out, size_t out_size,
                           size_t *out_len);

/**
 * Decode next item as byte string.
 * Returns pointer into input buffer (zero-copy) + length.
 */
mcbor_err_t mcbor_dec_bytes_ref(mcbor_dec_t *dec, const uint8_t **out,
                                 size_t *out_len);

/** Decode array header. Returns item count. */
mcbor_err_t mcbor_dec_array(mcbor_dec_t *dec, size_t *count);

/** Decode map header. Returns pair count. */
mcbor_err_t mcbor_dec_map(mcbor_dec_t *dec, size_t *count);

/**
 * Skip the next CBOR item (including nested containers).
 * Useful for ignoring unknown map keys.
 */
mcbor_err_t mcbor_dec_skip(mcbor_dec_t *dec);

#ifdef __cplusplus
}
#endif

#endif /* MCBOR_H */
