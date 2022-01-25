#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <stdlib.h>

//BUTool libraries
#include <BUTool/Launcher.hh>
#include <BUTool/CLI.hh>
#include <BUTool/CommandReturn.hh>
#include <BUTool/DeviceFactory.hh>
#include <BUException/ExceptionBase.hh>
#include <BUTool/helpers/parseHelpers.hh>

#include <readline/readline.h> //for rl_delete_text
#include <signal.h> //signals

#define BUTOOL_AUTOLOAD_LIBRARY_LIST "BUTOOL_AUTOLOAD_LIBRARY_LIST" //Legacy method to add libraries
//========================================================================
//Boost program options
#include <boost/program_options.hpp> //for configfile parsing
#define DEFAULT_CONFIG_FILE "/etc/BUTool" //path to default config file
namespace po = boost::program_options; //making life easier for boost                                 

//==========================================================================

#define DevFac BUTool::DeviceFactory::Instance()

using namespace BUTool;

volatile bool running = true;

void signal_handler(int sig){
  if(sig == SIGINT){
    //remove SIGINT from signal_handler
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);    
       
    //Set an alarm for a second from now
    alarm(1);
    
    //cleanup line
#ifdef LINE_CTRL_C
    rl_delete_text(0,rl_end);
    printf("\n");
    rl_on_new_line();
    rl_done=1;
#endif
    rl_forced_update_display();
    
  }else if (sig == SIGALRM){    

    //re-enable SIGINT capture                                                                                                                                                                                    
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
  }
}

