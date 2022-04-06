#include <RegisterHelper/RegisterHelperIO.hh>
#include <boost/algorithm/string/case_conv.hpp>

std::string BUTool::RegisterHelperIO::ReadString(std::string const & /*reg*/){
  //=============================================================================
  //placeholder for string read
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  return std::string();
}


std::vector<uint32_t> BUTool::RegisterHelperIO::ReadAddressFIFO(uint32_t addr,size_t count){
  //=============================================================================
  //placeholder for fifo read
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  std::vector<uint32_t> ret;
  for(size_t iRead = 0; iRead < count; iRead++){
    ret.push_back(ReadAddress(addr)); 
  }
  return ret;
}
std::vector<uint32_t> BUTool::RegisterHelperIO::ReadRegisterFIFO(std::string const & reg,size_t count){
  //=============================================================================
  //placeholder for fifo read
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  uint32_t address = GetRegAddress(reg);
  return ReadAddressFIFO(address,count);
}

std::vector<uint32_t> BUTool::RegisterHelperIO::BlockReadAddress(uint32_t addr,size_t count){
  //=============================================================================
  //placeholder for block read
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  std::vector<uint32_t> ret;
  uint32_t addrEnd = addr + uint32_t(count);
  for(;addr < addrEnd;addr++){
    ret.push_back(ReadAddress(addr)); 
  }
  return ret;
}
std::vector<uint32_t> BUTool::RegisterHelperIO::BlockReadRegister(std::string const & reg,size_t count){
  //=============================================================================
  //placeholder for block read
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  uint32_t address = GetRegAddress(reg);
  return BlockReadAddress(address,count);
}

void BUTool::RegisterHelperIO::WriteAddressFIFO(uint32_t addr,std::vector<uint32_t> const & data){
  //=============================================================================
  //placeholder for fifo write
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  for(size_t i = 0; i < data.size();i++){
    WriteAddress(addr,data[i]);
  }
}
void BUTool::RegisterHelperIO::WriteRegisterFIFO(std::string const & reg, std::vector<uint32_t> const & data){
  //=============================================================================
  //placeholder for fifo write
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  for(size_t i = 0; i < data.size();i++){
    WriteRegister(reg,data[i]);
  }
}
void BUTool::RegisterHelperIO::BlockWriteAddress(uint32_t addr,std::vector<uint32_t> const & data){
  //=============================================================================
  //placeholder for block write
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  for(size_t i =0;i < data.size();i++){
    WriteAddress(addr,data[i]);
    addr++;
  }
}
void BUTool::RegisterHelperIO::BlockWriteRegister(std::string const & reg, std::vector<uint32_t> const & data){
  //=============================================================================
  //placeholder for block write
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  uint32_t addr = GetRegAddress(reg);
  BlockWriteAddress(addr,data);
}

std::string BUTool::RegisterHelperIO::GetRegDebug(std::string const & /*reg*/){
  //=============================================================================
  //placeholder for register debug info
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  return "";
}
std::string BUTool::RegisterHelperIO::GetRegHelp(std::string const & /*reg*/){
  //=============================================================================
  //placeholder for register help
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  return "";
}




void BUTool::RegisterHelperIO::ReCase(std::string & name){
  switch(regCase){
  case LOWER:
    boost::algorithm::to_lower(name);    
    break;
  case UPPER:
    boost::algorithm::to_upper(name);    
    break;
  case CASE_SENSITIVE:
    //Do nothing
  default:
    break;
  }
}



