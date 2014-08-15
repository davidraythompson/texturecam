/**
 * \file tc_preproc.c
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
/*#include "tc_bar.h"*/
#include "tc_bar_fixed.h"
#include "tc_preproc.h"

#ifndef TC_PREPROC_C
#define TC_PREPROC_C

/* 2D convolution of oriented bar filters at multiple scales */
/* Take the maximum response over all scales */
int tc_bar(tc_image *dst, tc_image *src)
{
    int r,c,s,or,rr,cc;
    pixel_t val, cand_scaled;
    int cand;

    int norients = tc_bar_filter->norients;
    int nscales  = tc_bar_filter->nscales;
    int support  = tc_bar_filter->support;
    int radius   = support/2;

    if (src == NULL || dst == NULL)
    {
        tc_write_log("tc_bar: NULL image.\r\n");
        return ERR;
    }

    if ((support%2)==0)
    {
        tc_write_log("tc_bar: support should be odd.\r\n");
        return ERR;
    }

    if (dst->chans != 1)
    {
        printf("Output channels = %d\n", dst->chans);
        tc_write_log("tc_bar: incorrect number of output channels.\r\n");
        return ERR;
    }

    if (src->rows != dst->rows ||
            src->cols != dst->cols)
    {
        tc_write_log("tc_bar: dimensions do not match.\r\n");
        return ERR;
    }

    if (src->rows < 1 ||
            src->cols < 1)
    {
        tc_write_log("tc_bar: illegal image size.\r\n");
        return ERR;
    }

    /* Can cheat and only fill in the border, since rest will be calculated later */
    int nrows = src->rows;
    int ncols = src->cols;

    for (r=0; r<radius; r++)
        for (c=0; c<ncols; c++)
            tc_set(dst,r,c,0,0);
    for (r=radius; r<(nrows-radius); r++)
    {
        for (c=0; c<radius; c++)
            tc_set(dst,r,c,0,0);
        for (c=(ncols-radius); c<ncols; c++)
            tc_set(dst,r,c,0,0);
    }
    for (r=(nrows-radius); r<nrows; r++)
        for (c=0; c<ncols; c++)
            tc_set(dst,r,c,0,0);

    register int SUPPORT_SQ = support * support;
    register int BAR_SCALES_TIMES_SUPPORT_SQ = nscales * SUPPORT_SQ;
    /* Cache a local copy of the window of interest
       to reduce lookup overhead */
    int window[2*radius+1][2*radius+1];

    /* Inefficient 2D Convolution */
    for (r=radius; r<(src->rows-radius); r++)
    {
        for (c=radius; c<(src->cols-radius); c++)
        {
            /* Copy items of interest into the window */
            /* store as ints so later math is well behaved.
             * (multiplication was treating BAR values as unsigned) */
            for (rr=-radius; rr<=radius; rr++)
                for (cc=-radius; cc<=radius; cc++)
                    window[rr+radius][cc+radius] = (int)tc_get(src,r+rr,c+cc,0);

            val=0;  /* accumulate the max over all orientations and scales */
            cand_scaled = 0;
            for (or=0; or<norients; or++)
            {
                for (s=0; s<nscales; s++)
                {
                    cand=0;
                    for (rr=-radius; rr<=radius; rr++)
                    {
                        for (cc=-radius; cc<=radius; cc++)
                        {
                            cand = cand + window[rr+radius][cc+radius] *
                                   tc_bar_filter->bar[or * BAR_SCALES_TIMES_SUPPORT_SQ +
                                                      s  * SUPPORT_SQ +
                                                      (rr+radius) * support +
                                                      (cc+radius)];
                        }
                    }
                    /* rescale for pgm */
                    if( cand > 0 )
                    {
                        cand_scaled = (pixel_t)(cand >> BAR_SCALE_SHIFT);
                    }

                    /* update the max over all orientations and scales */
                    if (cand_scaled>val)
                    {
                        val = cand_scaled;
                    }
                }
                tc_set(dst,r,c,0,val);
            }
        }
        /* report progress */
        fprintf(stdout,"\rProgress: %d%%.", (int)((r+1)*100)/(src->rows-radius));
    }
    fprintf(stdout,"\r\n");

    return OK;
}

