/**
 * \file tc_prep.c
 * \brief Header for tc_prep
 * \author Dmitriy Bekker
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#ifndef TC_PREP_H_
#define TC_PREP_H_

#include "tc_image.h"

#define TC_PREP_GREYWORLD        (0)
#define TC_PREP_BANDPASS         (1)
#define TC_PREP_BANDPASS_OCTAVES (2)
#define TC_PREP_HSV              (3)
#define TC_PREP_INTENSITY        (4)
#define TC_PREP_IPEX             (5)
#define TC_PREP_TEXTURECAM       (6)
#define TC_PREP_GREY2RGB         (7)
#define TC_PREP_BAR              (8)
#define TC_PREP_BARHSV           (9)
#define TC_PREP_NONE             (10)

typedef struct tc_prep_s
{
    int method;
    int outchans;
    char *ffname;
    tc_image *in;
    tc_image *out;
    tc_image *intens;
    tc_image *scratch;
    tc_image *scratchB;
    tc_image *ff;
    int bandpass_filter_small;
    int bandpass_filter_big;
    /* Bar filters: currently, norients and nscales are fixed,
     * but support can be specified on the command line.
     * See tc_bar_fixed.h and export_filters_binary.m . */
    int bar_filter_norients;
    int bar_filter_nscales;
    int bar_filter_support;
} tc_prep_t;

int  tc_prep_parse( tc_prep_t *tc_prep_opt, int argc, char **argv );
void tc_prep_free_intens_io( tc_prep_t *tc_prep_opt );

#if defined(XILINX_PROC) || defined(GSE_VIEW)
int tc_prep( tc_prep_t *tc_prep_opt );
#endif

#endif /* TC_PREP_H_ */
