/**
 * @file bignum_mod_sqrt_benchmark_adapter.c
 * @brief Deterministic modular-square-root adapter for benchmark-framework.
 */
#include "bignum_mod_sqrt_benchmark_adapter.h"
#include "bignum_mod_sqrt.h"
#include <bignum.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define FNV_OFFSET UINT64_C(1469598103934665603)
#define FNV_PRIME  UINT64_C(1099511628211)
#define MODULUS_VALUE UINT64_C(11)

typedef struct {
    bignum_t input;
    bignum_t modulus;
    bignum_t root;
} bignum_mod_sqrt_benchmark_state_t;

/** Returns the next deterministic non-zero xorshift value. */
static uint64_t next_value(uint64_t *state)
{
    if (*state == 0U) {
        *state = UINT64_C(0x9E3779B97F4A7C15);
    }
    *state ^= *state << 7U;
    *state ^= *state >> 9U;
    *state ^= *state << 8U;
    return *state;
}

/** Returns true when value is in the supplied NULL-terminated vocabulary. */
static int allowed(const char *value, const char *const *values)
{
    if (value == NULL || values == NULL) {
        return 0;
    }
    for (size_t index = 0U; values[index] != NULL; ++index) {
        if (strcmp(value, values[index]) == 0) {
            return 1;
        }
    }
    return 0;
}

/** Selects the logical input length from a documented size profile. */
static size_t choose_length(const char *profile, uint64_t *state)
{
    if (strcmp(profile, "one") == 0) {
        return 1U;
    }
    if (strcmp(profile, "quarter") == 0) {
        return BIGNUM_CAPACITY / 4U == 0U ? 1U : BIGNUM_CAPACITY / 4U;
    }
    if (strcmp(profile, "half") == 0) {
        return BIGNUM_CAPACITY / 2U == 0U ? 1U : BIGNUM_CAPACITY / 2U;
    }
    if (strcmp(profile, "near-capacity") == 0) {
        return BIGNUM_CAPACITY;
    }
    return 1U + (size_t)(next_value(state) %
        (BIGNUM_CAPACITY / 2U == 0U ? 1U : BIGNUM_CAPACITY / 2U));
}

/** Initializes one normalized input/modulus/root benchmark state. */
static benchmark_adapter_status_t initialize_state(
    void *opaque_state,
    uint64_t sequence_index,
    const benchmark_workload_t *workload,
    void *adapter_context)
{
    bignum_mod_sqrt_benchmark_state_t *state = opaque_state;
    uint64_t random_state;
    size_t length;
    int zero_input;

    (void)adapter_context;
    if (state == NULL || workload == NULL ||
        bignum_mod_sqrt_benchmark_validate_workload(workload) !=
            BIGNUM_MOD_SQRT_BENCHMARK_STATUS_SUCCESS) {
        return BENCHMARK_ADAPTER_STATUS_INPUT_ERROR;
    }
    memset(state, 0, sizeof(*state));
    state->modulus.words[0] = MODULUS_VALUE;
    state->modulus.len = 1U;
    zero_input = strcmp(workload->input_kind, "zero") == 0 ||
        (strcmp(workload->input_kind, "mixed") == 0 && (sequence_index & 1U) == 0U);
    if (zero_input || strcmp(workload->operation_kind, "root-zero") == 0) {
        return BENCHMARK_ADAPTER_STATUS_SUCCESS;
    }

    random_state = workload->seed ^ (sequence_index + UINT64_C(0x9E3779B97F4A7C15));
    length = choose_length(workload->size_profile, &random_state);
    if (length == 0U || length > BIGNUM_CAPACITY) {
        return BENCHMARK_ADAPTER_STATUS_INPUT_ERROR;
    }
    state->input.len = length;
    for (size_t index = 0U; index < length; ++index) {
        state->input.words[index] = next_value(&random_state);
    }
    state->input.words[length - 1U] |= UINT64_C(1);
    if (strcmp(workload->operation_kind, "root-residue") == 0) {
        state->input.len = 1U;
        state->input.words[0] = 4U;
    } else {
        /* Choose a square target and compensate the low word modulo 11. */
        const uint64_t target_root = 2U + (sequence_index % 3U);
        uint64_t contribution = 0U;
        uint64_t factor = 1U;
        for (size_t index = 1U; index < length; ++index) {
            factor = (factor * 5U) % MODULUS_VALUE; /* 2^64 mod 11 == 5. */
            contribution = (contribution +
                (state->input.words[index] % MODULUS_VALUE) * factor) % MODULUS_VALUE;
        }
        state->input.words[0] = ((target_root * target_root) % MODULUS_VALUE +
            MODULUS_VALUE - contribution) % MODULUS_VALUE;
    }
    return BENCHMARK_ADAPTER_STATUS_SUCCESS;
}

