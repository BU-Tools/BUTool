#################################################################################
# make stuff
#################################################################################
SHELL=/bin/bash -o pipefail

#add path so build can be more generic
#
MAKE_PATH ?= $(shell dirname $(realpath ./))

OUTPUT_MARKUP= 2>&1 | tee -a ../make_log.txt | ccze -A

init:
	git submodule update --init --recursive 
	$(git remote -v | grep push | sed 's/https:\/\//git@/g' | sed 's/.com\//.com:/g' | awk '{print "git remote set-url --push " $1 " " $2}')


#################################################################################
# gcc fallthrough checking
#################################################################################
HAS_FALLTHROUGH=$(shell ./mk/has_Wimplicit-fallthrough)
ifeq (${HAS_FALLTHROUGH},1)
     FALLTHROUGH_FLAGS= -Wimplicit-fallthrough=0
else
     FALLTHROUGH_FLAGS=
endif

export
