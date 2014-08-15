/**
 * \file tc_bar_fixed.h
 * \brief Oriented bar filters, fixed-point
 * \author David Ray Thompson
 *
 * Copyright 2013, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#ifndef TC_BAR_FIXED_H
#define TC_BAR_FIXED_H

#define   BAR_SCALE_SHIFT      (13)		/* divide by 8192 */

/* Oriented bar filters */

typedef struct tc_bar_s
{
    int norients;
    int nscales;
    int support;
    short* bar;
} tc_bar_t;

tc_bar_t* tc_bar_filter;  /* initialized in tc_prep.c */

#endif
