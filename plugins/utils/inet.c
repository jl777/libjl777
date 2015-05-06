/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#ifdef DEFINES_ONLY
#ifndef crypto777_inet_h
#define crypto777_inet_h
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <netdb.h>

#endif
#else

#ifndef crypto777_system777_c
#define crypto777_system777_c

#ifndef crypto777_system777_h
#define DEFINES_ONLY
#include "inet.c"
#undef DEFINES_ONLY
#endif

static int inet_ntop4(unsigned char *src, char *dst, size_t size);
static int inet_ntop6(unsigned char *src, char *dst, size_t size);
static int inet_pton4(char *src, unsigned char *dst);
static int inet_pton6(char *src, unsigned char *dst);

int32_t portable_ntop(int af, void* src, char* dst, size_t size)
{
    switch (af) {
        case AF_INET:
            return (inet_ntop4(src, dst, size));
        case AF_INET6:
            return (inet_ntop6(src, dst, size));
        default:
            return -1;
    }
    /* NOTREACHED */
}


static int inet_ntop4(unsigned char *src, char *dst, size_t size) {
    static const char fmt[] = "%u.%u.%u.%u";
    char tmp[sizeof "255.255.255.255"];
    int l;
    
#ifndef _WIN32
    l = snprintf(tmp, sizeof(tmp), fmt, src[0], src[1], src[2], src[3]);
#else
    l = _snprintf(tmp, sizeof(tmp), fmt, src[0], src[1], src[2], src[3]);
#endif
    if (l <= 0 || (size_t) l >= size) {
        return -1;
    }
    strncpy(dst, tmp, size);
    dst[size - 1] = '\0';
    return 0;
}


static int inet_ntop6(unsigned char *src, char *dst, size_t size) {
    /*
     * Note that int32_t and int16_t need only be "at least" large enough
     * to contain a value of the specified size.  On some systems, like
     * Crays, there is no such thing as an integer variable with 16 bits.
     * Keep this in mind if you think this function should have been coded
     * to use pointer overlays.  All the world's not a VAX.
     */
    char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
    struct { int base, len; } best, cur;
    unsigned int words[sizeof(struct in6_addr) / sizeof(uint16_t)];
    int i;
    
    /*
     * Preprocess:
     *  Copy the input (bytewise) array into a wordwise array.
     *  Find the longest run of 0x00's in src[] for :: shorthanding.
     */
    memset(words, '\0', sizeof words);
    for (i = 0; i < (int) sizeof(struct in6_addr); i++)
        words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
    best.base = -1;
    best.len = 0;
    cur.base = -1;
    cur.len = 0;
    for (i = 0; i < (int)(sizeof(struct in6_addr) / sizeof(uint16_t)); i++) {
        if (words[i] == 0) {
            if (cur.base == -1)
                cur.base = i, cur.len = 1;
            else
                cur.len++;
        } else {
            if (cur.base != -1) {
                if (best.base == -1 || cur.len > best.len)
                    best = cur;
                cur.base = -1;
            }
        }
    }
    if (cur.base != -1) {
        if (best.base == -1 || cur.len > best.len)
            best = cur;
    }
    if (best.base != -1 && best.len < 2)
        best.base = -1;
    
    /*
     * Format the result.
     */
    tp = tmp;
    for (i = 0; i < (int)(sizeof(struct in6_addr) / sizeof(uint16_t)); i++) {
        /* Are we inside the best run of 0x00's? */
        if (best.base != -1 && i >= best.base &&
            i < (best.base + best.len)) {
            if (i == best.base)
                *tp++ = ':';
            continue;
        }
        /* Are we following an initial run of 0x00s or any real hex? */
        if (i != 0)
            *tp++ = ':';
        /* Is this address an encapsulated IPv4? */
        if (i == 6 && best.base == 0 && (best.len == 6 ||
                                         (best.len == 7 && words[7] != 0x0001) ||
                                         (best.len == 5 && words[5] == 0xffff))) {
            int err = inet_ntop4(src+12, tp, sizeof tmp - (tp - tmp));
            if (err)
                return err;
            tp += strlen(tp);
            break;
        }
        tp += sprintf(tp, "%x", words[i]);
    }
    /* Was it a trailing run of 0x00's? */
    if (best.base != -1 && (best.base + best.len) == (sizeof(struct in6_addr) / sizeof(uint16_t)))
        *tp++ = ':';
    *tp++ = '\0';
    
    /*
     * Check for overflow, copy, and we're done.
     */
    if ((size_t)(tp - tmp) > size) {
        return ENOSPC;
    }
    strcpy(dst, tmp);
    return 0;
}


int portable_pton(int af, char* src, void* dst)
{
    switch (af) {
        case AF_INET:
            return (inet_pton4(src, dst));
        case AF_INET6:
            return (inet_pton6(src, dst));
        default:
            return EAFNOSUPPORT;
    }
    /* NOTREACHED */
}


static int inet_pton4(char *src, unsigned char *dst) {
    static const char digits[] = "0123456789";
    int saw_digit, octets, ch;
    unsigned char tmp[sizeof(struct in_addr)], *tp;
    
    saw_digit = 0;
    octets = 0;
    *(tp = tmp) = 0;
    while ((ch = *src++) != '\0') {
        char *pch;
        
        if ((pch = strchr(digits, ch)) != NULL) {
            unsigned int nw = (unsigned int)(*tp * 10 + (pch - digits));
            
            if (saw_digit && *tp == 0)
                return EINVAL;
            if (nw > 255)
                return EINVAL;
            *tp = nw;
            if (!saw_digit) {
                if (++octets > 4)
                    return EINVAL;
                saw_digit = 1;
            }
        } else if (ch == '.' && saw_digit) {
            if (octets == 4)
                return EINVAL;
            *++tp = 0;
            saw_digit = 0;
        } else
            return EINVAL;
    }
    if (octets < 4)
        return EINVAL;
    memcpy(dst, tmp, sizeof(struct in_addr));
    return 0;
}


