# bignum-mod-sqrt

[![C/ASM CI](https://github.com/kirill-bayborodov/bignum-mod-sqrt/actions/workflows/ci.yml/badge.svg)](https://github.com/kirill-bayborodov/bignum-mod-sqrt/actions/workflows/ci.yml)

`bignum-mod-sqrt` computes a modular square root for a bounded little-endian `bignum_t`. The C11 implementation is the reference used for correctness, coverage and baseline measurements. The x86-64 YASM implementation is an independent optimized path using scalar modular multiplication and exponentiation without C helper calls.

## Quick start

```bash
git clone --recurse-submodules https://github.com/kirill-bayborodov/bignum-mod-sqrt.git
cd bignum-mod-sqrt
git submodule update --init --recursive
make build CONFIG=release
make test CONFIG=release
```

## Distribution

The `libs/bignum-core` submodule defines `bignum_t` and `BIGNUM_CAPACITY`. The benchmark framework is supplied as a downloaded distribution under `libs/benchmark-framework/dist`; it provides `benchmark_framework.h`, `libbenchmark_framework.a`, `tools/bench_matrix` and `tools/benchmark_stats`. The framework distribution is a library artifact and is not modified by this project.

| Component | Location | Purpose |
|---|---|---|
| `bignum-core` | `libs/bignum-core` | Public bounded integer representation |
| `benchmark-framework` | `libs/benchmark-framework/dist` | Deterministic ST/MT benchmark lifecycle and JSON tools |

## Features

The module exposes named status values, validates all pointers and capacity lengths, preserves `root` on failure, normalizes successful output, and supports zero and non-residue cases. The C11 reference supports bounded multi-word operands and odd moduli. The YASM path supports scalar odd moduli, performs MSB-first reduction of multi-word inputs, handles normalized zero with multi-word moduli, and returns `BIGNUM_MOD_SQRT_ERROR_OVERFLOW` for nonzero multi-word moduli until a full multi-precision YASM Tonelli-Shanks leaf is added.

## Implementation notes

For one-word odd moduli, the reference uses a 64-bit Tonelli-Shanks implementation with a fast `p % 4 == 3` exponentiation branch and a general `p % 4 == 1` branch. The YASM path uses `mul`/`div` for exact 128-by-64 modular multiplication, binary square-and-multiply, transactional validation before publication, and a stack-aligned System V AMD64 frame. No global mutable state is used.

## Dependencies

| Dependency | Purpose |
|---|---|
| `make` and `gcc` | C11 build, test and link |
| `yasm` | x86-64 assembly build |
| `cppcheck` | Static analysis |
| `pthread` | Multithreaded tests and benchmark runner |
| `gcov` | C11 coverage evidence |
| `perf` and `taskset` | Optional performance workflow |

## API

The public API is declared in `include/bignum_mod_sqrt.h`:

```c
typedef enum {
    BIGNUM_MOD_SQRT_SUCCESS        =  0,
    BIGNUM_MOD_SQRT_ERROR_NULL_ARG = -1,
    BIGNUM_MOD_SQRT_ERROR_MODULUS  = -2,
    BIGNUM_MOD_SQRT_ERROR_NOT_RESIDUE = -3,
    BIGNUM_MOD_SQRT_ERROR_OVERFLOW = -4
} bignum_mod_sqrt_status_t;

bignum_mod_sqrt_status_t bignum_mod_sqrt(
    const bignum_t *a,
    const bignum_t *modulus,
    bignum_t *root);
```

### Contract

| Condition | Result |
|---|---|
| Any required pointer is `NULL` | Returns `BIGNUM_MOD_SQRT_ERROR_NULL_ARG`; no object is dereferenced |
| Modulus is zero or even | Returns `BIGNUM_MOD_SQRT_ERROR_MODULUS`; `root` is unchanged |
| Input or modulus length exceeds `BIGNUM_CAPACITY` | Returns `BIGNUM_MOD_SQRT_ERROR_OVERFLOW`; `root` is unchanged |
| No modular square root exists | Returns `BIGNUM_MOD_SQRT_ERROR_NOT_RESIDUE`; `root` is unchanged |
| A root exists | Returns success and writes a normalized root in `[0, modulus)` |

The C11 reference accepts an odd modulus as the algorithm precondition documented by the API; callers are responsible for supplying a prime modulus. Independent objects may be processed concurrently. A single object must not be shared concurrently without external synchronization.

### Example

```c
#include <assert.h>
#include "bignum_mod_sqrt.h"

int main(void)
{
    bignum_t value = {{4U}, 1U};
    bignum_t modulus = {{11U}, 1U};
    bignum_t root = {{0U}, 0U};

    assert(bignum_mod_sqrt(&value, &modulus, &root) ==
           BIGNUM_MOD_SQRT_SUCCESS);
    assert((root.words[0] * root.words[0]) % modulus.words[0] == value.words[0]);
    return 0;
}
```

## Build and test

```bash
make build CONFIG=release USE_ASM=no
make test CONFIG=release USE_ASM=no
make build CONFIG=release USE_ASM=yes
make test CONFIG=release USE_ASM=yes
make lint CONFIG=release
```

Run dynamic checks with:

```bash
make clean
make test_sanitize SAN=address CONFIG=debug USE_ASM=no
make clean
make test_sanitize SAN=undefined CONFIG=debug USE_ASM=no
make clean
make test_helgrind CONFIG=debug USE_ASM=no
```

The deterministic suite covers valid roots, zero, non-residue, invalid modulus, NULL arguments, overflow, normalized representation and transactional preservation. The extended suite includes exhaustive small-prime oracle cases, multi-word input reduction, helper arithmetic boundaries and multi-thread-independent objects.

## Benchmarks

The ST and MT entry points use the public `benchmark_framework.h` distribution header and the adapter in `benchmarks/adapter/`. The adapter state contains `input`, fixed modulus 11 and `root`. Its operation vocabulary is `root-zero`, `root-residue`, `root-random` and `root-mixed`; no shift-template vocabulary is used.

```bash
make bin/bench_bignum_mod_sqrt bin/bench_bignum_mod_sqrt_mt CONFIG=release USE_ASM=no
./bin/bench_bignum_mod_sqrt --input-kind nonzero --operation-kind root-residue \
  --size-profile one --measure-mode kernel-only --iterations 1000000 \
  --warmup 10000 --data-count 4096 --seed 123
```

Every successful runner prints a machine-readable `benchmark=bignum_mod_sqrt_st` or `benchmark=bignum_mod_sqrt_mt` line followed by `Benchmark finished.`. Keep implementation, seed, profile, warm-up, iteration count, data count, thread count and CPU affinity identical for paired comparisons.

## JSON matrix

The manifests are `benchmarks/profiles/bignum_mod_sqrt_standard.json` and `benchmarks/profiles/bignum_mod_sqrt_full.json`. Their adjacent Markdown companions define schema, vocabulary, lifecycle, examples and failure policy. The downloaded framework tools are available at `libs/benchmark-framework/dist/tools/bench_matrix` and `libs/benchmark-framework/dist/tools/benchmark_stats`.

```bash
libs/benchmark-framework/dist/tools/bench_matrix \
  --profile benchmarks/profiles/bignum_mod_sqrt_standard.json \
  --st ./bin/bench_bignum_mod_sqrt --mt ./bin/bench_bignum_mod_sqrt_mt \
  --repetitions 7 --iterations 1000000
```

The exact command-line options depend on the pinned framework distribution; use `--help` for the installed tool. Matrix reports belong in `benchmarks/reports/` and are not source inputs.

## Perf workflow

The optional Makefile targets `bench_stat_st`, `bench_stat_mt`, `bench_full` and `bench_cl` provide repeated measurements. Use `bench_cl` when only software perf events are available. A fair comparison keeps event selection, repetitions, workload, seed and CPU affinity constant. Tiny-input and MT results should be interpreted separately from sustained scalar-kernel results because scheduling and timer overhead can dominate them.

## Repository layout

| Path | Purpose |
|---|---|
| `include/` | Public API and status contract |
| `src/bignum_mod_sqrt.c` | C11 reference implementation |
| `src/bignum_mod_sqrt.asm` | Independent x86-64 YASM implementation |
| `tests/` | Deterministic, oracle, MT and distribution tests |
| `benchmarks/adapter/` | Domain bridge to benchmark-framework |
| `benchmarks/profiles/` | Standard/full JSON matrices and companions |
| `docs/` | Quality-gate and external audit evidence |
| `libs/` | Submodule and downloaded framework distribution |

## Linking the object file

```bash
gcc -std=c11 -Iinclude -Ilibs/bignum-core/include \
  example.c build/bignum_mod_sqrt.o build/bignum_core.o -o example
```

Use `make build CONFIG=release USE_ASM=yes` to select the YASM object. Use `USE_ASM=no` for the C11 reference baseline.

## Installation and distribution

```bash
make dist CONFIG=release USE_ASM=yes
```

The generated distribution is placed in `dist/`. It is intended for local review and integration testing; consumers should preserve the public header contract and link the selected implementation object or archive.

## Quality gates

The project applies the user-provided `QUALITY_GATES_DOCUMENTATION_C11_JSON.md` and the repository quality-gate checklist in `docs/QUALITY_GATES_REVIEW.md`. Release evidence must include clean C11 and YASM tests, sanitizer results, C11 gcov coverage above 90%, JSON validation, documentation audit, static analysis, benchmark metadata and a review confirming that `Makefile` and `.github/workflows/` were not modified.

## Contributing

Changes should preserve the public API, transactional failure behavior, normalized representation and System V AMD64 ABI. Add deterministic tests for every new branch and update the relevant JSON companion before submitting a change. Do not modify the protected Makefile or CI workflows without explicit project-level approval.

## License

This project is distributed under the MIT License. See [`LICENSE`](LICENSE) for the complete license text.
