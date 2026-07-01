# Cookbook

## Initialization

Encoder:

```c
uint8_t buf[32];
mcbor_enc_t enc;
mcbor_err_t err = mcbor_enc_init(&enc, buf, sizeof(buf));
```

Decoder:

```c
mcbor_dec_t dec;
mcbor_err_t err = mcbor_dec_init(&dec, buf, size);
```

## Checked quick start

```c
uint8_t buf[64];
mcbor_enc_t enc;
size_t used = 0;

if (mcbor_enc_init(&enc, buf, sizeof(buf)) != MCBOR_OK) return;
if (mcbor_enc_map(&enc, 2) != MCBOR_OK) return;
if (mcbor_enc_str(&enc, "temp") != MCBOR_OK) return;
if (mcbor_enc_float(&enc, 23.5f) != MCBOR_OK) return;
if (mcbor_enc_str(&enc, "ok") != MCBOR_OK) return;
if (mcbor_enc_bool(&enc, true) != MCBOR_OK) return;
if (mcbor_enc_size(&enc, &used) != MCBOR_OK) return;
if (mcbor_validate_one(buf, used) != MCBOR_OK) return;
```

## Notes

- `mcbor_enc_array()` and `mcbor_enc_map()` emit headers only.
- `mcbor_dec_str()` is transactional and NUL-terminates on success.
- `mcbor_validate_one()` validates one complete top-level supported item.
- Zero-copy spans returned by the decoder point into the caller-owned input buffer.
- `mcbor_enc_text()` accepts explicit-length UTF-8, including embedded NUL bytes.
- When `MCBOR_ENABLE_FLOAT32` is `0`, float encode/decode returns `MCBOR_ERR_UNSUPPORTED`.
- Separate instances with separate buffers are reentrant; shared instances need external synchronization.
