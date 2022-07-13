#ifndef __COMMANDLISTBASE_HPP__
#define __COMMANDLISTBASE_HPP__
#include <string>
#include <vector>
#include <map>
#include <BUTool/CommandReturn.hh>
//#include <BUTool/CommandDataStructure.hh>
//#include <boost/tokenizer.hpp>
//#include <BUTool/ToolException.hh>

#include <BUTextIO/BUTextIO.hh>

#include <boost/algorithm/string.hpp> //for to_upper

namespace BUTool{

  class CommandListBase {
  public:
    CommandListBase(std::string const & typeName):type(typeName){
      TextIO=std::make_shared<BUTextIO>();
    };
    virtual ~CommandListBase(){};
    virtual CommandReturn::status EvaluateCommand(std::vector<std::string> command)=0;
    virtual std::string AutoCompleteCommand(std::string const & line,int state)=0;
    virtual std::string AutoCompleteSubCommand(std::vector<std::string> const & line,
					       std::string const & currentToken,int state)=0;
    virtual std::string GetHelp(std::string const & command)=0;
    virtual std::map<std::string,std::vector<std::string> > const & GetCommandList()=0;
    virtual std::string GetType(){return type;};
    virtual std::string GetInfo(){return info;};

    //===========================================================================
    // TextIO interface
    //===========================================================================
    void AddOutputStream(Level::level level, std::ostream *os);
    void RemoveOutputStream(Level::level level, std::ostream* os);
    void ResetStreams(Level::level level);
    void Print(Level::level level, const char *fmt, ...);

    //===========================================================================
    // BUTool variables.
    //===========================================================================
    void                     SetVariable   (std::string name,std::string const & value);
    void                     UnsetVariable (std::string name);
    std::string              GetVariable   (std::string name);
    bool                     ExistsVariable(std::string name);
    std::vector<std::string> GetVariableNames();
    //===========================================================================
  protected:
    std::shared_ptr<BUTextIO> TextIO;
    void SetInfo(std::string _info){info=_info;};
  private:
    std::string info;
    std::string type;
    CommandListBase();
    static std::map<std::string,std::string> BUToolVariables;
  };


}
#endif
  