static int inet_pton6(char *src, unsigned char *dst) {
    static char xdigits_l[] = "0123456789abcdef",
    xdigits_u[] = "0123456789ABCDEF";
    unsigned char tmp[sizeof(struct in6_addr)], *tp, *endp, *colonp;
    char *xdigits, *curtok;
    int ch, seen_xdigits;
    unsigned int val;
    
    memset((tp = tmp), '\0', sizeof tmp);
    endp = tp + sizeof tmp;
    colonp = NULL;
    /* Leading :: requires some special handling. */
    if (*src == ':')
        if (*++src != ':')
            return EINVAL;
    curtok = src;
    seen_xdigits = 0;
    val = 0;
    while ((ch = *src++) != '\0' && ch != '%') {
        char *pch;
        
        if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
            pch = strchr((xdigits = xdigits_u), ch);
        if (pch != NULL) {
            val <<= 4;
            val |= (pch - xdigits);
            if (++seen_xdigits > 4)
                return EINVAL;
            continue;
        }
        if (ch == ':') {
            curtok = src;
            if (!seen_xdigits) {
                if (colonp)
                    return EINVAL;
                colonp = tp;
                continue;
            } else if (*src == '\0') {
                return EINVAL;
            }
            if (tp + sizeof(uint16_t) > endp)
                return EINVAL;
            *tp++ = (unsigned char) (val >> 8) & 0xff;
            *tp++ = (unsigned char) val & 0xff;
            seen_xdigits = 0;
            val = 0;
            continue;
        }
        if (ch == '.' && ((tp + sizeof(struct in_addr)) <= endp)) {
            int err;
            
            /* Scope id present, parse ipv4 addr without it */
            pch = strchr(curtok, '%');
            if (pch != NULL) {
                char tmp[sizeof "255.255.255.255"];
                
                memcpy(tmp, curtok, pch - curtok);
                curtok = tmp;
                src = pch;
            }
            
            err = inet_pton4(curtok, tp);
            if (err == 0) {
                tp += sizeof(struct in_addr);
                seen_xdigits = 0;
                break;  /*%< '\\0' was seen by inet_pton4(). */
            }
        }
        return EINVAL;
    }
    if (seen_xdigits) {
        if (tp + sizeof(uint16_t) > endp)
            return EINVAL;
        *tp++ = (unsigned char) (val >> 8) & 0xff;
        *tp++ = (unsigned char) val & 0xff;
    }
    if (colonp != NULL) {
        /*
         * Since some memmove()'s erroneously fail to handle
         * overlapping regions, we'll do the shift by hand.
         */
        int n = (int)(tp - colonp);
        int i;
        
        if (tp == endp)
            return EINVAL;
        for (i = 1; i <= n; i++) {
            endp[- i] = colonp[n - i];
            colonp[n - i] = 0;
        }
        tp = endp;
    }
    if (tp != endp)
        return EINVAL;
    memcpy(dst, tmp, sizeof tmp);
    return 0;
}

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "nn.h"
#include "pubsub.h"
#include "pipeline.h"
#include "survey.h"
#include "reqrep.h"
#include "bus.h"
#include "pair.h"
#include "../../nanomsg/tools/options.h"
#include "clock.h"
#include "sleep.h"
//#include "../../nanomsg/src/utils/sleep.c"
//#include "../../nanomsg/src/utils/clock.c"


struct nn_parse_context {
    /*  Initial state  */
    struct nn_commandline *def;
    struct nn_option *options;
    void *target;
    int argc;
    char **argv;
    unsigned long requires;
    
    /*  Current values  */
    unsigned long mask;
    int args_left;
    char **arg;
    char *data;
    char **last_option_usage;
};

static int nn_has_arg (struct nn_option *opt)
{
    switch (opt->type) {
        case NN_OPT_INCREMENT:
        case NN_OPT_DECREMENT:
        case NN_OPT_SET_ENUM:
        case NN_OPT_HELP:
            return 0;
        case NN_OPT_ENUM:
        case NN_OPT_STRING:
        case NN_OPT_BLOB:
        case NN_OPT_FLOAT:
        case NN_OPT_INT:
        case NN_OPT_LIST_APPEND:
        case NN_OPT_LIST_APPEND_FMT:
        case NN_OPT_READ_FILE:
            return 1;
    }
    printf("nn_assert called\n"); getchar();
    //nn_assert (0);
}

static void nn_print_usage (struct nn_parse_context *ctx, FILE *stream)
{
    int i;
    int first;
    struct nn_option *opt;
    
    fprintf (stream, "    %s ", ctx->argv[0]);
    
    /* Print required options (long names)  */
    first = 1;
    for (i = 0;; ++i) {
        opt = &ctx->options[i];
        if (!opt->longname)
            break;
        if (opt->mask_set & ctx->requires) {
            if (first) {
                first = 0;
                fprintf (stream, "{--%s", opt->longname);
            } else {
                fprintf (stream, "|--%s", opt->longname);
            }
        }
    }
    if (!first) {
        fprintf (stream, "} ");
    }
    
    /* Print flag short options */
    first = 1;
    for (i = 0;; ++i) {
        opt = &ctx->options[i];
        if (!opt->longname)
            break;
        if (opt->mask_set & ctx->requires)
            continue;  /* already printed */
        if (opt->shortname && !nn_has_arg (opt)) {
            if (first) {
                first = 0;
                fprintf (stream, "[-%c", opt->shortname);
            } else {
                fprintf (stream, "%c", opt->shortname);
            }
        }
    }
    if (!first) {
        fprintf (stream, "] ");
    }
    
    /* Print short options with arguments */
    for (i = 0;; ++i) {
        opt = &ctx->options[i];
        if (!opt->longname)
            break;
        if (opt->mask_set & ctx->requires)
            continue;  /* already printed */
        if (opt->shortname && nn_has_arg (opt) && opt->metavar) {
            fprintf (stream, "[-%c %s] ", opt->shortname, opt->metavar);
        }
    }
    
    fprintf (stream, "[options] \n");  /* There may be long options too */
}

static char *nn_print_line (FILE *out, char *str, size_t width)
{
    int i;
    if (strlen (str) < width) {
        fprintf (out, "%s", str);
        return "";
    }
    for (i = (int32_t)width; i > 1; --i) {
        if (isspace (str[i])) {
            fprintf (out, "%.*s", i, str);
            return str + i + 1;
        }
    }  /* no break points, just print as is */
    fprintf (out, "%s", str);
    return "";
}

static void nn_print_help (struct nn_parse_context *ctx, FILE *stream)
{
    int i;
    int optlen;
    struct nn_option *opt;
    char *last_group;
    char *cursor;
    
    fprintf (stream, "Usage:\n");
    nn_print_usage (ctx, stream);
    fprintf (stream, "\n%s\n", ctx->def->short_description);
    
    last_group = NULL;
    for (i = 0;; ++i) {
        opt = &ctx->options[i];
        if (!opt->longname)
            break;
        if (!last_group || last_group != opt->group ||
            strcmp (last_group, opt->group))
        {
            fprintf (stream, "\n");
            fprintf (stream, "%s:\n", opt->group);
            last_group = opt->group;
        }
        fprintf (stream, " --%s", opt->longname);
        optlen = 3 + (int32_t)strlen (opt->longname);
        if (opt->shortname) {
            fprintf (stream, ",-%c", opt->shortname);
            optlen += 3;
        }
        if (nn_has_arg (opt)) {
            if (opt->metavar) {
                fprintf (stream, " %s", opt->metavar);
                optlen += strlen (opt->metavar) + 1;
            } else {
                fprintf (stream, " ARG");
                optlen += 4;
            }
        }
        if (optlen < 23) {
            fputs (&"                        "[optlen], stream);
            cursor = nn_print_line (stream, opt->description, 80-24);
        } else {
            cursor = opt->description;
        }
        while (*cursor) {
            fprintf (stream, "\n                        ");
            cursor = nn_print_line (stream, cursor, 80-24);
        }
        fprintf (stream, "\n");
    }
}

