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
