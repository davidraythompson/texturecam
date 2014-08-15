/*
 * \file tc_texture_forest.c
 * \brief Simple image pixel classifiers.
 * \author David Ray Thompson \n
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 *
 * This file describes a decision forest that operates on a single pixel to
 * produce a classification or regression result using the output of local
 * filters.  Each node is either a (binary) branch, a classification decision,
 * or a regression result.
 *
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "tc_image.h"
#include "tc_colormap.h"
#include "tc_filter.h"
#include "tc_node.h"
#include "tc_tree.h"
#include "tc_forest.h"
#include "tc_io.h"

#ifndef tc_texture_forest_C
#define tc_texture_forest_C




/**
 * Classify each pixel of an input image, fill the output "classes"
 * image with the resulting classification values.
 * The array class_probs should have size MAX_N_CLASSES
 */
int tc_forest_classify(tc_forest *forest,
                       tc_image *image,
                       const int r,
                       const int c,
                       class_t *pixel_class,
                       float *class_probs_out)
{
    float best_prob = -1.0;
    float class_probs[MAX_N_CLASSES];
    int i, t;

    if (forest == NULL || image == NULL || pixel_class == NULL)
    {
        tc_write_log("tc_classify: NULL parameter\r\n");
        return ERR;
    }

    for (i=0; i<forest->nclasses; i++)
    {
        class_probs[i] = 0.0;
    }

    for (t = 0; t < forest->ntrees; t++)
    {

        /* start at the root node of this tree */
        tc_tree *tree = &(forest->trees[t]);
        tc_node *node, *root = &(tree->nodes[0]);
        feature_t result;

        if (tree == NULL)
        {
            return ERR;
        }

        node = root;
        while (1)
        {

            /* check for termination */
            if (tc_isleaf(node))
            {
                *pixel_class = (class_t) node->MAP_class;
                for (i=0; i<forest->nclasses; i++)
                {
                    class_probs[i] += node->class_probs[i];
                }
                break;
            }

            /* it's a decision branch. */
            if (tc_filter_pixel(&(node->filter), image, r, c, &result) == ERR)
            {
                /* any filter failure causes ERROR_CLASS to be returned */
                *pixel_class = ERROR_CLASS;
                return OK;
            }
            else if (result > node->threshold)
            {
                node = node->high;
            }
            else
            {
                node = node->low;
            }
        }
    }

    /* compute MAP classification */
    (*pixel_class) = (ERROR_CLASS);
    for (i=0; i<forest->nclasses; i++)
    {
        if ((class_probs[i]>0) && (class_probs[i] > MIN_PROB))
        {
            class_probs[i] /= forest->ntrees;
            if (class_probs[i] > best_prob)
            {
                best_prob = class_probs[i];
                (*pixel_class) = (class_t) i;
            }
        }
    }

    /* output class prob map if needed */
    if (class_probs_out)
    {
        for (i=0; i<forest->nclasses; i++)
        {
            class_probs_out[i] = class_probs[i];
        }
    }
    return OK;
}


/* allocate a pixel forest with the specified number of trees */
int tc_init_forest(tc_forest **forest, int ntrees,
                   int filterset, int nclasses, int winsize)
{
    int i;
    tc_forest *f = (tc_forest *) malloc(sizeof(tc_forest));
    if (f == NULL)
    {
        (*forest) = NULL;
        return ERR;
    }

    f->ntrees = ntrees;
    f->nclasses = nclasses;
    f->winsize  = winsize;
    f->filterset = filterset;
    f->trees = (tc_tree *) malloc(sizeof(tc_tree) * ntrees);
    if (f->trees == NULL)
    {
        tc_write_log("Out of memory in init_forest\r\n");
        free(f->trees);
        free(f);
        (*forest) = NULL;
        return ERR;
    }

    for (i=0; i<ntrees; i++)
    {
        tc_tree *tree = &(f->trees[i]);
        if (tc_init_tree(tree) == ERR)
        {
            tc_write_log("Couldn't initialize tree\r\n");
            free(f->trees);
            free(f);
            (*forest) = NULL;
            return ERR;
        }
    }
    (*forest) = f;
    return OK;
}


int tc_free_forest(tc_forest *forest)
{
    if (forest == NULL)
    {
        return ERR;
    }
    if (forest->trees != NULL)
    {
        free(forest->trees);
    }
    free(forest);
    return OK;
}


/** Use a file to initialize a new forest*/
int tc_load_forest(tc_forest **forest, tc_colormap **map, char *filename)
{
    const char *mode = "rb";
    if (filename == NULL)
    {
        return ERR;
    }

    void *tc_io = tc_init_io(filename, mode);
    if(tc_io == NULL)
    {
        tc_write_log("Tc_texture_forest_read_file:\r\n");
        char *msg = (char *) malloc(MAX_STRING * sizeof(char));
        snprintf(msg, MAX_STRING,"Can't read forest from %s.\r\n", filename);
        tc_write_log(msg);
        free(msg);
        return ERR;
    }
    else if (tc_read_forest(forest, map, tc_io) == ERR)
    {
        tc_close_io(tc_io);
        return ERR;
    }
    tc_close_io(tc_io);
    return OK;
}


