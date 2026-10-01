#ifndef RNG_H
#define RNG_H

/* xorshift64* pseudorandom number generator. */

#define RNG_DEFAULT_SEED 13121989ULL

typedef struct {
    unsigned long long state;
} rng_t;

void rng_init(rng_t *r, unsigned long long seed);

/* Independent stream number `stream` for a given seed. */
void rng_init_stream(rng_t *r, unsigned long long seed,
                     unsigned long long stream);

/* Uniform on [0, 1). */
double rng_uniform(rng_t *r);

#endif /* RNG_H */
