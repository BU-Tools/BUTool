# -----------------------
# Variable defines
# -----------------------

# You can defin some global variables here for all plugins

PLUGINS=$(wildcard */Makefile)
PLUGIN_TARGETS=$(patsubst %/Makefile,%,${PLUGINS})
PLUGIN_CLEAN=$(addsuffix clean ,${PLUGIN_TARGETS})
PLUGIN_INSTALL=$(addsuffix install ,${PLUGIN_TARGETS})


.PHONY: all _all clean ${PLUGIN_TARGETS} ${PLUGIN_CLEAN} 

default: build
clean: _cleanall
_cleanall: ${PLUGIN_CLEAN}
${PLUGIN_CLEAN}:
	make -C $(patsubst %clean,%, $@) clean

install: _installall
_installall: ${PLUGIN_INSTALL}
${PLUGIN_INSTALL}:
	make -C $(patsubst %install,%, $@) install

all: _all
build: _all
buildall: _all
_all:  ${PLUGIN_TARGETS}

export INCLUDE_PATH
export LIBRARY_PATH
# -----------------------
# make plug-ins 
# -----------------------

# You can define dependancies between plugins here

FLAGS = $(ifeq $(MAKEFLAGS) "","",-$(MAKEFLAGS))

-include Makefile.plugins

${PLUGIN_TARGETS}:
	 ${MAKE} ${FLAGS} -C $@ 
