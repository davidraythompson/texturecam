/**
 * \file tc_texture_forest.h
 * \brief Simple tc_pixel forests.
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
#include "tc_filter.h"
#include "tc_node.h"
#include "tc_tree.h"

#ifndef tc_forest_H
#define tc_forest_H


/**
 * \brief A list of classifier trees.
 */
typedef struct tc_forest_type
{
    tc_tree *trees;
    int ntrees;
    int nclasses;
    int filterset;
    int winsize;
    pixel_t **colormap;
} tc_forest;

int tc_init_forest(tc_forest **forest, const int ntrees,
                   int filterset, int nclasses, int winsize);

int tc_free_forest(tc_forest *forest);

/* Write to IO stream */
int tc_write_forest(tc_forest *forest, FILE *file, tc_colormap *colormap);

/* Read from IO stream */
int tc_read_forest(tc_forest **forest, tc_colormap **map, void *tc_io);

/* Read from a file at the given path, and initialize */
int tc_load_forest(tc_forest **forest, tc_colormap **map, char *filename);

/* Write to a file at the given path */
int tc_save_forest(tc_forest *forest, char *filename, tc_colormap *colormap);

/* Feturn the class probability distribution and MAP class */
int tc_forest_classify(tc_forest *forest, tc_image *image,
                       const int r, const int c, class_t *pixel_class,
                       float *class_probs);

#endif
