#include <BUTool/helpers/register_helper.hh>

#include <boost/algorithm/string/case_conv.hpp>
#include <stdio.h>
#include <stdlib.h> //strtoul
#include <map> //map

#define __STDC_FORMAT_MACROS
#include <inttypes.h> //for PRI

// used to check if RegisterHelper's TextIO pointer is NULL 
// (occurs when device class does not call SetupTextIO in its c'tor)
inline void CheckTextIO(BUTextIO* IO) {
  if (NULL == IO) {
    BUException::TEXTIO_BAD_INIT e;
    throw e;
  }
}

void BUTool::RegisterHelper::SetupTextIO() {
  char* TEXTIO_DEBUG = getenv("TEXTIO_DEBUG");
  if (dynamic_cast<BUTextIO*>(this)) {
    TextIO = dynamic_cast<BUTextIO*>(this);
    if (NULL != TEXTIO_DEBUG) {
      printf("dynamic_cast RegisterHelper this -> BUTextIO succeeded\n");
    }
  }
  else {
    TextIO = new BUTextIO();
    newTextIO = true;
    if (NULL != TEXTIO_DEBUG) {
      printf("dynamic_cast RegisterHelper this -> BUTextIO failed, creating new BUTextIO in RegisterHelper\n");
    }
  }
}

void BUTool::RegisterHelper::AddStream(Level::level level, std::ostream* os) {
  // call the BUTextIO method
  TextIO->AddOutputStream(level, os);
}

std::string BUTool::RegisterHelper::RegReadString(std::string const & /*reg*/){
  //=============================================================================
  //placeholder for string read
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  return std::string();
}

double BUTool::RegisterHelper::ConvertFloatingPoint16ToDouble(std::string const & reg){
  // Helper function to do the "fp16->double" conversion
  double doubleVal;

  union {
    struct {
      uint16_t significand   : 10;
      uint16_t exponent      : 5;
      uint16_t sign          : 1;
    } fp16;
    int16_t raw;
  } val;
  val.raw = RegReadRegister(reg);

  switch (val.fp16.exponent) {
  // Case where the exponent is minimum
  case 0:
    if (val.fp16.significand == 0) {
      doubleVal = 0.0;
    }
    else {
      doubleVal = pow(2,-14)*(val.fp16.significand/1024.0);
    }

    // Apply sign
    if (val.fp16.sign) {
      doubleVal *= -1.0;
    }
    break;
  // Case where the exponent is maximum
  case 31:
    if (val.fp16.significand == 0) {
      doubleVal = INFINITY;
      if (val.fp16.sign) {
	doubleVal *= -1.0;
      }   
    }
    else {
      doubleVal = NAN;
    }
    break;
  // Cases in between
  default:
    doubleVal = pow(2, val.fp16.exponent-15)*(1.0+(val.fp16.significand/1024.0));
    if (val.fp16.sign) {
      doubleVal *= -1.0;
    }
    break;   
  }

  return doubleVal;
}

double BUTool::RegisterHelper::ConvertIntegerToFloat(std::string const & reg, std::string const & format){
  // Helper function to convert an integer to float using the following format:
  // y = (sign)*(M_n/M_d)*x + (sign)*(b_n/b_d)
  //       [0]   [1] [2]        [3]   [4] [5]
  // sign == 0 -> negative, other values mean positive

  std::vector<uint64_t> mathValues;
  size_t iFormat=1;
 
  uint32_t rawVal = RegReadRegister(reg);
 
  while (mathValues.size() != 6 && iFormat < format.size()) {
    if (format[iFormat] == '_') {
      // Start parsing the value after the '_' and add the corresponding value to mathValues array
      for (size_t jFormat=++iFormat; jFormat < format.size(); jFormat++) {
        if ( (format[jFormat] == '_') || (jFormat == format.size() - 1) ) {
	  if (jFormat == format.size() - 1) { jFormat++; }
          // Convert the string to a number
          uint64_t val = strtoull(format.substr(iFormat, jFormat-iFormat).c_str(), NULL, 0);
          mathValues.push_back(val);
          iFormat = jFormat;
          break;
        }
      }
    }
    else {
      iFormat++;
    } 
  }

  // TODO: Some checks needed, check that mathValues.size() == 6, and no 0s in denominator

  // Compute the transformed value from the raw value
  // Will compute: (m*x) + b
  double transformedValue = rawVal;
  transformedValue *= double(mathValues[1]);
  transformedValue /= double(mathValues[2]);
  // Apply the sign of m
  if (mathValues[0] == 0) {
    transformedValue *= -1;
  }
  
  double b = double(mathValues[4]) / double(mathValues[5]);
  if (mathValues[3] == 0) {
    b *= -1;
  }
  transformedValue += b;

  return transformedValue;
}

