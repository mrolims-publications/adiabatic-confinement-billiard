#define _POSIX_C_SOURCE 200809L

#include "util.h"
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

FILE *data_open(const char *name, char *path, size_t n) {
    if (mkdir(DATA_DIR, 0755) != 0 && errno != EEXIST) {
        return NULL;
    }
    if (snprintf(path, n, "%s/%s", DATA_DIR, name) >= (int)n) {
        return NULL;
    }

    return fopen(path, "w");
}

void progress(long done, long total) {
    const char *bar = "########################################";
    const char *pad = "                                        ";
    const int w = 40;
    const int k = (int)((double)w * done / total);

    fprintf(stderr, "\r[%.*s%.*s] %ld/%ld", k, bar, w - k, pad, done, total);
    fflush(stderr);
}

int check_geometry(const char *prog, double h0, double L, double xm) {
    if (h0 <= 0.0 || L <= 0.0 || xm <= 0.0) {
        fprintf(stderr, "%s: h0, L and xm must be positive\n", prog);
        return 0;
    }

    return 1;
}
