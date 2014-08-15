/*
 * \file tc_colormap.c
 * \brief A lightweight colormap
 * \author David Ray Thompson, Greydon Foil\n
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "tc_image.h"
#include "tc_colormap.h"

#ifndef TC_COLORMAP_C
#define TC_COLORMAP_C


/* Iterate through the image and convert pixels to their class labels
 * classes[i]=1 if class i is present in the given image.
*/
int tc_label_image(tc_image **dst, tc_colormap *map, int **classes)
{
    tc_image *label;
    int r,c,b, i;
    int *dst_classes = NULL;

    if (map->colordepth > (*dst)->chans)
    {
        tc_write_log("Error: # colormap channels > label image channels.\r\n");
        return ERR;
    }

    dst_classes = (int*) malloc(sizeof(int)*map->nclasses);
    if (dst_classes == NULL)
    {
        tc_write_log("tc_label_image: Unable to allocate memory for dst_classes array.\r\n");
        *classes =  NULL;
        return ERR;
    }

    if (tc_alloc_image(&label, (*dst)->rows, (*dst)->cols, 1) == ERR)
    {
        tc_write_log("Unable to allocate memory for labeled image\r\n");
        free(dst_classes);
        *classes =  NULL;
        return ERR;
    }

    for (i = 0; i < map->nclasses; i++)
    {
        dst_classes[i] = 0;
    }

    for (r = 0; r < (*dst)->rows; r++)
    {
        for (c = 0; c < (*dst)->cols; c++)
        {
            int nomatch = 1;
            /* Iterate through the classes and the channels and try to
             * find matches */
            for (i = 0; i < map->nclasses; i++)
            {
                int classmatch = 1;
                for (b = 0; b < map->colordepth; b++)
                {
                    if (tc_get(*dst, r, c, b) != map->colormap[i][b])
                    {
                        classmatch = 0;
                        break;
                    }
                }

                if (classmatch)
                {
                    tc_set(label, r, c, 0, i);
                    dst_classes[i]++;
                    nomatch = 0;
                    break;
                }
            }

            if (nomatch)
            {
                tc_write_log("tc_label_image: Image color does not match a class.\r\n");
                tc_free_image(label);
                free(dst_classes);
                *classes =  NULL;
                return ERR;
            }
        }
    }

    *classes = dst_classes;

    tc_free_image(*dst);
    *dst = label;
    return OK;
}


int tc_init_colormap(tc_colormap **colormap, const int nchans)
{
    int i;
    tc_colormap *map = (tc_colormap*)malloc(sizeof(tc_colormap));

    if (map == NULL)
    {
        tc_write_log("tc_init_colormap: No memory for color map.\r\n");
        return ERR;
    }

    map->colormap = (pixel_t**) malloc(MAX_N_CLASSES*sizeof(pixel_t*));
    if (map->colormap == NULL)
    {
        tc_write_log("tc_init_colormap: No memory for color map data.\r\n");
        tc_free_colormap(map);
        return ERR;
    }

    for (i = 0; i < MAX_N_CLASSES; i++)
    {
        map->colormap[i] = (pixel_t*) malloc(nchans * sizeof(pixel_t));
        if (map->colormap[i] == NULL)
        {
            tc_write_log("tc_init_colormap: No memory for class data.\r\n");
            tc_free_colormap(map);
            return ERR;
        }
    }

    /* Always add a null/unlabeled class */
    for (i = 0; i < nchans; i++)
    {
        map->colormap[0][i] = 0;
    }

    map->colordepth = nchans;
    map->nclasses = 1;
    *colormap = map;

    return OK;
}


int tc_free_colormap(tc_colormap *map)
{
    int i;
    if (map == NULL)
    {
        tc_write_log("tc_free_colormap: NULL pointer.\r\n");
        return ERR;
    }
    if (map->colormap == NULL)
    {
        tc_write_log("tc_free_colormap: NULL colormap.\r\n");
        free(map);
        return ERR;
    }

    for (i = 0; i < MAX_N_CLASSES; i++)
    {
        if (map->colormap[i])
        {
            free(map->colormap[i]);
        }
    }
    free(map->colormap);
    free(map);
    return OK;
}


