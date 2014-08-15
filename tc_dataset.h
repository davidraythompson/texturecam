/**
 * \file tc_datum.h
 * \brief A training datapoint
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
#include "tc_colormap.h"

#ifndef TC_DATASET_H
#define TC_DATASET_H

#define MAX_N_IMAGES             (512)
#define ERROR_CLASS              (255)
#define UNCLASSIFIED             (0)

#define TC_RANDOM_SAMPLING     (0)
#define TC_BALANCED_SAMPLING   (1)



typedef int class_t;

typedef struct tc_datum_type
{
    int image, r, c, label;
    struct tc_datum_type *next; /* next datum in subset */
} tc_datum;


typedef struct tc_dataset_type
{
    int ndata;
    int nimages;
    int nclasses;
    tc_datum *data; /* array of training data, size ndata */
    tc_image *images[MAX_N_IMAGES];
    tc_image *labels[MAX_N_IMAGES];
    int represented[MAX_N_CLASSES];
    int *classes[MAX_N_IMAGES]; /* classes[i][j] -> # of class j \in image i */
} tc_dataset;


int tc_init_datum(tc_datum *datum);
int tc_init_dataset(tc_dataset *dataset);
int tc_free_dataset(tc_dataset *dataset);
int tc_random_dataset(tc_dataset **d,
                      char **image_filenames,
                      char **label_filenames,
                      tc_colormap *label_colormap,
                      int nimages,
                      int ndata,
                      int sampling_method,
                      long int seed);
#endif

