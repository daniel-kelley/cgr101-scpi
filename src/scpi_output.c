/*
   scpi_output.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include "scpi_output.h"

#define OUTPUT_SIZE (1024*32)

struct scpi_output {
    int                 need_sep;
    int                 num_elem;
    int                 overflow;
    size_t              len;
    uint8_t             buf[OUTPUT_SIZE];
};

struct scpi_output *scpi_output_init(void)
{
    struct scpi_output *output;

    output = calloc(1,sizeof(*output));

    return output;
}

int scpi_output_done(struct scpi_output *output)
{

    assert(output);
    free(output);
    return 0;
}

static size_t scpi_output_append_char(struct scpi_output *output, char c)
{
    size_t current = OUTPUT_SIZE - output->len - 1;

    if (current > 1) {
        *((char *)output->buf + output->len) = c;
        current -= 1;
        output->len += 1;
    } else {
        output->overflow = 1;
    }

    return current;
}

int scpi_output_printf(struct scpi_output *output,
                       const char *format,
                       ...)
{
    int err = 1;
    size_t current = OUTPUT_SIZE - output->len - 1;
    size_t actual;
    va_list ap;

    assert(format != NULL);

    /* Handle output separator. */
    if (output->need_sep) {
        current = scpi_output_append_char(output, ';');
        output->need_sep = 0;
        output->num_elem = 0;
    }

    /* Append ',' after the first element if any except if newline. */
    if (output->num_elem > 0 && *format != '\n') {
        current = scpi_output_append_char(output, ',');
    }

    va_start(ap, format);
    actual = (size_t)vsnprintf((char *)output->buf + output->len,
                               current,
                               format,
                               ap);
    /* 'old' GLIBC prints return -1 on overflow, not the actual length
     * needed, but the cast should result in a very large number
     * anyway.
     */
    if (actual > current) {
        output->overflow = 1;
    } else {
        output->len += actual;
        err = 0;
    }

    output->num_elem += 1;

    va_end(ap);

    return err;
}

int scpi_output_int(struct scpi_output *output, int value)
{
    return scpi_output_printf(output, "%d", value);
}

int scpi_output_fp(struct scpi_output *output, double value)
{
    return scpi_output_printf(output, "%+.14g", value);
}

int scpi_output_str(struct scpi_output *output, const char *value)
{
    return scpi_output_printf(output, "%s", value);
}

int scpi_output_cmd_sep(struct scpi_output *output)
{
    output->need_sep = 1;

    return 0;
}

void scpi_output_clear(struct scpi_output *output)
{
    memset(output, 0, sizeof(*output));
}

void scpi_output_reset(struct scpi_output *output)
{
    output->len = 0;
}

void scpi_output_flush(struct scpi_output *output, int fd)
{
    ssize_t outlen;
    int err;

    if (output->len > 0) {
        err = scpi_output_printf(output, "\n");
        assert(!err);

        outlen = write(fd, output->buf, output->len);

        /* Mark as copied out. */
        output->len = 0;

        /* Handle truncated writes. */
        assert(outlen >= 0);
    }
}