std::string BUTool::RegisterHelper::ConvertEnumToString(std::string const & reg, std::string const & format){
  // Helper function to convert enum to std::string
  std::map<uint64_t, std::string> enumMap;
  
  // Parse the format argument for enumerations
  size_t iFormat = 1;
  while (iFormat < format.size()) {
    if (format[iFormat] == '_') {
      // Read the integer corresponding to this enum
      uint64_t val = 0;
      for (size_t jFormat = ++iFormat; jFormat < format.size(); jFormat++) {
        if ((format[jFormat] == '_') | (jFormat == format.size() - 1)) {
          if (jFormat == format.size() - 1) { jFormat++; }
          // Convert the value into number
          val = strtoul(format.substr(iFormat, jFormat-iFormat).c_str(), NULL, 0);
          iFormat = jFormat;
          break;
        }
      }
      // Now read the corresponding string
      for (size_t jFormat = ++iFormat; jFormat < format.size(); jFormat++) {
        if ((format[jFormat] == '_') || (jFormat == (format.size()-1))) {
          if (jFormat == format.size() - 1) { jFormat++; }
          // Get the string
          enumMap[val] = format.substr(iFormat, jFormat-iFormat);
          iFormat = jFormat;
          break;  
        }
      } 
    }
    else {
      iFormat++;
    }
  }

  // Now we have the enumeration map, read the integer value from the register
  // Then return the corresponding string
  uint32_t regValue = RegReadRegister(reg);
  if (enumMap.find(regValue) != enumMap.end()) {
    // If format starts with 't', just return the string value
    // Otherwise, return the value together with the number
    if (format[0] == 't') {
      return enumMap[regValue].c_str();
    }
    return (enumMap[regValue] + " " + std::to_string(regValue)).c_str();
  }

  // Cannot find it, TODO: better handle this possibility
  return "NOT_FOUND";
}

void BUTool::RegisterHelper::RegReadConvert(std::string const & reg, double & val){
  // Read the value from the named register, and update the value in place
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  
  // Check the conversion type we want:
  // Is it a "fp16", or will we do some transformations? (i.e."m_...")
  std::string format = RegReadConvertFormat(reg);
  // 16-bit floating point to double transformation
  if (format == "fp16") {
    val = ConvertFloatingPoint16ToDouble(reg);
  }
  
  // Need to do some arithmetic to transform
  else if (format.rfind("m_", 0) == 0) {
    val = ConvertIntegerToFloat(reg, format);
  }

  // TODO: Will throw an exception here: Invalid format
  else {
    val = 0.0;
  }
}

void BUTool::RegisterHelper::RegReadConvert(std::string const & reg, std::string & val){
  // Read the value from the named register, and update the value in place
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  
  std::string format = RegReadConvertFormat(reg);
  
  if ((format.size() > 1) && (('t' == format[0]) || ('T' == format[0]))) {
    val = ConvertEnumToString(reg, format);
  }
  // TODO: Throw exception, we shouldn't fall into the else case
  else {
    val = "";
  }

}

