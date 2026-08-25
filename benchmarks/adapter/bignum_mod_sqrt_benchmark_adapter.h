/**
 * @file bignum_mod_sqrt_benchmark_adapter.h
 * @brief C11 domain adapter between bignum_mod_sqrt and benchmark-core.
 *
 * @details
 * This header defines the bignum-specific binding used by the generic
 * benchmark-framework v1.0.0 lifecycle. The generic framework deliberately
 * transports domain vocabulary as text. This adapter validates that vocabulary
 * before creating deterministic bignum_t source records and invoking the
 * in-place bignum_mod_sqrt operation.
 *
 * The required transport mapping is intentionally semantic rather than a
 * mechanical rename. benchmark-core `operation_kind` accepts
 * `shift-zero`, `shift-bit`, `shift-word`, `shift-combined`, `shift-random`,
 * or `shift-mixed`; the adapter removes the `shift-` transport prefix and
 * applies the resulting bignum shift path. benchmark-core `size_profile`
 * accepts `one`, `quarter`, `half`, `variable`, or `near-capacity`; the
 * adapter uses it as the bignum operand-length profile.
 */
#ifndef BIGNUM_MOD_SQRT_BENCHMARK_ADAPTER_H
#define BIGNUM_MOD_SQRT_BENCHMARK_ADAPTER_H

#include <benchmark_framework.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Named outcomes returned by bignum_mod_sqrt adapter construction and callbacks.
 *
 * @details
 * The enum prevents the adapter from exposing anonymous integer outcomes. The
 * construction API uses every code below; benchmark-core callback functions
 * map invalid configuration and unsuccessful bignum_mod_sqrt execution to the
 * corresponding benchmark_adapter_status_t expected by benchmark-core.
 */
typedef enum {
    BIGNUM_MOD_SQRT_BENCHMARK_STATUS_SUCCESS = 0, /**< The requested adapter action completed successfully. */
    BIGNUM_MOD_SQRT_BENCHMARK_STATUS_NULL_ARGUMENT = 1, /**< A required output, workload, state, or adapter pointer was NULL. */
    BIGNUM_MOD_SQRT_BENCHMARK_STATUS_INVALID_PROFILE = 2, /**< A transport field is not a valid bignum profile value. */
    BIGNUM_MOD_SQRT_BENCHMARK_STATUS_OPERATION_ERROR = 3 /**< bignum_mod_sqrt rejected the generated in-place operation. */
} bignum_mod_sqrt_benchmark_status_t;

/**
 * @brief Initialize a benchmark-core binding for the bignum_mod_sqrt operation.
 * @param[out] adapter Receives a complete benchmark-core callback binding.
 * @return A named bignum_mod_sqrt_benchmark_status_t result.
 *
 * @details
 * The algorithm validates the output pointer, clears all fields to avoid
 * partially initialized callbacks, and then installs deterministic initialize,
 * operation, and checksum callbacks. No heap allocation or global mutable
 * state is used, so the returned binding may be used independently by ST and
 * MT benchmark-core runs.
 */
bignum_mod_sqrt_benchmark_status_t bignum_mod_sqrt_benchmark_adapter_init(
    benchmark_adapter_t *adapter);

/**
 * @brief Validate all bignum transport fields accepted by benchmark-framework.
 * @param[in] workload Immutable generic workload descriptor to validate.
 * @return A named bignum_mod_sqrt_benchmark_status_t result.
 *
 * @details
 * The algorithm validates the five textual profile axes before any bignum
 * memory is initialized. `operation_kind` is restricted to the six
 * `shift-*` values, preserving the distinction between zero, bit, word,
 * combined, random, and mixed shift paths. `size_profile` is restricted to
 * bignum length values rather than the byte-transform example vocabulary.
 */
bignum_mod_sqrt_benchmark_status_t bignum_mod_sqrt_benchmark_validate_workload(
    const benchmark_workload_t *workload);

#ifdef __cplusplus
}
#endif

#endif
