/*
   scpi_dev.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "scpi.h"
#include "scpi_core.h"


#define CGR101_MIN_CHAN 1
#define CGR101_MAX_CHAN 21

void scpi_dev_abort(struct info *info)
{
    (void)info;
}

struct scpi_type *scpi_dev_channel_num(struct info *info,
                                       struct scpi_type *val)
{
    (void)info;

    if (val->type == SCPI_TYPE_INT &&
        val->val.ival >= CGR101_MIN_CHAN &&
        val->val.ival <= CGR101_MAX_CHAN)
    {
        val->val.chan = 1<<val->val.ival;
        val->type = SCPI_TYPE_CHAN;
    } else {
        /*scpi_error(info->scpi, SCPI_ERR_DATA_OUT_OF_RANGE, val->src);*/
    }
    return val;
}

struct scpi_type *scpi_dev_channel_range(struct info *info,
                                         struct scpi_type *v1,
                                         struct scpi_type *v2)
{
    long chan_start;
    long chan_end;
    (void)info;

    assert(v1->type == SCPI_TYPE_CHAN);
    assert(v2->type == SCPI_TYPE_CHAN);

    if (v1->val.chan < v2->val.chan) {
        chan_start = v1->val.chan;
        chan_end   = v2->val.chan;
    } else {
        chan_start = v2->val.chan;
        chan_end   = v1->val.chan;
    }

    assert(chan_start != 0);
    assert(chan_end != 0);
    assert(chan_start < chan_end);
    /* Set a range of channels */
    while (chan_start != chan_end) {
        v2->val.chan |= chan_start;
        chan_start <<= 1;
        assert(chan_start != 0);
    }

    return v2;
}

struct scpi_type *scpi_dev_channel_range_append(struct info *info,
                                                struct scpi_type *v1,
                                                struct scpi_type *v2)
{
    (void)info;

    assert(v1->type == SCPI_TYPE_CHAN);
    assert(v2->type == SCPI_TYPE_CHAN);

    v2->val.chan |= v1->val.chan;

    return v2;
}

void scpi_dev_measure_digital_dataq(struct info *info)
{
    /* STUB */
    scpi_output_int(&info->scpi->output, 0);
}