/** Executes one modular-square-root operation on an independent state copy. */
static benchmark_adapter_status_t operate_state(
    void *opaque_state,
    uint64_t iteration,
    const benchmark_workload_t *workload,
    void *adapter_context)
{
    bignum_mod_sqrt_benchmark_state_t *state = opaque_state;
    bignum_mod_sqrt_status_t status;

    (void)iteration;
    (void)adapter_context;
    if (state == NULL || workload == NULL) {
        return BENCHMARK_ADAPTER_STATUS_INPUT_ERROR;
    }
    if (strcmp(workload->operation_kind, "root-zero") == 0) {
        state->input.len = 0U;
        memset(state->input.words, 0, sizeof(state->input.words));
    }
    memset(&state->root, 0, sizeof(state->root));
    status = bignum_mod_sqrt(&state->input, &state->modulus, &state->root);
    if (status != BIGNUM_MOD_SQRT_SUCCESS) {
        return BENCHMARK_ADAPTER_STATUS_OPERATION_ERROR;
    }
    return BENCHMARK_ADAPTER_STATUS_SUCCESS;
}

/** Hashes the complete post-operation state and iteration observably. */
static uint64_t checksum_state(const void *opaque_state, uint64_t iteration, void *adapter_context)
{
    const bignum_mod_sqrt_benchmark_state_t *state = opaque_state;
    const uint64_t *words;
    uint64_t checksum = FNV_OFFSET;

    (void)adapter_context;
    if (state == NULL) {
        return 0U;
    }
    words = state->root.words;
    for (size_t index = 0U; index < BIGNUM_CAPACITY; ++index) {
        checksum ^= words[index];
        checksum *= FNV_PRIME;
    }
    checksum ^= (uint64_t)state->root.len;
    checksum *= FNV_PRIME;
    checksum ^= iteration;
    return checksum * FNV_PRIME;
}

bignum_mod_sqrt_benchmark_status_t bignum_mod_sqrt_benchmark_validate_workload(
    const benchmark_workload_t *workload)
{
    static const char *const inputs[] = {"zero", "nonzero", "mixed", NULL};
    static const char *const operations[] = {
        "root-zero", "root-residue", "root-random", "root-mixed", NULL
    };
    static const char *const measures[] = {"end-to-end", "kernel-only", NULL};
    static const char *const sizes[] = {
        "one", "quarter", "half", "variable", "near-capacity", NULL
    };
    static const char *const capacities[] = {"normal", "near-capacity", NULL};

    if (workload == NULL || workload->input_kind == NULL ||
        workload->operation_kind == NULL || workload->measure_mode == NULL ||
        workload->size_profile == NULL || workload->capacity_profile == NULL) {
        return BIGNUM_MOD_SQRT_BENCHMARK_STATUS_NULL_ARGUMENT;
    }
    if (!allowed(workload->input_kind, inputs) ||
        !allowed(workload->operation_kind, operations) ||
        !allowed(workload->measure_mode, measures) ||
        !allowed(workload->size_profile, sizes) ||
        !allowed(workload->capacity_profile, capacities)) {
        return BIGNUM_MOD_SQRT_BENCHMARK_STATUS_INVALID_PROFILE;
    }
    return BIGNUM_MOD_SQRT_BENCHMARK_STATUS_SUCCESS;
}

bignum_mod_sqrt_benchmark_status_t bignum_mod_sqrt_benchmark_adapter_init(
    benchmark_adapter_t *adapter)
{
    if (adapter == NULL) {
        return BIGNUM_MOD_SQRT_BENCHMARK_STATUS_NULL_ARGUMENT;
    }
    *adapter = (benchmark_adapter_t){
        .benchmark_name = "bignum_mod_sqrt",
        .state_size = sizeof(bignum_mod_sqrt_benchmark_state_t),
        .success_code = BENCHMARK_ADAPTER_STATUS_SUCCESS,
        .adapter_context = NULL,
        .initialize = initialize_state,
        .operation = operate_state,
        .checksum = checksum_state
    };
    return BIGNUM_MOD_SQRT_BENCHMARK_STATUS_SUCCESS;
}
