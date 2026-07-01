# microcbor

[![CI](https://img.shields.io/github/actions/workflow/status/Vanderhell/microcbor/ci.yml?branch=master&label=ci)](https://github.com/Vanderhell/microcbor/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![C99](https://img.shields.io/badge/language-C99-blue.svg)](#quick-start)
[![Zero Allocation](https://img.shields.io/badge/memory-zero%20allocation-orange.svg)](#features)

Minimal CBOR encoder/decoder for embedded systems.

C99 | Zero dependencies | Zero allocations | RFC 8949 subset | Portable

## Why microcbor?

IoT devices send sensor data over MQTT, CoAP, or BLE. JSON is the default choice,
but on a microcontroller it is expensive: string escaping, float-to-text conversion,
quotes around every key, and a parser that needs to handle arbitrary nesting.

CBOR (RFC 8949) is a binary format that is often 30-50% smaller than JSON, trivial
to encode, and fast to decode. `microcbor` provides a compact C99 implementation for
small firmware and constrained runtimes.

```c
/* Encode a telemetry message */
uint8_t buf[64];
mcbor_enc_t enc;
mcbor_enc_init(&enc, buf, sizeof(buf));
mcbor_enc_map(&enc, 3);
mcbor_enc_str(&enc, "temp");  mcbor_enc_float(&enc, 23.4f);
mcbor_enc_str(&enc, "hum");   mcbor_enc_uint(&enc, 55);
mcbor_enc_str(&enc, "ok");    mcbor_enc_bool(&enc, true);

size_t used = 0;
mcbor_enc_size(&enc, &used);

/* Send buf[0..used) over MQTT */
mqtt_publish("sensor/data", buf, used);
```

The same message in JSON would be `{"temp":23.4,"hum":55,"ok":true}` (35 bytes).
In CBOR it fits in 26 bytes. On a LoRa link with a 51-byte payload limit, that
difference matters.

## Features

- Sequential encoder that writes into a caller-provided buffer.
- Sequential decoder with typed convenience helpers.
- Zero-copy byte and text references for decode paths.
- Sticky overflow detection so encode chains can be checked once at the end.
- Compact integer encoding with automatic width selection.
- RFC 8949 subset focused on embedded telemetry workloads.
- No heap allocation and no external dependencies.

## Scope

Implemented:

- Unsigned and signed 32-bit integers
- Booleans and null
- Float32
- Text strings and byte strings
- Definite-length arrays and maps
- Recursive skip for unknown or forward-compatible fields

Not implemented:

- 64-bit integers
- Indefinite-length items
- Tags
- Dynamic allocation

## Quick Start

### Encode

```c
uint8_t buf[128];
mcbor_enc_t enc;
mcbor_enc_init(&enc, buf, sizeof(buf));

mcbor_enc_map(&enc, 2);
mcbor_enc_str(&enc, "device");  mcbor_enc_str(&enc, "sensor-01");
mcbor_enc_str(&enc, "temp");    mcbor_enc_float(&enc, 22.5f);

size_t size = 0;
mcbor_enc_size(&enc, &size);
/* buf[0..size] contains valid CBOR */
```

### Decode

```c
mcbor_dec_t dec;
mcbor_dec_init(&dec, buf, size);

size_t count;
mcbor_dec_map(&dec, &count);

for (size_t i = 0; i < count; i++) {
    char key[24];
    size_t klen;
    mcbor_dec_str(&dec, key, sizeof(key), &klen);

    if (strcmp(key, "temp") == 0) {
        float temp;
        mcbor_dec_float(&dec, &temp);
    } else {
        mcbor_dec_skip(&dec);
    }
}
```

## Build And Test

Minimal GCC build:

```sh
gcc -std=c99 -Wall -Wextra -Wpedantic -Werror -Iinclude src/mcbor.c tests/test_all.c -lm -o test_all
./test_all
```

Using the bundled test makefile:

```sh
cd tests
make
```

On Windows, use MinGW, MSYS2, or WSL if `gcc` and `make` are not already installed.

## API At A Glance

### Encoder

| Function | Description |
|----------|-------------|
| `mcbor_enc_init` | Initialize encoder with buffer |
| `mcbor_enc_uint` | Encode unsigned integer |
| `mcbor_enc_int` | Encode signed integer |
| `mcbor_enc_bool` | Encode boolean |
| `mcbor_enc_null` | Encode null |
| `mcbor_enc_float` | Encode float32 |
| `mcbor_enc_str` | Encode NUL-terminated string |
| `mcbor_enc_text` | Encode string with explicit length |
| `mcbor_enc_bytes` | Encode byte array |
| `mcbor_enc_array` | Begin definite-length array |
| `mcbor_enc_map` | Begin definite-length map |
| `mcbor_enc_size` | Get bytes written |
| `mcbor_enc_overflow` | Check overflow flag |

### Decoder

| Function | Description |
|----------|-------------|
| `mcbor_dec_init` | Initialize decoder with buffer |
| `mcbor_dec_next` | Decode next item generically |
| `mcbor_dec_uint` | Decode unsigned integer |
| `mcbor_dec_int` | Decode signed integer |
| `mcbor_dec_bool` | Decode boolean |
| `mcbor_dec_float` | Decode float32 |
| `mcbor_dec_str` | Decode string with copy and NUL terminator |
| `mcbor_dec_bytes_ref` | Decode bytes as zero-copy reference |
| `mcbor_dec_array` | Decode array header |
| `mcbor_dec_map` | Decode map header |
| `mcbor_dec_skip` | Skip next item, including nested items |
| `mcbor_dec_remaining` | Return unread bytes |
| `mcbor_dec_done` | Check whether input is fully consumed |

## Repository Layout

- `include/mcbor.h` public API
- `src/mcbor.c` implementation
- `tests/test_all.c` unit and round-trip tests
- `docs/API_REFERENCE.md` API notes
- `docs/DESIGN.md` design decisions
- `docs/PORTING_GUIDE.md` integration notes

## Ecosystem

| Library | Integration |
|---------|-------------|
| [iotspool](https://github.com/Vanderhell/iotspool) | CBOR encode -> spool -> MQTT publish |
| [microconf](https://github.com/Vanderhell/microconf) | Export config as CBOR for OTA |
| [microlog](https://github.com/Vanderhell/microlog) | Log CBOR encode and decode errors |

## Project Status

`microcbor` is suitable for publishing as a focused embedded utility library. The
main remaining quality lever after publishing is automated CI execution on GitHub.

## License

MIT License. Copyright (c) 2026 Vanderhell. See [LICENSE](LICENSE).
