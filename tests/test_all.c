/*
 * microcbor test suite.
 *
 * Build: gcc -std=c99 -Wall -Wextra -I../include ../src/mcbor.c test_all.c -o test_all -lm
 * Run:   ./test_all
 */

#include "mcbor.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ── Minimal test framework ────────────────────────────────────────────── */

static int tests_run = 0, tests_passed = 0, tests_failed = 0;

#define TEST(name) static void name(void)
#define RUN_TEST(name) do {                                     \
    tests_run++;                                                \
    printf("  %-55s ", #name);                                  \
    name();                                                     \
    printf("PASS\n");                                           \
    tests_passed++;                                             \
} while (0)

#define ASSERT_EQ(expected, actual) do {                        \
    if ((expected) != (actual)) {                               \
        printf("FAIL\n    %s:%d: expected %d, got %d\n",       \
               __FILE__, __LINE__, (int)(expected), (int)(actual)); \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_TRUE(expr) do {                                  \
    if (!(expr)) {                                              \
        printf("FAIL\n    %s:%d: expected true\n",              \
               __FILE__, __LINE__);                             \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_FALSE(expr) do {                                 \
    if ((expr)) {                                               \
        printf("FAIL\n    %s:%d: expected false\n",             \
               __FILE__, __LINE__);                             \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_STR_EQ(expected, actual) do {                    \
    if (strcmp((expected), (actual)) != 0) {                     \
        printf("FAIL\n    %s:%d: expected \"%s\", got \"%s\"\n",\
               __FILE__, __LINE__, (expected), (actual));       \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_FLOAT_EQ(expected, actual) do {                  \
    if (fabsf((expected) - (actual)) > 0.001f) {                \
        printf("FAIL\n    %s:%d: expected %f, got %f\n",       \
               __FILE__, __LINE__, (double)(expected), (double)(actual)); \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_MEM_EQ(expected, actual, len) do {               \
    if (memcmp((expected), (actual), (len)) != 0) {              \
        printf("FAIL\n    %s:%d: memory mismatch\n",            \
               __FILE__, __LINE__);                             \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

static size_t query_enc_size(const mcbor_enc_t *enc)
{
    size_t size = 0;
    if (mcbor_enc_size(enc, &size) != MCBOR_OK) {
        printf("FAIL\n    %s:%d: mcbor_enc_size failed\n", __FILE__, __LINE__);
        tests_failed++;
        return 0;
    }
    return size;
}

static bool query_enc_overflow(const mcbor_enc_t *enc)
{
    bool overflow = false;
    if (mcbor_enc_overflow(enc, &overflow) != MCBOR_OK) {
        printf("FAIL\n    %s:%d: mcbor_enc_overflow failed\n", __FILE__, __LINE__);
        tests_failed++;
        return false;
    }
    return overflow;
}

static size_t query_dec_remaining(const mcbor_dec_t *dec)
{
    size_t remaining = 0;
    if (mcbor_dec_remaining(dec, &remaining) != MCBOR_OK) {
        printf("FAIL\n    %s:%d: mcbor_dec_remaining failed\n", __FILE__, __LINE__);
        tests_failed++;
        return 0;
    }
    return remaining;
}

static bool query_dec_done(const mcbor_dec_t *dec)
{
    bool done = false;
    if (mcbor_dec_done(dec, &done) != MCBOR_OK) {
        printf("FAIL\n    %s:%d: mcbor_dec_done failed\n", __FILE__, __LINE__);
        tests_failed++;
        return false;
    }
    return done;
}

/* ── Common buffer ─────────────────────────────────────────────────────── */

static uint8_t buf[256];

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Encoder init
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_enc_init) {
    mcbor_enc_t enc;
    ASSERT_EQ(MCBOR_OK, mcbor_enc_init(&enc, buf, sizeof(buf)));
    ASSERT_EQ(0, (int)query_enc_size(&enc));
    ASSERT_FALSE(query_enc_overflow(&enc));
}

TEST(test_enc_init_null) {
    mcbor_enc_t enc;
    ASSERT_EQ(MCBOR_ERR_NULL, mcbor_enc_init(NULL, buf, sizeof(buf)));
    ASSERT_EQ(MCBOR_ERR_NULL, mcbor_enc_init(&enc, NULL, 10));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Encode integers
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_enc_uint_tiny) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_uint(&enc, 0);
    ASSERT_EQ(1, (int)query_enc_size(&enc));
    ASSERT_EQ(0x00, buf[0]);  /* major 0, value 0 */
}

TEST(test_enc_uint_23) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_uint(&enc, 23);
    ASSERT_EQ(1, (int)query_enc_size(&enc));
    ASSERT_EQ(0x17, buf[0]);  /* major 0, value 23 */
}

TEST(test_enc_uint_24) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_uint(&enc, 24);
    ASSERT_EQ(2, (int)query_enc_size(&enc));
    ASSERT_EQ(0x18, buf[0]);  /* major 0, 1-byte follows */
    ASSERT_EQ(24, buf[1]);
}

TEST(test_enc_uint_255) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_uint(&enc, 255);
    ASSERT_EQ(2, (int)query_enc_size(&enc));
    ASSERT_EQ(0x18, buf[0]);
    ASSERT_EQ(255, buf[1]);
}

TEST(test_enc_uint_256) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_uint(&enc, 256);
    ASSERT_EQ(3, (int)query_enc_size(&enc));
    ASSERT_EQ(0x19, buf[0]);  /* major 0, 2-byte follows */
    ASSERT_EQ(0x01, buf[1]);
    ASSERT_EQ(0x00, buf[2]);
}

TEST(test_enc_uint_65536) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_uint(&enc, 65536);
    ASSERT_EQ(5, (int)query_enc_size(&enc));
    ASSERT_EQ(0x1A, buf[0]);  /* major 0, 4-byte follows */
}

TEST(test_enc_int_positive) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_int(&enc, 42);
    ASSERT_EQ(2, (int)query_enc_size(&enc));
    /* Should encode as uint */
    ASSERT_EQ(0x18, buf[0]);
    ASSERT_EQ(42, buf[1]);
}

TEST(test_enc_int_negative_1) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_int(&enc, -1);
    ASSERT_EQ(1, (int)query_enc_size(&enc));
    ASSERT_EQ(0x20, buf[0]);  /* major 1, value 0 → -1 */
}

TEST(test_enc_int_negative_100) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_int(&enc, -100);
    ASSERT_EQ(2, (int)query_enc_size(&enc));
    ASSERT_EQ(0x38, buf[0]);  /* major 1, 1-byte follows */
    ASSERT_EQ(99, buf[1]);    /* -100 → argument 99 */
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Encode simple values
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_enc_bool_false) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_bool(&enc, false);
    ASSERT_EQ(1, (int)query_enc_size(&enc));
    ASSERT_EQ(0xF4, buf[0]);  /* major 7, simple 20 */
}

