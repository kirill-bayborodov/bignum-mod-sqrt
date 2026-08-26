# Full modular-square-root benchmark profile

## Purpose and scope

The full manifest exercises the independent C11 and x86-64 YASM implementations across zero, known-residue, generated-residue and mixed operations. It includes scalar one-word inputs, quarter/half capacity records, variable lengths, multi-word input reduction and near-capacity state allocation.

The adapter uses modulus 11 because it is a small odd prime with both zero and nonzero quadratic residues. Generated rows are constrained to residues so successful callback completion measures the operation rather than an intentional error.

## Framework schema

The JSON uses `schema_version: 1`. Each profile has a unique `id` and the five framework transport axes: `input_kind`, `operation_kind`, `measure_mode`, `size_profile` and `capacity_profile`.

| Field | Allowed values | Meaning |
|---|---|---|
| `input_kind` | `zero`, `nonzero`, `mixed` | Source input class or deterministic alternation |
| `operation_kind` | `root-zero`, `root-residue`, `root-random`, `root-mixed` | Modular-square-root workload path |
| `measure_mode` | `end-to-end`, `kernel-only` | Whether framework preparation copy is timed |
| `size_profile` | `one`, `quarter`, `half`, `variable`, `near-capacity` | Logical input word length |
| `capacity_profile` | `normal`, `near-capacity` | Storage boundary condition |

## State and lifecycle

Each opaque state is a structure containing `input`, `modulus` and `root` `bignum_t` records. The framework allocates and copies states; the adapter owns no dataset memory and no global mutable state. Initialization is deterministic from `seed` and `sequence_index`. The operation callback invokes `bignum_mod_sqrt`, and the checksum callback hashes the complete root record and iteration.

## Reproducible workflow

Build the C11 baseline and YASM candidate independently:

```bash
make clean
make bin/bench_bignum_mod_sqrt bin/bench_bignum_mod_sqrt_mt CONFIG=release USE_ASM=no
make clean
make bin/bench_bignum_mod_sqrt bin/bench_bignum_mod_sqrt_mt CONFIG=release USE_ASM=yes
```

The installed distribution exposes tools below `libs/benchmark-framework/dist/tools`. Inspect exact options before running a matrix:

```bash
libs/benchmark-framework/dist/tools/bench_matrix --help
libs/benchmark-framework/dist/tools/benchmark_stats --help
```

For every paired run, keep manifest, binary arguments, seed, warm-up, data count, repetitions, thread count and CPU affinity identical. Preserve raw JSON output and the aggregated summary under `benchmarks/reports/`.

## Quality and comparison policy

A valid sample requires successful callback status, a checksum, one machine-readable completion line and the final `Benchmark finished.` marker. A malformed profile, unknown vocabulary, timeout or callback failure invalidates the matrix. Summaries should report median, MAD or another robust dispersion statistic and should not claim universal speedup when tiny-input or MT scheduling overhead dominates.

## Modification procedure

Add a unique profile id, use only the vocabulary table above, update this companion and validate JSON before commit. Changes to `Makefile` and `.github/workflows/` are outside this profile change and remain prohibited by the project workflow.
