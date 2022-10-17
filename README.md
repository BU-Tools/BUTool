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

## Build instructions for local build
After checking out the BUTool source:
```Bash
 $ cp mk/Makefile.local ./Makefile
 $ make
 $ source env.sh
 $ BUTool.exe
```


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


