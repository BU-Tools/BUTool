#ifndef __STATUS_DISPLAY_MATRIX_HH__
#define __STATUS_DISPLAY_MATRIX_HH__

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
#include "StatusDisplayCell.hh"
#include "RegisterHelper/RegisterHelperIO.hh"

#define STATUS_DISPLAY_DEFAULT_FORMAT "X"
#define STATUS_DISPLAY_PARAMETER_PARSE_TOKEN '_'


namespace BUTool{

  enum StatusMode {
    TEXT,
    HTML,
    BAREHTML,
    LATEX,
    GRAPHITE
  };

  

  class StatusDisplayMatrix{
  public:
    StatusDisplayMatrix(){Clear();};
    ~StatusDisplayMatrix(){Clear();};
    void Add(std::string registerName, RegisterHelperIO* regIO);
    void Render(std::ostream & stream,int status,StatusMode statusMode = TEXT) const;
    std::vector<std::string> GetTableRows() const;
    std::vector<std::string> GetTableColumns() const;
    const StatusDisplayCell* GetCell(const std::string& row, const std::string& col) const;
 private:
    void Clear();
    
    std::string NameBuilder(std::string const & markup,
			    std::string const & registerName) const;
    std::string BuildNameWithSingleUnderscore(std::string const & markup,
          std::vector<std::string> const & parsedName) const;
    std::string BuildNameWithMultipleUnderscores(std::string const & markup,
          std::vector<std::string> const & parsedName) const;
    void CheckForInvalidCharacter(std::string const & name,
          char const & invalidChar) const;
    void CheckName(std::string const & newName);
    std::string ParseRowOrCol(RegisterHelperIO* regIO,
			 std::string const & registerName,
       std::string const & parameterName) const;

    std::vector<StatusDisplayCell*> row(std::string const &);
    std::vector<StatusDisplayCell*> col(std::string const &);


    void Print(std::ostream & stream,int status,bool force,int headerColWidth,
	       std::map<std::string,bool> & rowDisp,std::vector<int> & colWidth) const;
    void PrintHTML(std::ostream & stream,int status,bool force,int headerColWidth,
		   std::map<std::string,bool> & rowDisp,std::vector<int> & colWidth) const;
    void PrintLaTeX(std::ostream & stream) const;
    void PrintGraphite(std::ostream & stream,int status,bool force,int headerColWidth,
		       std::map<std::string,bool> & rowDisp,std::vector<int> & colWidth) const;


    std::string name;
    std::map<std::string,StatusDisplayCell*> cell;
    std::map<std::string,std::map<std::string,StatusDisplayCell *> > rowColMap;
    std::map<std::string,std::map<std::string,StatusDisplayCell *> > colRowMap;    
    mutable std::vector<std::string> rowName;
    mutable std::vector<std::string> colName;
  };
}
#endif