static void nn_print_option (struct nn_parse_context *ctx, int opt_index,
                             FILE *stream)
{
    char *ousage;
    char *oend;
    size_t olen;
    struct nn_option *opt;
    
    opt = &ctx->options[opt_index];
    ousage = ctx->last_option_usage[opt_index];
    if (*ousage == '-') {  /* Long option */
        oend = strchr (ousage, '=');
        if (!oend) {
            olen = strlen (ousage);
        } else {
            olen = (oend - ousage);
        }
        if (olen != strlen (opt->longname)+2) {
            fprintf (stream, " %.*s[%s] ",
                     (int)olen, ousage, opt->longname + (olen-2));
        } else {
            fprintf (stream, " %s ", ousage);
        }
    } else if (ousage == ctx->argv[0]) {  /* Binary name */
        fprintf (stream, " %s (executable) ", ousage);
    } else {  /* Short option */
        fprintf (stream, " -%c (--%s) ",
                 *ousage, opt->longname);
    }
}

static void nn_option_error (char *message, struct nn_parse_context *ctx,
                             int opt_index)
{
    fprintf (stderr, "%s: Option", ctx->argv[0]);
    nn_print_option (ctx, opt_index, stderr);
    fprintf (stderr, "%s\n", message);
    exit (1);
}


static void nn_memory_error (struct nn_parse_context *ctx) {
    fprintf (stderr, "%s: Memory error while parsing command-line",
             ctx->argv[0]);
    abort ();
}

static void nn_invalid_enum_value (struct nn_parse_context *ctx,
                                   int opt_index, char *argument)
{
    struct nn_option *opt;
    struct nn_enum_item *items;
    
    opt = &ctx->options[opt_index];
    items = (struct nn_enum_item *)opt->pointer;
    fprintf (stderr, "%s: Invalid value ``%s'' for", ctx->argv[0], argument);
    nn_print_option (ctx, opt_index, stderr);
    fprintf (stderr, ". Options are:\n");
    for (;items->name; ++items) {
        fprintf (stderr, "    %s\n", items->name);
    }
    exit (1);
}

static void nn_option_conflict (struct nn_parse_context *ctx,
                                int opt_index)
{
    unsigned long mask;
    int i;
    int num_conflicts;
    struct nn_option *opt;
    
    fprintf (stderr, "%s: Option", ctx->argv[0]);
    nn_print_option (ctx, opt_index, stderr);
    fprintf (stderr, "conflicts with the following options:\n");
    
    mask = ctx->options[opt_index].conflicts_mask;
    num_conflicts = 0;
    for (i = 0;; ++i) {
        opt = &ctx->options[i];
        if (!opt->longname)
            break;
        if (i == opt_index)
            continue;
        if (ctx->last_option_usage[i] && opt->mask_set & mask) {
            num_conflicts += 1;
            fprintf (stderr, "   ");
            nn_print_option (ctx, i, stderr);
            fprintf (stderr, "\n");
        }
    }
    if (!num_conflicts) {
        fprintf (stderr, "   ");
        nn_print_option (ctx, opt_index, stderr);
        fprintf (stderr, "\n");
    }
    exit (1);
}

static void nn_print_requires (struct nn_parse_context *ctx, unsigned long mask)
{
    int i;
    struct nn_option *opt;
    
    for (i = 0;; ++i) {
        opt = &ctx->options[i];
        if (!opt->longname)
            break;
        if (opt->mask_set & mask) {
            fprintf (stderr, "    --%s\n", opt->longname);
            if (opt->shortname) {
                fprintf (stderr, "    -%c\n", opt->shortname);
            }
        }
    }
    exit (1);
}

static void nn_option_requires (struct nn_parse_context *ctx, int opt_index) {
    fprintf (stderr, "%s: Option", ctx->argv[0]);
    nn_print_option (ctx, opt_index, stderr);
    fprintf (stderr, "requires at least one of the following options:\n");
    
    nn_print_requires (ctx, ctx->options[opt_index].requires_mask);
    exit (1);
}

static void nn_append_string (struct nn_parse_context *ctx,
                              struct nn_option *opt, char *str)
{
    struct nn_string_list *lst;
    
    lst = (struct nn_string_list *)(
                                    ((char *)ctx->target) + opt->offset);
    if (lst->items) {
        lst->num += 1;
        lst->items = realloc (lst->items, sizeof (char *) * lst->num);
    } else {
        lst->items = malloc (sizeof (char *));
        lst->num = 1;
    }
    if (!lst->items) {
        nn_memory_error (ctx);
    }
    lst->items[lst->num-1] = str;
}

static void nn_append_string_to_free (struct nn_parse_context *ctx,
                                      struct nn_option *opt, char *str)
{
    struct nn_string_list *lst;
    
    lst = (struct nn_string_list *)(
                                    ((char *)ctx->target) + opt->offset);
    if (lst->to_free) {
        lst->to_free_num += 1;
        lst->to_free = realloc (lst->items,
                                sizeof (char *) * lst->to_free_num);
    } else {
        lst->to_free = malloc (sizeof (char *));
        lst->to_free_num = 1;
    }
    if (!lst->items) {
        nn_memory_error (ctx);
    }
    lst->to_free[lst->to_free_num-1] = str;
}

