#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bignum_mod_sqrt.h"

static bignum_t bn(uint64_t x) { bignum_t v = {{0}, 0}; if (x) { v.words[0] = x; v.len = 1; } return v; }
static int eq(const bignum_t *a, const bignum_t *b) { return a->len == b->len && memcmp(a->words, b->words, sizeof(a->words)) == 0; }
static void check_root(uint64_t a, uint64_t p, uint64_t expected) { bignum_t x=bn(a), m=bn(p), r=bn(0), e=bn(expected); assert(bignum_mod_sqrt(&x,&m,&r)==BIGNUM_MOD_SQRT_SUCCESS); assert(eq(&r,&e)); }
int main(void) {
    check_root(4,11,9); check_root(9,11,3); check_root(4,7,2); check_root(1,3,1);
    { bignum_t z=bn(0),m=bn(11),r=bn(99); assert(bignum_mod_sqrt(&z,&m,&r)==BIGNUM_MOD_SQRT_SUCCESS); assert(r.len==0); }
    { bignum_t x=bn(2),m=bn(11),r=bn(99); assert(bignum_mod_sqrt(&x,&m,&r)==BIGNUM_MOD_SQRT_ERROR_NOT_RESIDUE); assert(r.words[0]==99); }
    { bignum_t x=bn(1),m=bn(10),r=bn(99); assert(bignum_mod_sqrt(&x,&m,&r)==BIGNUM_MOD_SQRT_ERROR_MODULUS); assert(r.words[0]==99); }
    { bignum_t x=bn(1),m=bn(11),r=bn(99); assert(bignum_mod_sqrt(NULL,&m,&r)==BIGNUM_MOD_SQRT_ERROR_NULL_ARG); assert(bignum_mod_sqrt(&x,NULL,&r)==BIGNUM_MOD_SQRT_ERROR_NULL_ARG); assert(bignum_mod_sqrt(&x,&m,NULL)==BIGNUM_MOD_SQRT_ERROR_NULL_ARG); }
    puts("bignum_mod_sqrt C11 tests: OK"); return 0;
}
