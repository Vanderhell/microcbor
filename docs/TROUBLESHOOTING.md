# Troubleshooting

- Sticky overflow: once overflow is set, later encode calls keep returning `MCBOR_ERR_OVERFLOW` until reinitialization.
- Decoder rollback: failed typed decode calls leave `dec.pos` unchanged.
- `mcbor_dec_str()` too small: on `MCBOR_ERR_SIZE`, grow the destination buffer and retry.
- Invalid UTF-8: text encode/decode rejects malformed sequences.
- Signed integer range: `mcbor_dec_int()` accepts only the `INT32_MIN..INT32_MAX` domain.
- Unsupported 64-bit arguments: additional-info `27` is not supported.
- Indefinite-length items: additional-info `31` is rejected.
- Semantic tags: major type `6` is unsupported.
- Canonical output: shortest-width integer and length heads are used, but map keys are not sorted and duplicate keys are not rejected.
- Float32 disabled: float encode/decode returns `MCBOR_ERR_UNSUPPORTED`.
- NaN behavior: no canonical NaN guarantee is made.
- Container completeness: `mcbor_enc_size()` and `mcbor_dec_done()` do not prove semantic completeness by themselves.
- Zero-copy lifetime: returned spans remain valid only while the input buffer remains valid and unchanged.
- `find_package()` prefix issues: set `CMAKE_PREFIX_PATH` to the install prefix containing `microcborConfig.cmake`.
