/**
 * \file tc_preproc.h
 * \brief Preprocess with colorspace, normalization, convolution
 * \author David Ray Thompson
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "tc_image.h"

#ifndef TC_PREPROC_H
#define TC_PREPROC_H

/* Apply oriented bar filters. */
/* dst must be preallocated to have one channel per orientation */
int tc_bar(tc_image *dst, tc_image *src);


/* Convert to intensity.  maxpx is the maximum value. */
/* dst must be preallocated to have a single channel. */
int tc_intensity(tc_image *dst, tc_image *src);


/* Normalize image intensity to have mean target_memu and standard deviation
 * target_stdev.  Images are preallocated. The parameter 'robust' tells us
 * how many standard deviations outside the mean to consider (0=use all). */
int tc_normalize_image(tc_image *dst, tc_image *src, const pixel_t target_mu,
                       const pixel_t target_stdev, const int robust);


/* Greyworld color constancy */
int tc_greyworld(tc_image *dst, tc_image *src, const pixel_t target_mu);


/* convert to hsv space.  maxpx is the maximum value */
int tc_rgbhsv(tc_image *dst, tc_image *src, const float maxpx);


/* needlessly slow */
int tc_moving_average(tc_image *dst, tc_image *src, const int wid);


/*! Slow bandpass filter (all images preallocated).
 * 'wbig' and 'wsmall' are widths of the moving average filters.*/
int tc_bandpass_image(tc_image *dst, tc_image *src, const int wbig,
                      const int wsmall, const int target_mu);


/* Flat field correction */
int tc_flatfield_image(tc_image *dst, tc_image *src, tc_image *ff);


/* Convert grey image to rgb image. Essentially, triplicate each pixel in
 * grey image to emulate a rgb image.
 */
int tc_greyrgb(tc_image *dst, tc_image *src);


#endif

