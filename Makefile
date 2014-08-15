# 
# Filename    : Makefile
# Description : Builds texturecam library
# Author      : David R Thompson, based on code by Ben Bornstein
# 
# Copyright 2014, by the California Institute of Technology. ALL RIGHTS 
# RESERVED. United States Government Sponsorship acknowledged. Any commercial 
# use must be negotiated with the Office of Technology Transfer at the 
# California Institute of Technology.  
#

CC       ?= gcc
AR       ?= ar
RANLIB   ?= ranlib
CFLAGS   += -Wall -O3 -DTC_LITTLE_ENDIAN 

objects = \
  tc_io.o \
  tc_image.o \
  tc_colormap.o \
  tc_preproc.o \
  tc_filter.o \
  tc_node.o \
  tc_tree.o \
  tc_dataset.o \
  tc_disjoint.o \
  tc_forest.o

sources = \
  tc_io.c \
  tc_image.c \
  tc_colormap.c \
  tc_preproc.c \
  tc_filter.c \
  tc_node.c \
  tc_tree.c \
  tc_dataset.c \
  tc_disjoint.c \
  tc_forest.c 

headers = \
  tc_io.h \
  tc_bar_fixed.h \
  tc_prep.h \
  tc_image.h \
  tc_colormap.h \
  tc_preproc.h \
  tc_filter.h \
  tc_node.h \
  tc_tree.h \
  tc_dataset.h \
  tc_disjoint.h \
  tc_forest.h

program = \
  tcprep \
  tctrain \
  tcclass \
  catpgm \
  catforest 

libs  += -lm 
libtc = libtc.a
libcutest = cutest-1.5/libcutest.a

# Build Actions

all:  $(program) $(libtc) 

catforest:	$(objects) $(sources) $(headers) tc_catforest.c 
	${CC} $(CFLAGS) -o catforest $(objects) tc_catforest.c $(libs)
catpgm:	$(objects) $(sources) $(headers) tc_catpgm.c
	${CC} $(CFLAGS) -o catpgm $(objects) tc_catpgm.c $(libs)
tcprep:	$(objects) $(sources) $(headers) tc_prep.c
	${CC} $(CFLAGS) -o tcprep $(objects) tc_prep.c $(libs)
tcclass:	$(objects) $(sources) $(headers) tc_classify.c
	${CC} $(CFLAGS) -o tcclass $(objects) tc_classify.c $(libs)
tctrain:	$(objects) $(sources) $(headers) tc_train.c
	${CC} $(CFLAGS) -o tctrain $(objects) tc_train.c $(libs) -lpthread

$(libtc): $(objects)
	@echo
	@echo "--------------------------------------------------"
	@echo "Building library $(libtc)"
	@echo "--------------------------------------------------"
	$(AR) -cru $@ $(objects)
	$(RANLIB) $@

clean:
	/bin/rm -f *.o *.a 

distclean: clean
	/bin/rm -f $(library) $(program) $(tester)



# -----------------------------------------------------------------------------
# End
# -----------------------------------------------------------------------------