std::vector<uint32_t> BUTool::RegisterHelper::RegReadAddressFIFO(uint32_t addr,size_t count){
  //=============================================================================
  //placeholder for fifo read
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  std::vector<uint32_t> ret;
  for(size_t iRead = 0; iRead < count; iRead++){
    ret.push_back(RegReadAddress(addr)); 
  }
  return ret;
}
std::vector<uint32_t> BUTool::RegisterHelper::RegReadRegisterFIFO(std::string const & reg,size_t count){
  //=============================================================================
  //placeholder for fifo read
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  uint32_t address = GetRegAddress(reg);
  return RegReadAddressFIFO(address,count);
}

std::vector<uint32_t> BUTool::RegisterHelper::RegBlockReadAddress(uint32_t addr,size_t count){
  //=============================================================================
  //placeholder for block read
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  std::vector<uint32_t> ret;
  uint32_t addrEnd = addr + uint32_t(count);
  for(;addr < addrEnd;addr++){
    ret.push_back(RegReadAddress(addr)); 
  }
  return ret;
}
std::vector<uint32_t> BUTool::RegisterHelper::RegBlockReadRegister(std::string const & reg,size_t count){
  //=============================================================================
  //placeholder for block read
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  uint32_t address = GetRegAddress(reg);
  return RegBlockReadAddress(address,count);
}

void BUTool::RegisterHelper::RegWriteAddressFIFO(uint32_t addr,std::vector<uint32_t> const & data){
  //=============================================================================
  //placeholder for fifo write
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  for(size_t i = 0; i < data.size();i++){
    RegWriteAddress(addr,data[i]);
  }
}
void BUTool::RegisterHelper::RegWriteRegisterFIFO(std::string const & reg, std::vector<uint32_t> const & data){
  //=============================================================================
  //placeholder for fifo write
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  for(size_t i = 0; i < data.size();i++){
    RegWriteRegister(reg,data[i]);
  }
}
void BUTool::RegisterHelper::RegBlockWriteAddress(uint32_t addr,std::vector<uint32_t> const & data){
  //=============================================================================
  //placeholder for block write
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  for(size_t i =0;i < data.size();i++){
    RegWriteAddress(addr,data[i]);
    addr++;
  }
}
void BUTool::RegisterHelper::RegBlockWriteRegister(std::string const & reg, std::vector<uint32_t> const & data){
  //=============================================================================
  //placeholder for block write
  //These should be overloaded if the firmware/software natively supports these features
  //=============================================================================
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  uint32_t addr = GetRegAddress(reg);
  RegBlockWriteAddress(addr,data);
}




