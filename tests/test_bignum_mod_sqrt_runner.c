#include <assert.h>
#include <stdio.h>
#include "bignum_mod_sqrt.h"
int main(void) {
    bignum_t a = {{4}, 1}, p = {{11}, 1}, root = {{0}, 0};
    assert(bignum_mod_sqrt(&a, &p, &root) == BIGNUM_MOD_SQRT_SUCCESS);
    assert(root.words[0] == 3 || root.words[0] == 9);
    puts("bignum_mod_sqrt distribution runner: OK");
    return 0;
}
