#include <BUTool/Launcher.hh>
#include <stdio.h>
#include <cassert>

using namespace BUTool;

Launcher::Launcher():CommandList<Launcher>("Launcher")
{
  verbosity = 0;
  activeDevice = -1;
  LoadCommandList(); //Load list
}
  
Launcher::~Launcher()
{
  //delete all devices
  while(!device.empty()){
    if(device.back() != NULL){
      delete device.back();
    }
    device.pop_back();
  }
  //delete all ostreams we created
  while(!ownedOutputStreams.empty()){
    if(ownedOutputStreams.back() != NULL){
      delete ownedOutputStreams.back();
    }
    ownedOutputStreams.pop_back();
  }
  
}

uint32_t Launcher::GetVerbosity(){
  return verbosity;
}

CommandReturn::status Launcher::EvaluateCommand(std::vector<std::string> commandLine){
  //Call this command on the inherited CommandList
  CommandReturn::status ret = CommandList<Launcher>::EvaluateCommand(commandLine);
  //If we can't find that command locally, try the active device
  if(ret == CommandReturn::NOT_FOUND){
    if((activeDevice >=0) && (size_t(activeDevice) < device.size())){
      std::vector<CommandListBase *>::iterator itActiveDevice = device.begin()+activeDevice;
      if(itActiveDevice != device.end()){
	ret = (*itActiveDevice)->EvaluateCommand(commandLine);
      }
    }
  }
  if(ret == CommandReturn::BAD_ARGS){
    std::vector<uint64_t> placeHolder(commandLine.size(),0);
    Help(commandLine,placeHolder);
  }
  return ret;
}  

std::string Launcher::AutoCompleteCommand(std::string const & line, int state){
  //Build state for active device
  static int activeDeviceState;
  if(state == 0){
    activeDeviceState = 0;
  }

  std::string ret = CommandList<Launcher>::AutoCompleteCommand(line,state);
  //Try to auto complete command in Launcher
  if(!ret.empty()){
    return ret;
  }else if((activeDevice >=0) && (size_t(activeDevice) < device.size())){
    //Search active device
    ret = device[activeDevice]->AutoCompleteCommand(line,activeDeviceState);
    activeDeviceState++; //Move to next state in active device
    return ret;
  }
  //nothing more
  return std::string("");
}

std::string Launcher::AutoCompleteSubCommand(std::vector<std::string> const & line,
					     std::string const & currentToken,int state){
  //Build state for active device
  static int activeDeviceState;
  if(state == 0){
    activeDeviceState = 0;
  }

  std::string ret = CommandList<Launcher>::AutoCompleteSubCommand(line,currentToken,state);
  //Try to auto complete command in Launcher
  if(!ret.empty()){
    return ret;
  }else if((activeDevice >=0) && (size_t(activeDevice) < device.size())){
    //Search active device
    ret = device[activeDevice]->AutoCompleteSubCommand(line,currentToken,activeDeviceState);
    activeDeviceState++; //Move to next state in active device
    return ret;
  }
  //nothing more
  return std::string("");
}
