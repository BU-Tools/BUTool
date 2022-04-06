#include <RegisterHelper/RegisterHelper.hh>

#include <boost/algorithm/string/case_conv.hpp>
#include <stdio.h>

#include <BUTool/ToolException.hh>

#define __STDC_FORMAT_MACROS
#include <inttypes.h> //for PRI


void BUTool::RegisterHelper::PrintRegAddressRange(uint32_t startAddress,std::vector<uint32_t> const & data,bool printWord64 ,bool skipPrintZero){
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
    regIO->ReCase(strArg[0]);
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
      readData.push_back(regIO->ReadAddress(intArg[0]+offset));
    }else{
      readData = regIO->BlockReadAddress(intArg[0]+offset,finalReadCount);
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
        bool actionRegister = (regIO->GetRegPermissions(names[iName]).find('r') == std::string::npos);
        if(!actionRegister){
          if(1 == readCount){
            //normal printing
            if(0 == offset){
              uint32_t val = regIO->ReadRegister(names[iName]);
              if(!skipPrintZero || (val != 0)){
                TextIO->Print(Level::INFO, "%50s: 0x%08X\n",names[iName].c_str(),val);
              }	  
            }else{
              uint32_t address = regIO->GetRegAddress(names[iName]);
              uint32_t val = regIO->ReadAddress(address+offset);
              if(!skipPrintZero || (val != 0)){
                TextIO->Print(Level::INFO, "%50s + 0x%08X: 0x%08X\n",names[iName].c_str(),offset,val);
              }
            }
          }else{
            //switch to numeric printing because of count
            uint32_t address = regIO->GetRegAddress(names[iName])+offset;
            if(0 == offset){
              TextIO->Print(Level::INFO, "%s:\n",names[iName].c_str());
            }else{
              TextIO->Print(Level::INFO, "%s + 0x%08X:\n",names[iName].c_str(),offset);
            }
            readData = regIO->BlockReadAddress(address,finalReadCount);
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
    data = regIO->ReadAddressFIFO(intArg[0],readCount);
    TextIO->Print(Level::INFO, "Read %zu words from 0x%08X:\n",data.size(),uint32_t(intArg[0]));
  }else{
    data = regIO->ReadRegisterFIFO(strArg[0],readCount);
    TextIO->Print(Level::INFO, "Read %zu words from %s:\n",data.size(),strArg[0].c_str());
  }
  PrintRegAddressRange(0,data,false,false);
  return CommandReturn::OK;
}

CommandReturn::status BUTool::RegisterHelper::ReadString(std::vector<std::string> strArg,
							 std::vector<uint64_t> /*intArg*/){
  if (strArg.size() ==0){
    return CommandReturn::BAD_ARGS;
  }
  TextIO->Print(Level::INFO, "%s: %s\n",strArg[0].c_str(),regIO->ReadString(strArg[0]).c_str());
  return CommandReturn::OK;
}

CommandReturn::status BUTool::RegisterHelper::Write(std::vector<std::string> strArg,
						    std::vector<uint64_t> intArg) {
  if (strArg.size() ==0){
    return CommandReturn::BAD_ARGS;
  }
  std::string saddr = strArg[0];
  regIO->ReCase(saddr);
  bool isNumericAddress = isdigit( saddr.c_str()[0]); 
  uint32_t count = 1;

  switch( strArg.size()) {
  case 1:			// address only means Action(masked) write
    TextIO->Print(Level::INFO, "Mask write to %s\n", saddr.c_str() );
    regIO->WriteAction(saddr);
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
      regIO->WriteAddress(uint32_t(intArg[0]),uint32_t(intArg[1]));    
    }else{
      std::vector<uint32_t> data(count,uint32_t(intArg[1]));
      TextIO->Print(Level::INFO, "address 0x%08X to 0x%08X\n", uint32_t(intArg[0]), uint32_t(intArg[0])+count );
      regIO->BlockWriteAddress(uint32_t(intArg[0]),data);
    }
    
  } else {
    if(1 == count){
      TextIO->Print(Level::INFO, "register %s\n", saddr.c_str());
      regIO->WriteRegister(saddr,uint32_t(intArg[1]));
    }else{
      std::vector<uint32_t> data(count,uint32_t(intArg[1]));
      uint32_t address = regIO->GetRegAddress(strArg[0]);
      TextIO->Print(Level::INFO, "address 0x%08X to 0x%08X\n", address, address+count );
      regIO->BlockWriteAddress(address,data);
    }
  }

  return CommandReturn::OK;
}

CommandReturn::status BUTool::RegisterHelper::WriteOffset(std::vector<std::string> strArg,std::vector<uint64_t> intArg){
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
	uint32_t addr = regIO->GetRegAddress(strArg[0]);
	strArg[0] = "0"; //make it a number 
	intArg[0] = addr + offset;
	return Write(strArg,intArg);
      }
      
    }
  }
  return CommandReturn::BAD_ARGS;  
}


CommandReturn::status BUTool::RegisterHelper::WriteFIFO(std::vector<std::string> strArg,std::vector<uint64_t> intArg){
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
    regIO->WriteAddressFIFO(intArg[0],data);
  }else{
    regIO->WriteRegisterFIFO(strArg[0],data);
  }
  return CommandReturn::OK;
}




std::vector<std::string> BUTool::RegisterHelper::RegNameRegexSearch(std::string regex)
{
  // return a list of nodes matching regular expression
  regIO->ReCase(regex);
  //Run the regex on the derived class's myMatchRegex
  return regIO->myMatchRegex(regex);
}


CommandReturn::status BUTool::RegisterHelper::ListRegs(std::vector<std::string> strArg,
						       std::vector<uint64_t> /*intArg*/){
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
    uint32_t addr = regIO->GetRegAddress(regName);
    uint32_t mask = regIO->GetRegMask(regName);
    uint32_t size = regIO->GetRegSize(regName);
    
    //Print main line
    TextIO->Print(Level::INFO, "  %3zu: %-60s (addr=%08x mask=%08x) ", iReg+1, regNames[iReg].c_str(), addr, mask);

    //Print mode attribute
    TextIO->Print(Level::INFO, "%s",regIO->GetRegMode(regName).c_str());

    //Print permission attribute
    TextIO->Print(Level::INFO, "%s",regIO->GetRegPermissions(regName).c_str());
    if(size > 1){
      //Print permission attribute
      TextIO->Print(Level::INFO, " size=0x%08X",size);
    }
    //End first line
    TextIO->Print(Level::INFO, "\n");

    //optional description
    if(describe){
      TextIO->Print(Level::DEBUG, "       %s\n",regIO->GetRegDescription(regName).c_str());
    }
    
    //optional debugging info
    if(debug){
      TextIO->Print(Level::DEBUG, "%s\n",regIO->GetRegDebug(regName).c_str());
    }
    //optional help
    if(help){
      TextIO->Print(Level::DEBUG, "%s\n",regIO->GetRegHelp(regName).c_str());
    }

  }
  return CommandReturn::OK;
}



std::string BUTool::RegisterHelper::RegisterAutoComplete(std::vector<std::string> const & /*line*/ , std::string const & currentToken,int state){
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
