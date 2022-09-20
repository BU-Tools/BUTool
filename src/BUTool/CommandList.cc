#include <BUTool/CommandListBase.hh>
std::map<std::string,std::string> BUTool::CommandListBase::BUToolVariables;

//=============================================================================
// TextIO interface
//=============================================================================
void BUTool::CommandListBase::AddOutputStream(Level::level level, std::ostream *os){
  TextIO->AddOutputStream(level,os);
}
void BUTool::CommandListBase::RemoveOutputStream(Level::level level, std::ostream* os){
  TextIO->RemoveOutputStream(level,os);
}
void BUTool::CommandListBase::ResetStreams(Level::level level){
  TextIO->ResetStreams(level);
}
void BUTool::CommandListBase::Print(Level::level level, const char *fmt, ...){
  va_list argp;
  va_start(argp, fmt);
  TextIO->Print(level,fmt,argp);
  va_end(argp);
}



//===========================================================================
// BUTool variables.
//===========================================================================

void BUTool::CommandListBase::SetVariable(std::string name,std::string const & value){
  boost::to_upper(name);
  BUToolVariables[name] = value;
}
void BUTool::CommandListBase::UnsetVariable(std::string name){
  boost::to_upper(name);
  auto it = BUToolVariables.find(name); 
  if(it != BUToolVariables.end()){
    BUToolVariables.erase(it);
  }      
}
std::string BUTool::CommandListBase::GetVariable(std::string name){
  boost::to_upper(name);
  auto it = BUToolVariables.find(name); 
  if(it == BUToolVariables.end()){
    return std::string("");}
  return it->second;
}
bool BUTool::CommandListBase::ExistsVariable(std::string name){
  boost::to_upper(name);
  bool ret = true;
  if(BUToolVariables.find(name) == BUToolVariables.end()){
    ret=false;
  }
  return ret;
}
std::vector<std::string> BUTool::CommandListBase::GetVariableNames(){
  std::vector<std::string> ret;
  for(auto it = BUToolVariables.begin(); it!=BUToolVariables.end();it++){
    ret.push_back(it->first);
  }
  return ret;
}

