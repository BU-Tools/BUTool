#ifndef __STATUS_DISPLAY_MATRIX_HH__
#define __STATUS_DISPLAY_MATRIX_HH__

#ifdef STD_UNORDERED_MAP
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
    void Add(std::string  address,uint32_t value, uint32_t value_mask, uMap const & parameters);
    void Render(std::ostream & stream,int status,StatusMode statusMode = TEXT) const;
    std::vector<std::string> GetTableRows() const;
    std::vector<std::string> GetTableColumns() const;
    const StatusDisplayCell* GetCell(const std::string& row, const std::string& col) const;
 private:
    void Clear();
    void CheckName(std::string const & newName);
    std::string ParseRow(uMap const & parameters,
			 std::string const & addressBase) const;
    std::string ParseCol(uMap const & parameters,
			 std::string const & addressBase) const;

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
