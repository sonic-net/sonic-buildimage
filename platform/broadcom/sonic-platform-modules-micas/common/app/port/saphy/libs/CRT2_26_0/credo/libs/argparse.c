/**
 * Copyright (C) 2012-2015 Yecheng Fu <cofyc.jackson at gmail dot com>
 * All rights reserved.
 *
 * Use of this source code is governed by a MIT-style license that can be found
 * in the LICENSE file.
 */
#include "argparse.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPT_UNSET 1
#define OPT_LONG  (1 << 1)

static const char *prefix_skip(const char *str, const char *prefix) {
    size_t len = strlen(prefix);
    return strncmp(str, prefix, len) ? NULL : str + len;
}

static int prefix_cmp(const char *str, const char *prefix) {
    for (;; str++, prefix++)
        if (!*prefix) {
            return 0;
        } else if (*str != *prefix) {
            return (unsigned char)*prefix - (unsigned char)*str;
        }
}

static int argparse_error(struct argparse *self, const struct argparse_option *opt, const char *reason, int flags) {
    (void)self;
    if (flags & OPT_LONG) {
        ARGP_PRINTF(self, "error: option `--%s` %s\n", opt->long_name, reason);
    } else {
        ARGP_PRINTF(self, "error: option `-%c` %s\n", opt->short_name, reason);
    }
    return -3;
}

static int argparse_getvalue(struct argparse *self, const struct argparse_option *opt, int flags) {
    const char *s = NULL;
    if (!opt->value) goto skipped;
    switch (opt->type) {
        case ARGPARSE_OPT_BOOLEAN:
            if (flags & OPT_UNSET) {
                *(int *)opt->value = *(int *)opt->value - 1;
            } else {
                *(int *)opt->value = *(int *)opt->value + 1;
            }
            if (*(int *)opt->value < 0) {
                *(int *)opt->value = 0;
            }
            break;
        case ARGPARSE_OPT_BIT:
            if (flags & OPT_UNSET) {
                *(int *)opt->value &= ~(int)opt->data;
            } else {
                *(int *)opt->value |= (int)opt->data;
            }
            break;
        case ARGPARSE_OPT_STRING:
            if (self->optvalue) {
                *(const char **)opt->value = self->optvalue;
                self->optvalue = NULL;
            } else if (self->argc > 1) {
                self->argc--;
                *(const char **)opt->value = *++self->argv;
            } else {
                return argparse_error(self, opt, "requires a value", flags);
            }
            break;
        case ARGPARSE_OPT_INTEGER:
            errno = 0;
            if (self->optvalue) {
                *(int *)opt->value = (int)strtol(self->optvalue, (char **)&s, 0);
                self->optvalue = NULL;
            } else if (self->argc > 1) {
                self->argc--;
                *(int *)opt->value = (int)strtol(*++self->argv, (char **)&s, 0);
            } else {
                return argparse_error(self, opt, "requires a value", flags);
            }
            if (errno == ERANGE) return argparse_error(self, opt, "numerical result out of range", flags);
            if (s[0] != '\0') {  // no digits or contains invalid characters
                return argparse_error(self, opt, "expects an integer value", flags);
            }
            break;
        case ARGPARSE_OPT_UINTEGER:
            errno = 0;

            if (self->optvalue) {
                if (self->optvalue[0] == '-') {
                    return argparse_error(self, opt, "requires a non-negative value", flags);
                }
                *(unsigned *)opt->value = (unsigned)strtoul(self->optvalue, (char **)&s, 0);
                self->optvalue = NULL;
            } else if (self->argc > 1) {
                self->argc--;
                const char *value = *++self->argv;
                if (value == NULL || value[0] == '-') {
                    return argparse_error(self, opt, "requires a non-negative value", flags);
                }
                *(unsigned *)opt->value = (unsigned)strtoul(value, (char **)&s, 0);
            } else {
                return argparse_error(self, opt, "requires a value", flags);
            }
            if (errno == ERANGE) return argparse_error(self, opt, "numerical result out of range", flags);
            if (s[0] != '\0') {  // no digits or contains invalid characters
                return argparse_error(self, opt, "expects an unsigned integer value", flags);
            }
            break;
        case ARGPARSE_OPT_FLOAT:
            errno = 0;
            if (self->optvalue) {
                *(float *)opt->value = strtof(self->optvalue, (char **)&s);
                self->optvalue = NULL;
            } else if (self->argc > 1) {
                self->argc--;
                *(float *)opt->value = strtof(*++self->argv, (char **)&s);
            } else {
                return argparse_error(self, opt, "requires a value", flags);
            }
            if (errno == ERANGE) {
                return argparse_error(self, opt, "numerical result out of range", flags);
            };
            if (s[0] != '\0') {  // no digits or contains invalid characters
                return argparse_error(self, opt, "expects a numerical value", flags);
            }
            break;
        default:
            assert(0);
    }

skipped:
    if (opt->callback) {
        return opt->callback(self, opt);
    }
    return 0;
}

