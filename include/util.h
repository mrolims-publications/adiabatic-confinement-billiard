#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdio.h>

#define DATA_DIR "data"

/* Open DATA_DIR/name for writing, creating DATA_DIR if needed.  The full
 * path is written to path.  Returns NULL on failure. */
FILE *data_open(const char *name, char *path, size_t n);

/* Progress bar on stderr. */
void progress(long done, long total);

/* Check h0 > 0, L > 0 and xm > 0.  Prints a message and returns 0 if any
 * of them fails. */
int check_geometry(const char *prog, double h0, double L, double xm);

#endif /* UTIL_H */
