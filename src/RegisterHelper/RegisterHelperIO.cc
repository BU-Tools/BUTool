#include <RegisterHelper/RegisterHelperIO.hh>
#include <boost/algorithm/string/case_conv.hpp>
#include <BUTool/ToolException.hh>

#include <boost/algorithm/string/predicate.hpp> //for iequals

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





void BUTool::RegisterHelperIO::ReadConvert(std::string const & reg, uint64_t & val){
  // Read the value from the named register, and update the value in place
  uint32_t rawVal = ReadRegister(reg);
  val = rawVal;
}


void BUTool::RegisterHelperIO::ReadConvert(std::string const & reg, int64_t & val){
  // Read the value from the named register, and update the value in place
  int rawVal = ReadRegister(reg); //convert bits to int from uint32_t
  int mask   = GetRegMask(reg);

  // Count number of bits in the mask
  uint64_t b;
  // Shift all the bits to right until we count all the bits in the mask
  for (b=0; mask; mask >>= 1)
  {
    b += mask & 1;
  }

  // Sign extend b-bit number to r
  int x = rawVal;
  int const m = 1U << (b - 1); 

  x = x & ((1U << b) - 1);
  val = (x ^ m) - m;
}

void BUTool::RegisterHelperIO::ReadConvert(std::string const & reg, double & val){
  // Read the value from the named register, and update the value in place
  // Check the conversion type we want:
  // Is it a "fp16", or will we do some transformations? (i.e."m_...")
  
  std::string format = GetConvertFormat(reg);
  // 16-bit floating point to double transformation
  if (boost::algorithm::iequals(format, "fp16")) {
    val = ConvertFloatingPoint16ToDouble(reg);
  }
  
  // Need to do some arithmetic to transform
  else if ((format[0] == 'M') | (format[0] == 'm')) {
    val = ConvertIntegerToDouble(reg, format);
  }

  else if (boost::algorithm::iequals(format, "linear11")) {
    val = ConvertLinear11ToDouble(reg);
  }

  // Undefined format, throw error
  else {
    BUException::FORMATTING_NOT_IMPLEMENTED e;
    e.Append("Format: " + format);
    e.Append("\n");
    throw e;
  }
}

void BUTool::RegisterHelperIO::ReadConvert(std::string const & reg, std::string & val){
  // Read the value from the named register, and update the value in place
  std::string format = GetConvertFormat(reg);
  
  if ((format.size() > 1) && (('t' == format[0]) || ('T' == format[0]))) {
    val = ConvertEnumToString(reg, format);
  }
  // IP addresses
  else if (boost::algorithm::iequals(format, std::string("IP"))) {
    val = ConvertIPAddressToString(reg);
  }
  // Hex numbers in string
  else if ((format[0] == 'X') || (format[0] == 'x')) {
    val = ConvertHexNumberToString(reg);
  }
  // Undefined format, throw error
  else {
    BUException::FORMATTING_NOT_IMPLEMENTED e;
    e.Append("Format: " + format);
    e.Append("\n");
    throw e;
  }

}

std::unordered_map<std::string,std::string> static emptyMap;
std::unordered_map<std::string,std::string> const & BUTool::RegisterHelperIO::GetRegParameters(std::string const & /*reg*/){

  BUException::FUNCTION_NOT_IMPLEMENTED e;
  e.Append("GetRegParameters wasn't overloaded, but called\n");
  throw e;
  return emptyMap;
}
std::string BUTool::RegisterHelperIO::GetRegParameterValue(std::string const & /*reg*/,
				 std::string const & /*name*/){
  BUException::FUNCTION_NOT_IMPLEMENTED e;
  e.Append("GetRegParameterValue wasn't overloaded, but called\n");
  throw e;
  return std::string("");
}
