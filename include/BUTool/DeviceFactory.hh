#ifndef __DEVICE_FACTORY_HH__
#define __DEVICE_FACTORY_HH__

#include <string>
#include <vector>
#include <map>

#include <BUTool/CommandList.hh>
#include <BUTool/VersionTracker.hh>

#define RegisterDevice(ClassName,ClassNickName,ClassHelp,CLIFlag,CLIFullFlag,CLIDescription,pluginVersion) \
  namespace {								\
    /*make creator function*/						\
    BUTool::CommandListBase * creator_function(std::vector<std::string> args){ \
      return new ClassName(args);					\
    }									\
    /*Register the device with the DeviceFactory*/			\
    const char type[] = #ClassName;					\
    const char name[] = ClassNickName;					\
    const char help[] = ClassHelp;					\
    const char CLI_flag[] = CLIFlag;					\
    const char CLI_full_flag[] = CLIFullFlag;				\
    const char CLI_description[] = CLIDescription;			\
    const VersionTracker version = pluginVersion;                       \
    const bool registered = BUTool::DeviceFactory::Instance()->Register(type, \
									name, \
									&creator_function, \
									help, \
									CLI_flag, \
									CLI_full_flag,\
									CLI_description,\
									version); \
  }

namespace BUTool{
  class DeviceFactory{
  public:
    //Singleton stuff
    static DeviceFactory * Instance() {
      if(NULL == pInstance){
	pInstance = new DeviceFactory;
      }
      return pInstance;
    }; 

    bool Register(std::string  type,
		  std::string  name, 
		  CommandListBase * (*fPtr)(std::vector<std::string>),
		  std::string  help,
		  std::string  CLI_flag,
		  std::string  CLI_full_flag,
		  std::string  CLI_description,
		  VersionTracker pluginVersion
		  );
    void UnRegister(std::string  name);


    CommandListBase * Create(std::string,std::vector<std::string>);
    std::vector<std::string> GetDeviceNames();  
    std::string Help(std::string);
    bool CLIArgs(std::string const & name,std::string & flag, std::string & full_flag, std::string &description);
    bool Exists(std::string name);
    VersionTracker GetVersion(std::string name = "BUTOOL" );
  private:
    //Singleton stuff
    //Never implement
    DeviceFactory(){};
    DeviceFactory(const DeviceFactory &);
    DeviceFactory & operator=(const DeviceFactory &);
  
    ~DeviceFactory();
    static DeviceFactory * pInstance;
    static VersionTracker const BUToolVersion;
    
    //List stuff
    struct device{
      std::string type;
      CommandListBase * (*fPtr)(std::vector<std::string>);
      std::string help;
      std::string  CLI_flag;
      std::string  CLI_full_flag;
      std::string  CLI_description;
      VersionTracker pluginVersion;
    };
    std::map<std::string,device> deviceMap;
  };
}
#endif
