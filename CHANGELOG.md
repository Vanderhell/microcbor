# Changelog

## [1.0.0] - 2026-03-20

### Added

- CBOR encoder: uint, int, bool, null, float32, text, bytes, array, map.
- Compact encoding with automatic integer width selection.
- Sticky overflow detection.
- CBOR decoder: generic next plus typed convenience decoders.
- Zero-copy byte and string references.
- Skip function for nested containers.
- Full round-trip tests including a telemetry payload.
- 43 tests passing with `-Werror`.
