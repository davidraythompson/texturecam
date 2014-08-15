/**
 * \file tc_catpgm.h
 * \brief Concatenate channels of a PGM into a multi-channel PGM image
 * \author David Ray Thompson
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tc_image.h"

int main(int argc, char **argv)
{

    int i, j, b, r, c, curchan, nimages, arg=1;
    int totchans = 0;
    char *outname = NULL;
    pixel_t val;
    tc_image **in = NULL, *out = NULL;

    /* Parse the command line */
    while ((arg < argc) && (argv[arg][0] == '-'))
    {
        if (strcmp(argv[arg],"-o") == 0)
        {
            arg++;
            outname = argv[arg];
            fprintf(stderr,"output to: %s\n", argv[arg]);
        }
        else
        {
            fprintf(stderr,"Unrecognized switch: %s\n", argv[arg]);
        }
        arg++;

    }
    nimages = argc - arg;
    fprintf(stderr,"%i input images.\n", nimages);
    if ((nimages < 2) || (outname == NULL))
    {
        fprintf(stderr,
                "Usage: catpgm -o <output.pgm> <input1.pgm> <input2.pgm> ...\n");
        exit(-1);
    }

    in = malloc(sizeof(tc_image *) * nimages);

    /* Read each image sequentially, checking integrity as we go */
    for (i=0; i<nimages; i++)
    {
        tc_read_image(&(in[i]), argv[arg+i]);
        totchans += in[i]->chans;

        if ((&(in[i]) == NULL) ||
                ((i>0) && (in[i]->rows != in[0]->rows)) ||
                ((i>0) && (in[i]->cols != in[0]->cols)))
        {
            fprintf(stderr,"Out of memory, or dimensions don't match!\n");
            for (j=0; j<=i; j++)
            {
                tc_free_image((in[i])); /* Free old images */
            }
            free(in);
            exit(-1);
        }
    }

    /* Allocate the output image */
    tc_alloc_image(&out, in[0]->rows, in[0]->cols, totchans);
    if (out == NULL)
    {
        fprintf(stderr,"Out of memory!\n");
        for (i=0; i<nimages; i++)
        {
            tc_free_image((in[i])); /* Free old images */
        }
        free(in);
        exit(-1);
    }

    /* Copy each pixel into the new channel stack */
    curchan = 0;
    for (i=0; i<nimages; i++)
    {
        for (b=0; b<in[i]->chans; b++)
        {
            for (r=0; r<in[0]->rows; r++)
            {
                for (c=0; c<in[0]->cols; c++)
                {
                    val = tc_get(in[i],r,c,b);
                    tc_set(out,r,c,curchan,val);
                }
            }
        }
        curchan++;
    }

    /* Write output */
    tc_write_image(out, outname);

    /* Clean up */
    for (i=0; i<nimages; i++)
    {
        tc_free_image(in[i]);
    }
    free(in);
    tc_free_image(out);

    return 0;
}