/** Safely read a pixel forest from an I/O stream. */
int tc_read_forest(tc_forest **forest, tc_colormap **map, void *tc_io)
{
    int i, j, ntrees, filterset, nclasses, winsize, my_index, colordepth, tmp;
    char* buffer = (char *) malloc(BUF_SIZE * sizeof(char));
    char delim = '\n';
    int status = OK;

    /* Read header information. */
    status = tc_getline_io(tc_io, buffer, BUF_SIZE, delim);
    if (status == ERR
            || sscanf(buffer,"forest %i %i %i %i\n", &ntrees, &filterset, &nclasses,
                      &winsize) != 4)
    {
        tc_write_log("tc_read_forest: syntax error in header.\r\n");
        free(buffer);
        return ERR;
    }
    if (tc_init_forest(forest, ntrees, filterset, nclasses, winsize) == ERR)
    {
        tc_write_log("tc_read_forest: failed to initialize forest.\r\n");
        free(buffer);
        return ERR;
    }
    /* read trees one at a time */
    for (i=0; i<((*forest)->ntrees); i++)
    {
        /* Read line (eat a line).*/
        status = tc_getline_io(tc_io, buffer, BUF_SIZE, delim);
        /* Read line. */
        status = status & tc_getline_io(tc_io, buffer, BUF_SIZE, delim);
        if (status == ERR ||
                (sscanf(buffer,"tree %d\n",&(my_index)) != 1)  ||
                (i != my_index) ||
                (tc_read_tree(&((*forest)->trees[i]), tc_io,
                              (*forest)->nclasses) == ERR))
        {
            tc_write_log("tc_read_forest: syntax error in file.\r\n");
            free(buffer);
            return ERR;
        }
    }

    /* Populate the colormap if the file has that information */
    status = tc_getline_io(tc_io, buffer, BUF_SIZE, '\n'); // Eat empty line.
    status = status & tc_getline_io(tc_io, buffer, BUF_SIZE, '\n');
    if (status == OK && sscanf(buffer,"colormap %i\n", &colordepth) == 1)
    {
        tc_init_colormap(map, colordepth);

        for (i=0; i<((*forest)->nclasses); i++)
        {
            for (j = 0; j < colordepth; j++)
            {
                status = tc_getline_io(tc_io, buffer, BUF_SIZE, ' ');
                if (status == ERR || sscanf(buffer,"%i ",&tmp) != 1)
                {
                    tc_write_log("tc_read_forest: syntax error in file.\r\n");
                    free(buffer);
                    return ERR;
                }
                else
                {
                    (*map)->colormap[i][j] = tmp;
                }
            }

            /* Make sure there IS an empty space at end of line. */
            if (tc_getline_io(tc_io, buffer, BUF_SIZE, '\n') == ERR || buffer[0] != '\0' )
            {
                tc_write_log("tc_read_forest: syntax error in file.\r\n");
                free(buffer);
                return ERR;
            }
            (*map)->nclasses++;
        }
    }

    free(buffer);
    return OK;
}



/** Safely write a pixel forest to a file. */
int tc_save_forest(tc_forest *forest, char *filename, tc_colormap *colormap)
{
    FILE *file;
    if (forest == NULL || filename == NULL)
    {
        tc_write_log("tc_save_forest: NULL parameter\n");
        return ERR;
    }
    if ((file = fopen(filename, "wb")) == NULL)
    {
        tc_write_log("tc_write_forest: Can't write to desired file.\n");
        return ERR;
    }
    else if (tc_write_forest(forest, file, colormap) == ERR)
    {
        fclose(file);
        return ERR;
    }
    fclose(file);
    return OK;
}



/** Safely write a pixel forest to a stream. */
int tc_write_forest(tc_forest *forest, FILE *file, tc_colormap *colormap)
{
    int i,j;
    if (forest == NULL || file == NULL)
    {
        tc_write_log("tc_write_forest: NULL parameter\n");
        return ERR;
    }

    fprintf(file,"forest %i %i %i %i\n", (int) forest->ntrees,
            (int) forest->filterset,
            (int) forest->nclasses, (int) forest->winsize);
    for (i=0; i<forest->ntrees; i++)
    {
        fprintf(file,"\ntree %d\n", i);
        if (tc_write_tree(&(forest->trees[i]), file, forest->nclasses) == ERR)
        {
            return ERR;
        }
    }

    if (colormap != NULL)
    {
        fprintf(file,"\ncolormap %d\n", colormap->colordepth);
        for (i=0; i<forest->nclasses; i++)
        {
            for (j = 0; j < colormap->colordepth; j++)
            {
                fprintf(file, "%d ", (int)colormap->colormap[i][j]);
            }
            fprintf(file, "\n");
        }
    }
    tc_write_log("Done writing forest.\n");
    return OK;
}


#endif

