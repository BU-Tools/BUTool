# BUTool v1.0

BUTool is a CLI tool used to interact with hardware built at the Boston University
Electronics Design Facility.
It makes use of plug-in libraries for interacting with different pieces of
hardware.
Instructions on how to add plug-ins can be found on the BUTool wiki
https://bu-edf.gitlab.io/BUTool/



### Common pre-req's
* boost-devel
* readline-devel
* ld

### Ubuntu packages required for build
```
$ sudo apt install libboost-regex-dev build-essential zlib1g-dev libreadline-dev libboost-dev libboost-program-options-dev libncurses5-dev
```

## Build Instructions

`BUTool` can be built either standalone, or together with all other plugins using the parent `ApolloTool`
repository [here](https://github.com/apollo-lhc/ApolloTool). Both methods use `make` as the build tool.

### Building With ApolloTool

Clone the `ApolloTool` repository recursively, with all the plugins and `BUTool` included:

```
git clone --recursive https://github.com/apollo-lhc/ApolloTool.git
```

Please refer to the build instructions [here](https://apollo-lhc.gitlab.io/Software/apollo-tool/) to build and
install the project. 

### Building Standalone

`BUTool` can be built standalone on a computer where plugins are already installed under `/opt/BUTool`. 
To do this, clone the BUTool source code from git. Do a recursive clone so that the `BUException`
sub-module is also cloned under the path `external/BUException`.

```
git clone --recursive https://github.com/BU-Tools/BUTool.git
```

Once the repo is cloned, the following commands can be executed to build `BUTool`, and install it under `/opt/BUTool`:

```
cd BUTool/

# Do the build
make RUNTIME_LDPATH=/opt/BUTool COMPILETIME_ROOT=--sysroot=/  LIBRARY_BUEXCEPTION_PATH=/opt/BUTool

# Install it under /opt/BUTool
make install RUNTIME_LDPATH=/opt/BUTool COMPILETIME_ROOT=--sysroot=/ \
  LIBRARY_BUEXCEPTION_PATH=/opt/BUTool INSTALL_PATH=/opt/BUTool
```

Note that `RUNTIME_LDPATH` is set to `/opt/BUTool` such that at runtime (when `BUTool` is launched), the linker looks for libraries under the path `/opt/BUTool/lib`. 

`INSTALL_PATH` is set to `/opt/BUTool` for the target install location, if this variable is not specified, the default install location is `./install`. Note that if the install location is different, `LD_LIBRARY_PATH` must be modified with the paths that have the `.so` library files. 

After this is done, you can simply run `BUTool` with an ApolloSM device attached as usual:

```
BUTool.exe -a
```

**Note about linting:** By default, `make` will attempt to run `clang-tidy` linter. If you want to disable it, just put `LINTER=:` before each `make` command shown above.

## Adding plugins
It is recommended to check out plugins into the plugins/ directory.
The main Makefile will call make for plugins added into the plugins/ directory.
The following variables will be passed on to plugin Makefiles:
* BUTOOL_PATH
* BUILD_MODE
* CXX
* RUNTIME_LDPATH
* COMPILETIME_ROOT
* FALLTHROUGH_FLAGS

Plugins can use these variables to build with the same settings as BUTool,
e.g. deciding whether building for linux native or for zynq petalinux

You can also specify common variables and cross-dependencies for plugins by modifying:
>  plugins/Makefile

To build only BUTool, call:
>  $ make self

To build only plugins, call:
>  $ make plugin

If you wish to build plugins seperately from their own location, source env.sh first.


