# Design Rationale

## 1. Definite-length only

Indefinite-length encoding complicates the decoder. IoT payloads are small and
known at encode time.

## 2. No 64-bit integers

`uint32_t` covers sensor values, timestamps until 2106, and most embedded use
cases. Many 8-bit and 16-bit MCUs do not handle `uint64_t` efficiently.

## 3. No tags

CBOR tags add semantic annotations, but for telemetry the schema is usually known
to both sides. Tags add bytes without much value in this target domain.

## 4. Sticky overflow flag

The API allows many encode calls to be chained and checked once at the end.

## 5. Zero-copy string decoding

`mcbor_dec_bytes_ref` returns a pointer into the input buffer. No copy and no
allocation are required.

## 6. Skip for forward compatibility

`mcbor_dec_skip` handles nested containers recursively and helps tolerate unknown
fields during protocol evolution.

| Decision | Gains | Costs |
|----------|-------|-------|
| Definite-length only | Simple, small decoder | Cannot stream encode |
| No 64-bit | Compact, portable | Max 4 billion |
| No tags | Smaller payloads | No semantic annotations |
| Sticky overflow | Ergonomic API | Must check after sequence |
| Zero-copy refs | No allocation | Lifetime tied to input buffer |