int main(int argc, char* argv[]) 
{
  //signal handling
  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGALRM,&sa, NULL);

  //Create CLI and Create Command launcher (early, so we can set things)
  CLI cli;
  Launcher launcher;

  //============================================================================
  // First add libraries from config file so device factory works
  //============================================================================
  //Big try catch here for all of program options
  try {
    //Single option for libraries
    po::options_description lib_options("BUTool Options");
    lib_options.add_options()
      ("lib",         po::value<std::vector<std::string> >(), "Device library plugin to load");
  
    //Create variable map for libraries and store arguments from config file
    std::ifstream configFile(DEFAULT_CONFIG_FILE);
    po::variables_map lib_map;
    if (configFile) {
      try {
	po::store(po::parse_config_file(configFile, lib_options, true), lib_map);
      } catch (std::exception &e) {
	//Probably superflous err message
	//fprintf(stderr, "ERROR in BOOST parse_config_file: %s\n", e.what());
      }
    }
    //iterate over library arguments
    if(lib_map.count("lib")){
      std::vector<std::string> libVec = lib_map["lib"].as<std::vector<std::string> >();
      for (auto iLib = libVec.begin(); iLib != libVec.end(); iLib++){
	std::string process = "add_lib " + *iLib;
	//Add a add_lib command for each library
	cli.ProcessString(process);
	//Ask the CLI to process this command.
	std::vector<std::string> command = cli.GetInput(&launcher);
	//If the command was well formed, tell the launcher to launch it. 
	if(command.size() > 0){
	  //Launch command function (for add lib)
	  launcher.EvaluateCommand(command);
	  //Ignore the return value.  It either works or not.
	}
      }
    
      /* Legacy method for adding libraries from system enviornmental variable,
	 will run if no libraries defined in config file */
    } else if (NULL != getenv(BUTOOL_AUTOLOAD_LIBRARY_LIST)){
      std::vector<std::string> libFiles = splitString(getenv(BUTOOL_AUTOLOAD_LIBRARY_LIST),":");
      for(size_t iLibFile = 0 ; iLibFile < libFiles.size() ; iLibFile++){
	//Add a add_lib command for each library
	cli.ProcessString("add_lib " + libFiles[iLibFile]);      
	//Ask the CLI to process this command.
	std::vector<std::string> command = cli.GetInput(&launcher);
	//If the command was well formed, tell the launcher to launch it. 
	if(command.size() > 0){
	  //Launch command function (for add lib)
	  launcher.EvaluateCommand(command);
	  //Ignore the return value.  It either works or not.
	}
      }
    }

    //============================================================================
    // Setup Boost programoptions
    //============================================================================
    po::options_description cli_options("BUTool Options");
    cli_options.add_options()
      ("help,h", "Help Screen")    
      ("lib,l",       po::value<std::string >()->multitoken(), "Device library plugin to load")
      ("script,X",    po::value<std::string >()->multitoken(), "Script files to load")    
      ("cmd",         po::value<std::string >()->multitoken(), "Command to run")
      ("var",         po::value<std::string >()->multitoken(), "Set local variable");
    
    std::vector<std::string> devices = DevFac->GetDeviceNames();
    std::vector<std::string> deviceNames;
    for(size_t iDevice = 0; iDevice < devices.size(); iDevice++){
      std::string  CLI_flag;      
      std::string  CLI_full_flag;
      std::string  CLI_description;  
      if(DevFac->CLIArgs(devices[iDevice],CLI_flag,CLI_full_flag,CLI_description)){
	std::string tmpName = CLI_full_flag + "," + CLI_flag;
	char *cName = new char[tmpName.size() + 1];
	char *cDesc = new char[CLI_description.size() + 1];
	strcpy(cName, tmpName.c_str());
	strcpy(cDesc, CLI_description.c_str());
	cli_options.add_options()
	  (cName, po::value<std::string>()->multitoken()->implicit_value(""), cDesc);
	delete[] cName;
	delete[] cDesc;
	deviceNames.push_back(CLI_full_flag);
      }
    }

    //============================================================================
    // Store arguments for program options
    //============================================================================
    //map of all parsed options
    std::map<std::string, std::vector<std::string> > allOptions;

    try {
      //Get parsed options from command line
      po::parsed_options cli_parsed = po::parse_command_line(argc, argv, cli_options);

      //Store cli parsed options into allOptions
      for(size_t iCli = 0; iCli < cli_parsed.options.size(); iCli++) { //iterate through all parsed cli options
	std::string name = cli_parsed.options[iCli].string_key; //get name "string_key" of option
	std::string value = ""; //Set value empty to start
	if(cli_parsed.options[iCli].value.size()) { //if value of option is not empty
	  for(size_t i = 0; i < cli_parsed.options[iCli].value.size(); i++) {//iterate through all values of this option, think vector
	    value += " " + cli_parsed.options[iCli].value[i];
	  } 
	}
	allOptions[name].push_back(value);
      }
    } catch (std::exception &e) {
      fprintf(stderr, "ERROR storing command line arguments: %s\n", e.what());
      std::cout << cli_options << std::endl;
      return 0;
    }
    
    po::options_description cfg_options("BUTool Options"); //This needs to exist but doesn't actually need any values
    try {
      //Get parsed options from config file
      std::ifstream configFile2(DEFAULT_CONFIG_FILE);
      po::parsed_options cfg_parsed = po::parse_config_file(configFile2, cfg_options, true);
      
      //Store cfg parsed options into allOptions
      for(size_t iCfg = 0; iCfg < cfg_parsed.options.size(); iCfg++) { //iterate through all parsed cfg options
	if(cfg_parsed.options[iCfg].string_key != "lib") { //Ignore libraries, we ran those before declaring cli_options
	  std::string name = cfg_parsed.options[iCfg].string_key; //get name "string_key" of option
	  std::string value = ""; //Set value empty to start
	  if(cfg_parsed.options[iCfg].value.size()) { //if value of option is not empty
	    for(size_t i = 0; i < cfg_parsed.options[iCfg].value.size(); i++) {//iterate through all values of this option, think vector
	      value +=  cfg_parsed.options[iCfg].value[i];
	    } 
	  }
	  allOptions[name].push_back(value);
	}
      }
    } catch (std::exception &e) {
      fprintf(stderr, "ERROR storing config file arguments: %s\n", e.what());
    }

    //Bail quickly if "help" was specified
    if(allOptions.find("help") != allOptions.end()){
      std::cout << cli_options << std::endl;
      return 0;
    }

    //Create a map of default arguments
    std::map<std::string, std::string> default_map;
    std::vector<std::string> defaults = allOptions["DEFAULT_ARGS"];
    for (auto iDefault = defaults.begin(); iDefault != defaults.end(); iDefault++) {
      std::string device = (*iDefault).substr(0, (*iDefault).find(" ")); //Device is first word before first " "
      std::string device_args = (*iDefault).substr((*iDefault).find(" "), (*iDefault).size() - 1); //Arguments is everything after first " "
      default_map.insert({device, device_args});
    }

    //============================================================================
    // Run Program Options
    //============================================================================
    //Add libraries
    std::vector<std::string> libraries = allOptions["lib"];
    for (auto iLib = libraries.begin(); iLib != libraries.end(); iLib++) {
      std::string command = "add_lib " + *iLib;
      cli.ProcessString(command);
    }

    //Add devices
    for (auto iDevice = deviceNames.begin(); iDevice != deviceNames.end(); iDevice++) {//iterate through devices
      std::vector<std::string> device = allOptions[*iDevice]; //get arguments for device
      for (auto iDeviceArgs = device.begin(); iDeviceArgs != device.end(); iDeviceArgs++) {//iterate device arguments
	std::string command = "add_device ";
	if (*iDeviceArgs == "") {//argument is empty so use value mapped in default_map
	  command += *iDevice + " " + default_map[*iDevice];
	} else {
	  command += *iDevice + " " + *iDeviceArgs;
	}
	cli.ProcessString(command);
      }
    }

    //set local variables
    std::vector<std::string> vars = allOptions["var"];
    for (auto iVar = vars.begin(); iVar != vars.end(); iVar++) {
      std::string command = "set_var " + *iVar;
      cli.ProcessString(command);
    }


    //Run scripts
    std::vector<std::string> scripts = allOptions["script"];
    for (auto iScript = scripts.begin(); iScript != scripts.end(); iScript++) {
      std::string command = "include " + *iScript;
      cli.ProcessString(command);
    }

    //Run commands from command line
    std::vector<std::string> cmds = allOptions["cmd"];
    for (auto iCmd = cmds.begin(); iCmd != cmds.end(); iCmd++) {
      std::string command = *iCmd;
      cli.ProcessString(command);
    }

  } catch (std::exception &e) {
    fprintf(stderr, "ERROR in program options: %s\n", e.what());
    running = false; //Dont run Main loop
  }
    
  //============================================================================
  // Main loop
  //============================================================================

  while (running) 
    {
      try {
	//Get parsed command
	std::vector<std::string> command = cli.GetInput(&launcher);

	//Check if this is just the user hitting return (do nothing if it is)
	if(command.size() > 0){
	  //Launch command function
	  CommandReturn::status ret = launcher.EvaluateCommand(command);

	  //check return value
	  if(ret == CommandReturn::EXIT){
	    //Shutdown tool
	    running = false;       
	  }else if ( ret == CommandReturn::NOT_FOUND ){
	    if(cli.InScript()){ //Fail to command line if in script
	      std::cout << "Bad command in script. Exit!\n";
	      running = false;	  
	    }else{
	      std::cout << "Bad command\n";
	    }
	  }
	}
      }catch (BUException::exBase & e){

	uint32_t verbose_level = launcher.GetVerbosity();

       	std::string errorstr(e.what());
       	std::cout << "\n\n\nCaught Tool exception: " << errorstr << std::endl;
	
	switch(verbose_level){
	case 1:
	  std::cout << LimitStringLines(e.Description(),5,5);
	  break;
	case 2:
	  std::cout << LimitStringLines(e.Description(),10,10);
	  break;
	case 3:
	  std::cout << LimitStringLines(e.Description(),20,20);
	  break;
	case 9:
	  std::cout << LimitStringLines(e.Description(),0,1000);
	  break;
	default:
	  std::cout << LimitStringLines(e.Description());
	  break;
	}
       	if(9 == verbose_level){
	  std::cout << e.StackTrace();
	}
	if(cli.InScript()){ //Fail to command line if in script
       	  running = false;
	}
       	cli.ClearInput(); //Clear any scripted commands
      }catch (std::exception  & e){
	std::string errorstr(e.what());
	errorstr.erase(std::remove(errorstr.begin(), 
				   errorstr.end(), 
				   '\n'), 
		       errorstr.end());
	std::cout << "\n\n\nCaught std::exception " << errorstr << ".\n";
	//std::cout << "\n\n\nCaught std::exception " << errorstr << ". Shutting down.\n";
	//running = false;
	cli.ClearInput(); //Clear any scripted commands
      }
    }
  return 0;
}
