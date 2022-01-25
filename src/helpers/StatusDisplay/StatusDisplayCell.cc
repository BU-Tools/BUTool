#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm> //std::max
#include <iomanip> //for std::setw
#include <ctype.h> // for isDigit()

#include <stdio.h> //snprintf
#include <stdlib.h> //strtoul

#include <BUTool/ToolException.hh>
#include <BUTool/helpers/StatusDisplay/StatusDisplayCell.hh>

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
  void StatusDisplayCell::Setup(std::string const & _address,  //stripped of Hi/Lo
		   std::string const & _description,
		   std::string const & _row, //Stripped of Hi/Lo
		   std::string const & _col, //Stripped of Hi/Lo
		   std::string const & _format,
		   std::string const & _rule,
		   std::string const & _statusLevel,
		   bool _enabled)
  {
    //These must all be the same
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
    if((format.size() > 1) && (('t' == format[0]) || ('T' == format[0]))){
      // t(T) is for enum
      std::map<uint64_t,std::string> enumMap;
      size_t iFormat = 1;
      while(iFormat < format.size()){
	if(format[iFormat] == '_'){
	  //start parsing 
	  uint64_t val = 0;
	  for(size_t jFormat=++iFormat;jFormat <format.size();jFormat++){
	    if((format[jFormat] == '_') || (jFormat == (format.size()-1))){
	      //convert value to number
	      if(jFormat == (format.size()-1)){
		jFormat++;
	      }
	      val = strtoul(format.substr(iFormat,jFormat-iFormat).c_str(),NULL,0);	      
	      iFormat = jFormat;
	      break;
	    }
	  }
	  for(size_t jFormat=++iFormat;jFormat <format.size();jFormat++){
	    if((format[jFormat] == '_') || (jFormat == (format.size()-1))){
	      //convert value to number
	      if(jFormat == (format.size()-1)){
		jFormat++;
	      }
	      enumMap[val] = format.substr(iFormat,jFormat-iFormat);
	      iFormat = jFormat;
	      break;
	    }
	  }
	}else{
	  iFormat++;
	}
      }
      uint64_t regValue = ComputeValue();
      if(enumMap.find(regValue) != enumMap.end()){
	if('t' == format[0]){
	  //Just format for 't'
	  snprintf(buffer,bufferSize,"%s",enumMap[regValue].c_str());
	}else{
	  //format and number in hex for 'T'
	  snprintf(buffer,bufferSize,"%s (0x%" PRIX64 ")",enumMap[regValue].c_str(),regValue);
	}       
      }else{
	snprintf(buffer,bufferSize,"0x%" PRIX64 ")",regValue);
      }
    }else if((format.size() > 1) && (('m' == format[0]) || ('M' == format[0]))){      
      //Convert from integer to floating point using
      //y = (sign)*(M_n/M_d)*x + (sign)*(b_n/b_d)
      //      [0]   [1] [2]       [3]    [4] [5]
      // sign == 0 is negative, all others mean positive
      //Split the '_' separated values
      std::vector<uint64_t> mathValues;
      size_t iFormat = 1;
      while(mathValues.size() != 6 && iFormat < format.size()){
	if(format[iFormat] == '_'){
	  //start parsing
	  for(size_t jFormat=++iFormat;jFormat <format.size();jFormat++){
	    if((format[jFormat] == '_') || (jFormat == (format.size()-1))){
	      //convert value to number
	      if(jFormat == (format.size()-1)){
		jFormat++;
	      }
	      uint64_t val = strtoull(format.substr(iFormat,jFormat-iFormat).c_str(),NULL,0);
	      mathValues.push_back(val);
	      iFormat = jFormat;
	      break;
	    }
	  }
	}else{
	  iFormat++;
	}
      }
      //check that there are 6 values
      if(mathValues.size() != 6){
	return std::string(buffer);	
      }
      //check that no demoniator is 0
      if((mathValues[2] == 0) || (mathValues[5] == 0)){
	return std::string(buffer);
      }

      //computer the value ((m * x) + b)      
      double transformedValue = ComputeValue();
      //multiply by absolute value of m
      transformedValue *= double(mathValues[1]);
      transformedValue /= double(mathValues[2]); 
      if(mathValues[0] == 0){
	//apply sign of m
	transformedValue *= -1;
      }
      
      double b = double(mathValues[4])/double(mathValues[5]);
      if(mathValues[3] != 0){
	transformedValue += b;
      }else{
	transformedValue -= b;
      }

      //print it
      snprintf(buffer,bufferSize,	       
	       "%3.2f",transformedValue);

    }else if(iequals(format,std::string("linear11"))){
      //union/struct magic to automatically convert the 11 base  and
      //5 bit mantissa into ints which are assigned via the raw value
      //This is the nused to build the floating point value
      union {
	struct {
	  int16_t integer  : 11;
	  int16_t exponent :  5;
	} linear11;
	int16_t raw;} val;
      val.raw = ComputeValue();
      double floatingValue = double(val.linear11.integer) * pow(2,val.linear11.exponent);
      snprintf(buffer,bufferSize,
	       "%3.3f",floatingValue);
    }else if(iequals(format,std::string("fp16"))){
      //union/struct magic to automatically convert the 1 sign, 10 base  and
      //5 bit mantissa into ints which are assigned via the raw value
      //This is the nused to build the floating point value
      union {
	struct {
	  uint16_t significand : 10;
	  uint16_t exponent    :  5;
	  uint16_t sign        :  1;
	} fp16;
	int16_t raw;} val;
      val.raw = ComputeValue();
      double floatingValue;
      switch (val.fp16.exponent){
      case 0:
	if (val.fp16.significand == 0){
	  floatingValue = 0.0;
	  if(val.fp16.sign){
	    floatingValue *= -1.0;
	  }
	}else{
	  floatingValue = pow(2,-14)*(val.fp16.significand/1024.0);
	  if(val.fp16.sign){
	    floatingValue *= -1.0;
	  }
	}
	break;
      case 31:
	if (val.fp16.significand == 0){
	  floatingValue = INFINITY;
	  if(val.fp16.sign){
	    floatingValue *= -1;
	  }
	}else{
	  floatingValue = NAN;
	}
	break;
      default:
	floatingValue = pow(2,val.fp16.exponent-15)*(1.0+(val.fp16.significand/1024.0));
	if(val.fp16.sign){
	  floatingValue *= -1.0;
	}
	break;
      }
      //only print e notation if very large or very small
      if((fabs(floatingValue) < 10000) ||
	 (fabs(floatingValue) > 0.001)
	 ){
	snprintf(buffer,bufferSize,
		 "%3.2f",floatingValue);	
      }else{
	snprintf(buffer,bufferSize,
		 "%3.2e",floatingValue);
      }
    }else if(iequals(format,std::string("IP"))){
      struct in_addr addr;
      addr.s_addr= in_addr_t(ComputeValue());
      snprintf(buffer,bufferSize,
	       "%s",inet_ntoa(addr));
      
    }else{
      //Normal numbers

      //hex formatting
      if(iequals(format,std::string("x")) && ComputeValue() >= 10){
	fmtString.assign("0x%");
	if(width >= 0){
	  width -= 2;
	}
      } else if (iequals(format,std::string("x")) && ComputeValue() < 10) {
	// get rid of the leading zeros, looks better
	fmtString.assign("%");
      }
      
      //if we are specifying the width, add a *
      if(width >= 0){
	fmtString.append("*");
      }

      //add the PRI stuff for our uint64_t
      if(iequals(format,std::string("x"))){
	fmtString.append(PRIX64);
      }else if(iequals(format,std::string("d"))){
	fmtString.append(PRId64);
      }else if(iequals(format,std::string("u"))){
	fmtString.append(PRIu64);
      }
 

      //Generatethe string
      if(width == -1){      
	snprintf(buffer,bufferSize,
		 fmtString.c_str(),ComputeValue());
      }else{
	snprintf(buffer,bufferSize,	       
		 fmtString.c_str(),width,ComputeValue());
      }
    }
    //return the string
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