static void nn_process_option (struct nn_parse_context *ctx,
                               int opt_index, char *argument)
{
    struct nn_option *opt;
    struct nn_enum_item *items;
    char *endptr;
    struct nn_blob *blob;
    FILE *file;
    char *data;
    size_t data_len;
    size_t data_buf;
    int bytes_read;
    
    opt = &ctx->options[opt_index];
    if (ctx->mask & opt->conflicts_mask) {
        nn_option_conflict (ctx, opt_index);
    }
    ctx->mask |= opt->mask_set;
    
    switch (opt->type) {
        case NN_OPT_HELP:
            nn_print_help (ctx, stdout);
            exit (0);
            return;
        case NN_OPT_INT:
            *(long *)(((char *)ctx->target) + opt->offset) = strtol (argument,
                                                                     &endptr, 0);
            if (endptr == argument || *endptr != 0) {
                nn_option_error ("requires integer argument",
                                 ctx, opt_index);
            }
            return;
        case NN_OPT_INCREMENT:
            *(int *)(((char *)ctx->target) + opt->offset) += 1;
            return;
        case NN_OPT_DECREMENT:
            *(int *)(((char *)ctx->target) + opt->offset) -= 1;
            return;
        case NN_OPT_ENUM:
            items = (struct nn_enum_item *)opt->pointer;
            for (;items->name; ++items) {
                if (!strcmp (items->name, argument)) {
                    *(int *)(((char *)ctx->target) + opt->offset) = \
                    items->value;
                    return;
                }
            }
            nn_invalid_enum_value (ctx, opt_index, argument);
            return;
        case NN_OPT_SET_ENUM:
            *(int *)(((char *)ctx->target) + opt->offset) = \
            *(int *)(opt->pointer);
            return;
        case NN_OPT_STRING:
            *(char **)(((char *)ctx->target) + opt->offset) = argument;
            return;
        case NN_OPT_BLOB:
            blob = (struct nn_blob *)(((char *)ctx->target) + opt->offset);
            blob->data = argument;
            blob->length = (int32_t)strlen (argument);
            blob->need_free = 0;
            return;
        case NN_OPT_FLOAT:
#if defined NN_HAVE_WINDOWS
            *(float *)(((char *)ctx->target) + opt->offset) =
            (float) atof (argument);
#else
            *(float *)(((char *)ctx->target) + opt->offset) =
            strtof (argument, &endptr);
            if (endptr == argument || *endptr != 0) {
                nn_option_error ("requires float point argument",
                                 ctx, opt_index);
            }
#endif
            return;
        case NN_OPT_LIST_APPEND:
            nn_append_string (ctx, opt, argument);
            return;
        case NN_OPT_LIST_APPEND_FMT:
            data_buf = strlen (argument) + strlen (opt->pointer);
            data = malloc (data_buf);
#if defined NN_HAVE_WINDOWS
            data_len = _snprintf_s (data, data_buf, _TRUNCATE, opt->pointer,
                                    argument);
#else
            data_len = snprintf (data, data_buf, opt->pointer, argument);
#endif
            assert (data_len < data_buf);
            nn_append_string (ctx, opt, data);
            nn_append_string_to_free (ctx, opt, data);
            return;
        case NN_OPT_READ_FILE:
            if (!strcmp (argument, "-")) {
                file = stdin;
            } else {
                file = fopen (argument, "r");
                if (!file) {
                    fprintf (stderr, "Error opening file ``%s'': %s\n",
                             argument, strerror (errno));
                    exit (2);
                }
            }
            data = malloc (4096);
            if (!data)
                nn_memory_error (ctx);
            data_len = 0;
            data_buf = 4096;
            for (;;) {
                bytes_read = (int32_t)fread (data + data_len, 1, data_buf - data_len,
                                    file);
                data_len += bytes_read;
                if (feof (file))
                    break;
                if (data_buf - data_len < 1024) {
                    if (data_buf < (1 << 20)) {
                        data_buf *= 2;  /* grow twice until not too big */
                    } else {
                        data_buf += 1 << 20;  /* grow 1 Mb each time */
                    }
                    data = realloc (data, data_buf);
                    if (!data)
                        nn_memory_error (ctx);
                }
            }
            if (data_len != data_buf) {
                data = realloc (data, data_len);
                assert (data);
            }
            if (ferror (file)) {
#if defined _MSC_VER
#pragma warning (push)
#pragma warning (disable:4996)
#endif
                fprintf (stderr, "Error reading file ``%s'': %s\n",
                         argument, strerror (errno));
#if defined _MSC_VER
#pragma warning (pop)
#endif
                exit (2);
            }
            if (file != stdin) {
                fclose (file);
            }
            blob = (struct nn_blob *)(((char *)ctx->target) + opt->offset);
            blob->data = data;
            blob->length = (int32_t)data_len;
            blob->need_free = 1;
            return;
    }
    abort ();
}

static void nn_parse_arg0 (struct nn_parse_context *ctx)
{
    int i;
    struct nn_option *opt;
    char *arg0;
    
    arg0 = strrchr (ctx->argv[0], '/');
    if (arg0 == NULL) {
        arg0 = ctx->argv[0];
    } else {
        arg0 += 1; /*  Skip slash itself  */
    }
    
    
    for (i = 0;; ++i) {
        opt = &ctx->options[i];
        if (!opt->longname)
            return;
        if (opt->arg0name && !strcmp (arg0, opt->arg0name)) {
            assert (!nn_has_arg (opt));
            ctx->last_option_usage[i] = ctx->argv[0];
            nn_process_option (ctx, i, NULL);
        }
    }
}


static void nn_error_ambiguous_option (struct nn_parse_context *ctx)
{
    struct nn_option *opt;
    char *a, *b;
    char *arg;
    
    arg = ctx->data+2;
    fprintf (stderr, "%s: Ambiguous option ``%s'':\n", ctx->argv[0], ctx->data);
    for (opt = ctx->options; opt->longname; ++opt) {
        for (a = opt->longname, b = arg; ; ++a, ++b) {
            if (*b == 0 || *b == '=') {  /* End of option on command-line */
                fprintf (stderr, "    %s\n", opt->longname);
                break;
            } else if (*b != *a) {
                break;
            }
        }
    }
    exit (1);
}

static void nn_error_unknown_long_option (struct nn_parse_context *ctx)
{
    fprintf (stderr, "%s: Unknown option ``%s''\n", ctx->argv[0], ctx->data);
    exit (1);
}

static void nn_error_unexpected_argument (struct nn_parse_context *ctx)
{
    fprintf (stderr, "%s: Unexpected argument ``%s''\n",
             ctx->argv[0], ctx->data);
    exit (1);
}

static void nn_error_unknown_short_option (struct nn_parse_context *ctx)
{
    fprintf (stderr, "%s: Unknown option ``-%c''\n", ctx->argv[0], *ctx->data);
    exit (1);
}

static int nn_get_arg (struct nn_parse_context *ctx)
{
    if (!ctx->args_left)
        return 0;
    ctx->args_left -= 1;
    ctx->arg += 1;
    ctx->data = *ctx->arg;
    return 1;
}

static void nn_parse_long_option (struct nn_parse_context *ctx)
{
    struct nn_option *opt;
    char *a, *b;
    int longest_prefix;
    int cur_prefix;
    int best_match;
    char *arg;
    int i;
    
    arg = ctx->data+2;
    longest_prefix = 0;
    best_match = -1;
    for (i = 0;; ++i) {
        opt = &ctx->options[i];
        if (!opt->longname)
            break;
        for (a = opt->longname, b = arg;; ++a, ++b) {
            if (*b == 0 || *b == '=') {  /* End of option on command-line */
                cur_prefix = (int32_t)(a - opt->longname);
                if (!*a) {  /* Matches end of option name */
                    best_match = i;
                    longest_prefix = cur_prefix;
                    goto finish;
                }
                if (cur_prefix == longest_prefix) {
                    best_match = -1;  /* Ambiguity */
                } else if (cur_prefix > longest_prefix) {
                    best_match = i;
                    longest_prefix = cur_prefix;
                }
                break;
            } else if (*b != *a) {
                break;
            }
        }
    }
finish:
    if (best_match >= 0) {
        opt = &ctx->options[best_match];
        ctx->last_option_usage[best_match] = ctx->data;
        if (arg[longest_prefix] == '=') {
            if (nn_has_arg (opt)) {
                nn_process_option (ctx, best_match, arg + longest_prefix + 1);
            } else {
                nn_option_error ("does not accept argument", ctx, best_match);
            }
        } else {
            if (nn_has_arg (opt)) {
                if (nn_get_arg (ctx)) {
                    nn_process_option (ctx, best_match, ctx->data);
                } else {
                    nn_option_error ("requires an argument", ctx, best_match);
                }
            } else {
                nn_process_option (ctx, best_match, NULL);
            }
        }
    } else if (longest_prefix > 0) {
        nn_error_ambiguous_option (ctx);
    } else {
        nn_error_unknown_long_option (ctx);
    }
}

