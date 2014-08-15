/**
 * \file tc_classify.c
 * \brief classify individual pixels of an image using a texton forest
 * \author David Ray Thompson
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.*
 *
 * After training with tctrain we can use the resulting decision forest
 * to classify any new image with the "tcclassify" command.  This
 * performs fast pixel-level texture classification by traversing the
 * trained decision forest for each pixel in the new image, computing
 * features dynamically as needed.
 *
 * Usage: tcclassify <forest.tf> <input.pgm> <output.pgm>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tc_image.h"
#include "tc_filter.h"
#include "tc_tree.h"
#include "tc_forest.h"
#include "tc_classify.h"
#include "tc_io.h"

#ifndef tc_CLASSIFY_C
#define tc_CLASSIFY_C

/* MATLAB "Jet" color palette */
static float interpolate(float v, float y0, float x0, float y1, float x1);
static float base(float v);

int main(int argc, char **argv)
{

    int r, c, ri, ci, b;
    class_t result = 0;
    char *msg = (char *) malloc(sizeof(char) * MAX_STRING);

    tc_class_t *tc_class_opt, tc_class_opt_local;
    char *inname = NULL;
    char *outname = NULL;
    int arg = 0;

    tc_class_opt_local.skip = 1;
    tc_class_opt_local.probname = NULL;
    tc_class_opt_local.forestname = NULL;
    tc_class_opt_local.in = NULL;
    tc_class_opt_local.out = NULL;
    tc_class_opt_local.forest = NULL;
    tc_class_opt_local.colormap = NULL;
    tc_class_opt_local.class_probs = NULL;
    tc_class_opt_local.compute_probs = 0;
    tc_class_opt = &tc_class_opt_local;

    /* parse the commands */
    arg = tc_class_parse( tc_class_opt, argc, argv );
    if( arg == -1 )
    {
        free(msg);
        return(-1);
    }

    inname = argv[arg];
    outname = argv[arg+1];

    /* load decision_forest */
    if (tc_load_forest(&tc_class_opt->forest, &tc_class_opt->colormap,
                       tc_class_opt->forestname) == ERR)
    {
        fprintf(stderr,"Failed to read decision forest %s\r\n",
                tc_class_opt->forestname);
        free(msg);
        return ERR;
    }

    /* load input image */
    snprintf(msg, MAX_STRING, "preproc: Reading image %s\r\n",inname);
    tc_write_log(msg);
    if (tc_read_image(&tc_class_opt->in, inname) == ERR)
    {
        fprintf(stderr,"Failed to read %s\r\n",inname);
        free(msg);
        return ERR;
    }

    /* allocate classes raster to hold result */
    int cols = tc_class_opt->in->cols;
    int rows = tc_class_opt->in->rows;
    int nclasses = tc_class_opt->forest->nclasses;
    int chans = 1;
    if (tc_class_opt->colormap != NULL)
    {
        chans = tc_class_opt->colormap->colordepth;
    }

    if (tc_alloc_image(&tc_class_opt->out, rows, cols, chans) == ERR)
    {
        fprintf(stderr,"Couldn't allocate memory for class image.\r\n");
        free(msg);
        return ERR;
    }

    /* If we're returning the probability map, make space for it.
     * It's an array of size [rows x cols x classes]*/
    if( &tc_class_opt->probname || (tc_class_opt->compute_probs > 0 &&
                                    tc_class_opt->compute_probs < nclasses) )
    {
        tc_class_opt->class_probs = (float *) calloc(cols*rows*nclasses,
                                    sizeof(float));
        if (!tc_class_opt->class_probs)
        {
            fprintf(stderr,"Couldn't allocate memory for probability map.\r\n");
            tc_free_image(tc_class_opt->out);
            free(msg);
            return ERR;
        }
        memset( (char *)tc_class_opt->class_probs, 0,
                cols*rows*nclasses*sizeof(float) );
    }

    /* initialize all pixels */
    for (r = 0; r < rows; r++)
    {
        for (c = 0; c < cols; c++)
        {
            for (b = 0; b < chans; b++)
            {
                tc_set(tc_class_opt->out,r,c,b,UNCLASSIFIED);
            }
        }
    }

    int npixels = rows * cols;

    /* classify all pixels */
    for (r = 0; r < rows; r+=tc_class_opt->skip)
    {
        for (c = 0; c < cols; c+=tc_class_opt->skip)
        {
            /* Only worry about the class probabilities if
             * we plan to output this data.  Otherwise we'll
             * pass a NULL pointer as a signal to ignore... */
            float *cppointer = NULL;
            if (tc_class_opt->class_probs)
            {
                cppointer = &(tc_class_opt->class_probs[r*cols*nclasses+c*nclasses]);
            }

            if (tc_forest_classify(tc_class_opt->forest, tc_class_opt->in, r, c, &result,
                                   cppointer ) == ERR)
            {
                fprintf(stderr,"tc_texture_forest_classify failed on pixel (%i, %i).\r\n", r,
                        c);
                free(msg);
                return -1;
            }

            if (result != ERROR_CLASS)
            {
                // copy the result to all pixels in the subchip
                // Note - this doesn't apply to the probability maps,
                // which will come out incomplete if skip != 1.
                for (ci=0; (ci<tc_class_opt->skip) && ((c+ci)<cols); ci++)
                {
                    for (ri=0; (ri<tc_class_opt->skip) && ((r+ri)<rows); ri++)
                    {
                        if (tc_class_opt->colormap != NULL)
                        {
                            if (tc_class_opt->compute_probs > 0 &&
                                    tc_class_opt->compute_probs < nclasses)
                            {
                                /* class probability values in a MATLAB "Jet" colormap */
                                float gray = cppointer[tc_class_opt->compute_probs];
                                pixel_t MAX_PIXEL = 255; /* problematic, should define elsewhere */
                                tc_set(tc_class_opt->out, r+ri, c+ci, 0, base(gray - 0.5) * MAX_PIXEL);
                                tc_set(tc_class_opt->out, r+ri, c+ci, 1, base(gray) * MAX_PIXEL);
                                tc_set(tc_class_opt->out, r+ri, c+ci, 2, base(gray + 0.5) * MAX_PIXEL);
                            }

                            /* Use the map's colors to label the image */
                            else for (b = 0; b < chans; b++)
                                {
                                    tc_set(tc_class_opt->out, r+ri, c+ci, b,
                                           tc_class_opt->colormap->colormap[result][b]);
                                }
                        }
                        else
                        {
                            tc_set(tc_class_opt->out, r+ri, c+ci, 0,
                                   (pixel_t) result);
                        }
                    }
                }
            }
        }
        /* report progress */
        fprintf(stdout,"\rProgress: %d%%.", (int)((r+1)*cols*100)/npixels);
        fflush(stdout);
    }
    fprintf(stdout,"\r\n");

    /* write image output */
    snprintf(msg, MAX_STRING, "preproc: Writing output image %s\r\n",outname);
    tc_write_log(msg);
    if (tc_write_image(tc_class_opt->out, outname) == ERR)
    {
        fprintf(stderr,"Couldn't write image to %s\r\n",outname);
        free(msg);
        return ERR;
    }

    /* clean up */
    tc_free_image(tc_class_opt->in);
    tc_free_image(tc_class_opt->out);

    tc_free_forest(tc_class_opt->forest);

    if (tc_class_opt->colormap != NULL)
    {
        tc_free_colormap(tc_class_opt->colormap);
    }

    /* write probability map output */
    if (tc_class_opt->probname)
    {
        const char *mode = "wb";
        void *tc_io = tc_init_io(tc_class_opt->probname, mode);

        if (tc_io)
        {
            snprintf(msg, MAX_STRING, "Writing probability map to: %s\r\n",
                     tc_class_opt->probname);
            tc_write_log(msg);
            tc_write_io( tc_io, (char *)tc_class_opt->class_probs,
                         sizeof(float)*rows*cols*nclasses );
            tc_close_io( tc_io );
            tc_write_log( "\r\nDone.\r\n");
        }
        else
        {
            fprintf(stderr,"Couldn't write probability map to %s\r\n",
                    tc_class_opt->probname);
            free(msg);
            return ERR;
        }
    }

    if (tc_class_opt->class_probs)
    {

        free(tc_class_opt->class_probs);
    }

    free(msg);
    return 0;
}