TEST(test_enc_bool_true) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_bool(&enc, true);
    ASSERT_EQ(1, (int)query_enc_size(&enc));
    ASSERT_EQ(0xF5, buf[0]);  /* major 7, simple 21 */
}

TEST(test_enc_null) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_null(&enc);
    ASSERT_EQ(1, (int)query_enc_size(&enc));
    ASSERT_EQ(0xF6, buf[0]);  /* major 7, simple 22 */
}

TEST(test_enc_float) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_float(&enc, 3.14f);
    ASSERT_EQ(5, (int)query_enc_size(&enc));
    ASSERT_EQ(0xFA, buf[0]);  /* major 7, 4-byte float */
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Encode strings and bytes
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_enc_str) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_str(&enc, "hello");
    ASSERT_EQ(6, (int)query_enc_size(&enc));
    ASSERT_EQ(0x65, buf[0]);  /* major 3, length 5 */
    ASSERT_MEM_EQ("hello", buf + 1, 5);
}

TEST(test_enc_str_empty) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_str(&enc, "");
    ASSERT_EQ(1, (int)query_enc_size(&enc));
    ASSERT_EQ(0x60, buf[0]);  /* major 3, length 0 */
}

TEST(test_enc_bytes) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    uint8_t data[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    mcbor_enc_bytes(&enc, data, 4);
    ASSERT_EQ(5, (int)query_enc_size(&enc));
    ASSERT_EQ(0x44, buf[0]);  /* major 2, length 4 */
    ASSERT_MEM_EQ(data, buf + 1, 4);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Encode containers
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_enc_array) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_array(&enc, 3);
    mcbor_enc_uint(&enc, 1);
    mcbor_enc_uint(&enc, 2);
    mcbor_enc_uint(&enc, 3);
    ASSERT_EQ(4, (int)query_enc_size(&enc));
    ASSERT_EQ(0x83, buf[0]);  /* major 4, length 3 */
}

