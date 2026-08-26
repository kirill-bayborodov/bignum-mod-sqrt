# External audit notes

The repository was obtained from https://github.com/kirill-bayborodov/bignum-mod-sqrt.

The documentation standard is the user-provided `QUALITY_GATES_DOCUMENTATION_C11_JSON.md`.

Family repositories inspected through GitHub CLI/API include:

- https://github.com/kirill-bayborodov/bignum-bit-test
- https://github.com/kirill-bayborodov/bignum-to-hex
- https://github.com/kirill-bayborodov/bignum-from-hex
- https://github.com/kirill-bayborodov/bignum-fast-div
- https://github.com/kirill-bayborodov/bignum-mod-exp
- https://github.com/kirill-bayborodov/bignum-montgomery-mul
- https://github.com/kirill-bayborodov/bignum-karatsuba-mul

The common submodule is https://github.com/kirill-bayborodov/bignum-core.git, checked out in this repository at commit `c78e5756a237c6443e0cb5083548aeb714286830` (`v0.0.1`).

The latest successful benchmark-framework distribution was retrieved from the `dist` artifact of successful GitHub Actions run `32469640055` in https://github.com/kirill-bayborodov/benchmark-framework. It is installed locally under `libs/benchmark-framework/dist/` and provides `benchmark_framework.h`, `libbenchmark_framework.a`, `tools/bench_matrix`, `tools/benchmark_stats`, and profile guides.

Family assembly sources inspected include:

- `bignum-mod-exp/src/bignum_mod_exp.asm`, a self-contained System V AMD64 modular-arithmetic implementation with 32-word little-endian records and `len` at offset 256.
- `bignum-montgomery-mul/src/bignum_montgomery_mul.asm`, including a one-word fast leaf and full multi-word path.
- `bignum-fast-div/src/bignum_fast_div.asm`.
- `bignum-karatsuba-mul/src/bignum_karatsuba_mul.asm` and recursive model assembly sources.

The current mod-sqrt repository initially had no `src/bignum_mod_sqrt.asm`. Its existing README and benchmark adapter were mechanically derived from a shift-left module: they described an in-place logical left shift and used `shift-*` vocabulary, while the public header and C11 source implement modular square root. The Makefile is protected and currently expects framework tools under `libs/benchmark-framework/build/tools`; the downloaded distribution exposes them under `libs/benchmark-framework/dist/tools`.
