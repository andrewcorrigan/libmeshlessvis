# this file is based on the nvidia linux makefile which had the following copyright notice:
################################################################################
#
# Copyright 1993-2006 NVIDIA Corporation.  All rights reserved.
#
# NOTICE TO USER:   
#
# This source code is subject to NVIDIA ownership rights under U.S. and 
# international Copyright laws.  
#
# NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE 
# CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR 
# IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH 
# REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF 
# MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.   
# IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL, 
# OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS 
# OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE 
# OR PERFORMANCE OF THIS SOURCE CODE.  
#
# U.S. Government End Users.  This source code is a "commercial item" as 
# that term is defined at 48 C.F.R. 2.101 (OCT 1995), consisting  of 
# "commercial computer software" and "commercial computer software 
# documentation" as such terms are used in 48 C.F.R. 12.212 (SEPT 1995) 
# and is provided to the U.S. Government only as a commercial end item.  
# Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through 
# 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the 
# source code with only those rights set forth herein.
#

include ../common.mk

INCLUDES := -I$(CUDA_INSTALL_PATH)/include -I$(CUDA_SDK_INSTALL_PATH)/common/inc -I. -I../include $(GLEW_INCLUDE_PATH)

LIBRARY := libmeshless_vis

CUFILES	:= meshless_vis.cu fourier_transform.cu
CCFILES := meshless.cpp

.SUFFIXES : .cu .cu_dbg_o .c_dbg_o .cpp_dbg_o .cu_rel_o .c_rel_o .cpp_rel_o .cubin

SRCDIR     ?= 
ROOTOBJDIR ?= obj

NVCC       := nvcc 
CXX        := g++
CC         := gcc
LINK       := g++ -fPIC

CXXWARN_FLAGS := \
	-W -Wall \
	-Wimplicit \
	-Wswitch \
	-Wformat \
	-Wchar-subscripts \
	-Wparentheses \
	-Wmultichar \
	-Wtrigraphs \
	-Wpointer-arith \
	-Wcast-align \
	-Wreturn-type \
	-Wno-unused-function \
	$(SPACE)

CWARN_FLAGS := $(CXXWARN_FLAGS) \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wmissing-declarations \
	-Wnested-externs \
	-Wmain \

CXXFLAGS  := $(CXXWARN_FLAGS)
CFLAGS    := $(CWARN_FLAGS)

COMMONFLAGS = $(INCLUDES) -DUNIX

ifeq ($(dbg),1)
	COMMONFLAGS += -g
	NVCCFLAGS   += -D_DEBUG
	BINSUBDIR   := debug
else
	COMMONFLAGS += -O3
	BINSUBDIR   := release
endif

ifeq ($(emu), 1)
	NVCCFLAGS   += -deviceemu
	BINSUBDIR   := emu$(BINSUBDIR)
endif

LIBDIR := ../lib
TARGET = $(LIBDIR)/$(LIBRARY)$(SUFFIX).a

LINKLINE  = ar qv $(TARGET) $(OBJS)

ifeq ($(fastmath), 1)
	NVCCFLAGS += -use_fast_math
endif

NVCCFLAGS += $(COMMONFLAGS)
CXXFLAGS  += $(COMMONFLAGS)
CFLAGS    += $(COMMONFLAGS)

OBJDIR := $(ROOTOBJDIR)/$(BINSUBDIR)
OBJS :=  $(patsubst %.cpp,$(OBJDIR)/%.cpp_o,$(notdir $(CCFILES)))
OBJS +=  $(patsubst %.c,$(OBJDIR)/%.c_o,$(notdir $(CFILES)))
OBJS +=  $(patsubst %.cu,$(OBJDIR)/%.cu_o,$(notdir $(CUFILES)))

CUBINDIR := $(SRCDIR)data
CUBINS +=  $(patsubst %.cu,$(CUBINDIR)/%.cubin,$(notdir $(CUBINFILES)))

all: $(TARGET)

$(OBJDIR)/%.c_o : $(SRCDIR)%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/%.cpp_o : $(SRCDIR)%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/%.cu_o : $(SRCDIR)%.cu
	$(NVCC) -o $@ -c $< $(NVCCFLAGS)

$(CUBINDIR)/%.cubin : $(SRCDIR)%.cu cubindirectory
	$(NVCC) -o $@ -cubin $< $(NVCCFLAGS)

$(TARGET): makedirectories $(OBJS) $(CUBINS) makefile
	$(LINKLINE)

cubindirectory:
	@mkdir -p $(CUBINDIR)

makedirectories:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)

tidy :
	@find | egrep "#" | xargs rm -f
	@find | egrep "\~" | xargs rm -f

clean : tidy
	rm -f $(OBJS)
	rm -f $(CUBINS)
	rm -f $(TARGET)

clobber : clean
	rm -rf $(ROOTOBJDIR)
