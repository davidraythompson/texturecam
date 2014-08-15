/**
 * \file tc_prep.c
 * \brief Preprocess with colorspace, normalization, convolution
 * \author David Ray Thompson
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 *
 * This includes some code from RIT that was posted to:
 *  http://www.cs.rit.edu/~ncs/color/t_convert.html
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "tc_image.h"
#include "tc_bar_fixed.h"
#include "tc_preproc.h"
#include "tc_prep.h"

const int bpbig[3]= {11,33,99};
const int bpsmall[3]= {0,3,9};

int main(int argc, char **argv)
{

    float maxpx = 255.0;
    int r, c, octave=0;
    pixel_t tmu = 128;

    tc_prep_t *tc_prep_opt, tc_prep_opt_local;
    char *inname = NULL;
    char *msg = malloc(sizeof(char) * MAX_STRING);
    char *outname = NULL;
    int arg = 0;

    tc_prep_opt_local.method = TC_PREP_BANDPASS;
    tc_prep_opt_local.outchans = 1;
    tc_prep_opt_local.ffname = NULL;
    tc_prep_opt_local.in = NULL;
    tc_prep_opt_local.out = NULL;
    tc_prep_opt_local.intens = NULL;
    tc_prep_opt_local.scratch = NULL;
    tc_prep_opt_local.scratchB = NULL;
    tc_prep_opt_local.ff = NULL;
    tc_prep_opt_local.bandpass_filter_small = 3;
    tc_prep_opt_local.bandpass_filter_big   = 11;
    tc_prep_opt = &tc_prep_opt_local;

    tc_bar_filter = (tc_bar_t*)malloc(sizeof(tc_bar_t));
    if (tc_bar_filter == NULL)
    {
        tc_write_log("preproc: could not allocate memory for bar filter struct. \r\n");
        free(msg);
        return(-1);
    }

    tc_bar_filter->nscales  = 3;
    tc_bar_filter->norients = 8;
    tc_bar_filter->support  = 19;

    tc_write_log("preproc: Starting.\r\n");

    /* parse the commands */
    arg = tc_prep_parse( tc_prep_opt, argc, argv );
    if( arg == -1 )
    {
        free(msg);
        return(-1);
    }

    inname = argv[arg];
    outname = argv[arg+1];

    /* Read each image sequentially, checking integrity as we go */
    snprintf(msg,MAX_STRING,"preproc: Reading image %s\r\n",inname);
    tc_write_log(msg);

    tc_read_image(&tc_prep_opt->in, inname);
    if (tc_prep_opt->in == NULL)
    {
        tc_write_log("preproc: Couldn't read image.\r\n");
        free(msg);
        return(-1);
    }
    snprintf(msg,MAX_STRING,
             "preproc: Input image is %d x %d pixels x %d channels\r\n",
             tc_prep_opt->in->cols, tc_prep_opt->in->rows,
             tc_prep_opt->in->chans);
    tc_write_log(msg);

    /* Propagate the number of channels, if no processing to be done */
    if (tc_prep_opt->outchans == -1)
        tc_prep_opt->outchans = tc_prep_opt->in->chans;

    /* Allocate the output image */
    tc_alloc_image(&tc_prep_opt->out,
                   tc_prep_opt->in->rows, tc_prep_opt->in->cols,
                   tc_prep_opt->outchans);
    if (tc_prep_opt->out == NULL)
    {
        tc_write_log("preproc: Out of memory!\r\n");
        tc_free_image(tc_prep_opt->in);
        free(msg);
        return(-1);
    }

    /* Allocate the intensity image */
    tc_alloc_image(&tc_prep_opt->intens,
                   tc_prep_opt->in->rows, tc_prep_opt->in->cols, 1);
    if (tc_prep_opt->intens == NULL)
    {
        tc_write_log("preproc: Out of memory!\r\n");
        tc_free_image(tc_prep_opt->in);
        tc_free_image(tc_prep_opt->out);
        free(msg);
        return(-1);
    }

    /* convert intensity */
    if (tc_intensity(tc_prep_opt->intens, tc_prep_opt->in) == ERR)
    {
        tc_write_log("preproc: intensity conversion failed\r\n");
        tc_prep_free_intens_io( tc_prep_opt );
        free(msg);
        return(-1);
    }

    /* load bar filter info if needed */
    if (tc_prep_opt->method == TC_PREP_BAR ||
            tc_prep_opt->method == TC_PREP_BARHSV)
    {
        char* bar_fname = malloc(sizeof(char) * MAX_STRING);
        FILE* fin;
        int nvals = tc_bar_filter->norients * tc_bar_filter->nscales *
                    tc_bar_filter->support * tc_bar_filter->support;

        /* Read in the bar filter info from a file */
        snprintf(bar_fname, MAX_STRING,"bar_%04i.dat", tc_bar_filter->support);
        fin = fopen(bar_fname, "rb");
        free(bar_fname);
        if (fin == NULL)
        {
            tc_write_log("Could not open bar filter file; check if it needs to be generated.\r\n");
            free(msg);
            return(-1);
        }
        tc_bar_filter->bar = (short*)malloc(sizeof(short*) * nvals);
        if (tc_bar_filter->bar == NULL)
        {
            tc_write_log("preproc: could not allocate memory for bar filter. \r\n");
            free(msg);
            return(-1);
        }
        if (fread(tc_bar_filter->bar, sizeof(short), nvals, fin) != nvals)
        {
            tc_write_log("preproc: bar filter read failed \r\n");
            free(msg);
            return(-1);
        }
        fclose(fin);
    }

    switch (tc_prep_opt->method)
    {
    case TC_PREP_GREYWORLD:

        tc_write_log("preproc: normalizing intensity tc_prep_opt->in each channel.\r\n");
        tc_greyworld(tc_prep_opt->out, tc_prep_opt->in, tmu);
        break;

    case TC_PREP_BANDPASS:

        tc_write_log("preproc: bandpass filter\r\n");
        tc_bandpass_image(tc_prep_opt->out, tc_prep_opt->intens,
                          tc_prep_opt->bandpass_filter_big, tc_prep_opt->bandpass_filter_small, tmu);
        break;

    case TC_PREP_BANDPASS_OCTAVES:

        tc_alloc_image(&tc_prep_opt->scratch, tc_prep_opt->in->rows,
                       tc_prep_opt->in->cols, 1);
        if (tc_prep_opt->scratch == NULL)
        {
            tc_write_log("preproc: Out of memory!\r\n");
            tc_prep_free_intens_io( tc_prep_opt );
            free(msg);
            return(-1);
        }
        for (octave=0; octave<3; octave++)
        {
            if (tc_bandpass_image(tc_prep_opt->scratch, tc_prep_opt->intens, bpbig[octave],
                                  bpsmall[octave],tmu) == ERR)
            {
                tc_write_log("preproc: intensity conversion failed\r\n");
                tc_free_image(tc_prep_opt->scratch);
                tc_prep_free_intens_io( tc_prep_opt );
                free(msg);
                return(-1);
            }

            /* Copy the result into the appropriate output channel */
            for (r=0; r<tc_prep_opt->in->rows; r++)
            {
                for (c=0; c<tc_prep_opt->in->cols; c++)
                {
                    tc_set(tc_prep_opt->out,r,c,octave,tc_get(tc_prep_opt->scratch,r,c,0));
                }
            }
        }
        tc_free_image(tc_prep_opt->scratch);
        break;

    case TC_PREP_HSV:

        if (tc_rgbhsv(tc_prep_opt->out, tc_prep_opt->in, maxpx) == ERR)
        {
            tc_write_log("preproc: hsv conversion failed\r\n");
            tc_prep_free_intens_io( tc_prep_opt );
            free(msg);
            return(-1);
        }
        break;

    case TC_PREP_TEXTURECAM:

        if (tc_read_image(&tc_prep_opt->ff, tc_prep_opt->ffname) == ERR);
        {
            tc_write_log("preproc: could not find flat field image.\r\n");
            tc_prep_free_intens_io( tc_prep_opt );
            free(msg);
            return(-1);
        }
        if (tc_flatfield_image(tc_prep_opt->scratch, tc_prep_opt->in,
                               tc_prep_opt->ff) == ERR)
            for (r=0; r<tc_prep_opt->in->rows; r++)
            {
                for (c=0; c<tc_prep_opt->in->cols; c++)
                {
                    tc_set(tc_prep_opt->out,r,c,0,tc_get(tc_prep_opt->scratch,r,c,0));
                    tc_set(tc_prep_opt->out,r,c,1,tc_get(tc_prep_opt->scratch,r,c,1));
                    tc_set(tc_prep_opt->out,r,c,2,tc_get(tc_prep_opt->scratch,r,c,2));
                }
            }
        tc_free_image(tc_prep_opt->ff);
        break;

    case TC_PREP_INTENSITY:

        if (tc_greyworld(tc_prep_opt->out, tc_prep_opt->intens, tmu) == ERR)
        {
            tc_write_log("preproc: intensity conversion failed\r\n");
            tc_prep_free_intens_io( tc_prep_opt );
            free(msg);
            return(-1);
        }
        break;

    case TC_PREP_IPEX:

        tc_alloc_image(&tc_prep_opt->scratch, tc_prep_opt->in->rows,
                       tc_prep_opt->in->cols, 1);
        if (tc_prep_opt->scratch == NULL)
        {
            tc_write_log("preproc: Out of memory!\r\n");
            tc_prep_free_intens_io( tc_prep_opt );
            free(msg);
            return(-1);
        }

        /* local highpass filter */
        if (tc_bandpass_image(tc_prep_opt->scratch, tc_prep_opt->intens, 11, 0,
                              tmu) == ERR)
        {
            tc_write_log("preproc: bandpass failed\r\n");
            tc_free_image(tc_prep_opt->scratch);
            tc_prep_free_intens_io( tc_prep_opt );
            free(msg);
            return(-1);
        }
        /* Copy the result into the appropriate output channel */
        for (r=0; r<tc_prep_opt->in->rows; r++)
        {
            for (c=0; c<tc_prep_opt->in->cols; c++)
            {
                tc_set(tc_prep_opt->out,r,c,0,tc_get(tc_prep_opt->scratch,r,c,0));
            }
        }
        tc_free_image(tc_prep_opt->scratch);

        tc_alloc_image(&tc_prep_opt->scratchB, tc_prep_opt->in->rows,
                       tc_prep_opt->in->cols, 3);
        if (tc_prep_opt->scratchB == NULL)
        {
            tc_write_log("preproc: Out of memory!\r\n");
            tc_prep_free_intens_io( tc_prep_opt );
            free(msg);
            return(-1);
        }

        if (tc_rgbhsv(tc_prep_opt->scratchB, tc_prep_opt->in, maxpx) == ERR)
        {
            tc_write_log("preproc: hsv conversion failed\r\n");
            tc_free_image(tc_prep_opt->scratchB);
            tc_prep_free_intens_io( tc_prep_opt );
            free(msg);
            return(-1);
        }

        for (r=0; r<tc_prep_opt->in->rows; r++)
        {
            for (c=0; c<tc_prep_opt->in->cols; c++)
            {
                tc_set(tc_prep_opt->out,r,c,1,tc_get(tc_prep_opt->scratchB,r,c,0));
                tc_set(tc_prep_opt->out,r,c,2,tc_get(tc_prep_opt->scratchB,r,c,2));
            }
        }
        tc_free_image(tc_prep_opt->scratchB);
        break;

    /* only callable internally, not from command prompt */
    case TC_PREP_GREY2RGB:

        tc_write_log("preproc: greyrgb conversion\r\n");
        if (tc_greyrgb(tc_prep_opt->out, tc_prep_opt->in) == ERR)
        {
            tc_write_log("preproc: greyrgb conversion failed\r\n");
            tc_prep_free_intens_io( tc_prep_opt );
            free(msg);
            return(-1);
        }
        break;

    case TC_PREP_BAR:

        if (tc_bar(tc_prep_opt->out, tc_prep_opt->intens) == ERR)
        {
            tc_write_log("preproc: oriented bars failed\r\n");
            tc_prep_free_intens_io( tc_prep_opt );
            free(msg);
            return(-1);
        }
        break;

    case TC_PREP_NONE:
        /* just copy data into output buffer */
        memcpy( tc_prep_opt->out->data, tc_prep_opt->in->data,
                tc_prep_opt->in->rows * tc_prep_opt->in->cols * tc_prep_opt->in->chans);
        break;

    case TC_PREP_BARHSV:
        /* Allocate scratch space */
        tc_alloc_image(&tc_prep_opt->scratch, tc_prep_opt->in->rows,
                       tc_prep_opt->in->cols, 1);
        if (tc_prep_opt->scratch == NULL)
        {
            tc_write_log("preproc: Out of memory!\r\n");
            tc_prep_free_intens_io( tc_prep_opt );
            free(msg);
            return(-1);
        }

        /* Oriented bar filter */
        if (tc_bar(tc_prep_opt->scratch, tc_prep_opt->intens) == ERR)
        {
            tc_write_log("preproc: oriented bars failed\r\n");
            tc_free_image(tc_prep_opt->scratch);
            tc_prep_free_intens_io( tc_prep_opt );
            free(msg);
            return(-1);
        }

        /* Copy the result into the appropriate output channel */
        for (r=0; r<tc_prep_opt->in->rows; r++)
        {
            for (c=0; c<tc_prep_opt->in->cols; c++)
            {
                tc_set(tc_prep_opt->out,r,c,0,tc_get(tc_prep_opt->scratch,r,c,0));
            }
        }
        tc_free_image(tc_prep_opt->scratch);

        /* Allocate a new scratch space image */
        tc_alloc_image(&tc_prep_opt->scratchB, tc_prep_opt->in->rows,
                       tc_prep_opt->in->cols, 3);
        if (tc_prep_opt->scratchB == NULL)
        {
            tc_write_log("preproc: Out of memory!\r\n");
            tc_prep_free_intens_io( tc_prep_opt );
            free(msg);
            return(-1);
        }

        /* Now convert HSV */
        if (tc_rgbhsv(tc_prep_opt->scratchB, tc_prep_opt->in, maxpx) == ERR)
        {
            tc_write_log("preproc: hsv conversion failed\r\n");
            tc_free_image(tc_prep_opt->scratchB);
            tc_prep_free_intens_io( tc_prep_opt );
            free(msg);
            return(-1);
        }

        /* Copy the result into the appropriate output channel */
        for (r=0; r<tc_prep_opt->in->rows; r++)
        {
            for (c=0; c<tc_prep_opt->in->cols; c++)
            {
                tc_set(tc_prep_opt->out,r,c,1,tc_get(tc_prep_opt->scratchB,r,c,0));
                tc_set(tc_prep_opt->out,r,c,2,tc_get(tc_prep_opt->scratchB,r,c,1));
                tc_set(tc_prep_opt->out,r,c,3,tc_get(tc_prep_opt->scratchB,r,c,2));
            }
        }
        tc_free_image(tc_prep_opt->scratchB);
        break;
    default:
        break;
    }

    /* Write output */
    snprintf(msg,MAX_STRING,"preproc: Writing output image %s\r\n",outname);
    tc_write_log(msg);
    tc_write_image(tc_prep_opt->out, outname);

    /* Clean up */
    tc_write_log("preproc: Cleanup (intens).\r\n");
    tc_prep_free_intens_io( tc_prep_opt );
    free(msg);

    tc_write_log("preproc: Exiting.\r\n");
    /* normal return */
    return 0;
}

