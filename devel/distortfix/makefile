# Copyright 2009, 2010 IPOL Image Processing On Line http://www.ipol.im/
# Author: Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
#
# Copying and distribution of this file, with or without
# modification, are permitted in any medium without royalty provided
# the copyright notice and this notice are preserved.  This file is
# offered as-is, without any warranty.

CSRC	= ami_tif.c ami_pol.c lens_distortion.c lens_distortion_estimation.c

SRC	= $(CSRC)
OBJ	= $(CSRC:.c=.o)
BIN	= lens_distortion_estimation

COPT	= -funroll-loops -fomit-frame-pointer 
CFLAGS	+= -ansi -Wall -Wextra -Werror #$(COPT)

default: $(BIN)

%.o	: %.c
	$(CC) $(CFLAGS) -c -o $@ $< 

lens_distortion_estimation	: ami_tif.o ami_pol.o lens_distortion.o \
					lens_distortion_estimation.o
	$(CC) $(CFLAGS) -o $@  $^ -lm

.PHONY	: clean distclean
clean	:
	$(RM) $(OBJ)
distclean	: clean
	$(RM) $(BIN)
