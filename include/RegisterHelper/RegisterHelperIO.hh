#ifndef __REGISTER_HELPER_IO_HH__
#define __REGISTER_HELPER_IO_HH__


#include <string>
#include <vector>
#include <stdint.h>


namespace BUTool{  
  class RegisterHelperIO {  
  public:
    enum RegisterNameCase {UPPER,LOWER,CASE_SENSITIVE};

    RegisterHelperIO()  {regCase = UPPER;}
    RegisterHelperIO(RegisterNameCase _regCase) {regCase = _regCase;}
    ~RegisterHelperIO() {}

    virtual std::vector<std::string> myMatchRegex(std::string regex)=0;
    
    //reads
    virtual uint32_t              ReadAddress      (uint32_t addr          )=0;
    virtual uint32_t              ReadRegister     (std::string const & reg)=0;
    virtual std::vector<uint32_t> ReadAddressFIFO  (uint32_t addr,          size_t count);
    virtual std::vector<uint32_t> ReadRegisterFIFO (std::string const & reg,size_t count);
    virtual std::vector<uint32_t> BlockReadAddress (uint32_t addr,          size_t count);
    virtual std::vector<uint32_t> BlockReadRegister(std::string const & reg,size_t count);
    virtual std::string           ReadString       (std::string const & reg);

    //writes
    virtual void WriteAddress      (uint32_t addr,           uint32_t data)=0;
    virtual void WriteRegister     (std::string const & reg, uint32_t data)=0;
    virtual void WriteAddressFIFO  (uint32_t addr,           std::vector<uint32_t> const & data);
    virtual void WriteRegisterFIFO (std::string const & reg, std::vector<uint32_t> const & data);
    virtual void BlockWriteAddress (uint32_t addr,           std::vector<uint32_t> const & data);
    virtual void BlockWriteRegister(std::string const & reg, std::vector<uint32_t> const & data);

    //action writes
    virtual void WriteAction(std::string const & reg)=0;

    virtual uint32_t    GetRegAddress(std::string const & reg)=0;
    virtual uint32_t    GetRegMask(std::string const & reg)=0;
    virtual uint32_t    GetRegSize(std::string const & reg)=0;
    virtual std::string GetRegMode(std::string const & reg)=0;
    virtual std::string GetRegPermissions(std::string const & reg)=0;
    virtual std::string GetRegDescription(std::string const & reg)=0;
    virtual std::string GetRegDebug(std::string const & reg);
    virtual std::string GetRegHelp(std::string const & reg);



    //Handle address table name case (default is upper case)
    RegisterNameCase GetCase(){return regCase;};
    void SetCase(RegisterNameCase _regCase){regCase = _regCase;};

    void ReCase(std::string & name);
  private:
    RegisterNameCase regCase;
  };

}
#endif
