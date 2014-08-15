/**
 * \file tc_classify.c
 * \brief Header for tc_classify
 * \author Dmitriy Bekker
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#ifndef TC_CLASSIFY_H_
#define TC_CLASSIFY_H_

#include "tc_image.h"
#include "tc_forest.h"
#include "tc_colormap.h"

typedef struct tc_class_s
{
    int skip;
    char *probname;
    char *forestname;
    tc_image *in;
    tc_image *out;
    tc_forest *forest;
    tc_colormap *colormap;
    float *class_probs;
#ifdef XILINX_PROC
    int probeth;
#endif
    int compute_probs;
} tc_class_t;

int tc_class_parse( tc_class_t *tc_class_opt, int argc, char **argv );

#if defined(XILINX_PROC) || defined(GSE_VIEW)
int tc_class( tc_class_t *tc_class_opt );
#endif

#endif /* TC_CLASSIFY_H_ */