static void nn_parse_short_option (struct nn_parse_context *ctx)
{
    int i;
    struct nn_option *opt;
    
    for (i = 0;; ++i) {
        opt = &ctx->options[i];
        if (!opt->longname)
            break;
        if (!opt->shortname)
            continue;
        if (opt->shortname == *ctx->data) {
            ctx->last_option_usage[i] = ctx->data;
            if (nn_has_arg (opt)) {
                if (ctx->data[1]) {
                    nn_process_option (ctx, i, ctx->data+1);
                } else {
                    if (nn_get_arg (ctx)) {
                        nn_process_option (ctx, i, ctx->data);
                    } else {
                        nn_option_error ("requires an argument", ctx, i);
                    }
                }
                ctx->data = "";  /* end of short options anyway */
            } else {
                nn_process_option (ctx, i, NULL);
                ctx->data += 1;
            }
            return;
        }
    }
    nn_error_unknown_short_option (ctx);
}


static void nn_parse_arg (struct nn_parse_context *ctx)
{
    if (ctx->data[0] == '-') {  /* an option */
        if (ctx->data[1] == '-') {  /* long option */
            if (ctx->data[2] == 0) {  /* end of options */
                return;
            }
            nn_parse_long_option (ctx);
        } else {
            ctx->data += 1;  /* Skip minus */
            while (*ctx->data) {
                nn_parse_short_option (ctx);
            }
        }
    } else {
        nn_error_unexpected_argument (ctx);
    }
}

void nn_check_requires (struct nn_parse_context *ctx) {
    int i;
    struct nn_option *opt;
    
    for (i = 0;; ++i) {
        opt = &ctx->options[i];
        if (!opt->longname)
            break;
        if (!ctx->last_option_usage[i])
            continue;
        if (opt->requires_mask &&
            (opt->requires_mask & ctx->mask) != opt->requires_mask) {
            nn_option_requires (ctx, i);
        }
    }
    
    if ((ctx->requires & ctx->mask) != ctx->requires) {
        fprintf (stderr, "%s: At least one of the following required:\n",
                 ctx->argv[0]);
        nn_print_requires (ctx, ctx->requires & ~ctx->mask);
        exit (1);
    }
}

void nn_parse_options (struct nn_commandline *cline,
                       void *target, int argc, char **argv)
{
    struct nn_parse_context ctx;
    int num_options;
    
    ctx.def = cline;
    ctx.options = cline->options;
    ctx.target = target;
    ctx.argc = argc;
    ctx.argv = argv;
    ctx.requires = cline->required_options;
    
    for (num_options = 0; ctx.options[num_options].longname; ++num_options);
    ctx.last_option_usage = calloc (sizeof (char *), num_options);
    if  (!ctx.last_option_usage)
        nn_memory_error (&ctx);
    
    ctx.mask = 0;
    ctx.args_left = argc - 1;
    ctx.arg = argv;
    
    nn_parse_arg0 (&ctx);
    
    while (nn_get_arg (&ctx)) {
        nn_parse_arg (&ctx);
    }
    
    nn_check_requires (&ctx);
    
    free (ctx.last_option_usage);
    
}

void nn_free_options (struct nn_commandline *cline, void *target) {
    int i, j;
    struct nn_option *opt;
    struct nn_blob *blob;
    struct nn_string_list *lst;
    
    for (i = 0;; ++i) {
        opt = &cline->options[i];
        if (!opt->longname)
            break;
        switch(opt->type) {
            case NN_OPT_LIST_APPEND:
            case NN_OPT_LIST_APPEND_FMT:
                lst = (struct nn_string_list *)(((char *)target) + opt->offset);
                if(lst->items) {
                    free(lst->items);
                    lst->items = NULL;
                }
                if(lst->to_free) {
                    for(j = 0; j < lst->to_free_num; ++j) {
                        free(lst->to_free[j]);
                    }
                    free(lst->to_free);
                    lst->to_free = NULL;
                }
                break;
            case NN_OPT_READ_FILE:
            case NN_OPT_BLOB:
                blob = (struct nn_blob *)(((char *)target) + opt->offset);
                if(blob->need_free && blob->data) {
                    free(blob->data);
                    blob->need_free = 0;
                }
                break;
            default:
                break;
        }
    }
}

enum echo_format {
    NN_NO_ECHO,
    NN_ECHO_RAW,
    NN_ECHO_ASCII,
    NN_ECHO_QUOTED,
    NN_ECHO_MSGPACK,
    NN_ECHO_HEX
};

typedef struct nn_options {
    /* Global options */
    int verbose;
    
    /* Socket options */
    int socket_type;
    struct nn_string_list bind_addresses;
    struct nn_string_list connect_addresses;
    float send_timeout;
    float recv_timeout;
    struct nn_string_list subscriptions;
    char *socket_name;
    
    /* Output options */
    float send_delay;
    float send_interval;
    struct nn_blob data_to_send;
    
    /* Input options */
    enum echo_format echo_format;
} nn_options_t;

/*  Constants to get address of in option declaration  */
static const int nn_push = NN_PUSH;
static const int nn_pull = NN_PULL;
static const int nn_pub = NN_PUB;
static const int nn_sub = NN_SUB;
static const int nn_req = NN_REQ;
static const int nn_rep = NN_REP;
static const int nn_bus = NN_BUS;
static const int nn_pair = NN_PAIR;
static const int nn_surveyor = NN_SURVEYOR;
static const int nn_respondent = NN_RESPONDENT;


struct nn_enum_item socket_types[] = {
    {"PUSH", NN_PUSH},
    {"PULL", NN_PULL},
    {"PUB", NN_PUB},
    {"SUB", NN_SUB},
    {"REQ", NN_REQ},
    {"REP", NN_REP},
    {"BUS", NN_BUS},
    {"PAIR", NN_PAIR},
    {"SURVEYOR", NN_SURVEYOR},
    {"RESPONDENT", NN_RESPONDENT},
    {NULL, 0},
};


/*  Constants to get address of in option declaration  */
static const int nn_echo_raw = NN_ECHO_RAW;
static const int nn_echo_ascii = NN_ECHO_ASCII;
static const int nn_echo_quoted = NN_ECHO_QUOTED;
static const int nn_echo_msgpack = NN_ECHO_MSGPACK;
static const int nn_echo_hex = NN_ECHO_HEX;

struct nn_enum_item echo_formats[] = {
    {"no", NN_NO_ECHO},
    {"raw", NN_ECHO_RAW},
    {"ascii", NN_ECHO_ASCII},
    {"quoted", NN_ECHO_QUOTED},
    {"msgpack", NN_ECHO_MSGPACK},
    {"hex", NN_ECHO_HEX},
    {NULL, 0},
};

/*  Constants for conflict masks  */
#define NN_MASK_SOCK 1
#define NN_MASK_WRITEABLE 2
#define NN_MASK_READABLE 4
#define NN_MASK_SOCK_SUB 8
#define NN_MASK_DATA 16
#define NN_MASK_ENDPOINT 32
#define NN_NO_PROVIDES 0
#define NN_NO_CONFLICTS 0
#define NN_NO_REQUIRES 0
#define NN_MASK_SOCK_WRITEABLE (NN_MASK_SOCK | NN_MASK_WRITEABLE)
#define NN_MASK_SOCK_READABLE (NN_MASK_SOCK | NN_MASK_READABLE)
#define NN_MASK_SOCK_READWRITE  (NN_MASK_SOCK_WRITEABLE|NN_MASK_SOCK_READABLE)

