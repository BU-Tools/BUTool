#include <BUTool/Launcher.hh>
#include <BUTool/DeviceFactory.hh>

using namespace BUTool;

#include <dlfcn.h>

CommandReturn::status Launcher::AddLib(std::vector<std::string> strArg,std::vector<uint64_t> intArg){  
  (void) intArg; // to make compiler not complain about unused arguments
  if(!strArg.empty()){
    void * lib_handle = NULL;
    lib_handle = dlopen(strArg[0].c_str(),RTLD_NOW);
    if(lib_handle == NULL){
      char * errorString = dlerror();
      if(errorString != NULL){
	printf("Error: %s\n",errorString);
      }else{
	return CommandReturn::BAD_ARGS;
      }
    }
  }else{
    return CommandReturn::BAD_ARGS;
  }
  return CommandReturn::OK;
}


CommandReturn::status Launcher::AddDevice(std::vector<std::string> strArg,std::vector<uint64_t> intArg){  
  (void) intArg; // to make compiler not complain about unused arguments

  if(!strArg.empty()){
    CommandListBase * newDevice = NULL;

    //Generate the arguments to be sent to the device constructor
    std::vector<std::string> args;
    for(size_t iArg = 1;
	iArg < strArg.size();
	iArg++){
      args.push_back(strArg[iArg]);
    }


    try{
      newDevice = BUTool::DeviceFactory::Instance()->Create(strArg[0],args);
    }catch(BUException::exBase & e){
      //Something went wrong above
      printf("Exception: %s\n%s\n",e.what(),e.Description());
      //Check if the name is in the list
      if(BUTool::DeviceFactory::Instance()->Exists(strArg[0])){
	//The name was on the list, but something still went wrong.
	if(! BUTool::DeviceFactory::Instance()->Help(strArg[0]).empty()){
	  //Print the help if there is any
	  printf("Usage: %s %s\n",strArg[0].c_str(),BUTool::DeviceFactory::Instance()->Help(strArg[0]).c_str());
	}	
      }
      newDevice = NULL;
    }catch(std::exception & e){
      printf("Exception: %s\n",e.what());
      newDevice = NULL;
    }

    //Add a sucessful creation to list and set it as the active device
    if(newDevice != NULL){
      device.push_back(newDevice);          
      activeDevice = device.size()-1;
    }else{
      return CommandReturn::BAD_ARGS;
    }
  }
  return CommandReturn::OK;
}

std::string Launcher::autoComplete_AddDevice(std::vector<std::string> const & line,std::string const & currentToken ,int state)
{  
  if(!line.empty()){
    if((line.size() > 1) && (!currentToken.empty())){
      return std::string("");
    }

    static std::vector<std::string> commandName;
    static size_t iCommand;

    //Reload lists if we are just starting out at state == 0
    if(state == 0){
      commandName = BUTool::DeviceFactory::Instance()->GetDeviceNames();
      iCommand = 0;
    }else{
      iCommand++;
    }

    for(;iCommand < commandName.size();iCommand++)
      {
      //Do the string compare and make sure the token is found a position 0 (the start)
	if(commandName[iCommand].find(currentToken) == 0){
	  return commandName[iCommand];
	}
      }
  }
  //not found
  return std::string("");  
}


CommandReturn::status Launcher::GetVersion(std::vector<std::string> iArg,
					   std::vector<uint64_t> /*asdf*/){
  std::vector<std::string> version;
  
  if(iArg.size() > 0){
    version.push_back(iArg[0]);
  }else{
    //Add the tool version to the list
    version.push_back(std::string("BUTOOL"));
    //Add all the devices to the list
    std::vector<std::string> const pluginList = BUTool::DeviceFactory::Instance()->GetDeviceNames();
    version.insert(version.end(),pluginList.begin(),pluginList.end());
  }

  Print(Level::INFO,"Name           Version                     notes\n");
  for(auto itVer = version.begin(); itVer != version.end();itVer++){
    VersionTracker pluginVersion = BUTool::DeviceFactory::Instance()->GetVersion(*itVer);
    Print(Level::INFO,
	  "%-13s  %-27s %s\n",
	  itVer->c_str(),
	  pluginVersion.GetHumanVer().c_str(),
	  pluginVersion.GetRepositoryURI().c_str()
	  );
  }
  return CommandReturn::OK;
}
