/*
 * \file tc_filter.c
 * \brief Fast filtering options based on the integral image.
 * \author David Ray Thompson \n
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "tc_image.h"
#include "tc_filter.h"
#include "tc_io.h"

#ifndef TC_FILTER_C
#define TC_FILTER_C


/** return the maximum value */
int infm(int a, int b)
{
    if (a<=b) return a;
    return b;
}


/** return the maximum value */
int supm(int a, int b)
{
    if (a>=b) return a;
    return b;
}

/** Area of skewed rectangle */
int diag_rect_area(const int fw, const int fh)
{
    return 2*fw*fh + fw + fh;
}


/** Apply a filter to an image pixel */
int tc_filter_pixel(tc_filter *filter,
                    tc_image *image,
                    const int r,
                    const int c,
                    feature_t *result)
{
    if (filter == NULL || image == NULL || result == NULL)
    {
        return ERR;
    }

    /* find absolute position in image */
    int rowA  = filter->rowA + r;
    int colA  = filter->colA + c;
    int chanA = filter->chanA;
    int rowB  = filter->rowB + r;
    int colB  = filter->colB + c;
    int chanB = filter->chanB;
    feature_t diff = 0;

    /* image bounds check */
    if (rowA  >= image->rows  || rowA  < 0 ||
            colA  >= image->cols  || colA  < 0 ||
            chanA >= image->chans || chanA < 0)
    {
        (*result) = TC_FILTER_NODATA;
        return ERR;
    }

    if ((filter->function != TC_FILTER_RAW) &&
            (rowB  >= image->rows  || rowB  < 0 ||
             colB  >= image->cols  || colB  < 0 ||
             chanB >= image->chans || chanB < 0))
    {
        (*result) = TC_FILTER_NODATA;
        return ERR;
    }

    switch (filter->function)
    {
    case TC_FILTER_RAW:
        (*result) = tc_get(image, rowA, colA, chanA);
        return OK;
    case TC_FILTER_SUM:
        (*result) = tc_get(image, rowA, colA, chanA) +
                    tc_get(image, rowB, colB, chanB);
        return OK;
    case TC_FILTER_DIFF:
        (*result) = tc_get(image, rowA, colA, chanA) -
                    tc_get(image, rowB, colB, chanB);
        return OK;
    case TC_FILTER_ABS:
        (*result) = abs(tc_get(image, rowA, colA, chanA) -
                        tc_get(image, rowB, colB, chanB));
        return OK;
    case TC_FILTER_RATIO:
        diff = tc_get(image, rowA, colA, chanA) *
               TC_FIXEDPT_PRECIS_FACTOR -
               tc_get(image, rowB, colB, chanB) *
               TC_FIXEDPT_PRECIS_FACTOR;
        (*result) = diff / (tc_get(image, rowA, colA, chanA)+1);
        return OK;
    case TC_FILTER_RECT:
        /* used with summed area tables (integral image) */
        (*result) = tc_get(image, rowA, colA, chanA) +
                    tc_get(image, rowB, colB, chanA) -
                    tc_get(image, rowA, colB, chanA) -
                    tc_get(image, rowB, colA, chanA);
        return OK;
    default:
        tc_write_log("unrecognized filter function\r\n");
        return ERR;

    }
    return OK;
}



/** Initialize to default settings */
int tc_init_filter(tc_filter *filter)
{
    if (filter == NULL)
    {
        return ERR;
    }
    filter->function = TC_FILTER_RAW;
    filter->rowA = 0;
    filter->colA = 0;
    filter->chanA = 0;
    filter->rowB = 0;
    filter->colB = 0;
    filter->chanB = 0;
    return OK;
}


/** Write filter to filestream*/
int tc_write_filter(tc_filter *filter, FILE *file)
{
    char *fstring = malloc(sizeof(char) * TC_FILTER_STRINGSIZE);
    if (filter == NULL || file == NULL)
    {
        free(fstring);
        return ERR;
    }
    if (tc_filter_tostring(filter, fstring) == ERR)
    {
        free(fstring);
        return ERR;
    }
    if (fprintf(file, "%s", fstring) < 1)
    {
        tc_write_log("filter_write: I/O Error.\r\n");
        free(fstring);
        return ERR;
    }
    free(fstring);
    return OK;
}


/** Write filter to string */
int tc_filter_tostring(tc_filter *filter, char *str)
{
    if (filter == NULL || str == NULL) return ERR;

    if (snprintf(str, TC_FILTER_STRINGSIZE, 
                "F%i_(%i,%i,%i)_(%i,%i,%i)",
                 (int)(filter->function),
                 (filter->rowA),
                 (filter->colA),
                 (filter->chanA),
                 (filter->rowB),
                 (filter->colB),
                 (filter->chanB)) < 18)
    {
        tc_write_log("filter_tostring: Syntax error.\r\n");
        return ERR;
    }
    return OK;
}


