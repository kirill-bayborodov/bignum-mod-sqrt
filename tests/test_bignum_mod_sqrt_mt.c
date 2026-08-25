#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include "bignum_mod_sqrt.h"

typedef struct { int ok; } job_t;
static void *worker(void *arg) {
    job_t *job = arg;
    bignum_t a = {{4}, 1}, p = {{11}, 1}, root = {{0}, 0};
    bignum_mod_sqrt_status_t status = bignum_mod_sqrt(&a, &p, &root);
    job->ok = status == BIGNUM_MOD_SQRT_SUCCESS && (root.words[0] == 3 || root.words[0] == 9);
    return NULL;
}
int main(void) {
    pthread_t threads[4]; job_t jobs[4] = {{0}};
    for (int i=0; i<4; ++i) assert(pthread_create(&threads[i], NULL, worker, &jobs[i]) == 0);
    for (int i=0; i<4; ++i) { assert(pthread_join(threads[i], NULL) == 0); assert(jobs[i].ok); }
    puts("bignum_mod_sqrt MT tests: OK"); return 0;
}
