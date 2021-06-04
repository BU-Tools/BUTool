#ifndef __COMMANDLIST_HPP__
#define __COMMANDLIST_HPP__
#include <string>
#include <vector>
#include <map>
#include "CommandReturn.hh"
#include "CommandDataStructure.hh"
#include <boost/tokenizer.hpp>
#include "ToolException.hh"

namespace BUTool{

  class CommandListBase{
  public:
    CommandListBase(std::string const & typeName):type(typeName){};
    virtual ~CommandListBase(){};
    virtual CommandReturn::status EvaluateCommand(std::vector<std::string> command)=0;
    virtual std::string AutoCompleteCommand(std::string const & line,int state)=0;
    virtual std::string AutoCompleteSubCommand(std::vector<std::string> const & line,
					       std::string const & currentToken,int state)=0;
    virtual std::string GetHelp(std::string const & command)=0;
    virtual std::map<std::string,std::vector<std::string> > const & GetCommandList()=0;
    virtual std::string GetType(){return type;};
    virtual std::string GetInfo(){return info;};
  protected:
    void SetInfo(std::string _info){info=_info;};
  private:
    std::string info;
    std::string type;
    CommandListBase();
  };

  template<class T>
  class CommandList: public CommandListBase {
  public:
    CommandList(std::string const & deviceType):CommandListBase(deviceType){
      itCommand = command.begin();
      itAlias = command_alias.begin();
    };
    virtual ~CommandList(){};
    /*=============================================================================
      Description: 
      Return a text string with the device type
      =============================================================================*/
    //    std::string GetType();  

    /*=============================================================================
      Description: 
      Return a text string with the device information
      =============================================================================*/
    //    std::string GetInfo();  


    /*=============================================================================
      Description: 
      This function takes a parsed line and checks to see if the first argument 
      (the command) is in it's map and executes the associated function if it is. 
      Return:
      This returns a CommandReturn::status enum type. 
      If the command requested does not exist, then it returns NOT_FOUND
      If the command is found, it will return that command's return (normally OK)
      =============================================================================*/
    CommandReturn::status EvaluateCommand(std::vector<std::string> command);

    /*=============================================================================
      Description: 
      Hook for readline to auto complete the commands in this function.
      Return:
      Next possible command for auto-complete, empty if none
      =============================================================================*/
    std::string AutoCompleteCommand(std::string const & line,int state);


    /*=============================================================================
      Description: 
      Hook for readline to auto complete arguments to the current command
      Return:
      Next possible auto-complete for the current command, empty if none
      =============================================================================*/
    std::string AutoCompleteSubCommand(std::vector<std::string> const & line,
				       std::string const & currentToken,int state);

    /*=============================================================================
      Description: 
      Return: help string for this command
      =============================================================================*/
    std::string GetHelp(std::string const & command);

    /*=============================================================================
      Description: 
      Return: help string for this command
      =============================================================================*/
    std::map<std::string,std::vector<std::string> > const & GetCommandList();

  protected:
    /*=============================================================================
      Description: 
      Adds a new command to the map of commands.
      Requires a name string, function pointer, and help string.
      Optionally takes an auto-complete function pointer
      =============================================================================*/
    void AddCommand(std::string name, 
		    CommandReturn::status (T::* fPtr)(std::vector<std::string>,std::vector<uint64_t>),
		    std::string help, 
		    std::string (T::* acPtr)(std::vector<std::string> const &,std::string const &,int)=NULL){
      if(name.size() == 0){
	BUException::COMMAND_LIST_ERROR e;
	e.Append("Trying to set command with empty name\n");
	throw e;
      }
      //Add teh command
      command[name].set(fPtr,help,acPtr);

      //Clear the current mapping
      commandAndAliasList.clear();
      //Update the commandAndAliasList;
      GetCommandList();
    };
    /*=============================================================================
      Description: 
      Makes a duplicate entry for an existing command with a new name. 
      =============================================================================*/
    void AddCommandAlias( std::string alias, std::string existingCommand ){
      command_map_iterator it = command.find(existingCommand);    
      //Only add an alias if the command exists and the alias has a non-zero size
      if(it != command.end() && (alias.size() > 0)){
	command_alias[alias] = it->first;
      
	//Clear the current mapping
	commandAndAliasList.clear();
	//Update the commandAndAliasList;
	GetCommandList();

      }      
    };
  private:    

    typedef typename std::map<std::string,CMD_DS<T> >::iterator command_map_iterator;
    typedef typename std::map<std::string,std::string>::iterator alias_map_iterator;
    typedef typename std::map<std::string,std::vector<std::string> >::iterator commandAndAliasList_iterator;
    //Store of command stuctures
    std::map<std::string,CMD_DS<T> > command; 
    std::map<std::string,std::string> command_alias;
    std::map<std::string, std::vector<std::string> > commandAndAliasList;
    command_map_iterator itCommand;
    alias_map_iterator itAlias;

  };

//  template<class T>
//  std::string CommandList<T>::GetType(){
////    std::string info("Type: ");
////    size_t typeColumnSize = 8;
////    if(type.size() > typeColumnSize){
////      //punt on padding and just print the type
////      info+=type;
////    }else{
////      //right justify the type
////      for(size_t paddingCharacter = 0;
////	  paddingCharacter <  (typeColumnSize - type.size());
////	  paddingCharacter++){
////	info+=" ";
////      }
////      info+=type;
////    }
////    return info;
//    return type;
//  }