void BUTool::RegisterHelper::ReCase(std::string & name){
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


void BUTool::RegisterHelper::PrintRegAddressRange(uint32_t startAddress,std::vector<uint32_t> const & data,bool printWord64 ,bool skipPrintZero){
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  uint32_t addr_incr = printWord64 ? 2 : 1;
  uint32_t readNumber = 0;
  uint32_t lineWordCount = printWord64 ? 4 : 8;
  uint32_t readCount = data.size();

  //Use the RegBlockReadRegister

  for(uint32_t addr = startAddress; addr < (startAddress + readCount*addr_incr);addr+=addr_incr){

    //Print the address
    if(readNumber % lineWordCount == 0){
      TextIO->Print(Level::INFO, "0x%08x: ",  addr);
    }      
    //read the value
    uint64_t val = data[readNumber];
    readNumber++;
    if(printWord64){
      //Grab the upper bits if we are base 64
      val |= (uint64_t(data[readNumber]) << 32);
    }
    //Print the value if we are suppose to
    if(!skipPrintZero ||  (val != 0)){
      TextIO->Print(Level::INFO, " 0x%0*" PRIX64, printWord64?16:8, val);
    }else{
      TextIO->Print(Level::INFO, "   %*s", printWord64?16:8," ");
    }
    //End line
    if(readNumber % lineWordCount == 0){
      TextIO->Print(Level::INFO, "\n");
    }
  }
  //final end line
  TextIO->Print(Level::INFO, "\n");
}

CommandReturn::status BUTool::RegisterHelper::Read(std::vector<std::string> strArg,
						   std::vector<uint64_t> intArg){
  //Call the print with offset code with a zero offset
  return ReadWithOffsetHelper(0,strArg,intArg);
}

CommandReturn::status BUTool::RegisterHelper::ReadConvert(std::vector<std::string> strArg,
						   std::vector<uint64_t> intArg){
  // There should be only "0" default integer argument
  if (intArg.size() > 1) {
    return CommandReturn::BAD_ARGS; 
  }

  // We expect only a single string argument: Name of the register
  if (strArg.size() != 1) {
    return CommandReturn::BAD_ARGS; 
  }
  
  // The conversion type we want
  std::string reg = strArg[0];
  std::string format = RegReadConvertFormat(reg);

  // Depending on the format, we'll call the appropriate function with the appropriate value
  if ((format[0] == 'T') || (format[0] == 't')) {
    std::string val;
    RegReadConvert(reg, val);
    // Display the value to the screen
    TextIO->Print(Level::INFO, val.c_str());
    TextIO->Print(Level::INFO, "\n");
  }
  else if ((format[0] == 'M') || (format[0] == 'm') || (format == "fp16")) {
    double val;
    RegReadConvert(reg, val);
    // Display the value to the screen
    TextIO->Print(Level::INFO, std::to_string(val).c_str());
    TextIO->Print(Level::INFO, "\n");
  }
  else {
    return CommandReturn::BAD_ARGS;
  }

  return CommandReturn::OK; 
}

CommandReturn::status BUTool::RegisterHelper::ReadOffset(std::vector<std::string> strArg,std::vector<uint64_t> intArg){
  if(strArg.size() >= 2){
    //check that argument 2 is a number
    if(isdigit(strArg[1][0])){
      uint32_t offset = intArg[1];
      //remove argument 2
      strArg.erase(strArg.begin()+1);
      intArg.erase(intArg.begin()+1);
      return ReadWithOffsetHelper(offset,strArg,intArg);
    }
  }
  return CommandReturn::BAD_ARGS;
}



CommandReturn::status BUTool::RegisterHelper::ReadWithOffsetHelper(uint32_t offset,std::vector<std::string> strArg,std::vector<uint64_t> intArg){
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  // sort out arguments
  size_t readCount = 1;
  std::string flags("");
  bool numericAddr = true;

  switch( strArg.size() ) {
  case 0:                     // no arguments is an error
    //===================================
    return CommandReturn::BAD_ARGS;
  case 3:                     // third is flags
    //===================================
    flags = strArg[2];
    // fall through to others
  case 2:                     // second is either count or flags
    //===================================
    if( isdigit( strArg[1].c_str()[0])){
      readCount = intArg[1];
    } else if(2 == strArg.size()){
      //Since this isn't a digit it must be the flags
      // of a two argument request
      flags = strArg[1];
    }else{
      //This is a non-digit second argument of a three argument.
      return CommandReturn::BAD_ARGS;
    }
    // fall through to address
  case 1:                     // one must be an address
    //===================================
    numericAddr = isdigit( strArg[0].c_str()[0]);
    ReCase(strArg[0]);
    break;
  default:
    //===================================
    TextIO->Print(Level::INFO, "Too many arguments after command\n");
    return CommandReturn::BAD_ARGS;
  }

  //Convert flags to capital
  boost::algorithm::to_upper(flags);
  bool printWord64      = (flags.find("D") != std::string::npos);
  bool skipPrintZero    = (flags.find("N") != std::string::npos);
  //Scale the read count by two if we are doing 64 bit reads
  size_t finalReadCount = printWord64 ? 2*readCount : readCount; 


  //DO the read(s) and the printing
  std::vector<uint32_t> readData;    //Vector if we need to read multiple words
  if(numericAddr){
    //Do the read
    if(1 == readCount){
      readData.push_back(RegReadAddress(intArg[0]+offset));
    }else{
      readData = RegBlockReadAddress(intArg[0]+offset,finalReadCount);
    }
    //Print the read data
    if(0 != offset){
      TextIO->Print(Level::INFO, "Applying offset 0x%08X to 0x%08X\n",offset,uint32_t(intArg[0]));
    }
    PrintRegAddressRange(intArg[0]+offset,readData,printWord64,skipPrintZero);
  } else {
      std::vector<std::string> names = RegNameRegexSearch(strArg[0]);
      for(size_t iName = 0; iName < names.size();iName++){
        //figure out if this is an action register (write only) so we don't read it. 
        bool actionRegister = (GetRegPermissions(names[iName]).find('r') == std::string::npos);
        if(!actionRegister){
          if(1 == readCount){
            //normal printing
            if(0 == offset){
              uint32_t val = RegReadRegister(names[iName]);
              if(!skipPrintZero || (val != 0)){
                TextIO->Print(Level::INFO, "%50s: 0x%08X\n",names[iName].c_str(),val);
              }	  
            }else{
              uint32_t address = GetRegAddress(names[iName]);
              uint32_t val = RegReadAddress(address+offset);
              if(!skipPrintZero || (val != 0)){
                TextIO->Print(Level::INFO, "%50s + 0x%08X: 0x%08X\n",names[iName].c_str(),offset,val);
              }
            }
          }else{
            //switch to numeric printing because of count
            uint32_t address = GetRegAddress(names[iName])+offset;
            if(0 == offset){
              TextIO->Print(Level::INFO, "%s:\n",names[iName].c_str());
            }else{
              TextIO->Print(Level::INFO, "%s + 0x%08X:\n",names[iName].c_str(),offset);
            }
            readData = RegBlockReadAddress(address,finalReadCount);
            PrintRegAddressRange(address,readData,printWord64,skipPrintZero);
            TextIO->Print(Level::INFO, "\n");
          }
        }else{
	        if(1 == readCount){
            TextIO->Print(Level::INFO, "%50s: write-only\n",names[iName].c_str());
	        }
        }
      }
    }
  return CommandReturn::OK;
}



CommandReturn::status BUTool::RegisterHelper::ReadFIFO(std::vector<std::string> strArg,std::vector<uint64_t> intArg){
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  // sort out arguments
  size_t readCount = 1;
  bool numericAddr = true;

  if(strArg.size() == 2){
    //Check if this is a numeric address or a named register
    if(! isdigit( strArg[0].c_str()[0])){    
      numericAddr = false;
    }
    //Get read count
    if(isdigit( strArg[1].c_str()[0])){    
      readCount = intArg[1];
    }else{
      //bad count
      return CommandReturn::BAD_ARGS;
    }
  }else{
    //bad arguments
    return CommandReturn::BAD_ARGS;
  }
  
  std::vector<uint32_t> data;
  if(numericAddr){
    data = RegReadAddressFIFO(intArg[0],readCount);
    TextIO->Print(Level::INFO, "Read %zu words from 0x%08X:\n",data.size(),uint32_t(intArg[0]));
  }else{
    data = RegReadRegisterFIFO(strArg[0],readCount);
    TextIO->Print(Level::INFO, "Read %zu words from %s:\n",data.size(),strArg[0].c_str());
  }
  PrintRegAddressRange(0,data,false,false);
  return CommandReturn::OK;
}

CommandReturn::status BUTool::RegisterHelper::ReadString(std::vector<std::string> strArg,
							 std::vector<uint64_t> /*intArg*/){
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  if (strArg.size() ==0){
    return CommandReturn::BAD_ARGS;
  }
  TextIO->Print(Level::INFO, "%s: %s\n",strArg[0].c_str(),RegReadString(strArg[0]).c_str());
  return CommandReturn::OK;
}

CommandReturn::status BUTool::RegisterHelper::Write(std::vector<std::string> strArg,
						    std::vector<uint64_t> intArg) {
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  if (strArg.size() ==0){
    return CommandReturn::BAD_ARGS;
  }
  std::string saddr = strArg[0];
  ReCase(saddr);
  bool isNumericAddress = isdigit( saddr.c_str()[0]); 
  uint32_t count = 1;

  switch( strArg.size()) {
  case 1:			// address only means Action(masked) write
    TextIO->Print(Level::INFO, "Mask write to %s\n", saddr.c_str() );
    RegWriteAction(saddr);
    return CommandReturn::OK;
  case 3:                       // We have a count
    if(! isdigit(strArg[2][0])){
      return CommandReturn::BAD_ARGS;
    }
    count = intArg[2];
  case 2:                       //data to write
    //Data must be a number
    if(!isdigit(strArg[1][0])){
      return CommandReturn::BAD_ARGS;
    }
    break;
  default:
    return CommandReturn::BAD_ARGS;
  }	

  TextIO->Print(Level::INFO, "Write to ");
  if(isNumericAddress ) {
    if(1 == count){
      TextIO->Print(Level::INFO, "address 0x%08X\n", uint32_t(intArg[0]) );
      RegWriteAddress(uint32_t(intArg[0]),uint32_t(intArg[1]));    
    }else{
      std::vector<uint32_t> data(count,uint32_t(intArg[1]));
      TextIO->Print(Level::INFO, "address 0x%08X to 0x%08X\n", uint32_t(intArg[0]), uint32_t(intArg[0])+count );
      RegBlockWriteAddress(uint32_t(intArg[0]),data);
    }
    
  } else {
    if(1 == count){
      TextIO->Print(Level::INFO, "register %s\n", saddr.c_str());
      RegWriteRegister(saddr,uint32_t(intArg[1]));
    }else{
      std::vector<uint32_t> data(count,uint32_t(intArg[1]));
      uint32_t address = GetRegAddress(strArg[0]);
      TextIO->Print(Level::INFO, "address 0x%08X to 0x%08X\n", address, address+count );
      RegBlockWriteAddress(address,data);
    }
  }

  return CommandReturn::OK;
}

CommandReturn::status BUTool::RegisterHelper::WriteOffset(std::vector<std::string> strArg,std::vector<uint64_t> intArg){
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  if(strArg.size() >= 2){
    //check that argument 2 is a number
    if(isdigit(strArg[1][0])){
      uint32_t offset = intArg[1];
      //remove argument 2
      strArg.erase(strArg.begin()+1);
      intArg.erase(intArg.begin()+1);
  
      if(isdigit(strArg[0][0])){
	//numeric address
	if(0 != offset){
	  TextIO->Print(Level::INFO, "Addr 0x%08X + 0x%08X\n",uint32_t(intArg[0]),offset);
	}
	//Numeric address, just update it. 
	strArg[0] = "0"; //make it a number
	intArg[0] += offset;
	return Write(strArg,intArg);
      }else{
	//String address, convert to a numeric address 
	if(0 != offset){
	  TextIO->Print(Level::INFO, "Addr %s + 0x%08X\n",strArg[0].c_str(),offset);
	}
	uint32_t addr = GetRegAddress(strArg[0]);
	strArg[0] = "0"; //make it a number 
	intArg[0] = addr + offset;
	return Write(strArg,intArg);
      }
      
    }
  }
  return CommandReturn::BAD_ARGS;  
}


CommandReturn::status BUTool::RegisterHelper::WriteFIFO(std::vector<std::string> strArg,std::vector<uint64_t> intArg){
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  uint32_t count = 1;
  uint32_t dataVal;
  
  switch(strArg.size()){
  case 3:
    //Count, must be a number
    if(isdigit(strArg[2][0])){
      count = intArg[2];
    }else{
      return CommandReturn::BAD_ARGS;
    }
  case 2:
    if(!isdigit(strArg[1][0])){
      return CommandReturn::BAD_ARGS;
    }
    dataVal = intArg[1];
    break;
  default:
    return CommandReturn::BAD_ARGS;
  }

  //create the data
  std::vector<uint32_t> data(count,dataVal);

  //Check if the address is name or number
  if(isdigit(strArg[0][0])){
    RegWriteAddressFIFO(intArg[0],data);
  }else{
    RegWriteRegisterFIFO(strArg[0],data);
  }
  return CommandReturn::OK;
}




std::vector<std::string> BUTool::RegisterHelper::RegNameRegexSearch(std::string regex)
{
  // return a list of nodes matching regular expression
  ReCase(regex);
  //Run the regex on the derived class's myMatchRegex
  return myMatchRegex(regex);
}


CommandReturn::status BUTool::RegisterHelper::ListRegs(std::vector<std::string> strArg,
						       std::vector<uint64_t> intArg){
  CheckTextIO(TextIO); // make sure TextIO pointer isn't NULL
  (void) intArg; // keeps compiler from complaining about unused args
  std::vector<std::string> regNames;
  std::string regex;


  //Get display parameters
  bool debug = false;
  bool describe = false;
  bool help = false;
  if( strArg.size() < 1) {
    TextIO->Print(Level::INFO, "Need regular expression after command\n");
    return CommandReturn::BAD_ARGS;
  }    
  regex = strArg[0];

  if( strArg.size() > 1) {
    boost::algorithm::to_upper(strArg[1]);
    switch(strArg[1][0]) {
    case 'D':
      debug = true;
      break;
    case 'V':
      describe = true;
      break;
    case 'H':
      help = true;
      break;
    }
  }

  //Get the list of registers associated with the search term
  regNames = RegNameRegexSearch(regex);
  size_t matchingRegCount = regNames.size();
  for(size_t iReg = 0; iReg < matchingRegCount;iReg++){
    std::string const & regName = regNames[iReg];
    //Get register parameters
    uint32_t addr = GetRegAddress(regName);
    uint32_t mask = GetRegMask(regName);
    uint32_t size = GetRegSize(regName);
    
    //Print main line
    TextIO->Print(Level::INFO, "  %3zu: %-60s (addr=%08x mask=%08x) ", iReg+1, regNames[iReg].c_str(), addr, mask);

    //Print mode attribute
    TextIO->Print(Level::INFO, "%s",GetRegMode(regName).c_str());

    //Print permission attribute
    TextIO->Print(Level::INFO, "%s",GetRegPermissions(regName).c_str());
    if(size > 1){
      //Print permission attribute
      TextIO->Print(Level::INFO, " size=0x%08X",size);
    }
    //End first line
    TextIO->Print(Level::INFO, "\n");

    //optional description
    if(describe){
      TextIO->Print(Level::DEBUG, "       %s\n",GetRegDescription(regName).c_str());
    }
    
    //optional debugging info
    if(debug){
      TextIO->Print(Level::DEBUG, "%s\n",GetRegDebug(regName).c_str());
    }
    //optional help
    if(help){
      TextIO->Print(Level::DEBUG, "%s\n",GetRegHelp(regName).c_str());
    }

  }
  return CommandReturn::OK;
}



std::string BUTool::RegisterHelper::RegisterAutoComplete(std::vector<std::string> const & line , std::string const & currentToken,int state){
  (void) line; // casting to void to keep comiler from complaining about unused param
  static size_t pos;
  static std::vector<std::string> completionList;
  if(!state) {
    //Check if we are just starting out
    pos = 0;
    completionList = RegNameRegexSearch(currentToken+std::string("*"));
  } else {
    //move forward in pos
    pos++;
  }
    
  if(pos < completionList.size()){
    return completionList[pos];
  }
  //not found
  return std::string("");    
}
