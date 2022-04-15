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





void BUTool::RegisterHelperIO::ReadConvert(std::string const & reg, unsigned int & val){
  // Read the value from the named register, and update the value in place
  uint32_t rawVal = ReadRegister(reg);
  val = rawVal;
}

void BUTool::RegisterHelperIO::ReadConvert(std::string const & reg, int & val){
  // Read the value from the named register, and update the value in place
  uint32_t rawVal = ReadRegister(reg);

  // Now comes the check:
  // -55 is a placeholder for non-running fireflies
  // That would mean rawVal 256 + (-55) = 201 since the raw value is an unsigned int
  // We'll transform and return that value, in other cases our job is easier
  int MAX_8_BIT_INT = 256;
  if (rawVal == 201) {
    val = -(int)(MAX_8_BIT_INT - rawVal);
    return;
  }
  val = (int)rawVal;
 
}

void BUTool::RegisterHelperIO::ReadConvert(std::string const & reg, double & val){
  // Read the value from the named register, and update the value in place
  // Check the conversion type we want:
  // Is it a "fp16", or will we do some transformations? (i.e."m_...")
  std::string format = ReadConvertFormat(reg);
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
  std::string format = ReadConvertFormat(reg);
  
  if ((format.size() > 1) && (('t' == format[0]) || ('T' == format[0]))) {
    val = ConvertEnumToString(reg, format);
  }
  // IP addresses
  else if (boost::algorithm::iequals(format, std::string("IP"))) {
    val = ConvertIPAddressToString(reg);
  }
  // Hex numbers in string
  else if ((format[0] == 'X') || (format[0] == 'x')) {
    val = ReadRegister(reg);
  }
  // Undefined format, throw error
  else {
    BUException::FORMATTING_NOT_IMPLEMENTED e;
    e.Append("Format: " + format);
    e.Append("\n");
    throw e;
  }

}


std::vector<std::string> BUTool::RegisterHelperIO::GetRegParameters(std::string const & /*reg*/){
  std::vector<std::string> ret;
  return ret;
}
std::string BUTool::RegisterHelperIO::GetRegParameterValue(std::string const & /*reg*/,
				 std::string const & /*name*/){
  BUException::FUNCTION_NOT_IMPLEMENTED e;
  e.Append("GetRegParameterValue wasn't overloaded, but called\n");
  throw e;
  return std::string("");
}
