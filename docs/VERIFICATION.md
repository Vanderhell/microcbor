# Verification

No build, test, sanitizer, analyzer, or CI job was executed in this session.

## Verified by inspection

- Public fixed-width ABI typedefs and `size_t` caller-facing sizes are present.
- Transactional encode/decode and `mcbor_validate_one()` entry points are present in the source tree.
- Repository-local test, fuzz, compile-fail, packaging, consumer, and CI definitions are present.

## Not verified

- Unit tests
- Fuzz execution
- Compile-fail execution
- GCC/Clang/MSVC builds
- `-fshort-enums`
- float32 enabled and disabled builds
- ASan/UBSan
- Static analysis
- C++ consumer
- install-tree `find_package()` consumer
- `add_subdirectory` consumer
- ARM Cortex-M compile-only jobs

## Release blockers

- The verification matrix still needs to be run by the user or CI.
- Documentation claims should remain limited to the evidence above until that execution happens.
