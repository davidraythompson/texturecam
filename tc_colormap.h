/**
 * \file image.h
 * \brief A simple lightweight image class
 * \author David Ray Thompson
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#ifndef TC_COLORMAP_H
#define TC_COLORMAP_H

typedef struct tc_colormap_type
{
    int nclasses;
    int colordepth;
    pixel_t **colormap;
} tc_colormap;


int tc_init_colormap(tc_colormap **colormap, const int nchans);
int tc_free_colormap(tc_colormap *colormap);
int tc_label_image(tc_image **dst, tc_colormap *map, int **classes);
int tc_find_classes(tc_colormap **map, const char *filename, const int nchans);
int tc_binary_colormap(tc_colormap **map);



#endif
