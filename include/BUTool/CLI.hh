#ifndef __CLI_HPP__
#define __CLI_HPP__

#include <string>
#include <deque>
#include <vector>

#include <fstream>
#include <iostream>
#include <cstdlib>
 
//gnu readline
#include <readline/readline.h>
#include <readline/history.h>

//helper for auto-complete
#include <BUTool/CLIHelper.hh>

//std::string LimitStringLines(std::string, size_t, size_t);

namespace BUTool {
  
  class CLI
  {
  public:
    CLI();
    ~CLI();
    
    //Load a script file
    int ProcessFile(std::string filename);
    
    //Inject a command string
    int ProcessString(std::string command);
    
    //Get the next command (std::cin or from file)
    //From file first, then parse cin
    std::vector<std::string> GetInput(Launcher * launcher = NULL);

    //Clear input (if there is a queue of input)
    void ClearInput(){Commands.clear();}

    //Check if we are running commands from a script
    bool InScript(){return commandsFromScript;}
  private:
    //Get a command string
    int ProcessLine(std::string line);
    
    //Interally stored commands (bool is for if this command should be in the history)
    std::deque<std::string> Commands;

    //include file recusion overflow stopper
    int fileLevel;

    //Boolean for script
    bool commandsFromScript;
    
    //include command
    std::string prompt;
    std::string includeString;
    std::string loadString;
  };
}
#endif
