/**
 * @file bignum_mod_sqrt_benchmark_adapter.h
 * @brief benchmark-framework binding for bignum_mod_sqrt.
 *
 * The adapter owns only modular-square-root domain generation and status mapping.
 * The framework owns CLI parsing, dataset copies, timing, worker lifecycle and
 * checksum reduction. Each state contains an input, the fixed odd modulus 11,
 * and a caller-owned root output.
 */
#ifndef BIGNUM_MOD_SQRT_BENCHMARK_ADAPTER_H
#define BIGNUM_MOD_SQRT_BENCHMARK_ADAPTER_H

#include <benchmark_framework.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Named construction and validation results owned by this adapter. */
typedef enum {
    BIGNUM_MOD_SQRT_BENCHMARK_STATUS_SUCCESS = 0,
    BIGNUM_MOD_SQRT_BENCHMARK_STATUS_NULL_ARGUMENT = 1,
    BIGNUM_MOD_SQRT_BENCHMARK_STATUS_INVALID_PROFILE = 2,
    BIGNUM_MOD_SQRT_BENCHMARK_STATUS_OPERATION_ERROR = 3
} bignum_mod_sqrt_benchmark_status_t;

/**
 * @brief Initializes the project binding used by benchmark-framework.
 * @param[out] adapter Receives the complete callback binding.
 * @return A named adapter status.
 * @post On success all callback pointers, state size and benchmark name are set.
 * @note The binding owns no heap memory and is safe to copy for independent runs.
 */
bignum_mod_sqrt_benchmark_status_t bignum_mod_sqrt_benchmark_adapter_init(
    benchmark_adapter_t *adapter);

/**
 * @brief Validates every domain axis accepted by the JSON manifests.
 * @param[in] workload Immutable framework workload descriptor.
 * @return Success when all strings belong to the documented vocabulary.
 * @note No state is modified and no allocation is performed.
 */
bignum_mod_sqrt_benchmark_status_t bignum_mod_sqrt_benchmark_validate_workload(
    const benchmark_workload_t *workload);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_MOD_SQRT_BENCHMARK_ADAPTER_H */