/** Read filter from filestream */
int tc_read_filter(tc_filter *filter, void *tc_io)
{
    if (filter == NULL || tc_io == NULL) return ERR;

    char * buffer = (char *) malloc(sizeof(char) * BUF_SIZE);

    if(tc_getline_io(tc_io, buffer, BUF_SIZE, ' ') == ERR)
    {
        tc_write_log("filter_read: reading filter error.\r\n");
        free(buffer);
        return ERR;
    }

    /* Extract data from string using sscanf. */
    int matched = sscanf(buffer,"F%i_(%i,%i,%i)_(%i,%i,%i)",
                         (int *) &(filter->function),
                         &(filter->rowA),
                         &(filter->colA),
                         &(filter->chanA),
                         &(filter->rowB),
                         &(filter->colB),
                         &(filter->chanB));
    if (matched != 7)
    {
        char * msg = (char *) malloc(sizeof(char) * MAX_STRING);
        snprintf(msg, MAX_STRING, "Only matched %i tokens.\r\n", matched);
        tc_write_log(msg);
        tc_write_log("filter_read: Syntax error.\r\n");
        free(msg);
        return ERR;
    }
    free(buffer);
    return OK;
}


/** Read filter from string */
int tc_filter_fromstring(tc_filter *filter, char *str)
{
    if (filter == NULL || str == NULL) return ERR;

    if (sscanf(str,"F%i_(%i,%i,%i)_(%i,%i,%i)",
               (int *) &(filter->function),
               &(filter->rowA),
               &(filter->colA),
               &(filter->chanA),
               &(filter->rowB),
               &(filter->colB),
               &(filter->chanB)) != 7)
    {
        tc_write_log("filter_fromstring: Syntax error.\r\n");
        return ERR;
    }
    return OK;
}


int tc_copy_filter(tc_filter *dst, tc_filter *src)
{
    if (dst == NULL || src == NULL) return ERR;

    dst->function = src->function;
    dst->rowA = src->rowA;
    dst->colA = src->colA;
    dst->chanA = src->chanA;
    dst->rowB = src->rowB;
    dst->colB = src->colB;
    dst->chanB = src->chanB;
    return OK;
}



int tc_randomize_filter(tc_filter *filter, const int chans,
                        const int filterset, const int winsize, const int crosschannel)
{
    int minrow, maxrow, mincol, maxcol;
    if (filter == NULL) return ERR;
    int halfwidth = winsize / 2;

    switch(filterset)
    {
    case TC_FILTERSET_POINTS:

        /* could be any channel */
        filter->chanA = (rand() % chans);
        if (crosschannel)
        {
            filter->chanB = (rand() % chans);
        }
        else
        {
            filter->chanB = filter->chanA;
        }

        /* filter function could be one of 6 choices */
        do
        {
            filter->function = (filter_style)
                               (rand() % TC_FILTER_NFUNCTIONS);
        }
        /* exclude TC_FILTER_RECT */
        while (filter->function != TC_FILTER_RAW  &&
                filter->function != TC_FILTER_SUM  &&
                filter->function != TC_FILTER_DIFF &&
                filter->function != TC_FILTER_ABS  &&
                filter->function != TC_FILTER_RATIO);

        /* filter row and col within a pre-set window */
        filter->rowA = (rand() % winsize) - halfwidth;
        filter->rowB = (rand() % winsize) - halfwidth;
        filter->colA = (rand() % winsize) - halfwidth;
        filter->colB = (rand() % winsize) - halfwidth;
        break;

    case TC_FILTERSET_RATIOS:

        /* could be any channel */
        filter->chanA = (rand() % chans);
        if (crosschannel)
        {
            filter->chanB = (rand() % chans);
        }
        else
        {
            filter->chanB = filter->chanA;
        }

        filter->function = TC_FILTER_RATIO;

        /* filter row and col within a pre-set window */
        filter->rowA = (rand() % winsize) - halfwidth;
        filter->rowB = (rand() % winsize) - halfwidth;
        filter->colA = (rand() % winsize) - halfwidth;
        filter->colB = (rand() % winsize) - halfwidth;
        break;

    case TC_FILTERSET_RECTANGLES:

        if (crosschannel)
        {
            tc_write_log("Rectangle sum features use just one channel.\r\n");
            return ERR;
        }

        /* filter function must be a rectangle */
        filter->function = TC_FILTER_RECT;

        /* B channel must be the same as A */
        filter->chanA = (rand() % chans);
        filter->chanB = filter->chanA;

        /* filter row and col within a pre-set window */
        /* GTF: Is this correct? Range is from -19 to 22 using a winsize of 21. Maybe remove the -1
         * or use winsize instead of halfwidth*2?
        */
        filter->rowA = (rand() % (winsize*2)) - (halfwidth*2-1);
        filter->rowB = (rand() % (winsize*2)) - (halfwidth*2-1);
        filter->colA = (rand() % (winsize*2)) - (halfwidth*2-1);
        filter->colB = (rand() % (winsize*2)) - (halfwidth*2-1);

        /* A holds the upper left coordinate, B the lower right */
        minrow = infm(filter->rowA, filter->rowB);
        maxrow = supm(filter->rowA, filter->rowB);
        mincol = infm(filter->colA, filter->colB);
        maxcol = supm(filter->colA, filter->colB);
        filter->rowA = minrow;
        filter->rowB = maxrow;
        filter->colA = mincol;
        filter->colB = maxcol;
        break;

    default:
        tc_write_log("unrecognized filterset.\r\n");
        return ERR;
        break;
    }
    return OK;
}

#endif
