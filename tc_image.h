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
#include <time.h>

#ifndef TC_IMAGE_H
#define TC_IMAGE_H

/* minimum single precision floating point is ~1.18e-38 */
#define MIN_PROB                 (2.678636961808078e-33f)
#define SMALL                    (10e-8f)
#define BIG                      (99999)
#define MAX_STRING               (512)

// 255 and above is reserved, so this should never be that high
#define MAX_N_CLASSES            (8)

#define ERR                      (0)
#define OK                       (1)

/* our basic bit depth */
typedef uint8_t pixel_t;

typedef struct tc_image_type
{
    int rows;
    int cols;
    int chans;
    pixel_t *data;    /* row-major storage of pixels */
} tc_image;

pixel_t uchar_to_pixel(const unsigned char c);

pixel_t int_to_pixel(const int i);

unsigned char pixel_to_uchar(const pixel_t p);

void tc_write_log(const char *msg);

int tc_alloc_image(tc_image **img, const int rows,
                   const int cols, const int chans);

int tc_free_image(tc_image *img);

int tc_read_image(tc_image **img, const char *filename);

int tc_write_image(tc_image *img, const char *filename);

int tc_clone_image(tc_image **dst, tc_image *src);

int tc_crop_image(tc_image **dst, tc_image *src, const int top,
                  const int left, const int height, const int width);

int tc_copy_image(tc_image *dst, tc_image *src);

pixel_t tc_get(tc_image* img, const int row, const int col,
                      const int chan);

void tc_set(tc_image* img, const int row, const int col,
                   const int chan, const pixel_t val);

#endif
