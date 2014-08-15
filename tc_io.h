/**
 * \file tc_io.h
 * \brief Header for tc_io
 * \author Kevin Fabricio Ortega
 *
 * Copyright 2012, by the California Institute of Technology. ALL RIGHTS
 * RESERVED. United States Government Sponsorship acknowledged. Any
 * commercial use must be negotiated with the Office of Technology
 * Transfer at the California Institute of Technology.
 */

#ifndef TC_IO_H_
#define TC_IO_H_

#define BUF_SIZE 256

void tc_close_io(void *tc_io);
int tc_getline_io(void *tc_io, char *buffer, int buf_size, char delim);
int tc_write_io(void *tc_io, char *buffer, int buf_size);
void* tc_init_io(char *filename, const char *mode);

#endif /* TC_IO_H_ */
