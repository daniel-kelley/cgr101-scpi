/*
   cea.c

   Copyright (c) 2022 by Daniel Kelley

   Brute Force Conversion error analysis

  # there are three conversions (and the inversion)
  # (1) A/D offset binary integer representation to two's complement integer
  # (2) 2'SC int to raw voltage multiplying by step size
  # (3) Cooked voltage by subtracting input offset from raw value
  # invversion:
  # (4) Raw voltage by adding input offset to cooked value
  # (5) Raw voltage to 2'SC int steps by dividing by step size
  # (6) 2'SC integer to Offset binary integer

  k,o,v,d,m,s = var('k o v d m s')


  d2v(d,m,s,o) = (m - (d + o)) * s

  v2d(v,m,s,o) = (m + (v/s) - o)
** offset as voltage
  # offset data to offset voltage
  f1(o,k,s) = (k - o) * s
  # offset voltage to offset data
  f2(v,k,s) = k - (v/s)

  # scope data to voltage
  d2v(d,m,s,o) = ((m - d) * s) - o
  # scope voltage to data
  v2d(v,m,s,o) = m - ((v+o)/s)

# symbolic inversion should yield the input as output

sage: v2d(d2v(x,m,s,o),m,s,o)
x
sage: d2v(v2d(x,m,s,o),m,s,o)
x
sage: f1(f2(x,k,s),k,s)
x
sage: f2(f1(x,k,s),k,s)
x

** offset as 2's complement

  # offset AD data to offset 2's complement
  f1(d,k) = k - d
  # offset  2's complement to offset data
  f2(v,k) = k - v

  # scope data to voltage
  d2v(d,m,s,o) = (((m-d) - o) * s)
  # scope voltage to data
  v2d(v,m,s,o) = (m - ((v/s) + o))


# symbolic inversion should yield the input as output

sage: f1(f2(x,k),k)
x
sage: f2(f1(x,k),k)
x
sage: v2d(d2v(x,m,s,o),m,s,o)
x
sage: d2v(v2d(x,m,s,o),m,s,o)
x

*/

#include <assert.h>
#include <stdio.h>
#include <math.h>

/*
 * 10 bit
 *   m           a         c
 *   ------      ------    -----
 *   0x01ff  -   0x0000 -> 0x000001ff  +0x1ff
 *   0x01ff  -   0x01ff -> 0x00000000   0x0
 *   0x01ff  -   0x03fe -> 0xfffffe01  -0x1ff
 *   0x01ff  -   0x03ff -> 0xfffffe00  -0x200
 *
 * 8 bit
 *   m           a         c
 *   ------      ------    -----
 *   0x0080  -   0x0000 -> 0x00000080  +0x80
 *   0x0080  -   0x007f -> 0x00000001   0x1
 *   0x0080  -   0x0080 -> 0x00000000   0x0
 *   0x0080  -   0x00fe -> 0xffffff82  -0x7e
 *   0x0080  -   0x00ff -> 0xffffff81  -0x7f
 */

#define USE_DOUBLE 1

#if USE_DOUBLE
#define FLOAT double
#define ROUND round
#define FABS fabs
#else
#define FLOAT float
#define ROUND roundf
#define FABS fabsf
#endif

/* Unsigned value to 2's complement with zero at midpoint. */
static int a2c(int midpoint, int value)
{
    int result;

    assert(midpoint > 0);
    assert(value >= 0);

    if (midpoint & 1) {
        assert(value <= ((midpoint*2)+1));
    } else {
        assert(value <= ((midpoint*2)-1));
    }

    result = (midpoint - value);

    assert(result <= midpoint);
    if (midpoint & 1) {
        assert(result >= -(midpoint+1));
    } else {
        assert(result >= -(midpoint-1));
    }

    return result;
}

/* 2's complement to Unsigned value with zero at midpoint. */
static int c2a(int midpoint, int value)
{
    int result;

    assert(midpoint > 0);

    assert(value <= midpoint*2);
    if (midpoint & 1) {
        assert(value >= -(midpoint+1));
    } else {
        assert(value >= -(midpoint-1));
    }

    result = midpoint - value;

    assert(result >= 0);

    if (midpoint & 1) {
        assert(result <= ((midpoint*2)+1));
    } else {
        assert(result <= ((midpoint*2)-1));
    }

    return result;
}

static FLOAT offset_data_to_offset_voltage(int o, int k, FLOAT s)
{
    return (FLOAT)a2c(k,o) * s;
}

static int offset_voltage_to_offset_data(FLOAT v, int k, FLOAT s)
{
    return c2a(k,(int)ROUND(v/s));
}

/* scope data to voltage with offset as voltage */
static FLOAT scope_data_to_voltage_ov(int d,int m,FLOAT s,FLOAT o)
{
    return ((FLOAT)a2c(m,d) * s) - o;
}

/* scope voltage to data with offset as voltage */
static int scope_voltage_to_data_ov(FLOAT v,int m,FLOAT s,FLOAT o)
{
    return c2a(m,(int)ROUND((v+o)/s));
}

static int offset_data_to_offset_complement(int o, int k)
{
    return a2c(k,o);
}

static int offset_complement_to_offset_data(int v, int k)
{
    return c2a(k,v);
}

/* scope data to voltage with offset as 2's complement */
static FLOAT scope_data_to_voltage_oc(int d,int m,FLOAT s,int o)
{
    return (FLOAT)(a2c(m,d)-o) * s;
}

/* scope voltage to data with offset as  2's complement */
static int scope_voltage_to_data_oc(FLOAT v,int m,FLOAT s,int o)
{
    return c2a(m, (int)ROUND(v/s) + o);
}

#if USE_DOUBLE
#define STEP_HIGH 0.0521
#define STEP_LOW  0.00592
#else
#define STEP_HIGH 0.0521f
#define STEP_LOW  0.00592f
#endif
#define MP8 128
#define MP10 511
#define QUARTER_LSB_10 (1.0/1024.0/4.0)

static void check_d(int data, int offset, FLOAT step)
{
    FLOAT ov = offset_data_to_offset_voltage(offset, MP8, step);
    int oc = offset_data_to_offset_complement(offset, MP8);
    FLOAT v_ov = scope_data_to_voltage_ov(data, MP10, step, ov);
    FLOAT v_oc = scope_data_to_voltage_oc(data, MP10, step, oc);
    int d_ov = scope_voltage_to_data_ov(v_ov, MP10, step, ov);
    int d_oc = scope_voltage_to_data_oc(v_oc, MP10, step, oc);
    FLOAT delta = FABS(v_ov - v_oc);
    assert(d_ov == d_oc);
    assert(delta < QUARTER_LSB_10);
    printf("%d %d %g %g\n",data,offset,step,delta);
}

static void check_o(int offset, FLOAT step)
{
    FLOAT ov = offset_data_to_offset_voltage(offset, MP8, step);
    int dv = offset_voltage_to_offset_data(ov, MP8, step);
    int oc = offset_data_to_offset_complement(offset, MP8);
    int dc = offset_complement_to_offset_data(oc, MP8);
    assert(dv == dc);
}

#define MAX_OFFSET (1<<8)
#define MAX_DATA (1<<10)

int main(void)
{
    for (int offset=0; offset < MAX_OFFSET; offset++) {
        check_o(offset,STEP_LOW);
        check_o(offset,STEP_HIGH);
        for (int data=0; data < (MAX_DATA); data++) {
            check_d(data,offset,STEP_LOW);
            check_d(data,offset,STEP_HIGH);
        }
    }

    return 0;
}
