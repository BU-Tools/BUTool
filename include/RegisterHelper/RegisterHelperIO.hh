#ifndef __REGISTER_HELPER_IO_HH__
#define __REGISTER_HELPER_IO_HH__


#include <string>
#include <vector>
#include <stdint.h>
#include <unordered_map>

namespace BUTool{  
  class RegisterHelperIO {  
  public:
    enum RegisterNameCase {UPPER,LOWER,CASE_SENSITIVE};

    RegisterHelperIO()  {regCase = UPPER;}
    RegisterHelperIO(RegisterNameCase _regCase) {regCase = _regCase;}
    ~RegisterHelperIO() {}

    // Register search
    virtual std::vector<std::string> GetRegsRegex(std::string regex)=0;
    virtual std::vector<std::string> FindRegistersWithParameter(std::string const & parameterName,
								std::string const & parameterValue);

   
    //reads
    virtual uint32_t              ReadAddress      (uint32_t addr          )=0;
    virtual uint32_t              ReadRegister     (std::string const & reg)=0;
    virtual std::vector<uint32_t> ReadAddressFIFO  (uint32_t addr,          size_t count);
    virtual std::vector<uint32_t> ReadRegisterFIFO (std::string const & reg,size_t count);
    virtual std::vector<uint32_t> BlockReadAddress (uint32_t addr,          size_t count);
    virtual std::vector<uint32_t> BlockReadRegister(std::string const & reg,size_t count);
    virtual std::string           ReadString       (std::string const & reg);

    //convert functions
    enum ConvertType {NONE=0, UINT=1, INT=2, FP=4, STRING=8};
    ConvertType                   GetConvertType(std::string const & reg);
    std::string                   GetConvertFormat(std::string const & reg);
    // Named register read+conversion functions, overloaded depending on the conversion value type
    void                          ReadConvert(std::string const & reg, uint64_t & val);
    void                          ReadConvert(std::string const & reg, int & val);
    void                          ReadConvert(std::string const & reg, double & val);
    void                          ReadConvert(std::string const & reg, std::string & val);
    
    //writes
    virtual void WriteAddress      (uint32_t addr,           uint32_t data)=0;
    virtual void WriteRegister     (std::string const & reg, uint32_t data)=0;
    virtual void WriteAddressFIFO  (uint32_t addr,           std::vector<uint32_t> const & data);
    virtual void WriteRegisterFIFO (std::string const & reg, std::vector<uint32_t> const & data);
    virtual void BlockWriteAddress (uint32_t addr,           std::vector<uint32_t> const & data);
    virtual void BlockWriteRegister(std::string const & reg, std::vector<uint32_t> const & data);

    //action writes
    virtual void WriteAction(std::string const & reg)=0;

    //Other info
    virtual uint32_t    GetRegAddress(std::string const & reg)=0;
    virtual uint32_t    GetRegMask(std::string const & reg)=0;
    virtual uint32_t    GetRegSize(std::string const & reg)=0;
    virtual std::string GetRegMode(std::string const & reg)=0;
    virtual std::string GetRegPermissions(std::string const & reg)=0;
    virtual std::string GetRegDescription(std::string const & reg)=0;
    virtual std::string GetRegDebug(std::string const & reg);
    virtual std::string GetRegHelp(std::string const & reg);
    virtual std::unordered_map<std::string,std::string> const & GetRegParameters(std::string const & reg);
    virtual std::string GetRegParameterValue(std::string const & reg, std::string const & name);

    //Handle address table name case (default is upper case)
    RegisterNameCase GetCase(){return regCase;};
    void SetCase(RegisterNameCase _regCase){regCase = _regCase;};

    void ReCase(std::string & name);
  protected:
    // Helper functions for converting
    double      ConvertFloatingPoint16ToDouble(std::string const & reg);
    double      ConvertLinear11ToDouble(std::string const & reg);
    double      ConvertIntegerToDouble(std::string const & reg, std::string const & format);
    std::string ConvertEnumToString(std::string const & reg, std::string const & format);
    std::string ConvertIPAddressToString(std::string const & reg);
    std::string ConvertHexNumberToString(std::string const & reg);

  private:
    RegisterNameCase regCase;
  };

}
#endif
