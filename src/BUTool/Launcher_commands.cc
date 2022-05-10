#include <BUTool/Launcher.hh>
#include <boost/tokenizer.hpp>
#include <stdio.h>  
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <fstream> // For BUTextIO file redirection of ofstreams
using namespace BUTool;

void Launcher::LoadCommandList()
{
  //Very special commands that are really in the command line processor. 
  AddCommand("include",&Launcher::InvalidCommandInclude,"Read a script file"); //This command exists inside of the CLI class and should never be called.  
  //InvalidCommandInclude is the least wrong thing to put here, it will never be called.
  AddCommandAlias("load","include");               //This is the only allowed alias right now and is only allowed because it is also part of the CLI class
  //DO NOT ADD ANY MORE ALIASES TO include/load UNLESS THEY ARE IN THE CLI CLASS! 
  //LISTEN TO THE LINE ABOVE! YES YOU! -Dan dgastler@bu.edu
  // general commands (Launcher_commands)
  AddCommand("help",&Launcher::Help,"List commands. 'help <command>' for additional info and usage",
	     &Launcher::autoComplete_Help);
  AddCommandAlias( "h", "help");
  AddCommand("quit",&Launcher::Quit,"Close program");
  AddCommandAlias("q", "quit");
  AddCommand("exit",&Launcher::Quit,"Close program");
  AddCommand("echo",&Launcher::Echo,
	     "Echo to screen\n" \
	     "  Usage:\n" \
	     "  echo <text>");
  AddCommand("sleep",&Launcher::Sleep,
	     "Delay in seconds \n" \
	     "  Usage:\n" \
	     "  sleep <integer number of seconds to be delayed>");
  AddCommand("add_device",&Launcher::AddDevice,
	     "Add a new device and make it active.\n"\
	     "  Usage:\n"\
	     "  add_device <type> <args>\n",
	     &Launcher::autoComplete_AddDevice);  
  AddCommand("add_lib",&Launcher::AddLib,
	     "Load a new class of device.\n"\
	     "  Usage:\n"\
	     "  add_lib <path>\n");  
  AddCommand("list",&Launcher::ListDevices,
	     "List devices currently connected\n"\
	     "  * indicated the active device\n"
	     );  
  AddCommand("select",&Launcher::SelectDevice,
	     "Switch the active device.\n"\
	     "  Usage:\n"\
	     "  select <device number>\n");  
  AddCommand("verbose",&Launcher::SetVerbosity,
	     "Change the print level for exceptions (9 == all).\n"\
	     "  Usage:\n"\
	     "  verbose <level>\n");  
  AddCommand("add_dev_ofile",&Launcher::AddDeviceOutputFile,
	     "Place device print calls in filename.\n"\
	     "  Usage:\n"\
	     "  add_dev_ofile filename <device#>\n"\
             "  If no device# listed, it will be added to all devices\n");  
  AddCommand("set_var",&Launcher::SetVariable,
	     "Set an internal variable.\n"\
	     "  Usage:\n"\
	     "  set_var name value\n"\
	     "  If no value is set, the variable is unset\n");
  AddCommand("get_var",&Launcher::GetVariable,
	     "Get an internal variable.\n"\
	     "  Usage:\n"\
	     "  get_var name\n");
  AddCommand("list_vars",&Launcher::ListVariables,
	     "Get a list of internal variable.\n"\
	     "  Usage:\n"\
	     "  list_vars\n");

}

#define MAX_VERBOSITY 9
CommandReturn::status Launcher::SetVerbosity(std::vector<std::string> /*strArg*/,std::vector<uint64_t> intArg){
  if(!intArg.empty()){
    if(intArg[0] > MAX_VERBOSITY){
      verbosity = MAX_VERBOSITY;
    }else{
      verbosity = intArg[0];
    }
  }else{
    verbosity = 0;
  }
  return CommandReturn::OK;
}

CommandReturn::status Launcher::InvalidCommandInclude(std::vector<std::string> /*strArg*/ ,std::vector<uint64_t> /*intArg*/)
{
  return CommandReturn::OK;
}
  
CommandReturn::status Launcher::Quit(std::vector<std::string> /*strArg*/,
				     std::vector<uint64_t> /*intArg*/)
{
  //Quit CLI so return -1
  return CommandReturn::EXIT;
}
  
