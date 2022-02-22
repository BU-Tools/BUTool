#ifndef __REGISTER_HELPER_HH__
#define __REGISTER_HELPER_HH__

#include "../CommandReturn.hh"

#include <string>
#include <vector>
#include <stdint.h>
#include <iostream>

#include <BUTextIO/BUTextIO.hh>
#include <BUTextIO/PrintLevel.hh>

#include <BUException/ExceptionBase.hh>
#include <BUTool/ToolException.hh>

namespace BUTool{  
  class RegisterHelper {  
  protected:
    enum RegisterNameCase {UPPER,LOWER,CASE_SENSITIVE};

    // only let derived classes (device classes) use this BUTextIO functionality
    void AddStream(Level::level level, std::ostream*os);
    void SetupTextIO();

    RegisterHelper() : newTextIO(false) {regCase = UPPER;}
    RegisterHelper(RegisterNameCase _regCase) : newTextIO(false) {regCase = _regCase;}
    ~RegisterHelper() {if (newTextIO) {delete TextIO;}}

    virtual std::vector<std::string> myMatchRegex(std::string regex)=0;
    
    //reads
    virtual uint32_t              RegReadAddress(uint32_t addr)=0;
    virtual uint32_t              RegReadRegister(std::string const & reg)=0;
    //convert functions
    enum ConvertType {NONE=0,UINT=1,INT=2,FP=4,STRING=8};
    virtual ConvertType           RegReadConvertType(std::string const & reg)=0;
    virtual std::string           RegReadConvertFormat(std::string const & reg)=0; 
    
    // Named register read+conversion functions, overloaded depending on the conversion value type
    void    RegReadConvert(std::string const & reg, unsigned int & val);
    void    RegReadConvert(std::string const & reg, int & val);
    void    RegReadConvert(std::string const & reg, double & val);
    void    RegReadConvert(std::string const & reg, std::string & val);

    virtual std::vector<uint32_t> RegReadAddressFIFO(uint32_t addr,size_t count);
    virtual std::vector<uint32_t> RegReadRegisterFIFO(std::string const & reg,size_t count);
    virtual std::vector<uint32_t> RegBlockReadAddress(uint32_t addr,size_t count);
    virtual std::vector<uint32_t> RegBlockReadRegister(std::string const & reg,size_t count);
    virtual std::string           RegReadString(std::string const & reg);

    //writes
    virtual void RegWriteAddress(uint32_t addr,uint32_t data)=0;
    virtual void RegWriteRegister(std::string const & reg, uint32_t data)=0;
    virtual void RegWriteAddressFIFO(uint32_t addr,std::vector<uint32_t> const & data);
    virtual void RegWriteRegisterFIFO(std::string const & reg, std::vector<uint32_t> const & data);
    virtual void RegBlockWriteAddress(uint32_t addr,std::vector<uint32_t> const & data);
    virtual void RegBlockWriteRegister(std::string const & reg, std::vector<uint32_t> const & data);

    //action writes
    virtual void RegWriteAction(std::string const & reg)=0;

    virtual uint32_t    GetRegAddress(std::string const & reg)=0;
    virtual uint32_t    GetRegMask(std::string const & reg)=0;
    virtual uint32_t    GetRegSize(std::string const & reg)=0;
    virtual std::string GetRegMode(std::string const & reg)=0;
    virtual std::string GetRegPermissions(std::string const & reg)=0;
    virtual std::string GetRegDescription(std::string const & reg)=0;
    virtual std::string GetRegDebug(std::string const & /*reg*/){return "";}; 
    virtual std::string GetRegHelp(std::string const & /*reg*/){return "";};

    // Helper functions for converting
    double ConvertFloatingPoint16ToDouble(std::string const & reg);
    double ConvertLinear11ToDouble(std::string const & reg);
    double ConvertIntegerToDouble(std::string const & reg, std::string const & format);
    std::string ConvertEnumToString(std::string const & reg, std::string const & format);
    std::string ConvertIPAddressToString(std::string const & reg);

    //Handle address table name case (default is upper case)
    RegisterNameCase GetCase(){return regCase;};
    void SetCase(RegisterNameCase _regCase){regCase = _regCase;};

    void ReCase(std::string & name);

    CommandReturn::status Read(std::vector<std::string> strArg,std::vector<uint64_t> intArg);
    CommandReturn::status ReadConvert(std::vector<std::string> strArg,std::vector<uint64_t> intArg);
    CommandReturn::status ReadFIFO(std::vector<std::string> strArg,std::vector<uint64_t> intArg);
    CommandReturn::status ReadOffset(std::vector<std::string> strArg,std::vector<uint64_t> intArg);
    CommandReturn::status ReadString(std::vector<std::string> strArg,std::vector<uint64_t> intArg);    
    CommandReturn::status Write(std::vector<std::string> strArg,std::vector<uint64_t> intArg);
    CommandReturn::status WriteFIFO(std::vector<std::string> strArg,std::vector<uint64_t> intArg);
    CommandReturn::status WriteOffset(std::vector<std::string> strArg,std::vector<uint64_t> intArg);
    CommandReturn::status ListRegs(std::vector<std::string> strArg,std::vector<uint64_t> intArg);
    std::string RegisterAutoComplete(std::vector<std::string> const &,std::string const &,int);

    std::vector<std::string> RegNameRegexSearch(std::string regex);


  private:
    BUTextIO *TextIO;
    bool newTextIO;
    RegisterNameCase regCase;
    void PrintRegAddressRange(uint32_t startAddress,std::vector<uint32_t> const & data,bool printWord64 ,bool skipPrintZero);
    CommandReturn::status ReadWithOffsetHelper(uint32_t offset,std::vector<std::string> strArg,std::vector<uint64_t> intArg);
  };

}
#endif