/* Search through the given label image and add any new colors to
 * the color map. Classes become the index of the color in the
 * final colormap (0 through MAX_N_CLASSES).
 */
int tc_find_classes(tc_colormap **map, const char *filename, int colorchans)
{

    tc_image *label;
    int r, c, b, i;

    if (tc_read_image(&label, filename) == ERR)
    {
        tc_write_log("Could not load label image.\r\n");
        return ERR;
    }

    if (label->chans < colorchans)
    {
        printf("Error: label image %s not an RGB image\n", filename);
        return ERR;
    }
    else if (label->chans > colorchans)
    {
        printf("Error: mismatch in number of channels in label image vs. colormap\n");
        printf("Did you forget to add the --colorlabels option?\n");
        return ERR;
    }

    if (*map == NULL)
    {
        if (tc_init_colormap(map, colorchans) == ERR)
        {
            tc_write_log("tc_find_colormap: initialization fails\r\n");
            tc_free_image(label);
            return ERR;
        }
    }
    printf("Read label image, %d x %d, with %d channels.\n",
           label->rows, label->cols, label->chans);

    /* find any pixels that don't match the current class labels
     * and add them */
    for (r = 0; r < label->rows; r++)
    {
        for (c = 0; c < label->cols; c++)
        {
            if ((*map)->nclasses == 0)
            {
                for (b = 0; b < colorchans; b++)
                {
                    (*map)->colormap[(*map)->nclasses][b] =
                        tc_get(label, r, c, b);
                }
                (*map)->nclasses++;
            }
            else
            {
                /* Iterate through the labels and the channels
                 * and try to find matches */
                int labelmatch = 0;
                for (i = 0; i < (*map)->nclasses; i++)
                {
                    int localmatch = 1;
                    for (b = 0; b < colorchans; b++)
                    {
                        if (tc_get(label, r, c, b) != (*map)->colormap[i][b])
                        {
                            localmatch = 0;
                        }
                    }

                    /* If this isn't an observed label, add it to the list */
                    if (localmatch)
                    {
                        labelmatch = 1;
                    }
                }

                /* Add a new label if we haven't seen this color before */
                if (!labelmatch)
                {
                    if ((*map)->nclasses >= MAX_N_CLASSES)
                    {
                        tc_write_log("Max number of classes exceeded.\r\n");
                        tc_write_log("Is there a problem with your labels?\r\n");
                        tc_free_image(label);
                        tc_free_colormap(*map);
                        return ERR;
                    }
                    else
                    {
                        for (b = 0; b < colorchans; b++)
                        {
                            (*map)->colormap[(*map)->nclasses][b] =
                                tc_get(label, r, c, b);
                        }

                        (*map)->nclasses++;
                        i = (*map)->nclasses;
                    }
                }
            }
        }
    }

    tc_free_image(label);
    return OK;
}



/* Initialize a simple colormap for binary classification, using red and
 * blue labels.
 */
int tc_binary_colormap(tc_colormap **map)
{
    if (*map == NULL)
    {
        /* Assume a three channel label image */
        if (tc_init_colormap(map,3) == ERR)
        {
            tc_write_log("tc_binary_colormap: initialization fails\r\n");
            return ERR;
        }
    }
    else
    {
        tc_write_log("tc_binary_colormap: cannot initialize null map\r\n");
        return ERR;
    }

    /* Class 1 is the background (blue) */
    (*map)->colormap[1][0] = 0;
    (*map)->colormap[1][1] = 0;
    (*map)->colormap[1][2] = 255;

    /* Class 2 is the foreground (red) */
    (*map)->colormap[2][0] = 255;
    (*map)->colormap[2][1] = 0;
    (*map)->colormap[2][2] = 0;

    (*map)->colordepth = 3;
    (*map)->nclasses = 3;
    return OK;
}
#endif

