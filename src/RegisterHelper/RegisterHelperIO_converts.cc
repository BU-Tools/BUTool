#include <RegisterHelper/RegisterHelperIO.hh>
#include <map>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <math.h>

#include <boost/algorithm/string/predicate.hpp>

//For PRI macros
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using boost::algorithm::iequals;

#define DEFAULT_FORMAT "X"

std::string BUTool::RegisterHelperIO::GetConvertFormat(std::string const & reg){
  // From a given node address, retrieve the "Format" parameter of the node
  auto parameter = GetRegParameters(reg);
  std::string format(DEFAULT_FORMAT);
  auto formatVal = parameter.find("Format");
  if(formatVal != parameter.end()){
    format = formatVal->second;
  }
  return format;   
}

BUTool::RegisterHelperIO::ConvertType BUTool::RegisterHelperIO::GetConvertType(std::string const & reg){
  ConvertType ret = ConvertType::NONE;
  
  // get conversion type
  auto parameter = GetRegParameters(reg);
  auto formatVal = parameter.find("Format");
  //Search for Format
  if(formatVal != parameter.end()){
    std::string format = formatVal->second;
    if(!format.empty()) {
      // String data type
      if (( format[0] == 't') ||
          ( format[0] == 'T') ||
          ( iequals(format, "IP") )) 
      {
        ret = STRING;
      }
      // Floating point values
      else if ((format[0] == 'M') ||
          (format[0] == 'm') ||
          (format == "fp16")) {
        ret = FP;
      }
      // Signed integer data type
      else if ((format.size() == 1) &&
          (format[0] == 'd')) {
        ret = INT;
      }
      // Unsigned integer, can be in hex format as well if format='X' is specified
      else if ((format.size() == 1) &&
          ( (format[0] == 'u') || (iequals(format, "X")) ) ) {
        ret = UINT;
      }
    }
  }
  return ret;
}


double BUTool::RegisterHelperIO::ConvertFloatingPoint16ToDouble(uint64_t rawValue){
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
  val.raw = rawValue;

  switch (val.fp16.exponent) {
  // Case where the exponent is minimum
  case 0:
    if (val.fp16.significand == 0) {
      doubleVal = 0.0;
    }
    else {
      doubleVal = pow(2,-14)*(val.fp16.significand/1024.0); //NOLINT
    }

    // Apply sign
    if (val.fp16.sign) {
      doubleVal *= -1.0;
    }
    break;
  // Case where the exponent is maximum
  case 31: //NOLINT
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
    doubleVal = pow(2, val.fp16.exponent-15)*(1.0+(val.fp16.significand/1024.0)); //NOLINT
    if (val.fp16.sign) {
      doubleVal *= -1.0;
    }
    break;   
  }

  return doubleVal;
}

double BUTool::RegisterHelperIO::ConvertIntegerToDouble(uint64_t rawValue, std::string const & format){
  // Helper function to convert an integer to float using the following format:
  // y = (sign)*(M_n/M_d)*x + (sign)*(b_n/b_d)
  //       [0]   [1] [2]        [3]   [4] [5]
  // sign == 0 -> negative, other values mean positive

  std::vector<uint64_t> mathValues;
  size_t iFormat=1;
  const size_t requiredArgumentCount = 6;
  while (mathValues.size() != requiredArgumentCount &&
	 iFormat < format.size()) {
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
  double transformedValue = rawValue;
  transformedValue *= double(mathValues[1]);
  transformedValue /= double(mathValues[2]);
  // Apply the sign of m
  if (mathValues[0] == 0) {
    transformedValue *= -1;
  }
  
  double b = double(mathValues[4]) / double(mathValues[5]); //NOLINT
  if (mathValues[3] == 0) {
    b *= -1;
  }
  transformedValue += b;

  return transformedValue;
}

double BUTool::RegisterHelperIO::ConvertLinear11ToDouble(uint64_t rawValue){
  // Helper function to convert linear11 format to double

  union {
    struct {
      int16_t integer  : 11;
      int16_t exponent :  5;
    } linear11;
    int16_t raw;
  } val;
  
  val.raw = rawValue;
  double floatingValue = double(val.linear11.integer) * pow(2, val.linear11.exponent);
  
  return floatingValue;
}

std::string BUTool::RegisterHelperIO::ConvertIPAddressToString(uint64_t rawValue){
  // Helper function to convert IP addresses to string
  struct in_addr addr;
  addr.s_addr = in_addr_t(rawValue);
  return inet_ntoa(addr);
}

std::vector<std::string> BUTool::RegisterHelperIO::FindRegistersWithParameter(std::string const & parameterName, 
									      std::string const & parameterValue){
  // Helper function to get a list of register names with the given parameter value
  std::vector<std::string> registerNames;

  // All register names
  std::vector<std::string> allNames = GetRegsRegex("*");

  for (size_t idx=0; idx < allNames.size(); idx++) {
    auto params = GetRegParameters(allNames[idx]);
    auto pair = params.find(parameterName);
    if(pair != params.end()){      
      if (parameterValue == pair->second) {
	registerNames.push_back(allNames[idx]);
      }
    }
  }

  return registerNames;
}

std::string BUTool::RegisterHelperIO::ConvertEnumToString(uint64_t rawValue, std::string const & format){
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

  // Store the result in this C-style buffer
  // This function will return the content in this buffer
  // after converting it into a C++ string 
  const int bufferSize = 20;
  char buffer[bufferSize+1];
  memset(buffer,' ',bufferSize);
  buffer[bufferSize] = '\0';

  if (enumMap.find(rawValue) != enumMap.end()) {
    // If format starts with 't', just return the string value
    // Otherwise, return the value together with the number
    if (format[0] == 't') {
      snprintf(buffer,bufferSize,"%s",enumMap[rawValue].c_str());
    }
    else {
      snprintf(buffer,bufferSize,"%s (0x%" PRIX64 ")",enumMap[rawValue].c_str(),rawValue);
    }
  }
  
  // Could not find the value in enumeration map
  else {
    snprintf(buffer,bufferSize,"0x%" PRIX64 ")",rawValue);
  }

  return std::string(buffer);
}

std::string BUTool::RegisterHelperIO::ConvertHexNumberToString(uint64_t rawValue){
  // Helper function to convert uint64_t to string in hex format

  // Write the value into a buffer in hex-format,
  // return it as a C++ string
  const int bufferSize = 20;
  char buffer[bufferSize+1];
  buffer[bufferSize] = '\0';

  snprintf(buffer, bufferSize, "0x%" PRIX64, rawValue);

  return std::string(buffer);
}