TEST(test_enc_map) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_map(&enc, 2);
    mcbor_enc_str(&enc, "a");
    mcbor_enc_uint(&enc, 1);
    mcbor_enc_str(&enc, "b");
    mcbor_enc_uint(&enc, 2);
    /* map(2) + "a" + 1 + "b" + 2 = 1 + 2 + 1 + 2 + 1 = 7 */
    ASSERT_EQ(7, (int)query_enc_size(&enc));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Encoder overflow
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_enc_overflow) {
    uint8_t tiny[2];
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, tiny, sizeof(tiny));

    mcbor_enc_str(&enc, "this is way too long for 2 bytes");
    ASSERT_TRUE(query_enc_overflow(&enc));
}

TEST(test_enc_overflow_sticky) {
    uint8_t tiny[1];
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, tiny, sizeof(tiny));

    mcbor_enc_uint(&enc, 0);   /* OK — 1 byte */
    ASSERT_FALSE(query_enc_overflow(&enc));

    mcbor_enc_uint(&enc, 0);   /* overflow */
    ASSERT_TRUE(query_enc_overflow(&enc));

    /* Still overflow even after another attempt */
    ASSERT_EQ(MCBOR_ERR_OVERFLOW, mcbor_enc_uint(&enc, 0));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Round-trip (encode then decode)
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_roundtrip_uint) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_uint(&enc, 12345);

    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, query_enc_size(&enc));
    uint32_t val;
    ASSERT_EQ(MCBOR_OK, mcbor_dec_uint(&dec, &val));
    ASSERT_EQ(12345, (int)val);
    ASSERT_TRUE(query_dec_done(&dec));
}

TEST(test_roundtrip_int_negative) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_int(&enc, -500);

    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, query_enc_size(&enc));
    int32_t val;
    ASSERT_EQ(MCBOR_OK, mcbor_dec_int(&dec, &val));
    ASSERT_EQ(-500, val);
}

TEST(test_roundtrip_bool) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_bool(&enc, true);
    mcbor_enc_bool(&enc, false);

    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, query_enc_size(&enc));
    bool v1, v2;
    ASSERT_EQ(MCBOR_OK, mcbor_dec_bool(&dec, &v1));
    ASSERT_EQ(MCBOR_OK, mcbor_dec_bool(&dec, &v2));
    ASSERT_TRUE(v1);
    ASSERT_FALSE(v2);
}

TEST(test_roundtrip_float) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_float(&enc, -273.15f);

    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, query_enc_size(&enc));
    float val;
    ASSERT_EQ(MCBOR_OK, mcbor_dec_float(&dec, &val));
    ASSERT_FLOAT_EQ(-273.15f, val);
}

TEST(test_roundtrip_str) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_str(&enc, "temperature");

    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, query_enc_size(&enc));
    char out[32];
    size_t len;
    ASSERT_EQ(MCBOR_OK, mcbor_dec_str(&dec, out, sizeof(out), &len));
    ASSERT_STR_EQ("temperature", out);
    ASSERT_EQ(11, (int)len);
}

TEST(test_roundtrip_bytes) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    uint8_t data[] = { 0xCA, 0xFE, 0xBA, 0xBE };
    mcbor_enc_bytes(&enc, data, 4);

    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, query_enc_size(&enc));
    const uint8_t *out;
    size_t len;
    ASSERT_EQ(MCBOR_OK, mcbor_dec_bytes_ref(&dec, &out, &len));
    ASSERT_EQ(4, (int)len);
    ASSERT_MEM_EQ(data, out, 4);
}

