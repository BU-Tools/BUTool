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
    format.clear();
    displayRule.clear();
    enabled = true;
    statusLevel = 0;
  }
  void StatusDisplayCell::Setup(RegisterHelperIO * _regIO,
      std::string const & _address,  // Stripped of Hi/Lo
		  std::string const & _row, // Stripped of Hi/Lo
		  std::string const & _col  // Stripped of Hi/Lo
      )
  {
    /*
    Set up a single StatusDisplayCell.

    This function does the reads with the given RegisterHelperIO pointer it is provided,
    and stores the register parameters as class member data.
    */

    // Store RegisterHelperIO pointer as a class member
    regIO = _regIO;

    /* Read the 32-bit word from this register and store it as member data
     * This data will be used for determining whether the cell should be displayed or not,
     * without the need for calling regIO methods again.
     * If a BUS_ERROR happens here, the function calling Setup will catch it and skip this register. 
     */
    word = regIO->ReadRegister(_address);

    // Using the RegisterHelperIO pointer, retrieve data about this register
    std::string _description = regIO->GetRegDescription(_address);
    std::string _format;
    try {
      _format = regIO->GetRegParameterValue(_address, "Format");
    } catch (BUException::BAD_VALUE & e) { _format = STATUS_DISPLAY_DEFAULT_FORMAT; }

    std::string _statusLevel;
    try {
      _statusLevel = regIO->GetRegParameterValue(_address, "Status");
    } catch (BUException::BAD_VALUE & e) { _statusLevel = std::string(); }

    std::string _rule;
    try {
      _rule = regIO->GetRegParameterValue(_address, "Show");
    } catch (BUException::BAD_VALUE & e) { _rule = std::string(); }
    boost::to_upper(_rule);

    // Determine if this register is "enabled" to be shown
    bool _enabled=true;
    try {
      // True if string isn't equal to "0"
      _enabled=regIO->GetRegParameterValue(_address, "Enabled").compare("0");
    } catch (BUException::BAD_VALUE & e) {
      _enabled=true;
    }

    // Store additional data for this register
    convertType = regIO->GetConvertType(_address);
    mask = regIO->GetRegMask(_address);

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

  int StatusDisplayCell::DisplayLevel() const {return statusLevel;}

  bool StatusDisplayCell::SuppressRow( bool force) const
  {
    bool suppressRow = (iequals( displayRule, "nzr") && (word == 0)) && !force;
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
    // Decide if we should display this cell
    bool display = (level >= statusLevel) && (statusLevel != 0);

    // Check against the print rules
    if(iequals(displayRule,"nz")){
      display = display & (word != 0); //Show when non-zero
    } else if(iequals(displayRule,"z")){
      display = display & (word == 0); //Show when zero
    }

    // Apply "channel"-like enable mask
    display = display && enabled;

    // Force display if we want
    display = display || force;
    return display;
  }

  void StatusDisplayCell::ReadAndFormatHexString(char * buffer, int bufferSize, int width) const {
    /*
    Wrapper function to format a hex string for a register, and write it
    to the given buffer. The buffer will be modified in place.
    */
    
    // Read the 32-bit value from the register and convert to 64-bit value
    // so that it's ready for printing
    uint64_t val = regIO->ComputeValueFromRegister(address);

    // Now, do the formatting
    std::string fmtString = "%";
    if (val >= 10) {
      fmtString.assign("0x%");
      if (width >= 0) {
        width -= 2;
      }
    }

    // Zero padding or space padding, depending on the format
    if (width >= 0) {
      if (iequals(format, "x")) {
        fmtString.append("0*");
      }
      else {
        fmtString.append("*");
      }
    }

    // PRI macro to print 64-bit hex value    
    fmtString.append(PRIX64);

    if (width == -1) {
      snprintf(buffer, bufferSize, fmtString.c_str(), val);
    }
    else {
      snprintf(buffer, bufferSize, fmtString.c_str(), width, val);
    }
  }

  void StatusDisplayCell::ReadAndFormatDouble(char * buffer, int bufferSize, int /* width */) const {
    /*
    Wrapper function to read and properly format a double value from the register.
    The formatted double value will be written to the buffer in-place.
    */

    // Retrieve the double value
    double value;
    regIO->ReadConvert(address, value);

    // Do the formatting and write to the buffer
    if (iequals(format, std::string("fp16"))) {
      // If the double value is very large or very small, use scientific notation
      if ( ((fabs(value) > 10000) || (fabs(value) < 0.001)) && (value != 0) ) {
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
  }

  void StatusDisplayCell::ReadAndFormatInt(char * buffer, int bufferSize, int width) const {
    /*
    Wrapper function to read and format an integer value.
    The formatted integer value will be written to the buffer in-place.
    
    Please note that we're explicitly using 64-bit integers to avoid confusion.
    */
    
    // Retrieve the value
    int64_t value;
    regIO->ReadConvert(address, value);
    
    // Do the formatting and write to the buffer
    // Build the format string for snprintf
    std::string fmtString("%");
    
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
  }

  void StatusDisplayCell::ReadAndFormatUInt(char * buffer, int bufferSize, int width) const {
    /*
    Wrapper function to read and format an unsigned integer value.
    The formatted unsigned integer value will be written to the buffer in-place.

    Please note that we're explicitly using 64-bit unsigned integers to avoid confusion.
    */

    // Retrieve the value
    uint64_t value;
    regIO->ReadConvert(address, value);

    // Do the formatting and write to the buffer
    // Build the format string for snprintf
    std::string fmtString("%");

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
  }

  std::string StatusDisplayCell::Print(int width = -1,bool /*html*/) const
  { 
    const int bufferSize = 20;
    char buffer[bufferSize+1];  //64bit integer can be max 20 ascii chars (as a signed int)
    memset(buffer,' ',20);
    buffer[bufferSize] = '\0';

    // Read+write values to the buffer based on the convert type for this register
    switch(convertType) {
      case RegisterHelperIO::STRING:
      {
        std::string value;
        regIO->ReadConvert(address, value);

        // Special hex formatting for format='x' or format='X'
        if (iequals(format, std::string("x"))) { 
          ReadAndFormatHexString(buffer, bufferSize, width); 
        }
        // For other types, just write the resulting value as a C-string to the buffer
        else { snprintf(buffer,bufferSize,"%s",value.c_str()); }
        break;
      }
      case RegisterHelperIO::FP:
      {
        ReadAndFormatDouble(buffer, bufferSize, width);
        break;
      }
      case RegisterHelperIO::INT:
      {
        ReadAndFormatInt(buffer, bufferSize, width);
        break;
      }
      case RegisterHelperIO::UINT:
      {
        ReadAndFormatUInt(buffer, bufferSize, width);
        break;
      }
      // Default is hex format for StatusDisplay 
      case RegisterHelperIO::NONE:
      default:
      {
        ReadAndFormatHexString(buffer, bufferSize, width); 
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