/* Convert to intensity.  maxpx is the maximum value. */
/* dst must be preallocated to have a single channel. */
int tc_intensity(tc_image *dst, tc_image *src)
{
    int r,c,b;
    pixel_t max, val;

    if (src == NULL || dst == NULL)
    {
        tc_write_log("tc_intensity: NULL image.\r\n");
        return ERR;
    }

    if (dst->chans != 1)
    {
        tc_write_log("tc_intensity: one output channel, please.\r\n");
        return ERR;
    }

    if (src->rows != dst->rows ||
            src->cols != dst->cols)
    {
        tc_write_log("tc_intensity: dimensions do not match.\r\n");
        return ERR;
    }

    if (src->rows < 1 ||
            src->cols < 1)
    {
        tc_write_log("tc_intensity: illegal image size.\r\n");
        return ERR;
    }

    for (r=0; r<src->rows; r++)
    {
        for (c=0; c<src->cols; c++)
        {
            max = tc_get(src,r,c,0);
            for (b=1; b<src->chans; b++)
            {
                val = tc_get(src,r,c,b);
                if (val>max)
                {
                    max = val;
                }
            }
            tc_set(dst,r,c,0,max);
        }
    }

    return OK;
}



/* Greyworld color constancy */
int tc_greyworld(tc_image *dst, tc_image *src, const pixel_t target_mu)
{
    int r,c,b, area;
    pixel_t mu=0, val;
    unsigned long sum=0;
    float fval;

    if (src == NULL || dst == NULL)
    {
        tc_write_log("tc_greyworld: NULL image.\r\n");
        return ERR;
    }

    if (src->rows != dst->rows ||
            src->cols != dst->cols ||
            src->chans != dst->chans)
    {
        tc_write_log("tc_greyworld: dimensions do not match.\r\n");
        return ERR;
    }

    if (src->rows < 1 ||
            src->cols < 1 ||
            src->chans < 1)
    {
        tc_write_log("tc_greyworld: illegal image size.\r\n");
        return ERR;
    }

    for (b=0; b<src->chans; b++)
    {
        /* first pass - get area of each channel*/
        sum = 0;
        area = 0;
        for (r=0; r<src->rows; r++)
        {
            for (c=0; c<src->cols; c++)
            {
                val = tc_get(src,r,c,b);
                sum += val;
                area++;
            }
        }
        mu = sum/area;

        /* divide by the mean channel value*/
        for (r=0; r<src->rows; r++)
        {
            for (c=0; c<src->cols; c++)
            {
                val = tc_get(src,r,c,b);
                fval = ((float) (val)) / ((float) (mu)) * target_mu;
                tc_set(dst,r,c,b,(pixel_t) fval);
            }
        }
    }

    return OK;
}



/* Normalize image intensity to have mean target_mu and standard deviation
 * target_stdev.  Images are preallocated. The parameter 'robust' tells us
 * how many standard deviations outside the mean to consider (0=use all). */
int tc_normalize_image(tc_image *dst, tc_image *src, const pixel_t target_mu,
                       const pixel_t target_stdev, const int robust)
{
    int r,c,b, area;
    pixel_t mu=0, val;
    unsigned long musq=0, sum=0, sumsq=0, stdev, min, max;
    float fval;

    if (src == NULL || dst == NULL)
    {
        tc_write_log("tc_normalize_image: NULL image.\r\n");
        return ERR;
    }

    if (src->rows != dst->rows ||
            src->cols != dst->cols ||
            src->chans != dst->chans)
    {
        tc_write_log("tc_normalize_image: dimensions do not match.\r\n");
        return ERR;
    }

    if (src->rows < 1 ||
            src->cols < 1 ||
            src->chans < 1)
    {
        tc_write_log("tc_normalize_image: illegal image size.\r\n");
        return ERR;
    }

    for (b=0; b<src->chans; b++)
    {
        /* first pass - use all pixels */
        sum = 0;
        sumsq = 0;
        area = 0;
        for (r=0; r<src->rows; r++)
        {
            for (c=0; c<src->cols; c++)
            {
                val = tc_get(src,r,c,b);
                sum += val;
                sumsq += val*val;
                area++;
            }
        }
        mu = sum/area;
        musq = sumsq/area;
        stdev = sqrtf(musq - mu*mu);

        if (robust > 0)
        {
            /* second pass - pixels within n stdevs of the mean */
            sum = 0;
            sumsq = 0;
            area = 0;
            max = mu+stdev*robust;
            min = mu-stdev*robust;
            for (r=0; r<src->rows; r++)
            {
                for (c=0; c<src->cols; c++)
                {
                    val = tc_get(src,r,c,b);
                    if (val <= max && val >= min)
                    {
                        sum += val;
                        sumsq += val*val;
                        area++;
                    }
                }
            }
            if (area>0)
            {
                mu = sum/area;
                musq = sumsq/area;
                stdev = sqrtf(musq - mu*mu);
            }
        }

        /* finally do the transform */
        for (r=0; r<src->rows; r++)
        {
            for (c=0; c<src->cols; c++)
            {
                val = tc_get(src,r,c,b);
                fval = (((float) (val)) - ((float)(mu))) /
                       ((float) (stdev)) *
                       ((float)(target_stdev)) +
                       target_mu;
                tc_set(dst,r,c,b,(pixel_t) fval);
            }
        }
    }

    return OK;
}


