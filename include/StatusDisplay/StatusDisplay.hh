#ifndef __STATUS_DISPLAY_HH__
#define __STATUS_DISPLAY_HH__

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
#include <boost/tokenizer.hpp> //for tokenize

#include "StatusDisplayMatrix.hh"
#include "StatusDisplayCell.hh"
#include <RegisterHelper/RegisterHelperIO.hh>

#include "BUException/ExceptionBase.hh"

namespace BUTool{


  
  class StatusDisplay{
  public:
    StatusDisplay(RegisterHelperIO * _regIO);
    virtual ~StatusDisplay(){Clear();};

    void Clear(){tables.clear();};
    void ReportHeader(std::ostream & stream) const;
    std::string ReportHeader() const;
    void ReportStyle(std::ostream & stream) const;
    std::string ReportStyle() const;
    void ReportBody(size_t level,std::ostream & stream, std::string const & singleTable = std::string(""));
    std::string ReportBody(size_t level, std::string const & singleTable = std::string(""));
    void ReportTrailer(std::ostream & stream) const;
    std::string ReportTrailer() const;
    void ReportExceptions(std::ostream & stream) const;
    std::string ReportExceptions() const;

    /*! \brief Emit a status report to output stream
     *
     * Generate a status report using extra information in the XML address
     * table and output to stream.  By default the format is plain text, best
     * rendered with a fixed-width font.  Other options currently available are
     * HTML (with styling), bare HTML (no header or styling) and LaTeX source.
     *
     * \param level Verbosity level, from 1 to 9
     * \param stream Output stream
     * \param singleTable Optional name of one HTML table to emit (see address table for table names)
     */
    void Report(size_t level,
		std::ostream & stream=std::cout,
		std::string const & singleTable = std::string(""));
    /*! \brief Emit a bare HTML report for one HTML table
     *
     * Wrapper function which provides a convenient interface to
     * emit bare HTML for a single status table.  This is suited to
     * embedding the display in a parent HTML page
     *
     * \param level Verbosity level, from 1 to 9
     * \param stream Output stream
     * \param singleTable name of one HTML table to emit (see address table for table names)
     */
    std::string ReportBare( size_t level, std::string const & singleTable = std::string(""));
    /*! Select output mode */
    void SetOutputMode(StatusMode mode);
    /*! Get output mode */
    StatusMode GetOutputMode() const ;
    /*! Select "bare HTML" output mode */
    void SetBareHTML();
    /*! Unselect "bare HTML" output tmode */
    void UnsetBareHTML();
    /*! Select HTML output mode */
    void SetHTML();
    /*! Unselect HTML output mode */
    void UnsetHTML();
    /*! Select LaTeX source output mode */
    void SetLaTeX();
    /*! Unselect LaTeX source output mode */
    void UnsetLaTeX();
    /*! Select Graphite source output mode */
    void SetGraphite();
    /*! Unselect Graphite source output mode */
    void UnsetGraphite();
    /*! Get a const pointer to a StatusDisplayMatrix item */
    const StatusDisplayMatrix* GetTable(const std::string& table) const;
    /*! Get a list of tables */
    std::vector<std::string> GetTableList() const;
    /*! Get a list of Rows in a given table */
    std::vector<std::string> GetTableRows(std::string const & table) const;
    /*! Get a list of Columns in a givenTable */
    std::vector<std::string> GetTableColumns(std::string const & table) const;
//    /*! Get a const pointer to the cell in the given position */
//    const StatusDisplayCell* GetStatusDisplayCell(const std::string & table, const std::string & row, const std::string & col) const; 
    std::string TableStrip(const std::string & s1);
    bool TableNameCompare(const std::string & s1, const std::string & s2);
  protected:
    RegisterHelperIO * regIO;
    void AppendAuthor(std::string const & author);
    void SetTitle(std::string const & _title){title=_title;};
    void Process(std::string const & singleTable);
    std::map<std::string,StatusDisplayMatrix> tables;
    void SetVersion(int ver){version = ver;};
  private:
    StatusDisplay(); //do not implement
    std::string title;
    std::string authorList;
    bool debug;
    StatusMode statusMode;
    int version;
    // Array to store the caught exceptions during status display
    std::map<std::string, std::vector<std::pair<std::string, std::string>> > caughtExceptions;
  };
}
#endif
