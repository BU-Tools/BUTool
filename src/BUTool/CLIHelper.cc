#include <sstream>

#include <BUTool/CLIHelper.hh>
#include <BUTool/Launcher.hh>

//gnu readline
#include <readline/readline.h>
#include <readline/history.h>

#include <BUTool/helpers/parseHelpers.hh>

namespace BUTool {

  //Auto-completing state variables
  static Launcher * launcher;
  static std::string command;

  static char ** helperFunction(char const * text, int start, int end);
  static char * helperFunctionCommand(char const * text,int state);
  static char * helperFunctionSubCommand(char const * text,int state);


  std::vector<std::string> SplitString(std::string command)
  {
    return splitString(command," ");
  }

  //Set launcher state
  char ** (*CLISetAutoComplete(Launcher * _launcher))(char const *, int, int)
  {
    //Load the local launcher pointer
    launcher = _launcher;
    return helperFunction;
  }


  //===============================================================================
  // Readline helper functions
  //===============================================================================

  //Helper function for calling the AutoComplete function from Launcher
  static char * helperFunctionCommand(char const * text,int state)
  {
    if(launcher != NULL){
      //Auto complete from launcher
      std::string complete = std::string(launcher->AutoCompleteCommand(text,state));
      if(complete.empty()){
	return NULL;
      }
    
      //Allocate the return string (with NULL)
      char * ret = (char *) malloc(complete.size()+1);
      if(ret == NULL){
	std::bad_alloc e;
	throw e;
      }
      //copy string
      memcpy(ret,complete.c_str(),complete.size());
      ret[complete.size()] = '\0'; //Null terminate
      return ret;
    }
    return NULL;
  }

  //Helper function for calling the AutoComplete function from Launcher
  static char * helperFunctionSubCommand(char const * text,int state)
  {
    if(launcher != NULL){
      //Auto complete from launcher
      std::string complete = launcher->AutoCompleteSubCommand(SplitString(rl_line_buffer),text,state);
      if(complete.empty()){
	return NULL;
      }

      //Allocate the return string (with NULL)
      char * ret = (char *) malloc(complete.size()+1);
      if(ret == NULL){
	std::bad_alloc e;
	throw e;
      }
      //copy string
      memcpy(ret,complete.c_str(),complete.size());
      ret[complete.size()] = '\0'; //Null terminate
      return ret;
    }
    return NULL;
  }


  //helper function for calling... our helper functions
  //It determines if we are auto-completing a command or a sub command 
  //and then calls the appropriate matching functions
  static char ** helperFunction(char const * text, int start, int end)
  {
    (void) end; // makes compiler not complain about unused arg

    //Check if we are at the beginning of the line
    if(start == 0){

      //Reset state
      command.clear();

      //Find matches
      return  rl_completion_matches(text,helperFunctionCommand);
    } 

    //Determine if the command has been completed
    command.assign(rl_line_buffer);
    size_t commandEndPosition = command.find(' ');
    if(commandEndPosition == std::string::npos){
      command.clear();
    } else {
      command=command.substr(0,commandEndPosition);
    }

    //Match subcommands
    if (!command.empty()){
      return rl_completion_matches(text,helperFunctionSubCommand);
    }
    //auto complete filenames
    return NULL;
  }


  std::string LimitStringLines(std::string source, size_t beginLineCount, size_t endLineCount) {
  //Load the first beginLineCount lines.
  if( (!source.empty()) && (source.find('\n') == std::string::npos)){
    source=source+'\n';
  }
  std::string beginString;
  while( beginLineCount && !source.empty()) {
    //Find the next new line
    size_t pos = source.find('\n');
    if(pos == std::string::npos) {
      source.clear();
      break;
    }
    
    //append the line associated with it to our begin string with a tab at the beginning
    beginString += std::string("\t") + source.substr(0,pos) + std::string("\n");
    //Move past the newline
    pos++;
    //trim string
    source = source.substr(pos,source.size()-pos);
    
    beginLineCount--;
  }

  std::string endString;
  while(endLineCount && !source.empty()) {
    //Find the next new line
    size_t pos = source.rfind('\n');
    
    if(pos == std::string::npos) {
      //We didn't find a newline, so this is the last line
      pos = 0;
    } else if(++pos == source.size()) { //Move past the null line, but catch if it was the last char.
      source.resize(source.size()-1);
      continue;
    }
    
    //reverse append the line associated with it to our begin string with a tab at the beginning
    endString = std::string("\t") + source.substr(pos,source.size()-pos) + 
      std::string("\n") + endString;
    
    //trim source string
    if(pos >0) {
      pos--; //Move back to the newline
      source = source.substr(0,pos); //trim up to the newline
    } else { // nothing left, so clear
      source.clear();
    }
    
    endLineCount--;
  }
  
  //Build final string
  if(!source.empty()) {
    //Count the number of skipped lines if non-zero
    size_t skippedLineCount = 1;
    for(size_t iStr = 0; iStr < source.size();iStr++) {
      if(source[iStr] == '\n'){
	skippedLineCount++;
      }
    }
    std::ostringstream s;
    s << "*** Skipping " << skippedLineCount << " lines! ***\n";
    beginString += s.str();
  }
  beginString += endString;
  return beginString;
}


}