/* convert to hsv space.  maxpx is the maximum value */
int tc_rgbhsv(tc_image *dst, tc_image *src, const float maxpx)
{
    int r,c;
    unsigned char red, grn, blu, hue, sat, val, rgb_min, rgb_max;

    if (src == NULL || dst == NULL)
    {
        tc_write_log("tc_rgbhsv: NULL image.\r\n");
        return ERR;
    }

    if (src->chans != 3)
    {
        tc_write_log("tc_rgbhsv: I need a color image.\r\n");
        return ERR;
    }

    if (src->rows != dst->rows ||
            src->cols != dst->cols ||
            src->chans != dst->chans)
    {
        tc_write_log("tc_rgbhsv: dimensions do not match.\r\n");
        return ERR;
    }

    if (src->rows < 1 ||
            src->cols < 1)
    {
        tc_write_log("tc_rgbhsv: illegal image size.\r\n");
        return ERR;
    }

    for (r=0; r<src->rows; r++)
    {
        for (c=0; c<src->cols; c++)
        {
            red = ((unsigned char) tc_get(src,r,c,0));
            grn = ((unsigned char) tc_get(src,r,c,1));
            blu = ((unsigned char) tc_get(src,r,c,2));
            rgb_min = (grn <= blu ? (red <= grn ? red : grn) :
                       (red <= blu ? red : blu));
            rgb_max = (grn >= blu ? (red >= grn ? red : grn) :
                       (red >= blu ? red : blu));

            /* compute value */
            val = rgb_max;
            if (val == 0)
            {
                sat = 0;
            }

            /* compute saturation */
            else
            {
                sat = 255 * ((long)(rgb_max - rgb_min))/val;
            }

            if( sat != 0 )
            {
                /* compute hue */
                if (rgb_max == red)
                {
                    hue = 0 + 43*(grn-blu)/(rgb_max - rgb_min);
                }
                else if (rgb_max == grn)
                {
                    hue = 85 + 43*(blu-red)/(rgb_max - rgb_min);
                }
                else     /* rgb_max == blu */
                {
                    hue = 171 + 43*(red-grn)/(rgb_max - rgb_min);
                }
            }

            else
            {
                hue = 0;
                val = 0;
            }

            tc_set(dst,r,c,0,(pixel_t) hue);
            tc_set(dst,r,c,1,(pixel_t) sat);
            tc_set(dst,r,c,2,(pixel_t) val);
        }
    }
    return OK;
}


/* convert to hsv space.  maxpx is the maximum value */
int tc_float_rgbhsv(tc_image *dst, tc_image *src, const float maxpx)
{
    int r,c;
    float red, grn, blu, hue=0.0, sat=0.0, val=0.0;

    if (src == NULL || dst == NULL)
    {
        tc_write_log("tc_rgbhsv: NULL image.\r\n");
        return ERR;
    }

    if (src->chans != 3)
    {
        tc_write_log("tc_rgbhsv: I need a color image.\r\n");
        return ERR;
    }

    if (src->rows != dst->rows ||
            src->cols != dst->cols ||
            src->chans != dst->chans)
    {
        tc_write_log("tc_rgbhsv: dimensions do not match.\r\n");
        return ERR;
    }

    if (src->rows < 1 ||
            src->cols < 1)
    {
        tc_write_log("tc_rgbhsv: illegal image size.\r\n");
        return ERR;
    }

    for (r=0; r<src->rows; r++)
    {
        for (c=0; c<src->cols; c++)
        {
            red = ((float) tc_get(src,r,c,0))/maxpx;
            grn = ((float) tc_get(src,r,c,1))/maxpx;
            blu = ((float) tc_get(src,r,c,2))/maxpx;

            // RGB are each on [0, 1]. S and V are returned on [0, 1] and H is
            // // returned on [0, 6].

            if (blu>=red && blu>=grn) val = blu;
            else if (grn>=red && grn>=blu) val = grn;
            else val = red;

            if (blu<=red && blu<=grn) sat = val-blu;
            else if (grn<=red && grn<=blu) sat = val-grn;
            else sat = val-red;

            if (sat==0.0)
            {
                //sat = 1.0;  // is this okay?
                hue = 0.0;
            }
            else if (val==0.0)
            {
                sat = 0.0;
                hue = 0.0;
                val = 0.0;
            }
            else
            {
                if (red == val)
                {
                    hue = (grn-blu)/sat;
                }
                else if (grn == val)
                {
                    hue = 2.0+(blu - red)/sat;
                }
                else if (blu == val)
                {
                    hue = 4.0+(red - grn)/sat;
                }
                if (hue<0.0)
                {
                    hue = hue+1.0;
                }
                hue = hue/6.0;
                sat = sat/val;
            }

            tc_set(dst,r,c,0,(pixel_t) (hue*maxpx));
            tc_set(dst,r,c,1,(pixel_t) (sat*maxpx));
            tc_set(dst,r,c,2,(pixel_t) (val*maxpx));
        }
    }

    return OK;
}




