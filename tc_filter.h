/**
 * \file tc_filter.h
 * \brief Fast filtering options based on the integral image.
 * \author David Ray Thompson
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#include <stdlib.h>
#include <stdio.h>
#include "tc_image.h"

#ifndef TC_FILTER_H
#define TC_FILTER_H

#define TC_FILTER_NODATA         (999999)
#define TC_FIXEDPT_PRECIS_FACTOR ((pixel_t) 100)
#define TC_FILTER_STRINGSIZE     (32)

#define TC_FILTERSET_POINTS      (0)
#define TC_FILTERSET_RATIOS      (1)
#define TC_FILTERSET_RECTANGLES  (2)
#define TC_FILTERSET_DEFAULT     (0)
extern char* TC_FILTERSET_NAMES[];

/** filter styles */
#define TC_FILTER_RAW           (0)
#define TC_FILTER_SUM           (1)
#define TC_FILTER_DIFF          (2)
#define TC_FILTER_ABS           (3)
#define TC_FILTER_RATIO         (4)
#define TC_FILTER_RECT          (5)
#define TC_FILTER_NFUNCTIONS    (6)
typedef int64_t feature_t;

/**
 * \enum filter_styles_t
 * \brief All of the different tc-like filters we can use.
 *
 * The following filters are available:
 *  - RAW Just the pixel value
 *  - DIFF Difference of two pixels
 *  - ABS Absolute difference of two pixels
 **/

typedef int filter_style;

/**
 * \brief Parameters describing a single tc filter.
 *
 * The key parameters are the following:
 *
 * - rowA, colA, chanA : coordinates of first point relative to center
 * - rowB, colB, chanB : coordinates of second point
 *
 */
typedef struct tc_filter_type
{
    filter_style function;
    int rowA, colA, chanA;
    int rowB, colB, chanB;
} tc_filter;

int tc_init_filter(tc_filter *filter);
int tc_read_filter(tc_filter *filter, void *tc_io);
int tc_write_filter(tc_filter *filter, FILE *file);
int tc_filter_tostring(tc_filter *filter, char *str);
int tc_filter_fromstring(tc_filter *filter, char *str);
int tc_copy_filter(tc_filter *dst, tc_filter *src);
int tc_filter_pixel(tc_filter *filter,
                    tc_image *image,
                    const int r,
                    const int c,
                    feature_t *result);
int tc_randomize_filter(tc_filter *filter,
                        const int chans,
                        const int filterset,
                        const int winsize,
                        const int crosschannel);

#endif
