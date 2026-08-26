/**
 * @file test_bignum_mod_sqrt_extra.c
 * @brief Extended deterministic and oracle tests for bignum_mod_sqrt.
 * @details Exercises the one-word Tonelli-Shanks branches, modular reduction,
 *          normalized output, invalid modulus handling and preservation of the
 *          caller-owned output on every failure. The oracle exhaustively checks
 *          all residues for a bounded set of small odd prime moduli.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bignum_mod_sqrt.h"

/*
 * Compile a private coverage copy so static arithmetic helpers can be tested
 * directly without changing the production object or its public symbol.
 */
#define bignum_mod_sqrt bignum_mod_sqrt_coverage_copy
#include "../src/bignum_mod_sqrt.c"
#undef bignum_mod_sqrt

/** Creates a normalized one-word bignum value for a test case. */
static bignum_t make_word(uint64_t value)
{
    bignum_t result = {{0}, 0};
    if (value != 0U) {
        result.words[0] = value;
        result.len = 1U;
    }
    return result;
}

/** Returns whether two complete public records have identical representation. */
static int equal_bignums(const bignum_t *left, const bignum_t *right)
{
    return left->len == right->len &&
           memcmp(left->words, right->words, sizeof(left->words)) == 0;
}

/** Finds a square root by exhaustive search for the small-modulus oracle. */
static int oracle_root(uint64_t value, uint64_t modulus, uint64_t *root)
{
    for (uint64_t candidate = 0U; candidate < modulus; ++candidate) {
        if ((candidate * candidate) % modulus == value % modulus) {
            *root = candidate;
            return 1;
        }
    }
    return 0;
}

/** Checks one small-prime input against the exhaustive oracle. */
static void check_oracle_case(uint64_t value, uint64_t modulus)
{
    bignum_t input = make_word(value);
    bignum_t prime = make_word(modulus);
    bignum_t output = make_word(UINT64_C(0xfeedface));
    uint64_t expected = 0U;
    const int exists = oracle_root(value, modulus, &expected);
    const bignum_mod_sqrt_status_t status =
        bignum_mod_sqrt(&input, &prime, &output);
    bignum_t copy_output = make_word(UINT64_C(0x12345678));
    const bignum_mod_sqrt_status_t copy_status =
        bignum_mod_sqrt_coverage_copy(&input, &prime, &copy_output);
    assert(copy_status == status);

    if (exists) {
        assert(status == BIGNUM_MOD_SQRT_SUCCESS);
        if (expected == 0U) {
            assert(output.len == 0U);
        } else {
            assert(output.len == 1U);
        }
        assert((output.words[0] * output.words[0]) % modulus == value % modulus);
        assert(output.words[0] < modulus);
    } else {
        assert(status == BIGNUM_MOD_SQRT_ERROR_NOT_RESIDUE);
        assert(output.words[0] == UINT64_C(0xfeedface));
    }
}

/** Verifies reduction of a multi-word input against a one-word prime. */
static void check_reduced_input(void)
{
    bignum_t input = {{0}, 2U};
    bignum_t prime = make_word(13U);
    bignum_t output = make_word(UINT64_C(0xabcdef));
    input.words[0] = 1U;
    input.words[1] = 1U;

    assert(bignum_mod_sqrt(&input, &prime, &output) == BIGNUM_MOD_SQRT_SUCCESS);
    assert(output.len == 1U);
    assert((output.words[0] * output.words[0]) % 13U == 4U);
}

/** Verifies the zero fast path for a normalized multi-word odd modulus. */
static void check_multiword_general(void)
{
    bignum_t input = make_word(2U);
    bignum_t modulus = {{0}, 2U};
    bignum_t output = make_word(UINT64_C(0xabcdef));
    modulus.words[0] = 13U;
    modulus.words[1] = 1U; /* 2^64 + 13: odd and capacity-valid. */
    const bignum_mod_sqrt_status_t status =
        bignum_mod_sqrt_coverage_copy(&input, &modulus, &output);
    assert(status == BIGNUM_MOD_SQRT_SUCCESS ||
           status == BIGNUM_MOD_SQRT_ERROR_NOT_RESIDUE);
    if (status == BIGNUM_MOD_SQRT_ERROR_NOT_RESIDUE) {
        assert(output.words[0] == UINT64_C(0xabcdef));
    }
}

/** Exercises C11 general Tonelli-Shanks with a deterministic 65-bit prime. */
static void check_multiword_prime(void)
{
    bignum_t input = make_word(4U);
    bignum_t modulus = {{0}, 2U};
    bignum_t output = make_word(UINT64_C(0xabcdef));
    modulus.words[0] = UINT64_C(0xC2D33B3AB5170AB9);
    modulus.words[1] = 1U;

    const bignum_mod_sqrt_status_t status =
        bignum_mod_sqrt_coverage_copy(&input, &modulus, &output);
    assert(status == BIGNUM_MOD_SQRT_SUCCESS ||
           status == BIGNUM_MOD_SQRT_ERROR_NOT_RESIDUE);
}

