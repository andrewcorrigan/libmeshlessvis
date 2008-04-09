#
#libMeshlessVis
#Copyright (C) 2008 Andrew Corrigan
#
#This program is free software; you can redistribute it and/or
#modify it under the terms of the GNU General Public License
#as published by the Free Software Foundation; either version 2
#of the License, or (at your option) any later version.
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with this program; if not, write to the Free Software
#Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

include ../settings.mk

ifneq "$(strip $(shell uname -m | grep 64))" ""
	SUFFIX := $(SUFFIX)64
endif

ifeq ($(dbg),1)
	OPTIONS := -g
	SUFFIX := $(SUFFIX)D
else
	OPTIONS := -O3
endif

ifeq ($(cpu), 1)
	SUFFIX := $(SUFFIX)_cpu
else
	ifeq ($(emu), 1)
		SUFFIX := $(SUFFIX)_emu
	endif
endif


MESHLESS_VIS_INCLUDE_PATH := -I$(MESHLESS_VIS_PATH)/include
MESHLESS_VIS_LIB_PATH := -L$(MESHLESS_VIS_PATH)/lib

MAGICK_INCLUDE_PATH := -I$(MAGICK_PATH)/include
MAGICK_LIB_PATH := -L$(MAGICK_PATH)/lib

GLEW_INCLUDE_PATH := -I$(GLEW_PATH)/include
GLEW_LIB_PATH := -L$(GLEW_PATH)/lib

FFTW_INCLUDE_PATH := -I$(FFTW_PATH)/include
FFTW_LIB_PATH := -L$(FFTW_PATH)/lib

CUDA_INCLUDE_PATHS := -I$(CUDA_INSTALL_PATH)/include -I$(CUDA_SDK_INSTALL_PATH)/common/inc
CUDA_LIB_PATHS := -L$(CUDA_INSTALL_PATH)/lib -L$(CUDA_SDK_INSTALL_PATH)/lib -L$(CUDA_SDK_INSTALL_PATH)/common/lib

MESHLESS_VIS := $(MESHLESS_VIS_INCLUDE_PATH) $(MESHLESS_VIS_LIB_PATH) -lmeshless_vis$(SUFFIX) $(CUDA_LIB_PATHS)
ifeq ($(cpu), 1)
	MESHLESS_VIS +=  -D_LIBMESHLESSVIS_USE_CPU
else
	MESHLESS_VIS += -lcudpp$(SUFFIX)
endif

MAGICK := $(MAGICK_INCLUDE_PATH) $(MAGICK_LIB_PATH) -lMagick++

FFTW := $(FFTW_INCLUDE_PATH) $(FFTW_LIB_PATH) -lfftw3f -lfftw3f_threads

CUDA := $(CUDA_INCLUDE_PATHS) $(CUDA_LIB_PATHS)
ifneq ($(cpu), 1)
	CUDA += -lcuda -lcudart -lcutil
endif

ifneq ($(cpu), 1)
	ifeq ($(emu), 1)
		CUDA += -lcufftemu
	else
		CUDA += -lcufft
	endif
endif

GLEW := $(GLEW_INCLUDE_PATH) $(GLEW_LIB_PATH) -lGLEW -lGL
ifeq ($(needwx), 1)
	WX := $(shell wx-config --libs std,gl --cppflags)
endif




BINDIR := ../bin