static void argparse_options_check(const struct argparse_option *options) {
    for (; options->type != ARGPARSE_OPT_END; options++) {
        switch (options->type) {
            case ARGPARSE_OPT_END:
            case ARGPARSE_OPT_BOOLEAN:
            case ARGPARSE_OPT_BIT:
            case ARGPARSE_OPT_INTEGER:
            case ARGPARSE_OPT_FLOAT:
            case ARGPARSE_OPT_STRING:
            case ARGPARSE_OPT_UINTEGER:
            case ARGPARSE_OPT_GROUP:
                continue;
            default:
                fprintf(stderr, "wrong option type: %d", options->type);
                break;
        }
    }
}

static int argparse_short_opt(struct argparse *self, const struct argparse_option *options) {
    for (; options->type != ARGPARSE_OPT_END; options++) {
        if (options->short_name == *self->optvalue) {
            self->optvalue = self->optvalue[1] ? self->optvalue + 1 : NULL;
            return argparse_getvalue(self, options, 0);
        }
    }
    return -2;
}

static int argparse_long_opt(struct argparse *self, const struct argparse_option *options) {
    for (; options->type != ARGPARSE_OPT_END; options++) {
        const char *rest;
        int opt_flags = 0;
        if (!options->long_name) continue;

        rest = prefix_skip(self->argv[0] + 2, options->long_name);
        if (!rest) {
            // negation disabled?
            if (options->flags & OPT_NONEG) {
                continue;
            }
            // only OPT_BOOLEAN/OPT_BIT supports negation
            if (options->type != ARGPARSE_OPT_BOOLEAN && options->type != ARGPARSE_OPT_BIT) {
                continue;
            }

            if (prefix_cmp(self->argv[0] + 2, "no-")) {
                continue;
            }
            rest = prefix_skip(self->argv[0] + 2 + 3, options->long_name);
            if (!rest) continue;
            opt_flags |= OPT_UNSET;
        }
        if (*rest) {
            if (*rest != '=') continue;
            self->optvalue = rest + 1;
        }
        return argparse_getvalue(self, options, opt_flags | OPT_LONG);
    }
    return -2;
}

int argparse_init(struct argparse *self, struct argparse_option *options, const char *const *usages, int flags,
                  argparse_writer writer, void *ud) {
    memset(self, 0, sizeof(*self));
    self->options = options;
    self->usages = usages;
    self->flags = flags;
    self->description = NULL;
    self->epilog = NULL;
    self->writer = writer;
    self->ud = ud;
    return 0;
}

void argparse_describe(struct argparse *self, const char *description, const char *epilog) {
    self->description = description;
    self->epilog = epilog;
}

#define is_digit(c) (c) >= '0' && (c) <= '9'
int argparse_parse(struct argparse *self, int argc, const char **argv) {
    self->argc = argc - 1;
    self->argv = argv + 1;
    self->out = argv;

    argparse_options_check(self->options);

    for (; self->argc; self->argc--, self->argv++) {
        const char *arg = self->argv[0];
        // negative numbers are digits, not flags
        if ((arg[0] != '-' || !arg[1]) || (arg[0] == '-' && is_digit(arg[1]))) {
            if (self->flags & ARGPARSE_STOP_AT_NON_OPTION) {
                goto end;
            }
            // if it's not option or is a single char '-', copy verbatim
            self->out[self->cpidx++] = self->argv[0];
            continue;
        }
        // short option
        if (arg[1] != '-') {
            self->optvalue = arg + 1;
            switch (argparse_short_opt(self, self->options)) {
                case 1:
                    goto exit_early;
                case -1:
                    break;
                case -2:
                    goto unknown;
                case -3:
                    goto error_data;
            }
            while (self->optvalue) {
                switch (argparse_short_opt(self, self->options)) {
                    case 1:
                        goto exit_early;
                    case -1:
                        break;
                    case -2:
                        goto unknown;
                    case -3:
                        goto error_data;
                }
            }
            continue;
        }
        // if '--' presents
        if (!arg[2]) {
            self->argc--;
            self->argv++;
            break;
        }
        // long option
        switch (argparse_long_opt(self, self->options)) {
            case 1:
                goto exit_early;
            case -1:
                break;
            case -2:
                goto unknown;
            case -3:
                goto error_data;
        }
        continue;

    error_data:
        return ARGPARSE_EXIT_ERROR;
    exit_early:
        return ARGPARSE_EXIT_SKIP;

    unknown:
        ARGP_PRINTF(self, "error: unknown option `%s`\n", self->argv[0]);
        argparse_usage(self);
        if (!(self->flags & ARGPARSE_IGNORE_UNKNOWN_ARGS)) {
            return ARGPARSE_EXIT_ERROR;
        }
    }

end:
    memmove(self->out + self->cpidx, self->argv, self->argc * sizeof(*self->out));
    self->out[self->cpidx + self->argc] = NULL;

    return self->cpidx + self->argc;
}