int tc_class_parse( tc_class_t *tc_class_opt, int argc, char **argv )
{

    int arg=1, help=0, additional_args=1, chkval=0;

    /* Parse the command line */
    while (arg < argc && argv[arg][0] == '-')
    {
        switch (argv[arg][1])
        {

        case 'p':
            if ((arg+1)>=argc)
            {
                help=1;
                break;
            }
            tc_class_opt->probname = argv[arg+1];
            arg = arg+1;
            break;

        case 's':
            if ((arg+1)>=argc)
            {
                help=1;
                break;
            }
            chkval = atoi(argv[arg+1]);
            arg = arg+1;
            if (chkval<1 || chkval>32)
            {
                fprintf(stderr,"Subsampling factor out of range.\r\n");
                help=1;
                break;
            }
            tc_class_opt->skip = chkval;
            break;
        case 'c':
            if ((arg+1)>=argc)
            {
                help=1;
                break;
            }
            tc_class_opt->compute_probs = atoi(argv[arg+1]);
            arg = arg+1;
            break;
        default:
            help = 1;
            break;
        }
        arg++;
    }

    additional_args = 3;
    if ((arg+additional_args)>argc || help || argc==1)
    {

        tc_write_log("Usage is tcclass [OPTIONS] <forest.rf> <input.pgm> <output.pgm>\r\n");
        tc_write_log("  OPTIONS must be one of the following:\r\n");
        tc_write_log("  -p <file.dat>      output class probability map\r\n");
        tc_write_log("  -s <int>           subsampling factor (default: 1)\r\n");
        tc_write_log("  -c <int>           compute probabilities\r\n");
        tc_write_log("  -h                 help!\r\n");
        return(-1);
    }
    else
    {
        tc_class_opt->forestname = argv[arg];
        arg++;
    }

    return arg;

}

/* MATLAB "Jet" color palette */
float interpolate(float v, float y0, float x0, float y1, float x1)
{
    return (v-x0)*(y1-y0)/(x1-x0) + y0;
}

float base(float v)
{
    if (v <= -0.75) return 0;
    else if (v <= -0.25) return interpolate(v, 0.0, -0.75, 1.0, -0.25);
    else if (v <= 0.25) return 1.0;
    else if (v <= 0.75) return interpolate(v, 1.0, 0.25, 0.0, 0.75);
    else return 0.0;
}

#endif