/* This smoothes an image using a marching moving window average.
 * It's not exactly brute force, but still a bit slow */
int tc_moving_average(tc_image *dst, tc_image *src, const int wid)
{
    int r,c,b,radius,r2;
    unsigned int val, area;
    float fval;

    if (wid > 0 && (wid%2)!=1)
    {
        tc_write_log("tc_moving_average: width should be an odd number\r\n");
        return ERR;
    }

    if (src == NULL || dst == NULL)
    {
        tc_write_log("tc_moving_average: NULL image.\r\n");
        return ERR;
    }

    if ((wid)>=src->rows || (wid)>=src->cols)
    {
        tc_write_log("tc_moving_average: image is too small.\r\n");
        return ERR;
    }

    if (src->rows != dst->rows ||
            src->cols != dst->cols ||
            src->chans != dst->chans)
    {
        tc_write_log("tc_moving_average: dimensions do not match.\r\n");
        return ERR;
    }

    if (src->rows < 1 ||
            src->cols < 1 ||
            src->chans < 1)
    {
        tc_write_log("tc_moving_average: illegal image size.\r\n");
        return ERR;
    }

    /* handle the no-op case */
    if (wid==0)
    {
        if (tc_copy_image(dst, src) == ERR)
        {
            tc_write_log("tc_moving_average: copy failed.\r\n");
            return ERR;
        }
        return OK;
    }

    /* Initialize the average to zero */
    for (b=0; b<src->chans; b++)
    {
        for (r=0; r<src->rows; r++)
        {
            for (c=0; c<src->cols; c++)
            {
                tc_set(dst,r,c,b,0);
            }
        }
    }

    /* Integer division gives us the radius */
    radius = wid/2;
    area = wid*wid;

    for (b=0; b<src->chans; b++)
    {
        for (r=radius; r<(src->rows-radius); r++)
        {

            /* initialize our local average */
            val = 0;
            for (c=0; c<wid; c++)
            {
                for (r2=r-radius; r2<=(r+radius); r2++)
                {
                    val+=tc_get(src,r2,c,b);
                }
            }

            /* inductive step - march across the row */
            for (c=radius; c<(src->cols-radius); c++)
            {
                /* record the convolution */
                fval = ((float) val) / ((float) area);
                tc_set(dst,r,c,b, (pixel_t) fval);

                if (c > ((src->cols)-radius-2))
                {
                    break; /* done with this row */
                }
                for (r2=r-radius; r2<=(r+radius); r2++)
                {
                    /* add the new column, subtract the old */
                    val-=tc_get(src,r2,c-radius,b);
                    val+=tc_get(src,r2,c+radius+1,b);
                }
            }
        }
    }
    return OK;
}




/*! Slow bandpass filter (all images preallocated).
 * 'wbig' and 'wsmall' are widths of the moving average filters.*/