void argparse_usage(struct argparse *self) {
    if (self->usages) {
        ARGP_PRINTF(self, "Usage: %s\n", *self->usages++);
        while (*self->usages && **self->usages) ARGP_PRINTF(self, "   or: %s\n", *self->usages++);
    } else {
        ARGP_PRINTF(self, "Usage:\n");
    }

    // print description
    if (self->description) ARGP_PRINTF(self, "%s\n", self->description);

    ARGP_PRINTF(self, "\n");

    const struct argparse_option *options;

    // figure out best width
    size_t usage_opts_width = 0;
    size_t len;
    options = self->options;
    for (; options->type != ARGPARSE_OPT_END; options++) {
        len = 0;
        if ((options)->short_name) {
            len += 2;
        }
        if ((options)->short_name && (options)->long_name) {
            len += 2;  // separator ", "
        }
        if ((options)->long_name) {
            len += strlen((options)->long_name) + 2;
        }
        if (options->type == ARGPARSE_OPT_INTEGER) {
            len += strlen("=<int>");
        }
        if (options->type == ARGPARSE_OPT_FLOAT) {
            len += strlen("=<flt>");
        } else if (options->type == ARGPARSE_OPT_STRING) {
            len += strlen("=<str>");
        }
        len = (len + 3) - ((len + 3) & 3);
        if (usage_opts_width < len) {
            usage_opts_width = len;
        }
    }
    usage_opts_width += 4;  // 4 spaces prefix

    options = self->options;
    for (; options->type != ARGPARSE_OPT_END; options++) {
        size_t pos = 0;
        size_t pad = 0;
        if (options->type == ARGPARSE_OPT_GROUP) {
            ARGP_PRINTF(self, "\n");
            ARGP_PRINTF(self, options->help);
            ARGP_PRINTF(self, "\n");
            continue;
        }
        pos = snprintf(NULL, 0, "    ");
        ARGP_PRINTF(self, "    ");
        if (options->short_name) {
            pos += snprintf(NULL, 0, "-%c", options->short_name);
            ARGP_PRINTF(self, "-%c", options->short_name);
        }
        if (options->long_name && options->short_name) {
            pos += snprintf(NULL, 0, ", ");
            ARGP_PRINTF(self, ", ");
        }
        if (options->long_name) {
            pos += snprintf(NULL, 0, "--%s", options->long_name);
            ARGP_PRINTF(self, "--%s", options->long_name);
        }
        if (options->type == ARGPARSE_OPT_INTEGER) {
            pos += snprintf(NULL, 0, "=<int>");
            ARGP_PRINTF(self, "=<int>");
        } else if (options->type == ARGPARSE_OPT_FLOAT) {
            pos += snprintf(NULL, 0, "=<flt>");
            ARGP_PRINTF(self, "=<flt>");
        } else if (options->type == ARGPARSE_OPT_STRING) {
            pos += snprintf(NULL, 0, "=<str>");
            ARGP_PRINTF(self, "=<str>");
        }
        if (pos <= usage_opts_width) {
            pad = usage_opts_width - pos;
        } else {
            ARGP_PRINTF(self, "\n");
            pad = usage_opts_width;
        }
        ARGP_PRINTF(self, "%*s%s\n", (int)pad + 2, "", options->help);
    }

    // print epilog
    if (self->epilog) ARGP_PRINTF(self, "%s\n", self->epilog);
}

int argparse_help_cb_no_exit(struct argparse *self, const struct argparse_option *option) {
    (void)option;
    argparse_usage(self);
    return 1;
}
