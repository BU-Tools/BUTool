#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm> //std::max
#include <iomanip> //for std::setw
#include <ctype.h> // for isDigit()

#include <stdio.h> //snprintf
#include <stdlib.h> //strtoul

#include <BUTool/ToolException.hh>
#include <StatusDisplay/StatusDisplayCell.hh>

#include <arpa/inet.h> //for inet_ntoa and in_addr_t


//For PRI macros
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

namespace BUTool{

  using boost::algorithm::iequals;


  //=============================================================================
  //===== Cell Class
  //=============================================================================
  void StatusDisplayCell::Clear()
  {
    address.clear();
    description.clear();
    row.clear();
    col.clear();
    word.clear();
    wordShift.clear();
    format.clear();
    displayRule.clear();
    enabled = true;
    statusLevel = 0;
  }
  void StatusDisplayCell::Setup(RegisterHelperIO * _regIO,
      std::string const & _address,  // Stripped of Hi/Lo
		  std::string const & _row, // Stripped of Hi/Lo
		  std::string const & _col, // Stripped of Hi/Lo
      )
  {
    /*
    Set up a single StatusDisplayCell.

    This function does the reads with the given RegisterHelperIO pointer it is provided,
    and stores the register parameters as class member data.
    */

    // Store RegisterHelperIO pointer as a class member
    regIO = _regIO;

    // Using the RegisterHelperIO pointer, retrieve data about this register
    std::string _description = regIO->GetRegDescription(_address);
    std::string _format      = regIO->GetRegParameterValueWithDefault(_address, "Format", STATUS_DISPLAY_DEFAULT_FORMAT);
    std::string _statusLevel = regIO->GetRegParameterValueWithDefault(_address, "Status", std::string());
    std::string _rule        = regIO->GetRegParameterValueWithDefault(_address, "Show",   std::string());
    boost::to_upper(_rule);

    convertType = regIO->GetConvertType(_address);

    // Determine if this register is "enabled" to be shown
    bool _enabled=true;
    try {
      // True if string isn't equal to "0"
      _enabled=regIO->GetRegParameterValue(_address, "Enabled").compare("0");
    } catch (BUException::BAD_VALUE & e) {
      _enabled=true;
    }

    // Store all this information as class member variables
    // These must all be the same
    CheckAndThrow("Address",address,_address);
    CheckAndThrow(address + " row",row,_row);
    CheckAndThrow(address + " col",col,_col);
    CheckAndThrow(address + " format",format,_format);
    CheckAndThrow(address + " rule",displayRule,_rule);
    
    //Append the description for now
    description += _description;

    //any other formatting
    statusLevel = strtoul(_statusLevel.c_str(),
		     NULL,0);
    enabled = _enabled;
  }

  void StatusDisplayCell::Fill(uint32_t value,
		  size_t bitShift)
  {
    word.push_back(value);
    wordShift.push_back(bitShift);
  }

  int StatusDisplayCell::DisplayLevel() const {return statusLevel;}

  bool StatusDisplayCell::SuppressRow( bool force) const
  {
    // Compute the full value for this entry
    uint64_t val = ComputeValue();
    bool suppressRow = (iequals( displayRule, "nzr") && (val == 0)) && !force;
    return suppressRow;
  }

  std::string const & StatusDisplayCell::GetRow() const {return row;}
  std::string const & StatusDisplayCell::GetCol() const {return col;}
  std::string const & StatusDisplayCell::GetDesc() const {return description;}
  std::string const & StatusDisplayCell::GetAddress() const {return address;}

  void StatusDisplayCell::SetAddress(std::string const & _address){address = _address;}
  uint32_t const & StatusDisplayCell::GetMask() const {return mask;}
  void StatusDisplayCell::SetMask(uint32_t const & _mask){mask = _mask;}


