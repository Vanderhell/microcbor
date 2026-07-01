/*
 * microcbor — Implementation.
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/microcbor
 */

#include "mcbor.h"
#include <float.h>
#include <string.h>

/* ── CBOR wire format constants ────────────────────────────────────────── */

#define CBOR_MAJOR_SHIFT  5
#define CBOR_MAJOR_MASK   0xE0
#define CBOR_INFO_MASK    0x1F

#define CBOR_MAJOR_UINT   (0U << CBOR_MAJOR_SHIFT)   /* 0x00 */
#define CBOR_MAJOR_NEGINT (1U << CBOR_MAJOR_SHIFT)   /* 0x20 */
#define CBOR_MAJOR_BYTES  (2U << CBOR_MAJOR_SHIFT)   /* 0x40 */
#define CBOR_MAJOR_TEXT   (3U << CBOR_MAJOR_SHIFT)   /* 0x60 */
#define CBOR_MAJOR_ARRAY  (4U << CBOR_MAJOR_SHIFT)   /* 0x80 */
#define CBOR_MAJOR_MAP    (5U << CBOR_MAJOR_SHIFT)   /* 0xA0 */
#define CBOR_MAJOR_SIMPLE (7U << CBOR_MAJOR_SHIFT)   /* 0xE0 */

#define CBOR_INFO_1BYTE   24
#define CBOR_INFO_2BYTE   25
#define CBOR_INFO_4BYTE   26

#define CBOR_SIMPLE_FALSE 20
#define CBOR_SIMPLE_TRUE  21
#define CBOR_SIMPLE_VALUE_NULL 22

static mcbor_err_t check_u32_range(size_t value)
{
    if (value > UINT32_MAX) {
        return MCBOR_ERR_RANGE;
    }
    return MCBOR_OK;
}

static bool float32_supported(void)
{
    return FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128 &&
           sizeof(float) == 4;
}

static bool is_valid_utf8(const uint8_t *data, size_t len)
{
    size_t i = 0;

    while (i < len) {
        uint8_t c = data[i++];

        if (c <= 0x7F) {
            continue;
        }

        if (c >= 0xC2 && c <= 0xDF) {
            if (i >= len || (data[i] & 0xC0) != 0x80) return false;
            i++;
            continue;
        }

        if (c == 0xE0) {
            if (i + 1 >= len) return false;
            if (data[i] < 0xA0 || data[i] > 0xBF) return false;
            if ((data[i + 1] & 0xC0) != 0x80) return false;
            i += 2;
            continue;
        }

        if (c >= 0xE1 && c <= 0xEC) {
            if (i + 1 >= len) return false;
            if ((data[i] & 0xC0) != 0x80 || (data[i + 1] & 0xC0) != 0x80) {
                return false;
            }
            i += 2;
            continue;
        }

        if (c == 0xED) {
            if (i + 1 >= len) return false;
            if (data[i] < 0x80 || data[i] > 0x9F) return false;
            if ((data[i + 1] & 0xC0) != 0x80) return false;
            i += 2;
            continue;
        }

        if (c >= 0xEE && c <= 0xEF) {
            if (i + 1 >= len) return false;
            if ((data[i] & 0xC0) != 0x80 || (data[i + 1] & 0xC0) != 0x80) {
                return false;
            }
            i += 2;
            continue;
        }

        if (c == 0xF0) {
            if (i + 2 >= len) return false;
            if (data[i] < 0x90 || data[i] > 0xBF) return false;
            if ((data[i + 1] & 0xC0) != 0x80 || (data[i + 2] & 0xC0) != 0x80) {
                return false;
            }
            i += 3;
            continue;
        }

        if (c >= 0xF1 && c <= 0xF3) {
            if (i + 2 >= len) return false;
            if ((data[i] & 0xC0) != 0x80 ||
                (data[i + 1] & 0xC0) != 0x80 ||
                (data[i + 2] & 0xC0) != 0x80) {
                return false;
            }
            i += 3;
            continue;
        }

        if (c == 0xF4) {
            if (i + 2 >= len) return false;
            if (data[i] < 0x80 || data[i] > 0x8F) return false;
            if ((data[i + 1] & 0xC0) != 0x80 || (data[i + 2] & 0xC0) != 0x80) {
                return false;
            }
            i += 3;
            continue;
        }

        return false;
    }

    return true;
}

