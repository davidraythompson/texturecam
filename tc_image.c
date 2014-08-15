/*
 * \file tc_image.c
 * \brief A lightweight image class with I/O to PGM format.
 * \author David Ray Thompson \r\n
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "tc_image.h"

#ifndef TC_IMAGE_C
#define TC_IMAGE_C



/** Translate unsigned int to pixel. */
pixel_t uchar_to_pixel(const unsigned char c)
{
    return (pixel_t) c;
}



/** Translate int to pixel. */
pixel_t int_to_pixel(const int i)
{
    return (pixel_t) i;
}



/** Translate pixel to unsigned char. */
unsigned char pixel_to_uchar(const pixel_t p)
{
    return (unsigned char) (p);
}



/** log a message */
void tc_write_log(const char *msg)
{
    fprintf(stderr, "%li| %s", time(NULL),msg);
}



/** get a pixel */
pixel_t tc_get(tc_image* img,
               const int row,
               const int col,
               const int chan)
{
    register int rowstride = img->cols * img->chans;
    register int colstride = img->chans;
    const int bandstride = 1;
    return img->data[row*rowstride + col*colstride + chan*bandstride];
}



/** set a pixel */
void tc_set(tc_image* img,
            const int row,
            const int col,
            const int chan,
            const pixel_t val)
{
    register int rowstride = img->cols * img->chans;
    register int colstride = img->chans;
    const int bandstride = 1;
    img->data[row*rowstride + col*colstride + chan*bandstride] = val;
}



/** Allocate an image. */
int tc_alloc_image(
    tc_image **image,
    const int rows,
    const int cols,
    const int chans)
{
    tc_image *img = NULL;

    if (rows < 1 || cols < 1 || chans < 1)
    {
        tc_write_log("tc_alloc_img: invalid params.\r\n");
        return ERR;
    }

    /* allocate base img struct */
    img = (tc_image*) malloc(sizeof(tc_image));
    if (img == NULL)
    {
        tc_write_log("tc_alloc_img: No memory for img.\r\n");
        return ERR;
    }

    img->data = (pixel_t*) malloc(sizeof(pixel_t) * rows * cols * chans);
    if (img->data == NULL)
    {
        tc_write_log("tc_alloc_img: No memory for img data.\r\n");
        free(img);
        return ERR;
    }

    img->rows = rows;
    img->cols = cols;
    img->chans = chans;
    *image = img;
    return OK;
}



/*! Free an image. */
int tc_free_image(tc_image *image)
{
    if (image == NULL)
    {
        tc_write_log("tc_free_image: NULL pointer.\r\n");
        return ERR;
    }
    if (image->data == NULL)
    {
        if (image->rows < 1 ||
                image->cols < 1 ||
                image->chans < 1)
        {
            return OK;
        }
        else
        {
            tc_write_log("tc_free_image: NULL data.\r\n");
            free(image);
            return ERR;
        }
    }
    free(image->data);
    free(image);
    return OK;
}



/*
 *! Read from a binary or ascii pgm image file.
 *  Thanks to Shelley Research Group for base code.
 */
int tc_read_image( tc_image **img, const char *filename)
{

    FILE * file;
    int r, b, c;
    int rows, cols, chans, max;
    int ch_int;
    char type='0';
    char *buf = (char *) malloc(MAX_STRING * sizeof(char));
    tc_image *image;

    if (!strstr(filename, ".pgm") &&
            !strstr(filename, ".PGM") &&
            !strstr(filename, ".ppm") &&
            !strstr(filename, ".PPM"))
    {
        tc_write_log("tc_read_image: Don't recognize the file suffix.\r\n");
        free(buf);
        return ERR;
    }
    if ((file = fopen(filename, "rb")) == NULL)
    {
        snprintf(buf, MAX_STRING,
                 "tc_read_image: Can't open file %s for reading.\r\n", filename);
        tc_write_log(buf);
        free(buf);
        return ERR;
    }

    /* read header */
    char imgtype = (char) getc(file);
    char* bandcode = malloc(sizeof(char)*2);
    bandcode[0] = (char) getc(file);
    bandcode[1] = '\0';
    type = atoi(bandcode);
    free(bandcode);
    char eol = (char) getc(file);

    if (eol != '\n' && eol != ' ')
    {
        tc_write_log("read_image: bad header format.\r\n");
        free(buf);
        return ERR;
    }

    switch (imgtype)
    {
    case 'P':  /* pgm image */
        switch (type)
        {
        case 1:
        case 4:
            tc_write_log("tc_read_image: Don't support bitmaps\r\n");
            return ERR;
            break;
        case 2:
        case 5:
            chans = 1;
            break;
        case 3:
        case 6:
            chans = 3;
            break;
        default:
            tc_write_log("read_image: bad header format.\r\n");
            return ERR;
            break;
        }
        break;
    case 'H': /* custom format */
        chans = type;
        type = 0;
        printf("read_image: custom format, %d channels.\r\n", chans);
        break;
    default:
        tc_write_log("read_image: bad header format.\r\n");
        free(buf);
        return ERR;
    }

    /* read comment block */
    while (getc(file) == '#') while (getc(file) != '\n');

    /* read columns and rows, and max value */
    fseek(file, -1, SEEK_CUR);
    fscanf(file,"%d %d %d", &cols, &rows, &max);
    /* Consume the final newline */
    fgetc(file);

    if (tc_alloc_image(&image, rows, cols, chans) == ERR)
    {
        fclose(file);
        free(buf);
        return ERR;
    }

    for (r=0; r < rows; r++)
    {
        for (c=0; c < cols; c++)
        {
            for (b=0; b < chans; b++)
            {
                switch (type)
                {
                case 2:
                case 3:
                    /* ASCII */
                    if (fscanf(file,"%d", &ch_int) != 1)
                    {
                        tc_write_log("tc_read_image: Syntax error.\r\n");
                        tc_free_image(image);
                        img = NULL;
                        return ERR;
                    }
                    tc_set(image, r, c, b, int_to_pixel(ch_int));
                    break;
                default:
                    /* binary */
                    tc_set(image, r, c, b, uchar_to_pixel(getc(file)));
                    break;
                }
            }
        }
    }
    *img = image;
    fclose(file);
    free(buf);
    return OK;
}