struct nn_option nn_options[] = {
    /* Generic options */
    {"verbose", 'v', NULL,
        NN_OPT_INCREMENT, offsetof (nn_options_t, verbose), NULL,
        NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_NO_REQUIRES,
        "Generic", NULL, "Increase verbosity of the nanocat"},
    {"silent", 'q', NULL,
        NN_OPT_DECREMENT, offsetof (nn_options_t, verbose), NULL,
        NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_NO_REQUIRES,
        "Generic", NULL, "Decrease verbosity of the nanocat"},
    {"help", 'h', NULL,
        NN_OPT_HELP, 0, NULL,
        NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_NO_REQUIRES,
        "Generic", NULL, "This help text"},
    
    /* Socket types */
    {"push", 0, "nn_push",
        NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_push,
        NN_MASK_SOCK_WRITEABLE, NN_MASK_SOCK, NN_MASK_DATA,
        "Socket Types", NULL, "Use NN_PUSH socket type"},
    {"pull", 0, "nn_pull",
        NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_pull,
        NN_MASK_SOCK_READABLE, NN_MASK_SOCK, NN_NO_REQUIRES,
        "Socket Types", NULL, "Use NN_PULL socket type"},
    {"pub", 0, "nn_pub",
        NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_pub,
        NN_MASK_SOCK_WRITEABLE, NN_MASK_SOCK, NN_MASK_DATA,
        "Socket Types", NULL, "Use NN_PUB socket type"},
    {"sub", 0, "nn_sub",
        NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_sub,
        NN_MASK_SOCK_READABLE|NN_MASK_SOCK_SUB, NN_MASK_SOCK, NN_NO_REQUIRES,
        "Socket Types", NULL, "Use NN_SUB socket type"},
    {"req", 0, "nn_req",
        NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_req,
        NN_MASK_SOCK_READWRITE, NN_MASK_SOCK, NN_MASK_DATA,
        "Socket Types", NULL, "Use NN_REQ socket type"},
    {"rep", 0, "nn_rep",
        NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_rep,
        NN_MASK_SOCK_READWRITE, NN_MASK_SOCK, NN_NO_REQUIRES,
        "Socket Types", NULL, "Use NN_REP socket type"},
    {"surveyor", 0, "nn_surveyor",
        NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_surveyor,
        NN_MASK_SOCK_READWRITE, NN_MASK_SOCK, NN_MASK_DATA,
        "Socket Types", NULL, "Use NN_SURVEYOR socket type"},
    {"respondent", 0, "nn_respondent",
        NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_respondent,
        NN_MASK_SOCK_READWRITE, NN_MASK_SOCK, NN_NO_REQUIRES,
        "Socket Types", NULL, "Use NN_RESPONDENT socket type"},
    {"bus", 0, "nn_bus",
        NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_bus,
        NN_MASK_SOCK_READWRITE, NN_MASK_SOCK, NN_NO_REQUIRES,
        "Socket Types", NULL, "Use NN_BUS socket type"},
    {"pair", 0, "nn_pair",
        NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_pair,
        NN_MASK_SOCK_READWRITE, NN_MASK_SOCK, NN_NO_REQUIRES,
        "Socket Types", NULL, "Use NN_PAIR socket type"},
    
    /* Socket Options */
    {"bind", 0, NULL,
        NN_OPT_LIST_APPEND, offsetof (nn_options_t, bind_addresses), NULL,
        NN_MASK_ENDPOINT, NN_NO_CONFLICTS, NN_NO_REQUIRES,
        "Socket Options", "ADDR", "Bind socket to the address ADDR"},
    {"connect", 0, NULL,
        NN_OPT_LIST_APPEND, offsetof (nn_options_t, connect_addresses), NULL,
        NN_MASK_ENDPOINT, NN_NO_CONFLICTS, NN_NO_REQUIRES,
        "Socket Options", "ADDR", "Connect socket to the address ADDR"},
    {"bind-ipc", 'X' , NULL, NN_OPT_LIST_APPEND_FMT,
        offsetof (nn_options_t, bind_addresses), "ipc://%s",
        NN_MASK_ENDPOINT, NN_NO_CONFLICTS, NN_NO_REQUIRES,
        "Socket Options", "PATH", "Bind socket to the ipc address "
        "\"ipc://PATH\"."},
    {"connect-ipc", 'x' , NULL, NN_OPT_LIST_APPEND_FMT,
        offsetof (nn_options_t, connect_addresses), "ipc://%s",
        NN_MASK_ENDPOINT, NN_NO_CONFLICTS, NN_NO_REQUIRES,
        "Socket Options", "PATH", "Connect socket to the ipc address "
        "\"ipc://PATH\"."},
    {"bind-local", 'L' , NULL, NN_OPT_LIST_APPEND_FMT,
        offsetof (nn_options_t, bind_addresses), "tcp://127.0.0.1:%s",
        NN_MASK_ENDPOINT, NN_NO_CONFLICTS, NN_NO_REQUIRES,
        "Socket Options", "PORT", "Bind socket to the tcp address "
        "\"tcp://127.0.0.1:PORT\"."},
    {"connect-local", 'l' , NULL, NN_OPT_LIST_APPEND_FMT,
        offsetof (nn_options_t, connect_addresses), "tcp://127.0.0.1:%s",
        NN_MASK_ENDPOINT, NN_NO_CONFLICTS, NN_NO_REQUIRES,
        "Socket Options", "PORT", "Connect socket to the tcp address "
        "\"tcp://127.0.0.1:PORT\"."},
    {"recv-timeout", 0, NULL,
        NN_OPT_FLOAT, offsetof (nn_options_t, recv_timeout), NULL,
        NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_READABLE,
        "Socket Options", "SEC", "Set timeout for receiving a message"},
    {"send-timeout", 0, NULL,
        NN_OPT_FLOAT, offsetof (nn_options_t, send_timeout), NULL,
        NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_WRITEABLE,
        "Socket Options", "SEC", "Set timeout for sending a message"},
    {"socket-name", 0, NULL,
        NN_OPT_STRING, offsetof (nn_options_t, socket_name), NULL,
        NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_NO_REQUIRES,
        "Socket Options", "NAME", "Name of the socket for statistics"},
    
    /* Pattern-specific options */
    {"subscribe", 0, NULL,
        NN_OPT_LIST_APPEND, offsetof (nn_options_t, subscriptions), NULL,
        NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_SOCK_SUB,
        "SUB Socket Options", "PREFIX", "Subscribe to the prefix PREFIX. "
        "Note: socket will be subscribed to everything (empty prefix) if "
        "no prefixes are specified on the command-line."},
    