/* ── Error strings ─────────────────────────────────────────────────────── */

const char *mcbor_err_str(mcbor_err_t err)
{
    switch (err) {
    case MCBOR_OK:            return "ok";
    case MCBOR_ERR_NULL:      return "null pointer";
    case MCBOR_ERR_OVERFLOW:  return "buffer overflow";
    case MCBOR_ERR_UNDERFLOW: return "buffer underflow";
    case MCBOR_ERR_TYPE:      return "type mismatch";
    case MCBOR_ERR_INVALID:   return "invalid cbor";
    case MCBOR_ERR_SIZE:      return "size mismatch";
    case MCBOR_ERR_RANGE:     return "value out of range";
    case MCBOR_ERR_UNSUPPORTED:return "unsupported";
    default:                  return "unknown error";
    }
}

const char *mcbor_type_name(mcbor_type_t type)
{
    switch (type) {
    case MCBOR_UINT:   return "uint";
    case MCBOR_NEGINT: return "negint";
    case MCBOR_BYTES:  return "bytes";
    case MCBOR_TEXT:   return "text";
    case MCBOR_ARRAY:  return "array";
    case MCBOR_MAP:    return "map";
    case MCBOR_SIMPLE: return "simple";
    default:           return "?";
    }
}

/* ── Byte-order helpers (big-endian on wire) ───────────────────────────── */

static inline void put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v);
}

static inline void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)(v);
}

static inline uint16_t get_u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] << 8 | p[1]);
}

static inline uint32_t get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8  | (uint32_t)p[3];
}

/* ── Float bit-cast (avoids strict aliasing) ───────────────────────────── */

static inline uint32_t float_to_bits(float f)
{
    uint32_t bits;
    memcpy(&bits, &f, sizeof(bits));
    return bits;
}

