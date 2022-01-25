BUILD_MODE = x86

export CACTUS_ROOT
export UIO_UHAL_PATH

UHAL_VER_MAJOR ?= 2
UHAL_VER_MINOR ?= 7


HAS_FALLTHROUGH=$(shell ./mk/has_Wimplicit-fallthrough)
ifeq (${HAS_FALLTHROUGH},1)
     FALLTHROUGH_FLAGS= -Wimplicit-fallthrough=0
else
     FALLTHROUGH_FLAGS=
endif

export

ifdef BOOST_INC
INCLUDE_PATH +=-I$(BOOST_INC)
endif
ifdef BOOST_LIB
LIBRARY_PATH +=-L$(BOOST_LIB)
endif

