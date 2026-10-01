#include "rng.h"

void rng_init(rng_t *r, unsigned long long seed) {
    r->state = seed + 0x9e3779b97f4a7c15ULL;

    if (r->state == 0ULL) {
        r->state = 0x9e3779b97f4a7c15ULL;
    }
}

/* splitmix64 */
static unsigned long long mix(unsigned long long x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;

    return x ^ (x >> 31);
}

void rng_init_stream(rng_t *r, unsigned long long seed,
                     unsigned long long stream) {
    r->state = mix(mix(seed) ^ (stream + 0x9e3779b97f4a7c15ULL));

    if (r->state == 0ULL) {
        r->state = 0x9e3779b97f4a7c15ULL;
    }
}

double rng_uniform(rng_t *r) {
    r->state ^= r->state >> 12;
    r->state ^= r->state << 25;
    r->state ^= r->state >> 27;

    return (double)((r->state * 2685821657736338717ULL) >> 11) /
           9007199254740992.0;
}