static inline float bits_to_float(uint32_t bits)
{
    float f;
    memcpy(&f, &bits, sizeof(f));
    return f;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Encoder
 * ═══════════════════════════════════════════════════════════════════════════ */

mcbor_err_t mcbor_enc_init(mcbor_enc_t *enc, uint8_t *buf, size_t cap)
{
    if (enc == NULL || buf == NULL) return MCBOR_ERR_NULL;
    enc->buf      = buf;
    enc->capacity = cap;
    enc->pos      = 0;
    enc->overflow = false;
    return MCBOR_OK;
}

mcbor_err_t mcbor_enc_size(const mcbor_enc_t *enc, size_t *out_size)
{
    if (enc == NULL || out_size == NULL) return MCBOR_ERR_NULL;
    *out_size = enc->pos;
    return MCBOR_OK;
}

mcbor_err_t mcbor_enc_overflow(const mcbor_enc_t *enc, bool *out_overflow)
{
    if (enc == NULL || out_overflow == NULL) return MCBOR_ERR_NULL;
    *out_overflow = enc->overflow;
    return MCBOR_OK;
}

static mcbor_err_t enc_prepare_write(const mcbor_enc_t *enc, size_t len)
{
    if (enc->overflow) return MCBOR_ERR_OVERFLOW;
    if (enc->pos > enc->capacity) return MCBOR_ERR_INVALID;
    if (len > enc->capacity - enc->pos) {
        return MCBOR_ERR_OVERFLOW;
    }
    return MCBOR_OK;
}

static void enc_commit_bytes(mcbor_enc_t *enc, const uint8_t *data, size_t len)
{
    if (len != 0) {
        memcpy(enc->buf + enc->pos, data, len);
        enc->pos += len;
    }
}

/**
 * Encode a CBOR head: major type + unsigned argument.
 * This is the core building block — everything else calls this.
 */
static size_t enc_head(uint8_t major, uint32_t value, uint8_t out[5])
{
    if (value <= 23) {
        out[0] = (uint8_t)(major | (uint8_t)value);
        return 1;
    } else if (value <= 0xFF) {
        out[0] = (uint8_t)(major | CBOR_INFO_1BYTE);
        out[1] = (uint8_t)value;
        return 2;
    } else if (value <= 0xFFFF) {
        out[0] = (uint8_t)(major | CBOR_INFO_2BYTE);
        put_u16(out + 1, (uint16_t)value);
        return 3;
    } else {
        out[0] = (uint8_t)(major | CBOR_INFO_4BYTE);
        put_u32(out + 1, value);
        return 5;
    }
}

static bool ranges_overlap(const uint8_t *a, size_t a_len,
                           const uint8_t *b, size_t b_len)
{
    if (a_len == 0 || b_len == 0) return false;
    return a < b + b_len && b < a + a_len;
}

static mcbor_err_t enc_append_item(mcbor_enc_t *enc,
                                   const uint8_t *head, size_t head_len,
                                   const uint8_t *payload, size_t payload_len)
{
    mcbor_err_t err;
    size_t total_len;

    if (enc == NULL || head == NULL) return MCBOR_ERR_NULL;
    if (payload_len != 0 && payload == NULL) return MCBOR_ERR_NULL;
    if (head_len > SIZE_MAX - payload_len) return MCBOR_ERR_RANGE;

    total_len = head_len + payload_len;
    err = enc_prepare_write(enc, total_len);
    if (err != MCBOR_OK) {
        if (err == MCBOR_ERR_OVERFLOW) {
            enc->overflow = true;
        }
        return err;
    }

    if (payload_len != 0 &&
        ranges_overlap(payload, payload_len, enc->buf + enc->pos, total_len)) {
        return MCBOR_ERR_INVALID;
    }

    enc_commit_bytes(enc, head, head_len);
    enc_commit_bytes(enc, payload, payload_len);
    return MCBOR_OK;
}

static mcbor_err_t enc_append_head_only(mcbor_enc_t *enc, uint8_t major, uint32_t value)
{
    uint8_t head[5];
    size_t head_len = enc_head(major, value, head);
    return enc_append_item(enc, head, head_len, NULL, 0);
}

static mcbor_err_t enc_append_simple_byte(mcbor_enc_t *enc, uint8_t byte)
{
    return enc_append_item(enc, &byte, 1, NULL, 0);
}

mcbor_err_t mcbor_enc_uint(mcbor_enc_t *enc, uint32_t value)
{
    if (enc == NULL) return MCBOR_ERR_NULL;
    return enc_append_head_only(enc, CBOR_MAJOR_UINT, value);
}

mcbor_err_t mcbor_enc_int(mcbor_enc_t *enc, int32_t value)
{
    if (enc == NULL) return MCBOR_ERR_NULL;
    if (value >= 0) {
        return enc_append_head_only(enc, CBOR_MAJOR_UINT, (uint32_t)value);
    } else {
        /* CBOR negative: -1 → 0, -2 → 1, etc. */
        return enc_append_head_only(enc, CBOR_MAJOR_NEGINT, (uint32_t)(-(value + 1)));
    }
}

mcbor_err_t mcbor_enc_bool(mcbor_enc_t *enc, bool value)
{
    if (enc == NULL) return MCBOR_ERR_NULL;
    return enc_append_simple_byte(enc, (uint8_t)(CBOR_MAJOR_SIMPLE |
                                  (value ? CBOR_SIMPLE_TRUE : CBOR_SIMPLE_FALSE)));
}

mcbor_err_t mcbor_enc_null(mcbor_enc_t *enc)
{
    if (enc == NULL) return MCBOR_ERR_NULL;
    return enc_append_simple_byte(enc, (uint8_t)(CBOR_MAJOR_SIMPLE | CBOR_SIMPLE_VALUE_NULL));
}

mcbor_err_t mcbor_enc_float(mcbor_enc_t *enc, float value)
{
    uint8_t tmp[5];

    if (enc == NULL) return MCBOR_ERR_NULL;
    if (!float32_supported()) return MCBOR_ERR_UNSUPPORTED;

    tmp[0] = (uint8_t)(CBOR_MAJOR_SIMPLE | CBOR_INFO_4BYTE);
    put_u32(tmp + 1, float_to_bits(value));
    return enc_append_item(enc, tmp, sizeof(tmp), NULL, 0);
}

mcbor_err_t mcbor_enc_text(mcbor_enc_t *enc, const char *str, size_t len)
{
    uint8_t head[5];
    size_t head_len;
    mcbor_err_t err;

    if (enc == NULL) return MCBOR_ERR_NULL;
    if (str == NULL && len != 0) return MCBOR_ERR_NULL;
    err = check_u32_range(len);
    if (err != MCBOR_OK) return err;
    if (len != 0 && !is_valid_utf8((const uint8_t *)str, len)) {
        return MCBOR_ERR_INVALID;
    }

    head_len = enc_head(CBOR_MAJOR_TEXT, (uint32_t)len, head);
    return enc_append_item(enc, head, head_len, (const uint8_t *)str, len);
}

mcbor_err_t mcbor_enc_str(mcbor_enc_t *enc, const char *str)
{
    if (str == NULL) return MCBOR_ERR_NULL;
    return mcbor_enc_text(enc, str, strlen(str));
}

mcbor_err_t mcbor_enc_bytes(mcbor_enc_t *enc, const uint8_t *data, size_t len)
{
    uint8_t head[5];
    size_t head_len;
    mcbor_err_t err;

    if (enc == NULL) return MCBOR_ERR_NULL;
    if (data == NULL && len != 0) return MCBOR_ERR_NULL;
    err = check_u32_range(len);
    if (err != MCBOR_OK) return err;
    head_len = enc_head(CBOR_MAJOR_BYTES, (uint32_t)len, head);
    return enc_append_item(enc, head, head_len, data, len);
}

mcbor_err_t mcbor_enc_array(mcbor_enc_t *enc, size_t count)
{
    if (enc == NULL) return MCBOR_ERR_NULL;
    mcbor_err_t err = check_u32_range(count);
    if (err != MCBOR_OK) return err;
    return enc_append_head_only(enc, CBOR_MAJOR_ARRAY, (uint32_t)count);
}

mcbor_err_t mcbor_enc_map(mcbor_enc_t *enc, size_t count)
{
    if (enc == NULL) return MCBOR_ERR_NULL;
    mcbor_err_t err = check_u32_range(count);
    if (err != MCBOR_OK) return err;
    return enc_append_head_only(enc, CBOR_MAJOR_MAP, (uint32_t)count);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Decoder
 * ═══════════════════════════════════════════════════════════════════════════ */

mcbor_err_t mcbor_dec_init(mcbor_dec_t *dec, const uint8_t *buf, size_t size)
{
    if (dec == NULL || buf == NULL) return MCBOR_ERR_NULL;
    dec->buf  = buf;
    dec->size = size;
    dec->pos  = 0;
    return MCBOR_OK;
}

mcbor_err_t mcbor_dec_remaining(const mcbor_dec_t *dec, size_t *out_remaining)
{
    if (dec == NULL || out_remaining == NULL) return MCBOR_ERR_NULL;
    *out_remaining = dec->size - dec->pos;
    return MCBOR_OK;
}

mcbor_err_t mcbor_dec_done(const mcbor_dec_t *dec, bool *out_done)
{
    if (dec == NULL || out_done == NULL) return MCBOR_ERR_NULL;
    *out_done = dec->pos >= dec->size;
    return MCBOR_OK;
}

/** Read the argument value from a CBOR head. */
static mcbor_err_t dec_argument(mcbor_dec_t *dec, uint8_t info, uint32_t *value)
{
    if (info <= 23) {
        *value = info;
        return MCBOR_OK;
    }

    if (info == CBOR_INFO_1BYTE) {
        if (dec->pos + 1 > dec->size) return MCBOR_ERR_UNDERFLOW;
        *value = dec->buf[dec->pos++];
        return MCBOR_OK;
    }

    if (info == CBOR_INFO_2BYTE) {
        if (dec->pos + 2 > dec->size) return MCBOR_ERR_UNDERFLOW;
        *value = get_u16(dec->buf + dec->pos);
        dec->pos += 2;
        return MCBOR_OK;
    }

    if (info == CBOR_INFO_4BYTE) {
        if (dec->pos + 4 > dec->size) return MCBOR_ERR_UNDERFLOW;
        *value = get_u32(dec->buf + dec->pos);
        dec->pos += 4;
        return MCBOR_OK;
    }

    return MCBOR_ERR_INVALID;  /* 8-byte or reserved */
}

mcbor_err_t mcbor_dec_next(mcbor_dec_t *dec, mcbor_value_t *val)
{
    if (dec == NULL || val == NULL) return MCBOR_ERR_NULL;
    if (dec->pos >= dec->size) return MCBOR_ERR_UNDERFLOW;

    memset(val, 0, sizeof(*val));

    uint8_t initial = dec->buf[dec->pos++];
    uint8_t major = initial & CBOR_MAJOR_MASK;
    uint8_t info  = initial & CBOR_INFO_MASK;

    uint32_t argument = 0;
    mcbor_err_t err;

    switch (major) {
    case CBOR_MAJOR_UINT:
        err = dec_argument(dec, info, &argument);
        if (err != MCBOR_OK) return err;
        val->type = MCBOR_UINT;
        val->val.uint_val = argument;
        return MCBOR_OK;

    case CBOR_MAJOR_NEGINT:
        err = dec_argument(dec, info, &argument);
        if (err != MCBOR_OK) return err;
        val->type = MCBOR_NEGINT;
        val->val.int_val = -1 - (int32_t)argument;
        return MCBOR_OK;

    case CBOR_MAJOR_BYTES:
        err = dec_argument(dec, info, &argument);
        if (err != MCBOR_OK) return err;
        if (dec->pos + (size_t)argument > dec->size) return MCBOR_ERR_UNDERFLOW;
        val->type = MCBOR_BYTES;
        val->val.bytes.ptr = dec->buf + dec->pos;
        val->val.bytes.len = argument;
        dec->pos += argument;
        return MCBOR_OK;

    case CBOR_MAJOR_TEXT:
        err = dec_argument(dec, info, &argument);
        if (err != MCBOR_OK) return err;
        if (dec->pos + (size_t)argument > dec->size) return MCBOR_ERR_UNDERFLOW;
        val->type = MCBOR_TEXT;
        val->val.bytes.ptr = dec->buf + dec->pos;
        val->val.bytes.len = argument;
        dec->pos += argument;
        return MCBOR_OK;

    case CBOR_MAJOR_ARRAY:
        err = dec_argument(dec, info, &argument);
        if (err != MCBOR_OK) return err;
        val->type = MCBOR_ARRAY;
        val->val.container = argument;
        return MCBOR_OK;

    case CBOR_MAJOR_MAP:
        err = dec_argument(dec, info, &argument);
        if (err != MCBOR_OK) return err;
        val->type = MCBOR_MAP;
        val->val.container = argument;
        return MCBOR_OK;

    case CBOR_MAJOR_SIMPLE:
        if (info == CBOR_SIMPLE_FALSE) {
            val->type = MCBOR_SIMPLE;
            val->simple_kind = MCBOR_SIMPLE_BOOL;
            val->val.bool_val = false;
            return MCBOR_OK;
        }
        if (info == CBOR_SIMPLE_TRUE) {
            val->type = MCBOR_SIMPLE;
            val->simple_kind = MCBOR_SIMPLE_BOOL;
            val->val.bool_val = true;
            return MCBOR_OK;
        }
        if (info == CBOR_SIMPLE_VALUE_NULL) {
            val->type = MCBOR_SIMPLE;
            val->simple_kind = MCBOR_SIMPLE_NULL;
            return MCBOR_OK;
        }
        if (info == CBOR_INFO_4BYTE) {
            /* float32 */
            if (dec->pos + 4 > dec->size) return MCBOR_ERR_UNDERFLOW;
            uint32_t bits = get_u32(dec->buf + dec->pos);
            dec->pos += 4;
            val->type = MCBOR_SIMPLE;
            val->simple_kind = MCBOR_SIMPLE_FLOAT32;
            val->val.float_val = bits_to_float(bits);
            return MCBOR_OK;
        }
        return MCBOR_ERR_INVALID;

    default:
        return MCBOR_ERR_INVALID;
    }
}

/* ── Convenience typed decoders ────────────────────────────────────────── */

mcbor_err_t mcbor_dec_uint(mcbor_dec_t *dec, uint32_t *out)
{
    if (out == NULL) return MCBOR_ERR_NULL;
    mcbor_value_t val;
    mcbor_err_t err = mcbor_dec_next(dec, &val);
    if (err != MCBOR_OK) return err;
    if (val.type != MCBOR_UINT) return MCBOR_ERR_TYPE;
    *out = val.val.uint_val;
    return MCBOR_OK;
}

mcbor_err_t mcbor_dec_int(mcbor_dec_t *dec, int32_t *out)
{
    if (out == NULL) return MCBOR_ERR_NULL;
    mcbor_value_t val;
    mcbor_err_t err = mcbor_dec_next(dec, &val);
    if (err != MCBOR_OK) return err;
    if (val.type == MCBOR_UINT) {
        *out = (int32_t)val.val.uint_val;
        return MCBOR_OK;
    }
    if (val.type == MCBOR_NEGINT) {
        *out = val.val.int_val;
        return MCBOR_OK;
    }
    return MCBOR_ERR_TYPE;
}

mcbor_err_t mcbor_dec_bool(mcbor_dec_t *dec, bool *out)
{
    if (out == NULL) return MCBOR_ERR_NULL;
    mcbor_value_t val;
    mcbor_err_t err = mcbor_dec_next(dec, &val);
    if (err != MCBOR_OK) return err;
    if (val.type != MCBOR_SIMPLE || val.simple_kind != MCBOR_SIMPLE_BOOL) {
        return MCBOR_ERR_TYPE;
    }
    *out = val.val.bool_val;
    return MCBOR_OK;
}

mcbor_err_t mcbor_dec_float(mcbor_dec_t *dec, float *out)
{
    if (out == NULL) return MCBOR_ERR_NULL;
    mcbor_value_t val;
    mcbor_err_t err = mcbor_dec_next(dec, &val);
    if (err != MCBOR_OK) return err;
    if (val.type != MCBOR_SIMPLE || val.simple_kind != MCBOR_SIMPLE_FLOAT32) {
        return MCBOR_ERR_TYPE;
    }
    *out = val.val.float_val;
    return MCBOR_OK;
}

mcbor_err_t mcbor_dec_str(mcbor_dec_t *dec, char *out, size_t out_size,
                           size_t *out_len)
{
    if (out == NULL) return MCBOR_ERR_NULL;
    mcbor_value_t val;
    mcbor_err_t err = mcbor_dec_next(dec, &val);
    if (err != MCBOR_OK) return err;
    if (val.type != MCBOR_TEXT) return MCBOR_ERR_TYPE;
    if (val.val.bytes.len >= out_size) return MCBOR_ERR_SIZE;
    memcpy(out, val.val.bytes.ptr, val.val.bytes.len);
    out[val.val.bytes.len] = '\0';
    if (out_len != NULL) *out_len = val.val.bytes.len;
    return MCBOR_OK;
}

mcbor_err_t mcbor_dec_bytes_ref(mcbor_dec_t *dec, const uint8_t **out,
                                 size_t *out_len)
{
    if (out == NULL || out_len == NULL) return MCBOR_ERR_NULL;
    mcbor_value_t val;
    mcbor_err_t err = mcbor_dec_next(dec, &val);
    if (err != MCBOR_OK) return err;
    if (val.type != MCBOR_BYTES) return MCBOR_ERR_TYPE;
    *out = val.val.bytes.ptr;
    *out_len = val.val.bytes.len;
    return MCBOR_OK;
}

mcbor_err_t mcbor_dec_array(mcbor_dec_t *dec, size_t *count)
{
    if (count == NULL) return MCBOR_ERR_NULL;
    mcbor_value_t val;
    mcbor_err_t err = mcbor_dec_next(dec, &val);
    if (err != MCBOR_OK) return err;
    if (val.type != MCBOR_ARRAY) return MCBOR_ERR_TYPE;
    *count = val.val.container;
    return MCBOR_OK;
}

mcbor_err_t mcbor_dec_map(mcbor_dec_t *dec, size_t *count)
{
    if (count == NULL) return MCBOR_ERR_NULL;
    mcbor_value_t val;
    mcbor_err_t err = mcbor_dec_next(dec, &val);
    if (err != MCBOR_OK) return err;
    if (val.type != MCBOR_MAP) return MCBOR_ERR_TYPE;
    *count = val.val.container;
    return MCBOR_OK;
}

mcbor_err_t mcbor_dec_skip(mcbor_dec_t *dec)
{
    if (dec == NULL) return MCBOR_ERR_NULL;

    mcbor_value_t val;
    mcbor_err_t err = mcbor_dec_next(dec, &val);
    if (err != MCBOR_OK) return err;

    /* For non-containers, the item is already consumed */
    if (val.type == MCBOR_ARRAY) {
        for (uint32_t i = 0; i < val.val.container; i++) {
            err = mcbor_dec_skip(dec);
            if (err != MCBOR_OK) return err;
        }
    } else if (val.type == MCBOR_MAP) {
        for (uint32_t i = 0; i < val.val.container; i++) {
            err = mcbor_dec_skip(dec);  /* key */
            if (err != MCBOR_OK) return err;
            err = mcbor_dec_skip(dec);  /* value */
            if (err != MCBOR_OK) return err;
        }
    }

    return MCBOR_OK;
}
