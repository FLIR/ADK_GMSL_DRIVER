# Copyright (c) 2014-2016, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.

include ../make/nvdefs.mk

TARGETS = nvmimg_cc

CFLAGS   = $(NV_PLATFORM_OPT) $(NV_PLATFORM_CFLAGS)
CPPFLAGS = $(NV_PLATFORM_SDK_INC) $(NV_PLATFORM_CPPFLAGS) -std=c++11 -ggdb -I. -I../utils -I../BosonSDK/ClientFiles_C -I../BosonSDK/FSLP_Nvidia/src/inc
LDFLAGS  = $(NV_PLATFORM_SDK_LIB) $(NV_PLATFORM_TARGET_LIB) $(NV_PLATFORM_LDFLAGS)

OBJS   := main.o
OBJS   += nvidiaInterface.o
OBJS   += capture.o
OBJS   += commandListener.o
OBJS   += bosonInterface.o
OBJS   += bosonCommands.o
OBJS   += check_version.o
OBJS   += cmdline.o
OBJS   += helpers.o
OBJS   += display.o
OBJS   += i2cCommands.o
OBJS   += parser.o
OBJS   += save.o
OBJS   += ../utils/log_utils.o
OBJS   += ../utils/misc_utils.o
OBJS   += ../utils/surf_utils.o
OBJS   += ../utils/thread_utils.o

LDLIBS  := -L.
LDLIBS  += -lopencvConnector
LDLIBS	+= -l:C_SDK.so
LDLIBS 	+= -l:FSLP_64.so
LDLIBS	+= -L ../cv_install/lib
LDLIBS	+= -lopencv_imgcodecs
LDLIBS  += -L ../utils
LDLIBS  += -lnvmedia
LDLIBS  += -lnvmedia_isc
LDLIBS  += -lnvrawfile_interface
LDLIBS  += -lnvtvmr
LDLIBS  += -lz
LDLIBS  += -lm
LDLIBS  += -lnvtestutil_board
LDLIBS  += -lnvtestutil_i2c

CFLAGS  += -D_FILE_OFFSET_BITS=64

ifeq ($(NV_PLATFORM_OS), Linux)
    LDLIBS  += -lpthread
endif

ifeq ($(NV_PLATFORM_OS), QNX)
  CFLAGS += -DNVMEDIA_QNX
endif

include ../make/nvdefs.mk

$(TARGETS): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean clobber:
	rm -rf $(OBJS) $(TARGETS)
