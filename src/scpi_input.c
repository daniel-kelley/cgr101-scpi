/*
   scpi_input.c

   Copyright (c) 2022 by Daniel Kelley

*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "info.h"
#include "scpi_error.h"
#include "scpi_input.h"

int scpi_input_int(struct info *info,
                   struct scpi_type *in,
                   long low,
                   long high,
                   long *out)
{
    int err = 1;

    assert(in->type == SCPI_TYPE_INT);

    if (in->val.ival >= low && in->val.ival <= high) {
        err = 0;
        *out = in->val.ival;
    } else {
        scpi_error(info->error, SCPI_ERR_DATA_OUT_OF_RANGE, in->src);
    }

    return err;
}

int scpi_input_boolean(struct info *info,
                       struct scpi_type *in,
                       int *out)
{
    long ival;
    int err = scpi_input_int(info, in, 0, 1, &ival);

    if (!err) {
        *out = (int)ival;
    }

    return err;
}

int scpi_input_uint8(struct info *info,
                     struct scpi_type *in,
                     uint8_t *out)
{
    long ival;
    int err = scpi_input_int(info, in, 0, 255, &ival);

    if (!err) {
        *out = (uint8_t)ival;
    }

    return err;
}

int scpi_input_uint16(struct info *info,
                      struct scpi_type *in,
                      uint16_t *out)
{
    long ival;
    int err = scpi_input_int(info, in, 0, 0xffff, &ival);

    if (!err) {
        *out = (uint16_t)ival;
    }

    return err;
}

int scpi_input_fp(struct info *info, struct scpi_type *in, double *out)
{
    (void)info;
    switch (in->type) {
    case SCPI_TYPE_FLOAT:
    *out = in->val.fval;
    break;
    case SCPI_TYPE_INT:
        *out = (double)in->val.ival;
    break;
    default:
        assert(0);
    }

    return 0;
}

/* Reformat the list into an array of doubles in place. */
static void scpi_input_fp_block_copy(struct scpi_type *in,
                                     size_t *out_len,
                                     double **out_block)
{
    double *dp;
    size_t len;
    union scpi_type_val *val;

    assert(out_len);
    assert(out_block);
    val = in->val.list->val;
    len = in->val.list->len;
    /* Type conversion is done in place, which destroys the type
     * metadata. This is expected to happen when the block type is
     * passed to the device function, and the SCPI type data is no
     * longer needed. The SCPI type is changed to UNKNOWN so any
     * subsequent references will assert.
     */
    in->type = SCPI_TYPE_UNKNOWN;
    dp = (double *)in->val.list;
    *out_len = len;
    *out_block = dp;
    while(len--) {
        *dp++ = val->fval;
        val++;
    }

}

int scpi_input_fp_block(struct info *info,
                        struct scpi_type *in,
                        size_t *out_len,
                        double **out_block)
{
    (void)info;
    if (in->type == SCPI_TYPE_FLOAT) {
        *out_block = &in->val.fval;
        *out_len = 1;
    } else if (in->type == SCPI_TYPE_LIST &&
               in->val.list->type == SCPI_TYPE_FLOAT) {
        scpi_input_fp_block_copy(in, out_len, out_block);
    } else {
        assert(0); /* Some error */
    }

    return 0;
}

int scpi_input_str(struct info *info, struct scpi_type *in, const char **out)
{
    (void)info;

    assert(in->type == SCPI_TYPE_STR);
    *out = in->src;

    return 0;
}
