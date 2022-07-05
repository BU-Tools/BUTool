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


uint64_t BUTool::RegisterHelperIO::ComputeValueFromRegister(std::string const & reg){
  /*
  Reads the raw 32-bit unsigned integer value from the register, and returns a 64-bit
  unsigned integer.

  If the register name ("reg") contains a "_LO" or "_HI", this function will handle the
  merging of the 32-bit values and return the merged value.
  */
  uint32_t rawValue = ReadRegister(reg);

  // 64-bit unsigned integer we're going to return
  uint64_t result;
  
  // No values to merge, convert 32-bit value to 64-bit and return
  if ( !((reg.find("_LO") == reg.size()-3) || (reg.find("_HI") == reg.size()-3)) ) {
    result = uint64_t(rawValue);
    return result;
  }

  // Below this line, merging of different values is handled
  uint32_t regMask = GetRegMask(reg);

  // Figure out the number of bit shifts from the size of the word
  // 32 bits is the default, but it will be less for smaller values
  int numBitShifts = 32;
  
  while ( (regMask & 0x1) == 0 ) { regMask >>= 1; }
  int count = 0;
  while ( (regMask & 0x1) == 1 ) {
    count++;
    regMask >>= 1;
  }
  numBitShifts = count;
  
  // Check if this register contains a "_LO", so it is going to be merged with a "_HI"
  if ( reg.find("_LO") == reg.size()-3 ) {
    std::string baseRegisterName = reg.substr(0,reg.find("_LO"));

    // Name of the corresponding "_HI" register
    std::string highRegisterName = baseRegisterName;
    highRegisterName.append("_HI");

    // Read the 32-bit value from the "_HI" register
    uint32_t highValue = ReadRegister(highRegisterName);

    // Construct the result value
    result = uint64_t(rawValue) + ( uint64_t(highValue) << numBitShifts );
  }

  // Check if this register contains a "_HI", so it is going to be merged with a "_LO"
  // (the only possibility at this point)
  else {
    std::string baseRegisterName = reg.substr(0,reg.find("_HI"));

    // Name of the corresponding "_LO" register
    std::string lowRegisterName = baseRegisterName;
    lowRegisterName.append("_LO");

    // Read the 32-bit value from the "_HI" register
    uint32_t lowValue = ReadRegister(lowRegisterName);

    // Construct the result value
    result = ( uint64_t(rawValue) << numBitShifts ) + uint64_t(lowValue);
  }

  return result;

}


void BUTool::RegisterHelperIO::ReadConvert(std::string const & reg, uint64_t & val){
  // Read the value from the named register, and update the value in place
  val = ComputeValueFromRegister(reg);
}


void BUTool::RegisterHelperIO::ReadConvert(std::string const & reg, int64_t & val){
  // Read the value from the named register, and update the value in place
  int64_t rawVal = ComputeValueFromRegister(reg); 
  uint64_t mask  = GetRegMask(reg);

  // Count number of bits in the mask
  uint64_t b;
  // Shift all the bits to right until we count all the bits in the mask
  for (b=0; mask; mask >>= 1)
  {
    b += mask & 1;
  }

  // Sign extend b-bit number, override the value in x
  // See here: https://graphics.stanford.edu/~seander/bithacks.html#FixedSignExtend
  int64_t x = rawVal;
  int64_t const m = 1U << (b - 1); 

  x = x & ((1U << b) - 1);
  val = (x ^ m) - m;
}

void BUTool::RegisterHelperIO::ReadConvert(std::string const & reg, double & val){
  // Read the value from the named register, and update the value in place
  // Check the conversion type we want:
  // Is it a "fp16", or will we do some transformations? (i.e."m_...")
  
  std::string format = GetConvertFormat(reg);
  uint64_t rawValue = ComputeValueFromRegister(reg);

  // 16-bit floating point to double transformation
  if (boost::algorithm::iequals(format, "fp16")) {
    val = ConvertFloatingPoint16ToDouble(rawValue);
  }
  
  // Need to do some arithmetic to transform
  else if ((format[0] == 'M') | (format[0] == 'm')) {
    val = ConvertIntegerToDouble(rawValue, format);
  }

  else if (boost::algorithm::iequals(format, "linear11")) {
    val = ConvertLinear11ToDouble(rawValue);
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
  uint64_t rawValue = ComputeValueFromRegister(reg);
  
  if ((format.size() > 1) && (('t' == format[0]) || ('T' == format[0]))) {
    val = ConvertEnumToString(rawValue, format);
  }
  // IP addresses
  else if (boost::algorithm::iequals(format, std::string("IP"))) {
    val = ConvertIPAddressToString(rawValue);
  }
  // Hex numbers in string
  else if ((format[0] == 'X') || (format[0] == 'x')) {
    val = ConvertHexNumberToString(rawValue);
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
