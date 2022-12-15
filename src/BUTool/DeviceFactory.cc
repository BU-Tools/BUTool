#include <BUTool/DeviceFactory.hh>
#include <boost/algorithm/string/case_conv.hpp>

//Singleton
BUTool::DeviceFactory * BUTool::DeviceFactory::DeviceFactory::pInstance = NULL;

static bool CLIArgsValid(std::string const & flag,std::string const & full_flag){
  return (!flag.empty() && !full_flag.empty());
}

bool BUTool::DeviceFactory::Register(std::string type, 
				     std::string name, 
				     CommandListBase * (*fPtr)(std::vector<std::string>),
				     std::string help,
				     std::string  CLI_flag,
				     std::string  CLI_full_flag,
				     std::string  CLI_description,
				     VersionTracker pluginVersion){
  boost::algorithm::to_upper(name);
  std::map<std::string,device>::iterator it = deviceMap.find(name);
  
  if(it == deviceMap.end()){
    //Register a device for the first time
    device & dev = deviceMap[name];

    dev.type = type;
    dev.fPtr = fPtr;
    dev.help = help;
    dev.pluginVersion = pluginVersion;

    fprintf(stderr,"%s %s\n",pluginVersion.GetHumanVer().c_str(),pluginVersion.GetRepositoryURI().c_str());
    
    //Check that our registered flags don't conflict with anyone elses
    bool registerCLI = true;
    for(it = deviceMap.begin();
	it != deviceMap.end();
	it++){
      if(it->second.CLI_flag == CLI_flag){
	printf("Device %s's CLI arguments conflict with %s's\n",name.c_str(),it->first.c_str());
	printf("Device %s's CLI arguments ignored\n",name.c_str());
	registerCLI = false;
	break;
      }
    }
    if(registerCLI &&
       CLIArgsValid(CLI_flag,CLI_full_flag)){
      //We have well formed CLI flags that don't conflict.  
      dev.CLI_flag        = CLI_flag;
      dev.CLI_full_flag   = CLI_full_flag;
      dev.CLI_description = CLI_description;
    }else{
      //Empty flags that will be ignored
      dev.CLI_flag        = CLI_flag;
      dev.CLI_full_flag   = CLI_full_flag;
      dev.CLI_description = CLI_description;      
    }
    
    printf("Registered device: %s\n",name.c_str());          
  }else{
    //This name is already registered.
    //This can happen since multiple files can include the header. 
    if(it->second.type != type){
      //This is trying to register a device with the same name, but different type.
      //This is very bad and we are going to explode.
      BUException::CREATOR_UNREGISTERED e;
      e.Append("Conflicting registration of name ");
      e.Append(name);
      e.Append("\nExisting type ");
      e.Append(it->second.type);
      e.Append(" conflicts with new type ");
      e.Append(type);
      e.Append("\n");
      throw e;      
    }
  }

  //Register command line options
  

  return true;
}

void BUTool::DeviceFactory::UnRegister(std::string name){
  boost::algorithm::to_upper(name);
  std::map<std::string,device>::iterator it = deviceMap.find(name);
  if(it != deviceMap.end()){
    deviceMap.erase(it);
  }
}

BUTool::CommandListBase * BUTool::DeviceFactory::Create(std::string name ,std::vector<std::string> args){
  boost::algorithm::to_upper(name);
  std::map<std::string,device>::iterator it = deviceMap.find(name);
  if(it == deviceMap.end()){
    BUException::CREATOR_UNREGISTERED e;
    e.Append("Name: ");
    e.Append(name);
    throw e;
  }

  CommandListBase * (*fptr)(std::vector<std::string>) = it->second.fPtr;
  CommandListBase * ret = (*fptr)(args);
  return ret;
  
}


std::vector<std::string> BUTool::DeviceFactory::GetDeviceNames(){
  std::vector<std::string> names;
  for(std::map<std::string,device>::iterator it = deviceMap.begin();
      it != deviceMap.end();
      it++){
    names.push_back(it->first);
  }
  return names;
}

std::string BUTool::DeviceFactory::Help(std::string name){
  boost::algorithm::to_upper(name);
  std::map<std::string,device>::iterator it = deviceMap.find(name);
  if(it != deviceMap.end()){
    return it->second.help;
  }
  return std::string("");
}

bool BUTool::DeviceFactory::Exists(std::string name){
  boost::algorithm::to_upper(name);
  std::map<std::string,device>::iterator it = deviceMap.find(name);

  bool ret = it != deviceMap.end();
  return ret;
}

bool BUTool::DeviceFactory::CLIArgs(std::string const & name,std::string & flag, std::string & full_flag, std::string &description){
  //CHeck that name exists
  if(!Exists(name)){
    return false;
  }

  //Find this entry
  std::map<std::string,device>::iterator it = deviceMap.find(name);
  //check if it has valid flags  
  if(!CLIArgsValid(it->second.CLI_flag,
		   it->second.CLI_full_flag)){
    return false;
  }

  //Return the good values
  flag = it->second.CLI_flag;
  full_flag = it->second.CLI_full_flag;
  description = it->second.CLI_description;
  return true;
}


VersionTracker BUTool::DeviceFactory::GetVersion(std::string name){
  boost::algorithm::to_upper(name);
  //local BUTool version
  if(name == "BUTOOL"){
    return BUToolVersion;
  }else{
    //Plugin version
    if(Exists(name)){
      return deviceMap[name].pluginVersion;
    }
  }
  BUException::BAD_VALUE e;
  e.Append("Bad Version name: ");
  e.Append(name);
  return VersionTracker({0},"Not configured","Not configured");
}
