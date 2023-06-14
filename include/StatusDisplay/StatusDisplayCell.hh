#ifndef __STATUS_DISPLAY_CELL_HH__
#define __STATUS_DISPLAY_CELL_HH__

#if UHAL_VER_MAJOR >= 2 && UHAL_VER_MINOR >= 8
#include <unordered_map>
typedef std::unordered_map<std::string, std::string> uMap;
#else
#include <boost/unordered_map.hpp>
typedef boost::unordered_map<std::string, std::string> uMap;
#endif

#include <ostream>
#include <iostream> //for std::cout
#include <vector>
#include <string>
#include <map>
#include <boost/tokenizer.hpp> //for tokenizer
#include <cmath> // for pow

#include "RegisterHelper/RegisterHelperIO.hh"

#define STATUS_DISPLAY_DEFAULT_FORMAT "X"

namespace BUTool{

  /*! \brief One cell in a status display.
   *
   * One cell corresponds to one item from the AMC13 address table,
   * along with all the additional information required to display it.
   * see http://bucms.bu.edu/twiki/bin/view/BUCMSPublic/AMC13AddressTable.
   */
  class StatusDisplayCell{
  public:
    ///! Constructor
    StatusDisplayCell(){Clear();};
    ~StatusDisplayCell(){Clear();};
    ///! Fill in values for a cell
    void Setup(RegisterHelperIO * _regIO, // RegisterHelperIO instance to do reads 
        std::string const & _address,  /// address table name stripped of Hi/Lo
	      std::string const & _row,	 /// display row
	      std::string const & _col 	 /// display column
	      );

    ///! Determine if this cell should be displayed based on rule, statusLevel
    bool Display(int level,bool force = false) const;
    ///! Determine if this cell's row should be suppressed based on rule="nzr"
    bool SuppressRow( bool force) const;
    int DisplayLevel() const;
    std::string Print(int width,bool html = false, uint8_t displayFlag = HUMAN_READABLE) const;
    std::string const & GetRow() const;
    std::string const & GetCol() const;
    std::string const & GetDesc() const;
    std::string const & GetAddress() const;

    // Helper formatting functions for printing values
    void ReadAndFormatDouble   (char * buffer, int bufferSize, int width = -1) const;
    void ReadAndFormatInt      (char * buffer, int bufferSize, int width = -1) const;
    void ReadAndFormatUInt     (char * buffer, int bufferSize, int width = -1) const;

    void SetAddress(std::string const & _address);
    uint32_t const & GetMask() const;
    void SetMask(uint32_t const & _mask);

    void SetEnabled(bool d){enabled = d;}
    bool GetEnabled(){return enabled;}

  private:
    RegisterHelperIO * regIO;    
    void Clear();
    void CheckAndThrow(std::string const & name,
		       std::string & thing1,
		       std::string const & thing2) const;

    std::string address;
    std::string description;
    std::string row;
    std::string col;
    bool enabled;

    uint32_t word;
    uint32_t mask;
    
    std::string format;
    std::string displayRule;   
    RegisterHelperIO::ConvertType convertType;
    int statusLevel;
  };

}
#endif
