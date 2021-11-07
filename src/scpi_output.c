/*
   scpi_output.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "scpi_output.h"

int scpi_output_printf(struct scpi_output *output,
                       const char *format,
                       ...)
{
    int err = 1;
    size_t current = OUTPUT_SIZE - output->len - 1;
    size_t actual;
    va_list ap;

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

    va_end(ap);

    return err;
}


int scpi_output_int(struct scpi_output *output, int value)
{
    return scpi_output_printf(output, "%d", value);
}

int scpi_output_str(struct scpi_output *output, const char *value)
{
    return scpi_output_printf(output, "%s", value);
}

int scpi_output_cmd_sep(struct scpi_output *output)
{
    return scpi_output_str(output, ";");
}

void scpi_output_clear(struct scpi_output *output)
{
    memset(output, 0, sizeof(*output));
}

int scpi_output_get(struct scpi_output *output, uint8_t **buf, size_t *sz)
{
    int err = 0;

    if (output->len != 0) {
        /* If there is any output, terminate output with newline. */
        err = scpi_output_printf(output, "\n");
    }

    *buf = output->buf;
    *sz  = output->len;

    return err;
}
