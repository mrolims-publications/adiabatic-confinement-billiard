#include "args.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *type_name(arg_type_t t) {
    switch (t) {
    case ARG_DOUBLE:
        return "<float>";
    case ARG_LONG:
        return "<int>";
    default:
        return "<word>";
    }
}

static void print_value(FILE *f, const arg_t *a) {
    switch (a->type) {
    case ARG_DOUBLE:
        fprintf(f, a->fmt ? a->fmt : "%.5f", *(const double *)a->value);
        break;
    case ARG_LONG:
        fprintf(f, a->fmt ? a->fmt : "%ld", *(const long *)a->value);
        break;
    default:
        fprintf(f, a->fmt ? a->fmt : "%s", *(const char *const *)a->value);
        break;
    }
}

static void usage(FILE *f, const char *prog, const arg_t *spec, size_t n) {
    size_t i, w = 0;

    fprintf(f, "usage: %s", prog);
    for (i = 0; i < n; i++) {
        if (spec[i].required) {
            fprintf(f, " --%s %s", spec[i].name, type_name(spec[i].type));
        }
    }
    fprintf(f, " [options]\n");

    for (i = 0; i < n; i++) {
        const size_t len =
            strlen(spec[i].name) + strlen(type_name(spec[i].type)) + 1;

        if (len > w) {
            w = len;
        }
    }

    fprintf(f, "\nrequired:\n");
    for (i = 0; i < n; i++) {
        if (spec[i].required) {
            char lhs[64];

            snprintf(lhs, sizeof lhs, "%s %s", spec[i].name,
                     type_name(spec[i].type));
            fprintf(f, "  --%-*s  %s\n", (int)w, lhs, spec[i].help);
        }
    }

    fprintf(f, "\noptional:\n");
    for (i = 0; i < n; i++) {
        if (!spec[i].required) {
            char lhs[64];

            snprintf(lhs, sizeof lhs, "%s %s", spec[i].name,
                     type_name(spec[i].type));
            fprintf(f, "  --%-*s  %s, default = ", (int)w, lhs, spec[i].help);
            print_value(f, &spec[i]);
            fputc('\n', f);
        }
    }
}

static void die(const char *prog, const arg_t *spec, size_t n, const char *msg,
                const char *what) {
    if (what != NULL) {
        fprintf(stderr, "%s: %s: %s\n", prog, msg, what);
    } else {
        fprintf(stderr, "%s: %s\n", prog, msg);
    }
    usage(stderr, prog, spec, n);
    exit(1);
}

void args_parse(int argc, char **argv, const arg_t *spec, size_t n) {
    const char *prog = argv[0];
    int *seen;
    int i;
    size_t k;

    seen = calloc(n, sizeof *seen);
    if (seen == NULL) {
        fprintf(stderr, "%s: out of memory\n", prog);
        exit(1);
    }

    for (i = 1; i < argc; i++) {
        const char *flag = argv[i];
        char *end;

        if (strcmp(flag, "--help") == 0 || strcmp(flag, "-h") == 0) {
            usage(stdout, prog, spec, n);
            free(seen);
            exit(0);
        }
        if (strncmp(flag, "--", 2) != 0) {
            die(prog, spec, n, "expected a flag of the form --name", flag);
        }
        flag += 2;

        for (k = 0; k < n; k++) {
            if (strcmp(flag, spec[k].name) == 0) {
                break;
            }
        }
        if (k == n) {
            die(prog, spec, n, "unknown argument", flag);
        }
        if (i + 1 >= argc) {
            die(prog, spec, n, "missing value for", flag);
        }
        i++;

        errno = 0;
        switch (spec[k].type) {
        case ARG_DOUBLE:
            *(double *)spec[k].value = strtod(argv[i], &end);
            if (end == argv[i] || *end != '\0' || errno == ERANGE) {
                die(prog, spec, n, "value is not a number for", flag);
            }
            break;
        case ARG_LONG:
            *(long *)spec[k].value = strtol(argv[i], &end, 10);
            if (end == argv[i] || *end != '\0' || errno == ERANGE) {
                die(prog, spec, n, "value is not an integer for", flag);
            }
            break;
        default:
            *(const char **)spec[k].value = argv[i];
            break;
        }
        seen[k] = 1;
    }

    for (k = 0; k < n; k++) {
        if (spec[k].required && !seen[k]) {
            die(prog, spec, n, "missing required argument", spec[k].name);
        }
    }

    free(seen);
}

void args_echo(const arg_t *spec, size_t n) {
    size_t i;

    for (i = 0; i < n; i++) {
        printf("%s = ", spec[i].name);
        print_value(stdout, &spec[i]);
        putchar('\n');
    }
}
