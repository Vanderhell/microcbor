# Contributing to microcbor

In scope: bug fixes, docs, tests, and narrowly scoped performance work.

Out of scope unless explicitly re-scoped later: 64-bit integers, indefinite-length items, semantic tags, DOM/schema layers, hidden global state, and heap-backed features.

1. Keep the project C99-compatible.
2. Preserve fixed-memory behavior and avoid heap dependencies.
3. Do not weaken malformed-input, compile-fail, or fuzz coverage.
4. Keep changes small and aligned with the supported CBOR subset.
5. Do not create tags or releases unless explicitly requested.

By contributing, you agree to the MIT License.
