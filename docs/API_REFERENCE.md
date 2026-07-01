# API Reference

> **Header:** `#include "mcbor.h"` | **Config:** `mcbor_config.h`

See the header file for complete function signatures. All functions return
`mcbor_err_t`. Use `mcbor_err_str()` for human-readable errors.

## CBOR wire format quick reference

| Value range | Encoding | Bytes |
|-------------|----------|-------|
| 0-23 | value in header | 1 |
| 24-255 | header + 1 byte | 2 |
| 256-65535 | header + 2 bytes | 3 |
| 65536-4294967295 | header + 4 bytes | 5 |
| float32 | `0xFA` + 4 bytes | 5 |
| false / true / null | `0xF4` / `0xF5` / `0xF6` | 1 |
| string `"hi"` | `0x62` + `"hi"` | 3 |

## Error codes

| Code | Meaning |
|------|---------|
| `MCBOR_OK` | Success |
| `MCBOR_ERR_NULL` | NULL pointer |
| `MCBOR_ERR_OVERFLOW` | Buffer too small for encode |
| `MCBOR_ERR_UNDERFLOW` | Not enough data to decode |
| `MCBOR_ERR_TYPE` | Unexpected CBOR type |
| `MCBOR_ERR_INVALID` | Malformed CBOR |
| `MCBOR_ERR_SIZE` | Output buffer too small |
| `MCBOR_ERR_RANGE` | Value exceeds supported range |
| `MCBOR_ERR_UNSUPPORTED` | Feature disabled or unavailable |

## Contracts

- Public status and type representations use fixed-width typedefs.
- Caller-facing capacities, positions, and lengths use `size_t`.
- Encode and decode entry points are transactional on failure.
- `mcbor_validate_one()` validates exactly one complete top-level supported item.
- Float32 support is controlled by `MCBOR_ENABLE_FLOAT32`.