CommandReturn::status Launcher::Echo(std::vector<std::string> strArg,
				     std::vector<uint64_t> /*intArg*/) {
  for(size_t iArg = 0; iArg < strArg.size();iArg++)
    {
      Print(Level::INFO,"%s ",strArg[iArg].c_str());
    }
  Print(Level::INFO,"\n");
  return  CommandReturn::OK;
}

void Launcher::HelpPrinter(std::string const &commandName,
			   std::vector<std::string> const & commandAliases,
			   std::string const & commandHelp,
			   bool fullPrint){
  //Build a string with the command name and aliases
  std::string nameString(commandName);

  //Append the alias names if they exist
  if(!commandAliases.empty()){
    //We have sub commands
    nameString+='(';
    for(std::vector<std::string>::const_iterator it = commandAliases.begin();
	it != commandAliases.end();
	it++){
      nameString.append(*it);
      nameString+=',';
    }
    nameString[nameString.size()-1] = ')';
  }

  //print the help for this command
  if(!fullPrint && commandHelp.find('\n')){ 
    Print(Level::INFO," %-20s:   %s\n",nameString.c_str(),
	   commandHelp.substr(0,commandHelp.find('\n')).c_str());
  } else { // Print full help
    Print(Level::INFO," %-20s:   ",nameString.c_str());
    boost::char_separator<char> sep("\n");
    boost::tokenizer<boost::char_separator<char> > tokens(commandHelp, sep);
    boost::tokenizer<boost::char_separator<char> >::iterator it = tokens.begin();
    if(it != tokens.end()){
      Print(Level::INFO,"%s\n",(*it).c_str());
      it++;
    }
    for ( ;it != tokens.end();++it){
      Print(Level::INFO,"                         %s\n",(*it).c_str());
    }
  }  
}
CommandReturn::status Launcher::Help(std::vector<std::string> strArg,std::vector<uint64_t> /*intArg*/) 
{
  //Check the arguments to see if we are looking for
  //  simple help:    all commands listed with 1-line help
  //  specific help:  full help for one command
  //  full help:      full help for all commands
  
  //Get a list of commands from Launcher
  std::map<std::string,std::vector<std::string> > commandList = GetCommandList();
  //Get a list of commands from the active device
  std::map<std::string,std::vector<std::string> > deviceCommandList;

  std::vector<CommandListBase *>::iterator itActiveDevice = device.end();
  if(activeDevice >= 0){
    itActiveDevice = device.begin()+activeDevice;
    if(itActiveDevice != device.end()){  
      deviceCommandList = (*itActiveDevice)->GetCommandList();
    }
  }

  bool fullPrint = false;   
  if(!strArg.empty()){    
    if(!strArg[0].empty()){
      if(strArg[0][0] == '*'){
	//Check if we want a full print and skip to the end if we do
	fullPrint = true;
      } else {
	//We want a per command full help
	
	// Search Launcher commands and aliases
	// Iterate over all Launcher commands
	for(std::map<std::string,std::vector<std::string> >::iterator it = commandList.begin();
	    it != commandList.end();
	    it++){
	  //check if this is the command
	  if(strArg[0] == it->first) {
	    // We found a matching Launcher command alias for the command: it->first
	    std::string foundCommand = it->first;
	    HelpPrinter(foundCommand,
			commandList[foundCommand],
			GetHelp(foundCommand),
			true);
	    return CommandReturn::OK;
	  }else{
	    // Iterate over all aliases of each Launcher command and the command itself
	    for(std::vector<std::string>::iterator itAlias = it->second.begin();
		itAlias != it->second.end();
		itAlias++){
	      if(strArg[0] == (*itAlias)) {
		// We found a matching Launcher command alias for the command: it->first
		std::string foundCommand = it->first;
		HelpPrinter(foundCommand,
			    commandList[foundCommand],
			    GetHelp(foundCommand),
			    true);
		return CommandReturn::OK;
	      }
	    }
	  }
	}
	
	// Search device commands and aliases
	// Iterate over all device commands
	for(std::map<std::string,std::vector<std::string> >::iterator it = deviceCommandList.begin();
	    it != deviceCommandList.end();
	    it++){

	  //check if this is the command
	  if(strArg[0] == (it->first)) {
	    // We found a matching Launcher command alias for the command: it->first
	    std::string foundCommand = it->first;
	    HelpPrinter(foundCommand,
			deviceCommandList[foundCommand],
			(*itActiveDevice)->GetHelp(foundCommand),
			true);
	    return CommandReturn::OK;
	  }else{
	    // Iterate over all aliases of each device command and the command itself
	    for(std::vector<std::string>::iterator itAlias = it->second.begin();
		itAlias != it->second.end();
		itAlias++){
	      if(strArg[0] == (*itAlias)) {
		// We found a matching device command alias for the command: it->first
		std::string foundCommand = it->first;
		HelpPrinter(foundCommand,
			    deviceCommandList[foundCommand],
			    (*itActiveDevice)->GetHelp(foundCommand),
			    true);
		return CommandReturn::OK;
	      }
	    }
	  }
	}	
      }
    }	
  }
  
  //We are printing out all the helps

  //Print the Launcher helps
  for(std::map<std::string,std::vector<std::string> >::iterator it = commandList.begin();
      it != commandList.end();
      it++){
    HelpPrinter(it->first,
		it->second,
		GetHelp(it->first),
		fullPrint);
  }

  //Print the Device helps
  for(std::map<std::string,std::vector<std::string> >::iterator it = deviceCommandList.begin();
      it != deviceCommandList.end();
      it++){
    bool printDeviceCommand = true;
    //Skip help print if the device command is superseded by a Launcher command
    for(std::map<std::string,std::vector<std::string> >::iterator itPrimary = commandList.begin();
	itPrimary != commandList.end();
	itPrimary++){
      //check if the device command matches a Launcher command
      if(it->first == itPrimary->first){
	printDeviceCommand= false;
	break;
      }else{
	//check if the device command matches a launcher command alias
	for(std::vector<std::string>::iterator itAlias = itPrimary->second.begin();
	    itAlias != itPrimary->second.end();
	    itAlias++){
	  if(it->first == (*itAlias)){
	    printDeviceCommand= false;
	    itPrimary = commandList.end();
	    break;	    
	  }
	}
      }
    }
    //Print the device command help if needed
    if(printDeviceCommand){
      HelpPrinter(it->first,
		  it->second,
		  (*itActiveDevice)->GetHelp(it->first),
		  fullPrint);
    }
  }
  return CommandReturn::OK;
}