int tc_bandpass_image(tc_image *dst, tc_image *src, const int wbig,
                      const int wsmall,
                      const int target_mu)
{
    int r,c,b;
    unsigned int val;
    tc_image *fine, *coarse;
    int margin = wbig/2;

    if (wbig<1 || wsmall<0 || wsmall>=wbig)
    {
        tc_write_log("tc_bandpass_image: invalid parameters\r\n");
        return ERR;
    }

    if (src == NULL || dst == NULL)
    {
        tc_write_log("tc_bandpass_image: NULL image.\r\n");
        return ERR;
    }

    if (src->rows != dst->rows ||
            src->cols != dst->cols ||
            src->chans != dst->chans)
    {
        tc_write_log("tc_bandpass_image: dimensions do not match.\r\n");
        return ERR;
    }

    if (src->rows < 1 ||
            src->cols < 1 ||
            src->chans < 1)
    {
        tc_write_log("tc_bandpass_image: illegal image size.\r\n");
        return ERR;
    }

    if (tc_clone_image(&fine, src) == ERR)
    {
        tc_write_log("tc_bandpass_image: could not allocate image.\r\n");
        tc_free_image(fine);
        return ERR;
    }
    if (tc_clone_image(&coarse, src) == ERR)
    {
        tc_write_log("tc_bandpass_image: could not allocate image.\r\n");
        tc_free_image(fine);
        tc_free_image(coarse);
        return ERR;
    }

    if (tc_moving_average(fine, src, wsmall) == ERR)
    {
        tc_write_log("tc_bandpass_image: moving average failed.\r\n");
        tc_free_image(fine);
        tc_free_image(coarse);
        return ERR;
    }

    if (tc_moving_average(coarse, src, wbig) == ERR)
    {
        tc_write_log("tc_bandpass_image: moving average failed.\r\n");
        tc_free_image(fine);
        tc_free_image(coarse);
        return ERR;
    }


    /* subtract the coarse features from the fine */
    for (r=0; r<src->rows; r++)
    {
        for (c=0; c<src->cols; c++)
        {
            for (b=0; b<src->chans; b++)
            {

                if (r<margin || r>=(src->rows - margin) ||
                        c<margin || c>=(src->cols - margin))
                {
                    val = 0;
                }
                else
                {
                    val = target_mu + tc_get(fine,r,c,b)
                          - tc_get(coarse,r,c,b);
                }
                tc_set(dst,r,c,b,(pixel_t) val);
            }
        }
    }

    tc_free_image(fine);
    tc_free_image(coarse);
    return OK;
}


/* Flat field correction.*/
int tc_flatfield_image(tc_image *dst, tc_image *src, tc_image *ff)
{
    int r,c,b;

    if (src == NULL || dst == NULL || ff == NULL)
    {
        tc_write_log("tc_flatfield_image: NULL image.\r\n");
        return ERR;
    }

    if (src->rows != dst->rows ||
            src->cols != dst->cols ||
            src->chans != dst->chans ||
            src->rows != ff->rows ||
            src->cols != ff->cols ||
            src->chans != ff->chans)
    {
        tc_write_log("tc_flatfield_image: dimensions do not match.\r\n");
        return ERR;
    }

    if (src->rows < 1 ||
            src->cols < 1 ||
            src->chans < 1)
    {
        tc_write_log("tc_flatfield_image: illegal image size.\r\n");
        return ERR;
    }

    /* find min val. of flatfield */
    float val, flat;
    float min[dst->chans];
    for (b=0; b<ff->chans; b++)
    {
        min[b] = 9e99;
        for (r=0; r<ff->rows; r++)
        {
            for (c=0; c<ff->cols; c++)
            {
                val = (float) tc_get(ff,r,c,b);
                if (val<min[b])
                {
                    min[b] = val;
                }
            }
        }

    }

    for (b=0; b<ff->chans; b++)
    {
        for (r=0; r<ff->rows; r++)
        {
            for (c=0; c<ff->cols; c++)
            {
                val = (float) tc_get(src,r,c,b);
                flat = min[b]/((float) tc_get(ff,r,c,b));
                tc_set(dst,r,c,b,(pixel_t) val*flat);
            }
        }
    }
    return OK;
}

int tc_greyrgb(tc_image *dst, tc_image *src)
{
    int r,c;

    if (src == NULL || dst == NULL)
    {
        tc_write_log("tc_rgbhsv: NULL image.\r\n");
        return ERR;
    }

    if (src->chans != 1)
    {
        tc_write_log("tc_greyrgb: I need a greyscale image.\r\n");
        return ERR;
    }

    if (src->rows != dst->rows ||
            src->cols != dst->cols)
    {
        tc_write_log("tc_greyrgb: dimensions do not match.\r\n");
        return ERR;
    }

    if (src->rows < 1 ||
            src->cols < 1)
    {
        tc_write_log("tc_greyrgb: illegal image size.\r\n");
        return ERR;
    }

    for (r=0; r<src->rows; r++)
    {
        for (c=0; c<src->cols; c++)
        {
            pixel_t grayvalue = tc_get(src,r,c,0);
            tc_set(dst,r,c,0,grayvalue);
            tc_set(dst,r,c,1,grayvalue);
            tc_set(dst,r,c,2,grayvalue);
        }
    }
    return OK;
}

#endif
