/**
 * @file bignum_mod_sqrt.h
 * @brief Modular square-root API for bounded bignum_t values.
 * @details Computes a root modulo a positive odd prime using a stack-only,
 *          reentrant implementation. Inputs are borrowed and never modified.
 */
#ifndef BIGNUM_MOD_SQRT_H
#define BIGNUM_MOD_SQRT_H
#include <bignum.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BIGNUM_MOD_SQRT_SUCCESS = 0,
    BIGNUM_MOD_SQRT_ERROR_NULL_ARG = -1,
    BIGNUM_MOD_SQRT_ERROR_MODULUS = -2,
    BIGNUM_MOD_SQRT_ERROR_NOT_RESIDUE = -3,
    BIGNUM_MOD_SQRT_ERROR_OVERFLOW = -4
} bignum_mod_sqrt_status_t;

/**
 * @brief Compute a modular square root of @p a modulo @p modulus.
 * @param[in] a Non-negative borrowed operand.
 * @param[in] modulus Positive odd prime modulus.
 * @param[out] root Caller-owned output receiving a value in [0, modulus).
 * @return A named bignum_mod_sqrt_status_t value.
 * @retval BIGNUM_MOD_SQRT_SUCCESS A root was written.
 * @retval BIGNUM_MOD_SQRT_ERROR_NULL_ARG A required pointer was NULL.
 * @retval BIGNUM_MOD_SQRT_ERROR_MODULUS The modulus was zero or even.
 * @retval BIGNUM_MOD_SQRT_ERROR_NOT_RESIDUE No root exists modulo modulus.
 * @retval BIGNUM_MOD_SQRT_ERROR_OVERFLOW An operand exceeds capacity.
 * @note Inputs are not modified; root is unchanged on every error path.
 */
bignum_mod_sqrt_status_t bignum_mod_sqrt(const bignum_t *a,
                                          const bignum_t *modulus,
                                          bignum_t *root);
#ifdef __cplusplus
}
#endif
#endif /* BIGNUM_MOD_SQRT_H */