std::string Launcher::autoComplete_Help(std::vector<std::string> const & line,std::string const & currentToken ,int state)
{  
  if(!line.empty()){
    if((line.size() > 1) && (currentToken.empty())){
      return std::string("");
    }
   
    static std::vector<std::string> commandName;
    static size_t iCommand;

    //Reload lists if we are just starting out at state == 0
    if(state == 0){
      //Get a list of commands from Launcher
      std::map<std::string,std::vector<std::string> > commandList = GetCommandList();
      for(std::map<std::string,std::vector<std::string> >::iterator it = commandList.begin();
	  it != commandList.end();
	  it++)
      {
	//append the current command
	commandName.push_back(it->first);
	//append aliases to that command
	for(std::vector<std::string>::iterator itAlias = it->second.begin();
	    itAlias != it->second.end();
	    itAlias++){
	  commandName.push_back(*itAlias);
	}
      }

      if((activeDevice >= 0) && (size_t(activeDevice) < device.size())){
	//Get a list of commands from the active device
	std::map<std::string,std::vector<std::string> > deviceCommandList = device[activeDevice]->GetCommandList();
	for(std::map<std::string,std::vector<std::string> >::iterator it = deviceCommandList.begin();
	    it != deviceCommandList.end();
	    it++)
	{
	  //append the current command
	  commandName.push_back(it->first);
	  //append aliases to that command
	  for(std::vector<std::string>::iterator itAlias = it->second.begin();
	      itAlias != it->second.end();
	      itAlias++){
	    commandName.push_back(*itAlias);
	  }
	}	
      }
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

#define DELAY_TIME_SWITCH_VALUE 5.0
#define USEC_IN_SEC 1e6
CommandReturn::status Launcher::Sleep(std::vector<std::string> strArg,
				      std::vector<uint64_t> intArg) {
  if( strArg.empty()) {
    Print(Level::INFO,"Need a delay time in seconds\n");
  } else {
    double s_time = strtod( strArg[0].c_str(), NULL);
    if( s_time > DELAY_TIME_SWITCH_VALUE) {
      sleep( intArg[0]);
    } else {
      usleep( (useconds_t)(s_time * USEC_IN_SEC) );
    }
  }
  return CommandReturn::OK;
}

#define TYPE_PADDING 10
CommandReturn::status Launcher::ListDevices(std::vector<std::string> /*strArg*/,
					    std::vector<uint64_t> /*intArg*/){
  if(!device.empty()){
    Print(Level::INFO,"Connected devices\n");
    for(size_t iDevice = 0;
	iDevice < device.size();
	iDevice++){    
      if(iDevice==size_t(activeDevice)){
	//Add "*" to currently active device
	Print(Level::INFO,"*%-2zu: %-*s%s\n",iDevice,
	       TYPE_PADDING,device[iDevice]->GetType().c_str(),
	       device[iDevice]->GetInfo().c_str());
      }else{
	Print(Level::INFO,"%-3zu: %-*s%s\n",iDevice,
	       TYPE_PADDING,device[iDevice]->GetType().c_str(),
	       device[iDevice]->GetInfo().c_str());
      }
    }
  }
  return CommandReturn::OK;
}

CommandReturn::status Launcher::SelectDevice(std::vector<std::string> /*strArg*/,
					     std::vector<uint64_t> iArg){
  if(!iArg.empty()){
    if(iArg[0] < device.size()){
      activeDevice = iArg[0];
    }else{
      Print(Level::INFO,"Error: %" PRIu64 " out of range (%zu)",iArg[0],device.size());
    }
    return CommandReturn::OK;
  }
  return CommandReturn::BAD_ARGS;
}


CommandReturn::status Launcher::AddDeviceOutputFile(std::vector<std::string> strArg,
						    std::vector<uint64_t> iArg){
  if(!iArg.empty()){
    //create the file
    std::ofstream * newStream = new std::ofstream(strArg[0].c_str(),std::ofstream::trunc);
    //Check that the file is ok
    if(NULL == newStream || newStream->fail()){
      Print(Level::INFO,"Error: %s could not be opened\n",strArg[0].c_str());
      return CommandReturn::BAD_ARGS;      
    }
    //add the newStream to the lis of ostreams that Launcher needs to clean up at the end. 
    ownedOutputStreams.push_back(newStream);

    //By default we add this stream to all devices
    size_t iStartDev = 0;
    size_t iEndDev = device.size();
    if(iArg.size() > 1){
      //A specific device has been added.
      iStartDev = iArg[1];
      iEndDev = iArg[1]+1; //Cause the following loop to end after iArg[i]
    }

    for(size_t iDev = iStartDev; 
	iDev < device.size() && iDev < iEndDev;
	iDev++){
      if(iDev < device.size()){
	BUTextIO * text_ptr = NULL; 
	if(
	   (text_ptr = dynamic_cast<BUTextIO*>(device[iArg[0]])) != NULL
	   ){
	  text_ptr->AddOutputStream(Level::INFO,newStream);
	}	
      }else{
	Print(Level::INFO,"Error: %" PRIu64 " out of range (%zu)",iArg[0],device.size());
      }   
    } 
    return CommandReturn::OK;
  }
  return CommandReturn::BAD_ARGS;
}



CommandReturn::status Launcher::SetVariable(std::vector<std::string> args,
					    std::vector<uint64_t> /*arg*/){
  CommandReturn::status ret = CommandReturn::OK;
  switch (args.size()){
  case 2:
    ((CommandListBase *)this)->SetVariable(args[0],args[1]);
    break;
  case 1:
    ((CommandListBase *)this)->UnsetVariable(args[0]);
    break;
  default:
    ret = CommandReturn::BAD_ARGS;
  }
  return ret;
}

CommandReturn::status Launcher::GetVariable(std::vector<std::string> args,
					    std::vector<uint64_t> /*arg*/){
  CommandReturn::status ret = CommandReturn::OK;
  std::string val;
  switch (args.size()){
  case 1:
    val = ((CommandListBase *)this)->GetVariable(args[0]);
    break;
  default:
    ret = CommandReturn::BAD_ARGS;
  }
  if(ret == CommandReturn::OK){
    Print(Level::INFO,"%s = \"%s\"\n",args[0].c_str(),val.c_str());
  }

  return ret;
}

CommandReturn::status Launcher::ListVariables(std::vector<std::string> /*args*/,
					      std::vector<uint64_t> /*asdf*/){
  std::vector<std::string> vars =  ((CommandListBase *)this)->GetVariableNames();
  Print(Level::INFO,"BUTool Variables:\n");
  for(auto it = vars.begin();it!=vars.end();it++){
    Print(Level::INFO,"  %s\n",it->c_str());
  }
  return CommandReturn::OK;
}
