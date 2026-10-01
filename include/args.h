#ifndef ARGS_H
#define ARGS_H

#include <stddef.h>

/*
 * Command-line arguments of the form --name value, in any order.  Optional
 * arguments keep the value their variable holds when args_parse is called.
 * A missing or unknown argument, or --help, prints the usage and exits.
 */

typedef enum { ARG_DOUBLE, ARG_LONG, ARG_STRING } arg_type_t;

typedef struct {
    const char *name; /* flag, without the leading --       */
    arg_type_t type;
    int required;     /* 1 required, 0 optional              */
    void *value;      /* double *, long * or const char **   */
    const char *fmt;  /* printf format, NULL for the default */
    const char *help; /* one-line description                */
} arg_t;

#define ARGS_N(spec) (sizeof(spec) / sizeof((spec)[0]))

void args_parse(int argc, char **argv, const arg_t *spec, size_t n);

/* Print "name = value", one per line. */
void args_echo(const arg_t *spec, size_t n);

#endif /* ARGS_H */