TEST(test_roundtrip_null) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_null(&enc);

    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, query_enc_size(&enc));
    mcbor_value_t val;
    ASSERT_EQ(MCBOR_OK, mcbor_dec_next(&dec, &val));
    ASSERT_EQ(MCBOR_SIMPLE_NULL, val.simple_kind);
}

TEST(test_roundtrip_array) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_array(&enc, 3);
    mcbor_enc_uint(&enc, 10);
    mcbor_enc_uint(&enc, 20);
    mcbor_enc_uint(&enc, 30);

    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, query_enc_size(&enc));
    size_t count;
    ASSERT_EQ(MCBOR_OK, mcbor_dec_array(&dec, &count));
    ASSERT_EQ(3, (int)count);

    uint32_t v;
    mcbor_dec_uint(&dec, &v); ASSERT_EQ(10, (int)v);
    mcbor_dec_uint(&dec, &v); ASSERT_EQ(20, (int)v);
    mcbor_dec_uint(&dec, &v); ASSERT_EQ(30, (int)v);
    ASSERT_TRUE(query_dec_done(&dec));
}

TEST(test_roundtrip_map) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_map(&enc, 2);
    mcbor_enc_str(&enc, "temp");
    mcbor_enc_float(&enc, 22.5f);
    mcbor_enc_str(&enc, "hum");
    mcbor_enc_uint(&enc, 65);

    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, query_enc_size(&enc));

    size_t count;
    ASSERT_EQ(MCBOR_OK, mcbor_dec_map(&dec, &count));
    ASSERT_EQ(2, (int)count);

    char key[16];
    size_t klen;
    float fval;
    uint32_t uval;

    mcbor_dec_str(&dec, key, sizeof(key), &klen);
    ASSERT_STR_EQ("temp", key);
    mcbor_dec_float(&dec, &fval);
    ASSERT_FLOAT_EQ(22.5f, fval);

    mcbor_dec_str(&dec, key, sizeof(key), &klen);
    ASSERT_STR_EQ("hum", key);
    mcbor_dec_uint(&dec, &uval);
    ASSERT_EQ(65, (int)uval);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Telemetry payload (real-world pattern)
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_telemetry_payload) {
    /* Encode: {"device":"sensor-01","temp":23.4,"hum":55,"ok":true} */
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_map(&enc, 4);
    mcbor_enc_str(&enc, "device");  mcbor_enc_str(&enc, "sensor-01");
    mcbor_enc_str(&enc, "temp");    mcbor_enc_float(&enc, 23.4f);
    mcbor_enc_str(&enc, "hum");     mcbor_enc_uint(&enc, 55);
    mcbor_enc_str(&enc, "ok");      mcbor_enc_bool(&enc, true);

    size_t size = query_enc_size(&enc);
    ASSERT_FALSE(query_enc_overflow(&enc));

    /* Decode and verify */
    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, size);

    size_t count;
    mcbor_dec_map(&dec, &count);
    ASSERT_EQ(4, (int)count);

    char key[16], sval[32];
    size_t klen, slen;
    float fval;
    uint32_t uval;
    bool bval;

    mcbor_dec_str(&dec, key, sizeof(key), &klen);
    ASSERT_STR_EQ("device", key);
    mcbor_dec_str(&dec, sval, sizeof(sval), &slen);
    ASSERT_STR_EQ("sensor-01", sval);

    mcbor_dec_str(&dec, key, sizeof(key), &klen);
    ASSERT_STR_EQ("temp", key);
    mcbor_dec_float(&dec, &fval);
    ASSERT_FLOAT_EQ(23.4f, fval);

    mcbor_dec_str(&dec, key, sizeof(key), &klen);
    ASSERT_STR_EQ("hum", key);
    mcbor_dec_uint(&dec, &uval);
    ASSERT_EQ(55, (int)uval);

    mcbor_dec_str(&dec, key, sizeof(key), &klen);
    ASSERT_STR_EQ("ok", key);
    mcbor_dec_bool(&dec, &bval);
    ASSERT_TRUE(bval);

    ASSERT_TRUE(query_dec_done(&dec));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Decoder errors
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_dec_underflow) {
    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, 0);
    mcbor_value_t val;
    ASSERT_EQ(MCBOR_ERR_UNDERFLOW, mcbor_dec_next(&dec, &val));
}

