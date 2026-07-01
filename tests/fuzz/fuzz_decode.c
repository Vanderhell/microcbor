#include "mcbor.h"
#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    mcbor_dec_t dec;
    mcbor_value_t value;
    const uint8_t *bytes;
    size_t len;

    if (data == NULL || size == 0) {
        return 0;
    }

    if (mcbor_dec_init(&dec, data, size) != MCBOR_OK) {
        return 0;
    }

    (void)mcbor_dec_next(&dec, &value);

    if (mcbor_dec_init(&dec, data, size) == MCBOR_OK) {
        (void)mcbor_dec_skip(&dec);
    }

    if (mcbor_dec_init(&dec, data, size) == MCBOR_OK &&
        mcbor_dec_bytes_ref(&dec, &bytes, &len) == MCBOR_OK) {
        if (len > size) {
            __builtin_trap();
        }
        if (bytes < data || bytes + len > data + size) {
            __builtin_trap();
        }
    }

    return 0;
}
