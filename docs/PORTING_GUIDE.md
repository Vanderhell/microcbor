# Porting Guide

`microcbor` needs `mcbor.h`, `mcbor.c`, and a C99 compiler. No platform callbacks
are required because it operates purely on memory buffers.

## Integration

```cmake
add_library(microcbor STATIC lib/microcbor/src/mcbor.c)
target_include_directories(microcbor PUBLIC lib/microcbor/include)
```

Works on bare metal, RTOS, Linux, Windows, and macOS. The only libc dependency is
`string.h` for `memcpy`, `memset`, and `strlen`.

## Usage with iotspool

```c
/* Encode telemetry -> spool -> publish */
uint8_t cbor_buf[64];
mcbor_enc_t enc;
mcbor_enc_init(&enc, cbor_buf, sizeof(cbor_buf));
mcbor_enc_map(&enc, 2);
mcbor_enc_str(&enc, "temp");  mcbor_enc_float(&enc, temp);
mcbor_enc_str(&enc, "hum");   mcbor_enc_uint(&enc, hum);

size_t used = 0;
mcbor_enc_size(&enc, &used);
spool_enqueue("sensor/data", cbor_buf, used);
```
