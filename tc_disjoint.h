/**
 * \file disjoint.h
 * \author David Ray Thompson\n
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#include <stdio.h>
#include <string.h>
#include "tc_image.h"

#ifndef TC_DISJOINT_H
#define TC_DISJOINT_H


typedef struct tc_disjoint_type
{
    int *rank;
    int *parent;
    int nlabels;
} tc_disjoint;


int tc_init_disjoint(tc_disjoint **d, int nlabels);
void tc_free_disjoint(tc_disjoint *d);
int  tc_find_disjoint(tc_disjoint *d, int i);
void tc_merge_disjoint(tc_disjoint *d, int i, int j);

#endif
