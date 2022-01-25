#set BUTool.exe in path
export PATH=$PWD/bin/tool/:$PATH

#setup LD_LIBRARY_PATHs for BUTool and any plugins
#must be source AFTER build finishes
#plugin_paths=$(find $PWD/plugins/* -type d | xargs)
plugin_paths=$(ls -1 -d $PWD/plugins/*/ | xargs)
export LD_LIBRARY_PATH=$PWD/lib:$LD_LIBRARY_PATH
for path in $plugin_paths
do
    export LD_LIBRARY_PATH=$path/lib:$LD_LIBRARY_PATH
done
  