TEST(test_dec_type_mismatch) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_str(&enc, "hello");

    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, query_enc_size(&enc));
    uint32_t val;
    ASSERT_EQ(MCBOR_ERR_TYPE, mcbor_dec_uint(&dec, &val));
}

TEST(test_dec_str_too_small) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_str(&enc, "this is a long string");

    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, query_enc_size(&enc));
    char tiny[4];
    size_t len;
    ASSERT_EQ(MCBOR_ERR_SIZE, mcbor_dec_str(&dec, tiny, sizeof(tiny), &len));
}

TEST(test_dec_null_args) {
    mcbor_dec_t dec;
    ASSERT_EQ(MCBOR_ERR_NULL, mcbor_dec_init(NULL, buf, 1));
    ASSERT_EQ(MCBOR_ERR_NULL, mcbor_dec_init(&dec, NULL, 1));
    ASSERT_EQ(MCBOR_ERR_NULL, mcbor_dec_uint(NULL, NULL));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Skip
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_dec_skip_scalar) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_str(&enc, "skip me");
    mcbor_enc_uint(&enc, 42);

    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, query_enc_size(&enc));
    ASSERT_EQ(MCBOR_OK, mcbor_dec_skip(&dec));  /* skip the string */

    uint32_t val;
    ASSERT_EQ(MCBOR_OK, mcbor_dec_uint(&dec, &val));
    ASSERT_EQ(42, (int)val);
}

TEST(test_dec_skip_nested) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    /* array(2, map(1, "k", "v"), 99) */
    mcbor_enc_array(&enc, 2);
      mcbor_enc_map(&enc, 1);
        mcbor_enc_str(&enc, "k");
        mcbor_enc_str(&enc, "v");
      mcbor_enc_uint(&enc, 99);

    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, query_enc_size(&enc));

    /* Skip the entire array */
    ASSERT_EQ(MCBOR_OK, mcbor_dec_skip(&dec));
    ASSERT_TRUE(query_dec_done(&dec));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Edge cases
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_roundtrip_zero) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_uint(&enc, 0);
    mcbor_enc_int(&enc, 0);
    mcbor_enc_float(&enc, 0.0f);

    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, query_enc_size(&enc));
    uint32_t u; int32_t i; float f;
    mcbor_dec_uint(&dec, &u); ASSERT_EQ(0, (int)u);
    mcbor_dec_int(&dec, &i);  ASSERT_EQ(0, i);
    mcbor_dec_float(&dec, &f); ASSERT_FLOAT_EQ(0.0f, f);
}

TEST(test_roundtrip_large_uint) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_uint(&enc, 0xFFFFFFFF);

    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, query_enc_size(&enc));
    uint32_t val;
    mcbor_dec_uint(&dec, &val);
    ASSERT_EQ((int)0xFFFFFFFF, (int)val);
}

TEST(test_dec_remaining) {
    mcbor_enc_t enc;
    mcbor_enc_init(&enc, buf, sizeof(buf));
    mcbor_enc_uint(&enc, 1);
    mcbor_enc_uint(&enc, 2);

    mcbor_dec_t dec;
    mcbor_dec_init(&dec, buf, query_enc_size(&enc));
    ASSERT_EQ(2, (int)query_dec_remaining(&dec));
    ASSERT_FALSE(query_dec_done(&dec));

    uint32_t v;
    mcbor_dec_uint(&dec, &v);
    mcbor_dec_uint(&dec, &v);
    ASSERT_EQ(0, (int)query_dec_remaining(&dec));
    ASSERT_TRUE(query_dec_done(&dec));
}

