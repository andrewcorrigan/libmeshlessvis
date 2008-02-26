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

ifeq ($(emu), 1)
	ifeq ($(dbg),1)
		SUFFIX := _emuD
	else
		SUFFIX := _emu
	endif
else
	ifeq ($(dbg),1)
		SUFFIX := _D
	else
		SUFFIX := 
	endif
endif

ifeq ($(dbg),1)
	OPTIONS := -g
else
	OPTIONS := -O3
endif

MESHLESS_VIS_INCLUDE_PATH := -I$(MESHLESS_VIS_PATH)/include
MESHLESS_VIS_LIB_PATH := -L$(MESHLESS_VIS_PATH)/lib

MAGICK_INCLUDE_PATH := -I$(MAGICK_PATH)/include
MAGICK_LIB_PATH := -L$(MAGICK_PATH)/lib

GLEW_INCLUDE_PATH := -I$(GLEW_PATH)/include
GLEW_LIB_PATH := -L$(GLEW_PATH)/lib

CUDA_INCLUDE_PATHS := -I$(CUDA_INSTALL_PATH)/include -I$(CUDA_SDK_INSTALL_PATH)/common/inc
CUDA_LIB_PATHS := -L$(CUDA_INSTALL_PATH)/lib -L$(CUDA_SDK_INSTALL_PATH)/lib

MESHLESS_VIS := $(MESHLESS_VIS_INCLUDE_PATH) $(MESHLESS_VIS_LIB_PATH) -lmeshless_vis$(SUFFIX)
MAGICK := $(MAGICK_INCLUDE_PATH) $(MAGICK_LIB_PATH) -lMagick++
CUDA := $(CUDA_INCLUDE_PATHS) $(CUDA_LIB_PATHS)  -lcuda -lcudart -lcutil
GLEW := $(GLEW_INCLUDE_PATH) $(GLEW_LIB_PATH) -lGLEW
ifeq ($(needwx), 1)
	WX := $(shell wx-config --libs std,gl --cppflags)
endif
ifeq ($(emu), 1)
	CUDA += -lcufftemu
else
	CUDA += -lcufft
endif


BINDIR := ../bin