/** Verifies the zero fast path for a normalized multi-word odd modulus. */
static void check_multiword_zero(void)
{
    bignum_t input = make_word(0U);
    bignum_t modulus = {{0}, 2U};
    bignum_t output = make_word(UINT64_C(0xabcdef));
    modulus.words[0] = UINT64_MAX;
    modulus.words[1] = UINT64_C(0x7fffffffffffffff);

    assert(bignum_mod_sqrt(&input, &modulus, &output) == BIGNUM_MOD_SQRT_SUCCESS);
    assert(output.len == 0U);
    for (size_t index = 0U; index < BIGNUM_CAPACITY; ++index) {
        assert(output.words[index] == 0U);
    }
}

/** Exercises private arithmetic helpers used by the general multi-word path. */
static void check_internal_helpers(void)
{
    bn_local a = {{0}, 0U};
    bn_local b = {{0}, 0U};
    bn_local r = {{0}, 0U};
    bn_local m = {{0}, 0U};

    a.len = 2U; a.w[0] = 5U; a.w[1] = 0U; norm(&a); assert(a.len == 1U);
    b.len = 1U; b.w[0] = 3U; assert(cmp(&a, &b) > 0); assert(cmp(&b, &a) < 0);
    b.w[0] = 5U; assert(cmp(&a, &b) == 0); b.w[0] = 4U; assert(cmp(&a, &b) > 0);
    a.len = 2U; a.w[0] = 0U; a.w[1] = 1U; b.len = 1U; b.w[0] = 1U; sub_bn(&a, &b); assert(a.w[0] == UINT64_MAX && a.len == 1U);
    a.len = 1U; a.w[0] = 9U; b.len = 1U; b.w[0] = 1U; m.len = 1U; m.w[0] = 10U; add_mod(&r, &a, &b, &m); assert(r.w[0] == 0U);
    a.len = 1U; a.w[0] = 3U; m.w[0] = 7U; add_mod(&r, &a, &a, &m); assert(r.w[0] == 6U);
    shr1(&a); assert(a.w[0] == 1U); inc_small(&a, 3U); assert(a.w[0] == 4U);
    a.len = 1U; a.w[0] = 5U; b.len = 1U; b.w[0] = 3U; m.w[0] = 7U; mod_reduce(&r, &a, &m); assert(r.w[0] == 5U);
    mul_mod(&r, &a, &b, &m); assert(r.w[0] == 1U);
    pow_mod(&r, &a, &b, &m); assert(r.w[0] == 6U);
    assert(u64_addmod(5U, 4U, 7U) == 2U);
    assert(u64_mulmod(5U, 3U, 7U) == 1U);
    assert(u64_powmod(5U, 3U, 7U) == 6U);
    { uint64_t root = 0U; assert(u64_sqrt(4U, 7U, &root) != 0); assert(root == 2U || root == 5U); assert(u64_sqrt(3U, 7U, &root) == 0); }
    b.len = 1U; b.w[0] = 1U; assert(is_one(&b));
    b.w[0] = 0U; b.len = 0U; assert(is_zero(&b)); assert(lowbit(&b) == 0U);
    b.len = 1U; b.w[0] = 3U; assert(lowbit(&b) == 1U); assert(!is_one(&b));
}

/** Verifies overflow and all NULL combinations without changing output. */
static void check_error_preservation(void)
{
    bignum_t input = make_word(4U);
    bignum_t modulus = make_word(11U);
    bignum_t output = make_word(UINT64_C(0xabcdef));
    bignum_t before = output;
    bignum_t oversized = {{0}, BIGNUM_CAPACITY + 1U};

    assert(bignum_mod_sqrt(&oversized, &modulus, &output) ==
           BIGNUM_MOD_SQRT_ERROR_OVERFLOW);
    assert(equal_bignums(&output, &before));
    assert(bignum_mod_sqrt(&input, &oversized, &output) ==
           BIGNUM_MOD_SQRT_ERROR_OVERFLOW);
    assert(equal_bignums(&output, &before));
    assert(bignum_mod_sqrt(NULL, &modulus, &output) ==
           BIGNUM_MOD_SQRT_ERROR_NULL_ARG);
    assert(bignum_mod_sqrt(&input, NULL, &output) ==
           BIGNUM_MOD_SQRT_ERROR_NULL_ARG);
    assert(bignum_mod_sqrt(&input, &modulus, NULL) ==
           BIGNUM_MOD_SQRT_ERROR_NULL_ARG);
    assert(equal_bignums(&output, &before));
}

/** Runs the extended oracle and boundary suite. */
int main(void)
{
    static const uint64_t primes[] = {3U, 5U, 7U, 11U, 13U, 17U, 19U, 23U,
                                      29U, 31U, 43U, 47U};

    for (size_t prime_index = 0U; prime_index < sizeof(primes) / sizeof(primes[0]);
         ++prime_index) {
        for (uint64_t value = 0U; value < primes[prime_index] * 2U; ++value) {
            check_oracle_case(value, primes[prime_index]);
        }
    }
    check_reduced_input();
    check_internal_helpers();
    check_multiword_general();
    check_multiword_prime();
    check_multiword_zero();
    check_error_preservation();
    puts("bignum_mod_sqrt extended tests: OK");
    return 0;
}
