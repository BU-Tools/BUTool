
default: build
include mk/helpers.mk
include mk/uhal.mk
include mk/version.mk


BUTOOL_PATH=${MAKE_PATH}
CXX=g++
RUNTIME_LDPATH?=${PWD}


#Exception class
LIBRARY_BUEXCEPTION_PATH ?= ${BUTOOL_PATH}/external/BUException
LIBRARY_BUEXCEPTION      = ${LIBRARY_BUEXCEPTION_PATH}/lib/libToolException.so

# BUTextIO class
LIBRARY_BUTEXTIO = lib/libBUTool_BUTextIO.so
LIBRARY_BUTEXTIO_SOURCES = $(wildcard src/BUTextIO/*.cc)
LIBRARY_BUTEXTIO_OBJECT_FILES = $(patsubst src/%.cc,obj/%.o,${LIBRARY_BUTEXTIO_SOURCES})

LIBRARY_TOOL = lib/libBUTool.so
LIBRARY_TOOL_SOURCES  = $(wildcard src/BUTool/*.cc)
LIBRARY_TOOL_SOURCES  += $(VERSION_FILE)
LIBRARY_TOOL_OBJECT_FILES = $(patsubst src/%.cc,obj/%.o,${LIBRARY_TOOL_SOURCES})

LIBRARY_HELPERS = lib/libBUTool_Helpers.so
LIBRARY_HELPERS_SOURCES  = $(wildcard src/BUTool/helpers/*.cc)
LIBRARY_HELPERS_SOURCES += $(wildcard src/StatusDisplay/*.cc)
LIBRARY_HELPERS_SOURCES += $(wildcard src/RegisterHelper/*.cc)
LIBRARY_HELPERS_OBJECT_FILES = $(patsubst src/%.cc,obj/%.o,${LIBRARY_HELPERS_SOURCES})

EXECUTABLE_SOURCES = $(wildcard src/BUTool/*.cxx)
EXECUTABLE_OBJECT_FILES = $(patsubst src/%.cxx,obj/%.o,${EXECUTABLE_SOURCES})
EXECUTABLES = $(patsubst src/%.cxx,bin/%.exe,${EXECUTABLE_SOURCES})

PLUGIN_DIR=plugins/

RELATIVE_INCLUDE_PATH = \
			include \
			${LIBRARY_BUEXCEPTION_PATH}/include/
INCLUDE_PATH = $(patsubst %,-I%,$(abspath ${RELATIVE_INCLUDE_PATH}))

#export INCLUDE_PATH

RELATIVE_LIBRARY_PATH = \
			lib \
			${LIBRARY_BUEXCEPTION_PATH}/lib 
LIBRARY_PATH = $(patsubst %,-L%,$(abspath ${RELATIVE_LIBRARY_PATH}))
#export LIBRARY_PATH

LIBRARIES =    	-lToolException		\
		-lreadline 		\
		-lcurses 		\
		-lz 			\
		-lboost_regex 		\
		-lboost_program_options \
		-ldl                    \
		-lBUTool_BUTextIO

INSTALL_PATH ?= ./install


EXECUTABLE_LINKED_LIBRARIES = ${LIBRARY_TOOL} ${LIBRARY_HELPERS}
EXECUTABLE_LINKED_LIBRARY_FLAGS = $(patsubst lib%,-l%,$(patsubst %.so,%,$(notdir ${EXECUTABLE_LINKED_LIBRARIES})))

EXECUTABLE_LIBRARIES = ${LIBRARIES} ${EXECUTABLE_LINKED_LIBRARY_FLAGS} 


CXX_FLAGS = -g -O3 -rdynamic -Wall -MMD -MP -fPIC ${INCLUDE_PATH} -Werror -Wno-literal-suffix

CXX_FLAGS += -std=c++11 -fno-omit-frame-pointer -pedantic -Wno-ignored-qualifiers -Werror=return-type -Wextra -Wno-long-long -Winit-self -Wno-unused-local-typedefs  -Woverloaded-virtual -DUHAL_VER_MAJOR=${UHAL_VER_MAJOR} -DUHAL_VER_MINOR=${UHAL_VER_MINOR} ${COMPILETIME_ROOT} ${COMPILETIME_ROOT} ${FALLTHROUGH_FLAGS} -Wno-unused-result

ifdef MAP_TYPE
CXX_FLAGS += ${MAP_TYPE}
endif

# Compile-time flag to specify to use the new StatusDisplay name parser for row and column names
ifeq (${BUTOOL_SD_USE_NEW_PARSER}, true)
CXX_FLAGS += -DSD_USE_NEW_PARSER
endif

LINK_LIBRARY_FLAGS = -shared -fPIC -Wall -Wl,--no-as-needed -g -O3 -rdynamic ${LIBRARY_PATH} ${LIBRARIES} -Wl,-rpath=${RUNTIME_LDPATH}/lib ${COMPILETIME_ROOT}

LINK_EXECUTABLE_FLAGS = -Wall -Wl,--no-as-needed -g -O3 -rdynamic ${LIBRARY_PATH} ${EXECUTABLE_LIBRARIES} -Wl,-rpath=${RUNTIME_LDPATH}/lib ${COMPILETIME_ROOT}

### LINTER checking ###
#LINTER?=clang-tidy 
LINTER?=:
#second line sets warnings to errors except for certain things
LINTER_DEFAULT_ARGS=-extra-arg=-Wno-unknown-warning-option -checks=readability-* --warnings-as-errors=readability-*,-readability-implicit-bool-conversion,-readability-else-after-return,-readability-function-cognitive-complexity,-readability-convert-member-functions-to-static,-clang-diagnostic-unknown-warning-option,-readability-make-member-function-const

.PHONY: all _all clean _cleanall build _buildall self plugin init git_remote_to_ssh git_remote_to_https

clean: _cleanall clean_version
_cleanall:
	rm -rf obj
	rm -rf bin
	rm -rf lib
	make -C ${PLUGIN_DIR} clean
	make -C external/BUException clean


all: _all
build: _all
buildall: _all
_all: self plugin 
self: ${EXECUTABLE_LINKED_LIBRARIES} ${LIBRARY_HELPERS} ${LIBRARY_TOOL}  ${EXECUTABLES} 

# ------------------------
# exception library
# ------------------------
${LIBRARY_BUEXCEPTION}:
	${MAKE} -C external/BUException
#	${MAKE} ${FLAGS} LIBRARY_OUTPUT_PATH=${BUTOOL_PATH}/lib -C external/BUException


# ------------------------
# BUTextIO library
# ------------------------
${LIBRARY_BUTEXTIO}: ${LIBRARY_BUTEXTIO_OBJECT_FILES} 
	mkdir -p $(dir $@)
	${CXX} -shared -fPIC -Wall -g -O3 -rdynamic ${LIBRARY_BUTEXTIO_OBJECT_FILES} -o $@

${LIBRARY_BUTEXTIO_OBJECT_FILES}: obj/%.o : src/%.cc
	mkdir -p $(dir $@)
	${LINTER} $< ${LINTER_DEFAULT_ARGS} -- $(CXX_FLAGS)	
	${CXX} ${CXX_FLAGS} -c $< -o $@

# ------------------------
# Executables
# ------------------------
${EXECUTABLES}: bin/%.exe: obj/%.o ${LIBRARY_BUEXCEPTION} ${EXECUTABLE_OBJECT_FILES}  ${LIBRARY_TOOL}  ${EXECUTABLE_LINKED_LIBRARIES} ${LIBRARY_HELPERS} ${LIBRARY_BUTEXTIO}
	mkdir -p $(dir $@)
	${CXX} $< -o $@ ${LINK_EXECUTABLE_FLAGS}

${EXECUTABLE_OBJECT_FILES}: obj/%.o : src/%.cxx
	mkdir -p $(dir $@)
	${LINTER} $< ${LINTER_DEFAULT_ARGS} -- $(CXX_FLAGS)	
	${CXX} -c ${CXX_FLAGS}  $< -o $@

-include $(EXECUTABLE_OBJECT_FILES:.o=.d)

# ------------------------
# tool library
# ------------------------
${LIBRARY_TOOL}: ${LIBRARY_TOOL_OBJECT_FILES} ${LIBRARY_BUEXCEPTION} ${LIBRARY_BUTEXTIO}
	mkdir -p $(dir $@)
	${CXX} ${LIBRARY_TOOL_OBJECT_FILES} ${LINK_LIBRARY_FLAGS} -o $@

${LIBRARY_TOOL_OBJECT_FILES}: obj/%.o : src/%.cc 
	mkdir -p $(dir $@)
	${LINTER} $< ${LINTER_DEFAULT_ARGS} -- $(CXX_FLAGS)	
	${CXX} ${CXX_FLAGS} -c $< -o $@

# ------------------------
# helpers library
# ------------------------
${LIBRARY_HELPERS}: ${LIBRARY_HELPERS_OBJECT_FILES} ${LIBRARY_BUEXCEPTION} ${LIBRARY_BUTEXTIO}
	mkdir -p $(dir $@)
	${CXX} ${LINK_LIBRARY_FLAGS} ${LIBRARY_HELPERS_OBJECT_FILES} -o $@

${LIBRARY_HELPERS_OBJECT_FILES}: obj/%.o : src/%.cc 
	mkdir -p $(dir $@)
	${LINTER} $< ${LINTER_DEFAULT_ARGS} -- $(CXX_FLAGS)	
	${CXX} ${CXX_FLAGS} ${UHAL_CXX_FLAGHS} -c $< -o $@

-include $(LIBRARY_OBJECT_FILES:.o=.d)


# -----------------------
# install
# -----------------------
install: all
	 install -m 775 -d ${INSTALL_PATH}/bin
	 install -b -m 775 ./bin/BUTool/BUTool.exe ${INSTALL_PATH}/bin
	 install -m 775 -d ${INSTALL_PATH}/lib
	 install -b -m 775 ./lib/* ${INSTALL_PATH}/lib
	 install -m 775 -d ${INSTALL_PATH}/include
	 cp -r ./include/* ${INSTALL_PATH}/include
	 ${MAKE} install ${FLAGS} -C external/BUException
	 ${MAKE} install ${FLAGS} -C ${PLUGIN_DIR} 



# -----------------------
# make plug-ins 
# -----------------------

FLAGS = $(ifeq $(MAKEFLAGS) "","",-$(MAKEFLAGS))

plugin: ${LIBRARY_BUEXCEPTION} ${LIBRARY_BUTEXTIO} ${LIBRARY_HELPERS}
	 ${MAKE} ${FLAGS} -C ${PLUGIN_DIR} 