TEST(test_err_str) {
    ASSERT_STR_EQ("ok",              mcbor_err_str(MCBOR_OK));
    ASSERT_STR_EQ("buffer overflow", mcbor_err_str(MCBOR_ERR_OVERFLOW));
    ASSERT_STR_EQ("buffer underflow",mcbor_err_str(MCBOR_ERR_UNDERFLOW));
    ASSERT_STR_EQ("type mismatch",   mcbor_err_str(MCBOR_ERR_TYPE));
    ASSERT_STR_EQ("unknown error",   mcbor_err_str((mcbor_err_t)99));
}

TEST(test_type_name) {
    ASSERT_STR_EQ("uint",   mcbor_type_name(MCBOR_UINT));
    ASSERT_STR_EQ("text",   mcbor_type_name(MCBOR_TEXT));
    ASSERT_STR_EQ("map",    mcbor_type_name(MCBOR_MAP));
    ASSERT_STR_EQ("simple", mcbor_type_name(MCBOR_SIMPLE));
    ASSERT_STR_EQ("?",      mcbor_type_name((mcbor_type_t)99));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Main
 * ═══════════════════════════════════════════════════════════════════════════ */

int main(void) {
    printf("\n=== microcbor test suite ===\n\n");

    printf("[Encoder Init]\n");
    RUN_TEST(test_enc_init);
    RUN_TEST(test_enc_init_null);

    printf("\n[Encode Integers]\n");
    RUN_TEST(test_enc_uint_tiny);
    RUN_TEST(test_enc_uint_23);
    RUN_TEST(test_enc_uint_24);
    RUN_TEST(test_enc_uint_255);
    RUN_TEST(test_enc_uint_256);
    RUN_TEST(test_enc_uint_65536);
    RUN_TEST(test_enc_int_positive);
    RUN_TEST(test_enc_int_negative_1);
    RUN_TEST(test_enc_int_negative_100);

    printf("\n[Encode Simple Values]\n");
    RUN_TEST(test_enc_bool_false);
    RUN_TEST(test_enc_bool_true);
    RUN_TEST(test_enc_null);
    RUN_TEST(test_enc_float);

    printf("\n[Encode Strings/Bytes]\n");
    RUN_TEST(test_enc_str);
    RUN_TEST(test_enc_str_empty);
    RUN_TEST(test_enc_bytes);

    printf("\n[Encode Containers]\n");
    RUN_TEST(test_enc_array);
    RUN_TEST(test_enc_map);

    printf("\n[Encoder Overflow]\n");
    RUN_TEST(test_enc_overflow);
    RUN_TEST(test_enc_overflow_sticky);

    printf("\n[Round-trip]\n");
    RUN_TEST(test_roundtrip_uint);
    RUN_TEST(test_roundtrip_int_negative);
    RUN_TEST(test_roundtrip_bool);
    RUN_TEST(test_roundtrip_float);
    RUN_TEST(test_roundtrip_str);
    RUN_TEST(test_roundtrip_bytes);
    RUN_TEST(test_roundtrip_null);
    RUN_TEST(test_roundtrip_array);
    RUN_TEST(test_roundtrip_map);

    printf("\n[Telemetry Payload]\n");
    RUN_TEST(test_telemetry_payload);

    printf("\n[Decoder Errors]\n");
    RUN_TEST(test_dec_underflow);
    RUN_TEST(test_dec_type_mismatch);
    RUN_TEST(test_dec_str_too_small);
    RUN_TEST(test_dec_null_args);

    printf("\n[Skip]\n");
    RUN_TEST(test_dec_skip_scalar);
    RUN_TEST(test_dec_skip_nested);

    printf("\n[Edge Cases]\n");
    RUN_TEST(test_roundtrip_zero);
    RUN_TEST(test_roundtrip_large_uint);
    RUN_TEST(test_dec_remaining);
    RUN_TEST(test_err_str);
    RUN_TEST(test_type_name);

    printf("\n=== Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) printf(", %d FAILED", tests_failed);
    printf(" ===\n\n");

    return tests_failed > 0 ? 1 : 0;
}