    /* Input Options */
    {"format", 0, NULL,
        NN_OPT_ENUM, offsetof (nn_options_t, echo_format), &echo_formats,
        NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_READABLE,
        "Input Options", "FORMAT", "Use echo format FORMAT "
        "(same as the options below)"},
    {"raw", 0, NULL,
        NN_OPT_SET_ENUM, offsetof (nn_options_t, echo_format), &nn_echo_raw,
        NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_READABLE,
        "Input Options", NULL, "Dump message as is "
        "(Note: no delimiters are printed)"},
    {"ascii", 'A', NULL,
        NN_OPT_SET_ENUM, offsetof (nn_options_t, echo_format), &nn_echo_ascii,
        NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_READABLE,
        "Input Options", NULL, "Print ASCII part of message delimited by newline. "
        "All non-ascii characters replaced by dot."},
    {"quoted", 'Q', NULL,
        NN_OPT_SET_ENUM, offsetof (nn_options_t, echo_format), &nn_echo_quoted,
        NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_READABLE,
        "Input Options", NULL, "Print each message on separate line in double "
        "quotes with C-like character escaping"},
    {"msgpack", 0, NULL,
        NN_OPT_SET_ENUM, offsetof (nn_options_t, echo_format), &nn_echo_msgpack,
        NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_READABLE,
        "Input Options", NULL, "Print each message as msgpacked string (raw type)."
        " This is useful for programmatic parsing."},
    
    {"hex", 0, NULL,
        NN_OPT_SET_ENUM, offsetof (nn_options_t, echo_format), &nn_echo_hex,
        NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_READABLE,
        "Input Options", NULL, "Print each message on separate line in double "
        "quotes with hex values"},
    /* Output Options */
    {"interval", 'i', NULL,
        NN_OPT_FLOAT, offsetof (nn_options_t, send_interval), NULL,
        NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_WRITEABLE,
        "Output Options", "SEC", "Send message (or request) every SEC seconds"},
    {"delay", 'd', NULL,
        NN_OPT_FLOAT, offsetof (nn_options_t, send_delay), NULL,
        NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_NO_REQUIRES,
        "Output Options", "SEC", "Wait for SEC seconds before sending message"
        " (useful for one-shot PUB sockets)"},
    {"data", 'D', NULL,
        NN_OPT_BLOB, offsetof (nn_options_t, data_to_send), &echo_formats,
        NN_MASK_DATA, NN_MASK_DATA, NN_MASK_WRITEABLE,
        "Output Options", "DATA", "Send DATA to the socket and quit for "
        "PUB, PUSH, PAIR, BUS socket. Use DATA to reply for REP or "
        " RESPONDENT socket. Send DATA as request for REQ or SURVEYOR socket."},
    {"file", 'F', NULL,
        NN_OPT_READ_FILE, offsetof (nn_options_t, data_to_send), &echo_formats,
        NN_MASK_DATA, NN_MASK_DATA, NN_MASK_WRITEABLE,
        "Output Options", "PATH", "Same as --data but get data from file PATH"},
    
    /* Sentinel */
    {NULL, 0, NULL,
        0, 0, NULL,
        0, 0, 0,
        NULL, NULL, NULL},
};


struct nn_commandline nn_cli = {
    "A command-line interface to nanomsg",
    "",
    nn_options,
    NN_MASK_SOCK | NN_MASK_ENDPOINT,
};


void nn_assert_errno (int flag, char *description)
{
    int err;
    
    if (!flag) {
        err = errno;
        fprintf (stderr, "%s: %s\n", description, nn_strerror (err));
        exit (3);
    }
}

