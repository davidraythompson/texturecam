/**
 * \file tc_datum.c
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
#include <time.h>
#include "tc_dataset.h"
#include "tc_image.h"
#include "tc_colormap.h"

#ifndef TC_DATASET_C
#define TC_DATASET_C


int tc_init_datum(tc_datum *datum)
{
    if (datum == NULL) return ERR;
    datum->image = 0;
    datum->r = 0;
    datum->c = 0;
    datum->label = -1;
    datum->next = NULL;
    return OK;
}


int tc_init_dataset(tc_dataset *dataset)
{
    int i;
    if (dataset == NULL) return ERR;
    dataset->ndata = 0;
    dataset->data = NULL;
    dataset->nimages = 0;
    for (i=0; i<MAX_N_IMAGES; i++)
    {
        dataset->images[i] = NULL;
        dataset->labels[i] = NULL;
        dataset->classes[i] = NULL;
    }
    for (i=0; i<MAX_N_CLASSES; i++)
    {
        dataset->represented[i] = 0;
    }
    dataset->nclasses = 1;
    return OK;
}


int tc_free_dataset(tc_dataset *dataset)
{
    int i;
    if (dataset == NULL) return ERR;

    for (i=0; i<dataset->nimages; i++)
    {
        if (dataset->images[i] != NULL)
        {
            tc_free_image(dataset->images[i]);
        }
        dataset->images[i] = NULL;
    }
    for (i=0; i<dataset->nimages; i++)
    {
        if (dataset->labels[i] != NULL)
        {
            tc_free_image(dataset->labels[i]);
        }
        dataset->labels[i] = NULL;

        if (dataset->classes[i] != NULL)
        {
            free(dataset->classes[i]);
        }
        dataset->classes[i] = NULL;
    }
    if (dataset->data != NULL) free(dataset->data);
    free(dataset);
    return OK;
}


int tc_random_dataset(tc_dataset **d,
                      char **image_filenames,
                      char **label_filenames,
                      tc_colormap *label_colormap,
                      int nimages,
                      int ndata,
                      int sampling_method,
                      long int seed)
{
    srand(seed);
    tc_datum *datum;
    int i, j, current_label=1;

    if (d == NULL || image_filenames == NULL)
    {
        return ERR;
    }

    /* read images */
    tc_dataset *dataset;
    dataset = (tc_dataset *) malloc(sizeof(tc_dataset));
    if (dataset == NULL)
    {
        tc_write_log("Out of memory in random_dataset\n");
        *d = NULL;
        return ERR;
    }
    tc_init_dataset(dataset);

    dataset->nimages = nimages;

    /* read images */
    for (i=0; i<nimages; i++)
    {
        if (label_filenames == NULL)
        {
            tc_free_dataset(dataset);
            *d = NULL;
            return ERR;
        }

        tc_read_image(&(dataset->images[i]), image_filenames[i]);
        tc_read_image(&(dataset->labels[i]), label_filenames[i]);

        /* Change the values of the image if we're relabeling pixels
             * as classes (this reallocates labels[i] to have one channel) */
        if (label_colormap != NULL)
        {
            if (tc_label_image(&(dataset->labels[i]), label_colormap,
                               &(dataset->classes[i])) == ERR)
            {
                tc_free_dataset(dataset);
                *d = NULL;
                return ERR;
            }
            fprintf(stderr,"image %d contains:\n",i);
            for (j=0; j<label_colormap->nclasses; j++)
            {
                if (dataset->classes[i][j]>0)
                {
                    fprintf(stderr, "%d instances of class %d\n",
                            dataset->classes[i][j],j);
                }
            }
        }

        if (dataset->labels[i]->chans > 1)
        {
            tc_write_log("Use '--colorlabels' with multi-channel labels\n");
            tc_free_dataset(dataset);
            *d = NULL;
            return ERR;
        }
    }



    /* set up datasets */
    dataset->ndata = ndata;
    dataset->data = (tc_datum *) malloc(sizeof(tc_datum) * ndata);

    srand((uint64_t) NULL);
    for (i=0; i<ndata; i++)
    {
        datum = &(dataset->data[i]);
        int r, c, image, label = 0;

        while (!label)
        {
            /* look for a classified pixel */
            image = rand() % nimages;
            if (sampling_method == TC_BALANCED_SAMPLING &&
                    dataset->classes[image][current_label]==0)
            {
                /* image does not contain this class, skip if balancing */
                continue;
            }

            r = rand() % dataset->images[image]->rows;
            c = rand() % dataset->images[image]->cols;
            label = tc_get(dataset->labels[image], r, c, 0);

            if (label >= MAX_N_CLASSES ||
                    (sampling_method == TC_BALANCED_SAMPLING &&
                     label != current_label) )
            {
                //fprintf(stderr, "Label image value %i > %d, ignoring.\n",
                //         label, MAX_N_CLASSES);
                label = 0;
            }
        }

        datum->image = image;
        datum->r = r;
        datum->c = c;
        datum->label = label;
        dataset->represented[label]++;

        if (dataset->represented[label]==1)
        {
            fprintf(stderr, "new class %i at (%i,%i).\n",
                    label, r, c);
        }
        if ((datum->label+1) > (dataset->nclasses))
        {
            dataset->nclasses = (datum->label + 1);
        }

        if (i==ndata-1)
        {
            datum->next = NULL;
        }
        else
        {
            datum->next = &(dataset->data[i+1]);
        }

        if (sampling_method == TC_BALANCED_SAMPLING)
        {
            /* update current class \in [1,nclasses] */
            current_label = ((current_label+1) % (label_colormap->nclasses));
            if (current_label==0) current_label++; /* skip unlabeled */
        }
    }

    fprintf(stderr, "%i classes in dataset.\n", dataset->nclasses);
    for (i=1; i<dataset->nclasses; i++)
    {
        fprintf(stderr, "class %d (%d total samples)\n",i,dataset->represented[i]);
    }

    *d = dataset;

    return OK;
}



#endif

