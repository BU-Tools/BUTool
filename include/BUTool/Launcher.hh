#ifndef __LAUNCHER_HPP__
#define __LAUNCHER_HPP__
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <stdint.h>
#include <BUTool/CommandReturn.hh>
#include <BUTool/CommandList.hh>
#include <ostream> //for base class of vecetor of ostreams

namespace BUTool{

  class Launcher : public CommandList<Launcher> {
  public:
    Launcher();
    ~Launcher();   
    CommandReturn::status EvaluateCommand(std::vector<std::string> commandLine);
    std::string AutoCompleteCommand(std::string const & line, int state);
    std::string AutoCompleteSubCommand(std::vector<std::string> const & line,
				       std::string const & currentToken,int state);
    uint32_t GetVerbosity();
  private:    
    std::vector<std::ostream*> ownedOutputStreams;
    uint32_t verbosity;
    int activeDevice;
    //  std::vector<DeviceContainer *> device;
    std::vector<CommandListBase *> device;
    //====================================================
    //Add your commands here
    //====================================================
    
    //Here is where you update the map between string and function
    void LoadCommandList();

    void HelpPrinter(std::string const &commandName,
		     std::vector<std::string> const & commandAliases,
		     std::string const & commandHelp,
		     bool fullPrint = false);
  
  
  
    //Add new command functions here
    CommandReturn::status InvalidCommandInclude(std::vector<std::string>,std::vector<uint64_t>);	   
    CommandReturn::status Help(std::vector<std::string>,std::vector<uint64_t>);	   
    CommandReturn::status SetVerbosity(std::vector<std::string>,std::vector<uint64_t>);	   
    CommandReturn::status Quit(std::vector<std::string>,std::vector<uint64_t>);	   
    CommandReturn::status Echo(std::vector<std::string>,std::vector<uint64_t>);	   
    CommandReturn::status Sleep(std::vector<std::string>,std::vector<uint64_t>);	        

    CommandReturn::status AddDevice(std::vector<std::string>,std::vector<uint64_t>);	        
    CommandReturn::status AddLib(std::vector<std::string>,std::vector<uint64_t>);	        
    CommandReturn::status ListDevices(std::vector<std::string>,std::vector<uint64_t>);	        
    CommandReturn::status SelectDevice(std::vector<std::string>,std::vector<uint64_t>);	        
    CommandReturn::status AddDeviceOutputFile(std::vector<std::string>,std::vector<uint64_t>); 

    //Add new command (sub command) auto-complete files here
    std::string autoComplete_Help(std::vector<std::string> const &,std::string const &,int);
    std::string autoComplete_AddDevice(std::vector<std::string> const &,std::string const &,int);
  };
}
#endif  