void nn_sub_init (nn_options_t *options, int sock)
{
    int i;
    int rc;
    
    if (options->subscriptions.num) {
        for (i = 0; i < options->subscriptions.num; ++i) {
            rc = nn_setsockopt (sock, NN_SUB, NN_SUB_SUBSCRIBE,
                                options->subscriptions.items[i],
                                strlen (options->subscriptions.items[i]));
            nn_assert_errno (rc == 0, "Can't subscribe");
        }
    } else {
        rc = nn_setsockopt (sock, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
        nn_assert_errno (rc == 0, "Can't subscribe");
    }
}

void nn_set_recv_timeout (int sock, int millis)
{
    int rc;
    rc = nn_setsockopt (sock, NN_SOL_SOCKET, NN_RCVTIMEO,
                        &millis, sizeof (millis));
    nn_assert_errno (rc == 0, "Can't set recv timeout");
}

int nn_create_socket (nn_options_t *options)
{
    int sock;
    int rc;
    int millis;
    
    sock = nn_socket (AF_SP, options->socket_type);
    nn_assert_errno (sock >= 0, "Can't create socket");
    
    /* Generic initialization */
    if (options->send_timeout >= 0) {
        millis = (int)(options->send_timeout * 1000);
        rc = nn_setsockopt (sock, NN_SOL_SOCKET, NN_SNDTIMEO,
                            &millis, sizeof (millis));
        nn_assert_errno (rc == 0, "Can't set send timeout");
    }
    if (options->recv_timeout >= 0) {
        nn_set_recv_timeout (sock, (int) options->recv_timeout);
    }
    if (options->socket_name) {
        rc = nn_setsockopt (sock, NN_SOL_SOCKET, NN_SOCKET_NAME,
                            options->socket_name, strlen(options->socket_name));
        nn_assert_errno (rc == 0, "Can't set socket name");
    }
    
    /* Specific initialization */
    switch (options->socket_type) {
        case NN_SUB:
            nn_sub_init (options, sock);
            break;
    }
    
    return sock;
}

void nn_print_message (nn_options_t *options, char *buf, int buflen)
{
    switch (options->echo_format) {
        case NN_NO_ECHO:
            return;
        case NN_ECHO_RAW:
            fwrite (buf, 1, buflen, stdout);
            break;
        case NN_ECHO_ASCII:
            for (; buflen > 0; --buflen, ++buf) {
                if (isprint (*buf)) {
                    fputc (*buf, stdout);
                } else {
                    fputc ('.', stdout);
                }
            }
            fputc ('\n', stdout);
            break;
        case NN_ECHO_QUOTED:
            fputc ('"', stdout);
            for (; buflen > 0; --buflen, ++buf) {
                switch (*buf) {
                    case '\n':
                        fprintf (stdout, "\\n");
                        break;
                    case '\r':
                        fprintf (stdout, "\\r");
                        break;
                    case '\\':
                    case '\"':
                        fprintf (stdout, "\\%c", *buf);
                        break;
                    default:
                        if (isprint (*buf)) {
                            fputc (*buf, stdout);
                        } else {
                            fprintf (stdout, "\\x%02x", (unsigned char)*buf);
                        }
                }
            }
            fprintf (stdout, "\"\n");
            break;
        case NN_ECHO_MSGPACK:
            if (buflen < 256) {
                fputc ('\xc4', stdout);
                fputc (buflen, stdout);
                fwrite (buf, 1, buflen, stdout);
            } else if (buflen < 65536) {
                fputc ('\xc5', stdout);
                fputc (buflen >> 8, stdout);
                fputc (buflen & 0xff, stdout);
                fwrite (buf, 1, buflen, stdout);
            } else {
                fputc ('\xc6', stdout);
                fputc (buflen >> 24, stdout);
                fputc ((buflen >> 16) & 0xff, stdout);
                fputc ((buflen >> 8) & 0xff, stdout);
                fputc (buflen & 0xff, stdout);
                fwrite (buf, 1, buflen, stdout);
            }
            break;
        case NN_ECHO_HEX:
            fputc ('"', stdout);
            for (; buflen > 0; --buflen, ++buf) {
                fprintf (stdout, "\\x%02x", (unsigned char)*buf);
            }
            fprintf (stdout, "\"\n");
            break;
            
    }
    fflush (stdout);
}

void nn_connect_socket (nn_options_t *options, int sock)
{
    int i;
    int rc;
    
    for (i = 0; i < options->bind_addresses.num; ++i) {
        rc = nn_bind (sock, options->bind_addresses.items[i]);
        nn_assert_errno (rc >= 0, "Can't bind");
    }
    for (i = 0; i < options->connect_addresses.num; ++i) {
        rc = nn_connect (sock, options->connect_addresses.items[i]);
        nn_assert_errno (rc >= 0, "Can't connect");
    }
}

void nn_send_loop (nn_options_t *options, int sock,uint8_t *data,int32_t datalen)
{
    int rc;
    uint64_t start_time;
    int64_t time_to_sleep, interval;
    struct nn_clock clock;
    
    interval = (int)(options->send_interval*1000);
    nn_clock_init (&clock);
    
    for (;;) {
        start_time = nn_clock_now (&clock);
        rc = nn_send (sock,data,datalen, 0);
        if (rc < 0 && errno == EAGAIN) {
            fprintf (stderr, "Message not sent (EAGAIN)\n");
        } else {
            nn_assert_errno (rc >= 0, "Can't send");
        }
        if (interval >= 0) {
            time_to_sleep = (start_time + interval) - nn_clock_now (&clock);
            if (time_to_sleep > 0) {
                nn_sleep ((int) time_to_sleep);
            }
        } else {
            break;
        }
    }
    
    nn_clock_term(&clock);
}

void nn_recv_loop (nn_options_t *options, int sock)
{
    int rc;
    void *buf;
    
    for (;;) {
        rc = nn_recv (sock, &buf, NN_MSG, 0);
        if (rc < 0 && errno == EAGAIN) {
            continue;
        } else if (rc < 0 && (errno == ETIMEDOUT || errno == EFSM)) {
            return;  /*  No more messages possible  */
        } else {
            nn_assert_errno (rc >= 0, "Can't recv");
        }
        nn_print_message (options, buf, rc);
        nn_freemsg (buf);
    }
}

void nn_rw_loop (nn_options_t *options, int sock,uint8_t *data,int32_t datalen)
{
    int rc;
    void *buf;
    uint64_t start_time;
    int64_t time_to_sleep, interval, recv_timeout;
    struct nn_clock clock;
    
    interval = (int)(options->send_interval*1000);
    recv_timeout = (int)(options->recv_timeout*1000);
    nn_clock_init (&clock);
    
    for (;;) {
        start_time = nn_clock_now (&clock);
        rc = nn_send (sock,data,datalen,0);
        if (rc < 0 && errno == EAGAIN) {
            fprintf (stderr, "Message not sent (EAGAIN)\n");
        } else {
            nn_assert_errno (rc >= 0, "Can't send");
        }
        if (options->send_interval < 0) {  /*  Never send any more  */
            nn_recv_loop (options, sock);
            return;
        }
        
        for (;;) {
            time_to_sleep = (start_time + interval) - nn_clock_now (&clock);
            if (time_to_sleep <= 0) {
                break;
            }
            if (recv_timeout >= 0 && time_to_sleep > recv_timeout)
            {
                time_to_sleep = recv_timeout;
            }
            nn_set_recv_timeout (sock, (int) time_to_sleep);
            rc = nn_recv (sock, &buf, NN_MSG, 0);
            if (rc < 0) {
                if (errno == EAGAIN) {
                    continue;
                } else if (errno == ETIMEDOUT || errno == EFSM) {
                    time_to_sleep = (start_time + interval)
                    - nn_clock_now (&clock);
                    if (time_to_sleep > 0)
                        nn_sleep ((int) time_to_sleep);
                    continue;
                }
            }
            nn_assert_errno (rc >= 0, "Can't recv");
            nn_print_message (options, buf, rc);
            nn_freemsg (buf);
        }
    }
    
    nn_clock_term(&clock);
}

void nn_resp_loop (nn_options_t *options, int sock,uint8_t *data,int32_t datalen)
{
    int rc;
    void *buf;
    
    for (;;) {
        rc = nn_recv (sock, &buf, NN_MSG, 0);
        if (rc < 0 && errno == EAGAIN) {
            continue;
        } else {
            nn_assert_errno (rc >= 0, "Can't recv");
        }
        nn_print_message (options, buf, rc);
        nn_freemsg (buf);
        //rc = nn_send (sock,options->data_to_send.data, options->data_to_send.length,0);
        rc = nn_send (sock,data,datalen,0);
        if (rc < 0 && errno == EAGAIN) {
            fprintf (stderr, "Message not sent (EAGAIN)\n");
        } else {
            nn_assert_errno (rc >= 0, "Can't send");
        }
    }
}

int test_nn(int argc, char **argv,uint8_t *data,int32_t datalen)
{
    int sock;
    nn_options_t options = {
        /* verbose           */ 0,
        /* socket_type       */ 0,
        /* bind_addresses    */ {NULL, NULL, 0, 0},
        /* connect_addresses */ {NULL, NULL, 0, 0},
        /* send_timeout      */ -1.f,
        /* recv_timeout      */ -1.f,
        /* subscriptions     */ {NULL, NULL, 0, 0},
        /* socket_name       */ NULL,
        /* send_delay        */ 0.f,
        /* send_interval     */ -1.f,
        /* data_to_send      */ {NULL, 0, 0},
        /* echo_format       */ NN_NO_ECHO
    };
    
    nn_parse_options (&nn_cli, &options, argc, argv);
    sock = nn_create_socket (&options);
    nn_connect_socket (&options, sock);
    nn_sleep((int)(options.send_delay*1000));
    switch (options.socket_type) {
        case NN_PUB:
        case NN_PUSH:
            nn_send_loop (&options, sock,data,datalen);
            break;
        case NN_SUB:
        case NN_PULL:
            nn_recv_loop (&options, sock);
            break;
        case NN_BUS:
        case NN_PAIR:
            if (options.data_to_send.data) {
                nn_rw_loop (&options, sock,data,datalen);
            } else {
                nn_recv_loop (&options, sock);
            }
            break;
        case NN_SURVEYOR:
        case NN_REQ:
            nn_rw_loop (&options, sock,data,datalen);
            break;
        case NN_REP:
        case NN_RESPONDENT:
            if (options.data_to_send.data) {
                nn_resp_loop (&options, sock,data,datalen);
            } else {
                nn_recv_loop (&options, sock);
            }
            break;
    }
    
    nn_close (sock);
    nn_free_options(&nn_cli, &options);
    return 0;
}

#endif
#endif