int tc_prep_parse( tc_prep_t *tc_prep_opt, int argc, char **argv )
{

    int arg=1, help=0, additional_args=0;

    /* Parse the command line */
    while (arg < argc && argv[arg][0] == '-')
    {
        switch (argv[arg][1])
        {
        case 'n':
            tc_prep_opt->method = TC_PREP_INTENSITY;
            tc_prep_opt->outchans = 1;
            break;
        case 'c':
            tc_prep_opt->method = TC_PREP_HSV;
            tc_prep_opt->outchans = 3;
            break;
        case 'z':
            tc_prep_opt->method = TC_PREP_NONE;
            tc_prep_opt->outchans = -1;
            break;
        case 'B':
            tc_prep_opt->method = TC_PREP_BAR;
            tc_prep_opt->outchans = 1;
            if ((arg+1)>=argc)
            {
                help=1;
                break;
            }
            tc_bar_filter->support = atoi(argv[arg+1]);
            arg = arg+1;
            break;
        case 'a':
            tc_prep_opt->method = TC_PREP_BARHSV;
            tc_prep_opt->outchans = 4;
            if ((arg+1)>=argc)
            {
                help=1;
                break;
            }
            tc_bar_filter->support = atoi(argv[arg+1]);
            arg = arg+1;
            break;
        case 'b':
            tc_prep_opt->method = TC_PREP_BANDPASS;
            if ((arg+2)>=argc)
            {
                help=1;
                break;
            }
            tc_prep_opt->bandpass_filter_small = atoi(argv[arg+1]);
            tc_prep_opt->bandpass_filter_big = atoi(argv[arg+2]);
            tc_prep_opt->outchans = 1;
            arg = arg+2;
            break;
        case 'o':
            tc_prep_opt->method = TC_PREP_BANDPASS_OCTAVES;
            tc_prep_opt->outchans = 3;
            break;
        case 'g':
            tc_prep_opt->method = TC_PREP_GREYWORLD;
            tc_prep_opt->outchans = 3;
            break;
        case 'i':
            tc_prep_opt->method = TC_PREP_IPEX;
            tc_prep_opt->outchans = 3;
            break;
        case 't':
            tc_prep_opt->method = TC_PREP_TEXTURECAM;
            if ((arg+1)>=argc)
            {
                help=1;
                break;
            }
            tc_prep_opt->ffname = argv[arg+1];
            tc_prep_opt->outchans = 3;
            arg = arg+1;
            break;
        default:
            help = 1;
            break;
        }
        arg++;
    }

    additional_args = 2;

    if ((arg+additional_args)>argc || help || argc==1)
    {

        tc_write_log("Usage is tcprep [OPTIONS] <input.pgm> <output.pgm>\r\n");
        tc_write_log("  OPTIONS must be one of the following:\r\n");
        tc_write_log("  -c                 switch colorspace to hsv\r\n");
        tc_write_log("  -g                 greyworld color constancy\r\n");
        tc_write_log("  -n                 normalized intensity channel\r\n");
        tc_write_log("  -b <small> <big>   bandpass filter w/ given radii\r\n");
        tc_write_log("  -o                 three-channel bandpass suite\r\n");
        tc_write_log("  -i                 IPEX hsv & bandpass suite\r\n");
        tc_write_log("  -B <support>       oriented bar filter w/given support\r\n");
        tc_write_log("  -a <support>       oriented bar filter and HSV \r\n");
        tc_write_log("  -t <flatfield.ppm> TextureCam/Hitachi preprocessing\r\n");
        tc_write_log("  -z                 no pre processing\r\n");
        tc_write_log("  -h                 help!\r\n");
        return(-1);
    }

    return arg;

}


void tc_prep_free_intens_io( tc_prep_t *tc_prep_opt )
{
    tc_free_image(tc_prep_opt->intens);
    tc_free_image(tc_prep_opt->in);
    tc_free_image(tc_prep_opt->out);
}
