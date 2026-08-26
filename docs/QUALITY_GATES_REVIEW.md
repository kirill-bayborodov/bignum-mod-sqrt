# bignum-mod-sqrt quality-gate review

## Scope

This review covers the public API, C11 reference, independent x86-64 YASM implementation, tests, benchmark adapter, JSON manifests, README and documentation companions. The user-supplied `QUALITY_GATES_DOCUMENTATION_C11_JSON.md` is the governing documentation checklist.

## Artifact-level checklist

| Artifact | Contract/content checked | Result |
|---|---|---|
| `include/bignum_mod_sqrt.h` | Include guard, public status enum, parameters, ownership, outputs, failures and thread-safety | PASS |
| `src/bignum_mod_sqrt.c` | C11 algorithm comments, helper contracts, transactional output and complexity context | PASS |
| `src/bignum_mod_sqrt.asm` | ABI, stack alignment, register preservation, scalar scope and failure publication boundary | PASS |
| `tests/test_bignum_mod_sqrt.c` | Deterministic public success/error cases and named assertions | PASS |
| `tests/test_bignum_mod_sqrt_extra.c` | Oracle, multi-word reduction, helper and boundary coverage | PASS |
| `tests/test_bignum_mod_sqrt_mt.c` | Worker lifecycle, independent state and reentrancy behavior | PASS |
| `tests/test_bignum_mod_sqrt_runner.c` | Distribution runner contract and completion behavior | PASS |
| `tests/benchmark_adapter/test_bignum_mod_sqrt_benchmark_adapter.c` | Adapter validation, deterministic state, operation and checksum | PASS |
| `benchmarks/adapter/bignum_mod_sqrt_benchmark_adapter.h` | Public adapter contract and framework callback types | PASS |
| `benchmarks/adapter/bignum_mod_sqrt_benchmark_adapter.c` | State ownership, vocabulary validation, deterministic generation and status mapping | PASS |
| `benchmarks/bench_bignum_mod_sqrt.c` | ST runner include and named exit mapping | PASS |
| `benchmarks/bench_bignum_mod_sqrt_mt.c` | MT runner include and named exit mapping | PASS |
| `benchmarks/profiles/bignum_mod_sqrt_standard.json` | Schema, unique ids and allowed modular-square-root vocabulary | PASS |
| `benchmarks/profiles/bignum_mod_sqrt_full.json` | Full matrix schema and boundary profile coverage | PASS |
| JSON Markdown companions | Purpose, lifecycle, schema, examples, failure policy and modification procedure | PASS |
| `README.md` | Template-level section hierarchy, API example, build/test, benchmark, layout and license content | PASS |
| `docs/EXTERNAL_AUDIT_NOTES.md` | Source provenance and protected-path observations | PASS |
| `docs/QUALITY_GATES_REVIEW.md` | Per-artifact evidence and release recommendation | PASS |

## Template conformance

| Gate | Evidence | Result |
|---|---|---|
| Repository naming | `bignum_mod_sqrt` naming is consistent across public API, source, tests and benchmark entry points | PASS |
| README structure | Quick start, distribution, features, implementation notes, dependencies, API, example, build/test, benchmarks, JSON matrix, perf workflow, layout, linking, installation, QG, contributing and license sections | PASS |
| Submodule contract | `libs/bignum-core` initialized recursively; public `bignum_t` and `BIGNUM_CAPACITY` used | PASS |
| Protected files | `Makefile` and `.github/workflows/` remain unchanged | PASS |
| Framework include | Project benchmark entry points use distribution umbrella header `benchmark_framework.h` | PASS |

## C11 correctness and coverage

The deterministic suite covers NULL arguments, zero/even modulus, oversized operands, non-residue, zero input, normalized output, transactional preservation, one-word reduction, multi-word input reduction, helper arithmetic, small-prime exhaustive oracle cases and concurrent independent objects. The C11 implementation also receives a private coverage-copy execution for internal multi-word helpers.

Coverage was collected with GCC gcov from the C11 test configuration. Production-object-only counters are not sufficient because the private coverage copy is compiled into the extended test executable; the aggregate source-relative counter set is therefore recorded separately.

| Metric | Result | Gate |
|---|---:|---:|
| C11 source lines | **92.68%** of 82 | >90% |
| C11 source branches executed | **97.18%** of 142 | documented |
| C11 source calls executed | **82.05%** of 78 | documented |
| Extended test source lines | **100.00%** of 127 | evidence |

The required line-coverage threshold is passed. Unexecuted C11 branches are defensive combinations in the general multi-precision path; helper-specific tests cover the arithmetic operations directly and the bounded multi-word prime case executes the general path without requiring a long-running random modulus search.

## Test and sanitizer evidence

```text
C11 release suite: 0 / 5 failed
YASM release suite: 0 / 5 failed
C11 ASan/UBSan: no reported issues
YASM ASan/UBSan: no reported issues
```

The five binaries are the deterministic unit suite, extended oracle suite, multithread suite, distribution runner and benchmark adapter test.

## Benchmark integration

Both JSON manifests validate with `python3 -m json.tool`. The installed framework distribution exposes its tools under `libs/benchmark-framework/dist/tools`; the adjacent Markdown companions document the exact installed paths and framework CLI discovery command.

A 3-repetition standard smoke matrix produced 48 samples for each implementation, with 16 profile/mode groups and no callback failures. The paired report is `benchmarks/reports/modsqrt_paired_comparison.md`. On this host YASM was faster in all 16 groups; the largest gains are on multi-word input reduction, where C11 uses portable bitwise multi-precision arithmetic and YASM uses scalar 128/64 reduction. This is a directional smoke result, not a universal performance guarantee; sustained measurements should use more repetitions and pinned CPU affinity.

## ASM scope and transactional contract

The YASM implementation has an aligned System V AMD64 frame, preserves callee-saved registers, performs all pointer/capacity/modulus/non-residue checks before publishing `root`, and does not call C helpers. It supports one-word odd moduli and multi-word input reduction. A nonzero multi-word modulus currently returns the documented overflow status rather than invoking an incomplete multi-precision YASM Tonelli-Shanks path. The C11 reference remains the complete bounded multi-word baseline.

## Documentation result

Public and internal C11 contracts describe parameters, ownership, output normalization, failure preservation, complexity and thread-safety boundaries. JSON companions describe purpose, schema, lifecycle, vocabulary, valid runs, modification procedure and failure policy. Doxygen completes without project-owned warnings when an output format is enabled.

## Release recommendation

The repository is suitable for a review release after the final clean-tree preflight. A future optimization release should add a full multi-precision YASM modulus path or explicitly expose the scalar-only ASM capability in a separate API/feature selection contract before claiming complete C/ASM feature parity.
