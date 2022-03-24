#ifndef __REGISTER_HELPER_HH__
#define __REGISTER_HELPER_HH__

#include <BUTool/CommandReturn.hh>

#include <string>
#include <vector>
#include <stdint.h>
#include <iostream>

#include <BUTextIO/BUTextIO.hh>
#include <BUTextIO/PrintLevel.hh>

#include <RegisterHelper/RegisterHelperIO.hh>

namespace BUTool{  
  class RegisterHelper {      
  protected:
    // only let derived classes (device classes) use this BUTextIO functionality
    void AddStream(Level::level level, std::ostream*os);
    void SetupTextIO();

    RegisterHelper(std::shared_ptr<RegisterHelperIO> _regIO) :
      regIO(_regIO), newTextIO(false) {}
    ~RegisterHelper() {if (newTextIO) {delete TextIO;}}

    CommandReturn::status Read(std::vector<std::string> strArg,std::vector<uint64_t> intArg);
    CommandReturn::status ReadFIFO(std::vector<std::string> strArg,std::vector<uint64_t> intArg);
    CommandReturn::status ReadOffset(std::vector<std::string> strArg,std::vector<uint64_t> intArg);
    CommandReturn::status ReadString(std::vector<std::string> strArg,std::vector<uint64_t> intArg);    
    CommandReturn::status Write(std::vector<std::string> strArg,std::vector<uint64_t> intArg);
    CommandReturn::status WriteFIFO(std::vector<std::string> strArg,std::vector<uint64_t> intArg);
    CommandReturn::status WriteOffset(std::vector<std::string> strArg,std::vector<uint64_t> intArg);
    CommandReturn::status ListRegs(std::vector<std::string> strArg,std::vector<uint64_t> intArg);
    std::string RegisterAutoComplete(std::vector<std::string> const &,std::string const &,int);

    std::vector<std::string> RegNameRegexSearch(std::string regex);

  protected:
    std::shared_ptr<RegisterHelperIO> regIO;
    BUTextIO *TextIO;
    bool newTextIO;
  private:   
    void PrintRegAddressRange(uint32_t startAddress,std::vector<uint32_t> const & data,bool printWord64 ,bool skipPrintZero);
    CommandReturn::status ReadWithOffsetHelper(uint32_t offset,std::vector<std::string> strArg,std::vector<uint64_t> intArg);
  };

}
#endif