#############################################################################
# Setting up the global compiler settings...

# The names of the executables
CXX           = c++
RM            = rm -f
MAKE          = make

DBG           = off

ifeq (RyzomCompilerFlags.mk,$(wildcard RyzomCompilerFlags.mk))
include RyzomCompilerFlags.mk
endif

FLAGS_CMN     = -g -pipe -fno-stack-protector -fno-strict-aliasing -Wall -D_REENTRANT -D_GNU_SOURCE -DFINAL_VERSION=1
LD_FLAGS_CMN  = -rdynamic

FLAGS_DBG_on  = -O0 -finline-functions -DNL_DEBUG
FLAGS_DBG_off = -O3 -funroll-loops -DNL_RELEASE
DIR_DBG_on    = debug
DIR_DBG_off   = release

PACK_SHEETS_FLAGS = -A$RYZOM_PATH/server -L$RYZOM_PATH/server -C$RYZOM_PATH//server/sheet_pack_cfg -Q --nons


NEL_PATH = $(RYZOM_PATH)/../../code/install/$(DIR_DBG_$(DBG))

NEL_INCLUDE = $(RYZOM_PATH)/../../code/nel/include
RYZOM_COMMON_SRC = $(RYZOM_PATH)/common/src

ifeq (Objects.mk,$(wildcard Objects.mk))
include Objects.mk
endif