  template<class T>
  CommandReturn::status CommandList<T>::EvaluateCommand(std::vector<std::string> command_line){
    //Handle no command
    if(command_line.size() == 0) {
      return CommandReturn::NOT_FOUND;
    }    
    
    //Search for command
    command_map_iterator itCommand = command.find(command_line[0]);
    if(itCommand != command.end()){
      //Command found!
      //Build the string & uint64_t argument vector
      std::vector<std::string> argListString;
      std::vector<uint64_t> argListUint64_t;
      for(size_t iArg = 1; iArg < command_line.size();iArg++) {
	argListString.push_back(command_line[iArg]);
	argListUint64_t.push_back(strtoul(command_line[iArg].c_str(),NULL,0));
      }
      //Call the function associated with this command
      return itCommand->second((T*) this, argListString,argListUint64_t);
    }else{
      //Handle aliases
      alias_map_iterator itAlias = command_alias.find(command_line[0]);
      if(itAlias != command_alias.end()){
	//If we found an alias, call EvaluateCommand again with the real command name
	command_line[0] = itAlias->second;
	return EvaluateCommand(command_line);
      }
    }
    return CommandReturn::NOT_FOUND;    
  }

  template<class T>  
  std::string CommandList<T>::AutoCompleteCommand(std::string const & line,int state){
    // we want a static iterator that holds the current command we've considered
    if(!state) {
      //Check if we are just starting out (state = 0), then we reset it to the first commmand
      itCommand = command.begin();
      itAlias   = command_alias.begin();
    } else {
      //move forward in itCommand until we are at the end, and then move to itAlias   
      if(itCommand != command.end()){
	itCommand++;
      }else if(itAlias != command_alias.end()){
	itAlias++;
      }
    }
    //Find the next auto-complete option in command
    for(;itCommand != command.end();itCommand++){
      //Check if the beginning of the current command matches up to line
      if(itCommand->first.find(line) == 0){
	return itCommand->first;
      }
    }
    //Find the next auto-complete option in alias
    for(;itAlias != command_alias.end();itAlias++){
      //Check if the beginning of the current command alias matches up to line
      if(itAlias->first.find(line) == 0){
	return itAlias->first;
      }
    }
    //not found
    return std::string("");      
  }
  
  template<class T>
  std::string CommandList<T>::AutoCompleteSubCommand(std::vector<std::string> const & line,
						     std::string const & currentToken,int state){
    //find command
    if(line.size() >0){
      command_map_iterator itCommand = command.find(line[0]);	
      if(itCommand != command.end()){
	//Call auto-complete calling function
	return itCommand->second.autoComplete((T*)this,line,currentToken,state);
      }else{
	//If not found, check the alias list
	alias_map_iterator itAlias = command_alias.find(line[0]);
	if(itAlias != command_alias.end()){
	  //if we found an alias entry, find the real entry and call it. 
	  itCommand = command.find(itAlias->second);
	  if(itCommand != command.end()){
	    //Call auto-complete calling function
	    return itCommand->second.autoComplete((T*) this,line,currentToken,state);
	  }	  
	}	
      }
    }
    //no commmand or command not found
    return std::string("");
  }

  template<class T>
  std::string CommandList<T>::GetHelp(std::string const & commandName){        
    command_map_iterator itCommand = command.find(commandName);	
    if(itCommand != command.end()){
      //Call auto-complete calling function
      return itCommand->second.help();
    }
    return std::string("");  
  }

  template<class T>
  std::map<std::string,std::vector<std::string> > const & CommandList<T>::GetCommandList(){
    if(commandAndAliasList.empty()){
      //The list is empty, so re-create it. 
      for(command_map_iterator itCommand = command.begin(); 
	  itCommand != command.end();
	  itCommand++){
	//Create an entry for this command
	commandAndAliasList[itCommand->first];
	commandAndAliasList_iterator itCommandList = commandAndAliasList.find(itCommand->first);
	//Search through the command list and the aliases that map to this command
	for(alias_map_iterator itAlias = command_alias.begin();
	    itAlias != command_alias.end();
	    itAlias++){
	  if(itCommand->first.compare(itAlias->second) == 0){
	    //itAlias is an alias for itCommand
	    //Add it to the list
	    itCommandList->second.push_back(itAlias->first);
	  }
	}
      }
    }
    //REturn the list
    return commandAndAliasList;
  }
}
#endif
  