/*! Write a binary grayscale pgm. */
int tc_write_image(tc_image *img, const char *filename)
{

    int r, b, c;
    FILE * file;
    if ((file = fopen(filename, "wb")) == NULL)
    {
        tc_write_log("tc_write_image: Can't open file for writing.\r\n");
        return ERR;
    }
    if (!strstr(filename, ".pgm") &&
            !strstr(filename, ".PGM") &&
            !strstr(filename, ".PPM") &&
            !strstr(filename, ".ppm"))
    {
        tc_write_log("tc_write_image: Don't recognize image type.");
        fclose(file);
        return ERR;
    }

    switch(img->chans)
    {
    case 1:
        fprintf(file, "P5\n");
        fprintf(file, "%d %d\n", img->cols, img->rows);
        fprintf(file, "255\n");
        break;
    case 3:
        fprintf(file, "P6\n");
        fprintf(file, "%d %d\n", img->cols, img->rows);
        fprintf(file, "255\n");
        break;
    default:
        tc_write_log("tc_write_image: writing special pgm format");
        /* our convention for channels > 3 */
        fprintf(file, "H%i\n",img->chans);
        fprintf(file, "%d %d\n", img->cols, img->rows);
        fprintf(file, "255\n");
        break;
    }

    for (r=0; r < img->rows; r++)
    {
        for (c=0; c < img->cols; c++)
        {
            for (b=0; b < img->chans; b++)
            {
                putc(pixel_to_uchar(tc_get(img, r, c, b)), file);
            }
        }
    }
    fclose(file);
    return OK;
}



/*! Allocate a new image and fill it with a copy of src's data. */
int tc_clone_image(tc_image **dst, tc_image *src)
{
    if (src == NULL || dst == NULL)
    {
        tc_write_log("clone_image: NULL image.\r\n");
        return ERR;
    }
    if (src->rows < 1 || src->cols < 1 || src->chans < 1)
    {
        tc_write_log("clone_image: Bad dimensions.\r\n");
        return ERR;
    }
    if (tc_alloc_image(dst, src->rows, src->cols, src->chans) == ERR)
    {
        return ERR;
    }
    return tc_copy_image(*dst, src);
}



/*! Allocate a new image and fill it with a cropped portion of
 * the data in copy of src data. */
int tc_crop_image(tc_image **dst, tc_image *src, const int top,
                  const int left, const int height, const int width)
{
    int b,c,r;
    pixel_t x;

    if (src == NULL || dst == NULL)
    {
        tc_write_log("crop_image: NULL image.\r\n");
        return ERR;
    }
    if (src->rows < 1 || src->cols < 1 || src->chans < 1)
    {
        tc_write_log("clone_image: Bad dimensions.\r\n");
        return ERR;
    }
    if ((top < 0) || (top >= src->rows) ||
            (left < 0) || (left >= src->cols)  ||
            (height <= 0) || ((top+height) > src->rows) ||
            (width <= 0) || ((left+width) > src->cols))
    {
        tc_write_log("crop_image: Bad subwindow dimensions.\r\n");
        return ERR;
    }
    if (tc_alloc_image(dst, height, width, src->chans) == ERR)
    {
        tc_write_log("crop_image: Out of memory.\r\n");
        return ERR;
    }
    for (b=0; b<(*dst)->chans; b++)
    {
        for (r=0; r<(*dst)->rows; r++)
        {
            for (c=0; c<(*dst)->cols; c++)
            {
                x = tc_get(src, top+r, left+c, b);
                tc_set((*dst), r, c, b, x);
            }
        }
    }
    return OK;
}



/*! Copy src to dst (preallocated). */
int tc_copy_image(tc_image *dst, tc_image *src)
{
    long int sz;

    if (src == NULL || dst == NULL)
    {
        tc_write_log("tc_copy_image: NULL image.\r\n");
        return ERR;
    }
    sz = sizeof(pixel_t) * src->rows * src->cols * src->chans;

    if (src->rows != dst->rows ||
            src->cols != dst->cols ||
            src->chans != dst->chans)
    {
        tc_write_log("tc_copy_image: dimensions do not match.\r\n");
        return ERR;
    }

    if (src->rows < 1 ||
            src->cols < 1 ||
            src->chans < 1)
    {
        tc_write_log("tc_copy_image: illegal image size.\r\n");
        return ERR;
    }

    memcpy(dst->data, src->data, sz);
    return OK;
}


#endif