  bool StatusDisplayCell::Display(int level,bool force) const
  {
    // Compute the full value for this entry
    uint64_t val = ComputeValue();

    // Decide if we should display this cell
    bool display = (level >= statusLevel) && (statusLevel != 0);

    // Check against the print rules
    if(iequals(displayRule,"nz")){
      display = display & (val != 0); //Show when non-zero
    } else if(iequals(displayRule,"z")){
      display = display & (val == 0); //Show when zero
    }

    //Apply "channel"-like enable mask
    display = display && enabled;

    //Force display if we want
    display = display || force;
    return display;
  }
  uint64_t StatusDisplayCell::ComputeValue() const
  {
    //Compute full value
    uint64_t val = 0;
    for(size_t i = 0; i < word.size();i++){
      if(word.size() > 1){//If we have multiple values to merge
	val += (uint64_t(word[i]) << wordShift[i]);
      }else{//If we have just one value
	val += uint64_t(word[i]);
      }
    }
    
    //We do not support signed integers that have more than 32 bits
    if((iequals(format,std::string("d")) ||     //signed integer
	iequals(format,std::string("linear"))   // linear11 or linear16
	) &&
       word.size() == 1){
      //This is goign to be printed with "d", so we need to sign extend the number we just comptued
      uint64_t temp_mask = GetMask();

      //Count bits in mask 
      uint64_t b; // c accumulates the total bits set in v

      for (b = 0; temp_mask; temp_mask >>= 1)
	{
	  b += temp_mask & 1;
	}


      // sign extend magic from https://graphics.stanford.edu/~seander/bithacks.html#FixedSignExtend
      int64_t x = val;      // sign extend this b-bit number to r
      int64_t r;      // resulting sign-extended number
      int64_t const m = 1U << (b - 1); // mask can be pre-computed if b is fixed

      x = x & ((1U << b) - 1);  // (Skip this if bits in x above position b are already zero.)
      r = (x ^ m) - m;
      val = (uint64_t) r;
    }

    return val;
  }

  std::string StatusDisplayCell::Print(int width = -1,bool /*html*/) const
  { 
    const int bufferSize = 20;
    char buffer[bufferSize+1];  //64bit integer can be max 20 ascii chars (as a signed int)
    memset(buffer,' ',20);
    buffer[bufferSize] = '\0';

    //Build the format string for snprintf
    std::string fmtString("%");

    // Read+write values to the buffer based on the convert type for this register
    switch(convertType) {
      case RegisterHelperIO::STRING:
      {
        std::string value;
        regIO->ReadConvert(address, value);

        // Hex formatting for format='x' or format='X'
        if (iequals(format, std::string("x"))) {
          uint32_t val = regIO->ReadRegister(address);
          if (val >= 10) {
            fmtString.assign("0x%");
            if (width >= 0) {
              width -= 2;
            }
          }
          else {
            fmtString.assign("%");
          }

          if (width >= 0) {
            fmtString.append("*");
          }
          fmtString.append(PRIX64);

          if (width == -1) {
            snprintf(buffer, bufferSize, fmtString.c_str(), val);
          }
          else {
            snprintf(buffer, bufferSize, fmtString.c_str(), width, val);
          }
        }
        else {
          snprintf(buffer,bufferSize,"%s",value.c_str());
        }
        break;
      }
      case RegisterHelperIO::FP:
      {
        double value;
        regIO->ReadConvert(address, value);
        if (iequals(format, std::string("fp16"))) {
          // If the double value is very large or very small, use scientific notation
          if ((fabs(value) > 10000) || (fabs(value) < 0.001)) {
            snprintf(buffer,bufferSize,"%3.2e",value);
          }
          else {
            snprintf(buffer,bufferSize,"%3.2f",value);
          }
        }
        else if (iequals(format,std::string("linear11"))) {
          snprintf(buffer,bufferSize,"%3.3f",value);  
        }
        else {
          snprintf(buffer,bufferSize,"%3.2f",value);
        }
        break;
      }
      case RegisterHelperIO::INT:
      {
        int64_t value;
        regIO->ReadConvert(address, value);
        // If we are specifying the width, add a *
        if (width >= 0) {
          fmtString.append("*");
        }
        fmtString.append(PRId64);

        if (width == -1) {
          snprintf(buffer, bufferSize, fmtString.c_str(), value);
        }
        else {
          snprintf(buffer, bufferSize, fmtString.c_str(), width, value);
        }
        break;
      }
      case RegisterHelperIO::UINT:
      case RegisterHelperIO::NONE:
      default:
      {
        uint64_t value;
        regIO->ReadConvert(address, value);
        // If we are specifying the width, add a *
        if (width >= 0) {
          fmtString.append("*");
        }
        fmtString.append(PRIu64);
        if (width == -1) {
          snprintf(buffer, bufferSize, fmtString.c_str(), value);
        }
        else {
          snprintf(buffer, bufferSize, fmtString.c_str(), width, value);
        }
        break;
      }
    }
    return std::string(buffer);
  }

  void StatusDisplayCell::CheckAndThrow(std::string const & name,
			   std::string & thing1,
			   std::string const & thing2) const
  {
    //Checks
    if(thing1.size() == 0){
      thing1 = thing2;
    } else if(!iequals(thing1,thing2)) {
      BUException::BAD_VALUE e;
      e.Append(name);
      e.Append(" mismatch: "); 
      e.Append(thing1); e.Append(" != ");e.Append(thing2);
      throw e;
    }    
  }
  
  
}
