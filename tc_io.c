/**
 * \file tc_io.c
 * \brief Header for tc_io
 * \author Kevin Fabricio Ortega
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#include <stdio.h>
#include "tc_io.h"
#include "tc_image.h"


void tc_close_io(void *tc_io)
{
    fclose((FILE*)tc_io);
}


int tc_getline_io(void *tc_io, char *buffer, int buf_size, char delim)
{
    int i = 0;
    char endofline = '\n';
    for(i = 0; i < buf_size; i++)
    {
        fread(buffer+i, 1, 1, (FILE*)tc_io);
        if(buffer[i] == delim || buffer[i] == endofline)
            break;
    }
    if(buffer[i] != delim && buffer[i] != endofline)
    {
        return ERR;
    }
    else
    {
        buffer[i] = '\0';
        return OK;
    }
}


int tc_write_io(void *tc_io, char *buffer, int buf_size)
{
    int ret_val = 0;
    ret_val = fwrite(buffer, 1, buf_size, (FILE*)tc_io);
    if( ret_val > 0 )
    {
        return OK;
    }
    else
    {
        return ERR;
    }
}


void* tc_init_io(char *filename, const char *mode)
{
    void *tc_io = 0;
    if(filename != NULL)
    {
        if ((tc_io = (FILE*)fopen(filename, mode)) == NULL)
        {
            return NULL;
        }

        return tc_io;
    }
    else
    {
        return NULL;
    }
}
