#set BUTool.exe in path
export PATH=$PWD/bin/tool/:$PATH

#setup LD_LIBRARY_PATHs for BUTool and any plugins
#must be source AFTER build finishes

# Add BUTool's lib/ directory to LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$PWD/lib:$LD_LIBRARY_PATH

# Add the plugin libraries to the LD_LIBRARY_PATH

#plugin_paths=$(find $PWD/plugins/* -type d | xargs)
plugin_paths=$(ls -1 -d $PWD/plugins/* | xargs)

for path in $plugin_paths
do
    # Check if this is a directory
    if [ -d $path  ]; then
        export LD_LIBRARY_PATH=$path/lib:$LD_LIBRARY_PATH
    fi
done
  
# Add BUException submodule to the LD_LIBRARY_PATH
EXCEPTION_PATH="$PWD/external/BUException/lib"
export LD_LIBRARY_PATH=$EXCEPTION_PATH:$LD_LIBRARY_PATH
