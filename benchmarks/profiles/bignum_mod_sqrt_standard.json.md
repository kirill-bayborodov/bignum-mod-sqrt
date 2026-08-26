# Standard modular-square-root benchmark profile

## Purpose

This manifest is the deterministic smoke matrix for `bignum_mod_sqrt`. It covers the normalized zero fast path, scalar known residues, generated residues, mixed input rows, and near-capacity boundaries.

## Schema and vocabulary

The manifest uses `schema_version: 1` and the framework `profiles` array. Each profile contains `id`, `input_kind`, `operation_kind`, `measure_mode`, `size_profile`, and `capacity_profile`. `input_kind` is `zero`, `nonzero`, or `mixed`. `operation_kind` is `root-zero`, `root-residue`, `root-random`, or `root-mixed`. `measure_mode` is `end-to-end` or `kernel-only`; `size_profile` is `one`, `quarter`, `half`, `variable`, or `near-capacity`; and `capacity_profile` is `normal` or `near-capacity`.

The adapter fixes the benchmark modulus at 11 and generates inputs whose reduced value is a quadratic residue. The state contains input, modulus, and root records; the framework owns allocation and per-operation copies.

## Reproducible run

Build both benchmark binaries with the protected project Makefile, then inspect the installed tool contract:

```bash
make bin/bench_bignum_mod_sqrt bin/bench_bignum_mod_sqrt_mt CONFIG=release USE_ASM=no
libs/benchmark-framework/dist/tools/bench_matrix --help
```

For direct runner validation, use explicit limits:

```bash
./bin/bench_bignum_mod_sqrt --input-kind nonzero --operation-kind root-residue \
  --size-profile one --measure-mode kernel-only --iterations 100000 \
  --warmup 1000 --data-count 128 --seed 123
```

A successful runner prints one machine-readable benchmark line followed by `Benchmark finished.`. Any callback error invalidates the run.

## Baseline and comparison

Use identical manifest, implementation settings, seed, warm-up, data count, repetitions, thread count and CPU affinity for C11 and YASM. Report medians and dispersion rather than a single sample. Small-input and MT results are sensitive to framework and scheduling overhead and must be interpreted separately from sustained kernel measurements.

## Modification procedure

When adding a profile, use a unique stable `id`, retain all schema fields, select only adapter vocabulary, and update this companion. Validate JSON before committing. Do not reintroduce unrelated `shift-*` or byte-transform vocabulary.

## Failure policy

Malformed JSON, unknown vocabulary, callback failure, missing checksum, or missing completion marker invalidates the matrix result. `Makefile` and CI workflows are protected and are not edited for profile changes.
