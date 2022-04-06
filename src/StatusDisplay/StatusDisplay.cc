#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm> //std::max
#include <iomanip> //for std::setw
#include <ctype.h> // for isDigit()
#include <sstream> //string streamer

#include <StatusDisplay/StatusDisplay.hh>
#include <StatusDisplay/StatusDisplayMatrix.hh>
#include <StatusDisplay/StatusDisplayCell.hh>

#include <BUTool/ToolException.hh>

//For PRI macros
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

namespace BUTool{

  using boost::algorithm::iequals;

  //=============================================================================
  //===== Status Class
  //=============================================================================
  StatusDisplay::StatusDisplay(RegisterHelperIO * _regIO):
    regIO(_regIO){
    //store local copy of the svn version so we can reference it while debugging
    SetTitle("");
    AppendAuthor("Dan Gastler");
    version = -1;
    statusMode = TEXT;
    debug = false;
  }
  void StatusDisplay::AppendAuthor(std::string const & author){
    if(!authorList.size()){
      //list is empty
      authorList = author;
    }else{
      //prepend new name as they are the most recent person
      authorList= author + ", " + authorList;
    }
  } 

  std::string StatusDisplay::TableStrip(const std::string & s1){
    std::string strip(s1);
    //Look for an underscore
    size_t _pos = s1.find('_');
    if( (_pos != std::string::npos) &&  //found a '_'
	(_pos < (s1.size()-1) ) && // '_' isn't the last char
	(_pos > 0) ) {     // '_' isn't the first char
      //check if there are only numbers before the '_'
      bool isSortPrefix = true;
      for(int iChar = (int) _pos-1; (iChar >= 0) && isSortPrefix ; iChar--){	
	isSortPrefix = isdigit(s1[iChar]) && isSortPrefix; //check if digit is a number
      }
      if(isSortPrefix){
	strip = strip.substr(_pos+1,strip.size());
      }
    }
    return strip;
  }
  bool StatusDisplay::TableNameCompare(const std::string & s1, const std::string & s2){
    return iequals(TableStrip(s1),TableStrip(s2));
  }

  void StatusDisplay::ReportHeader(std::ostream & stream) const
  {
    if(statusMode == LATEX) {			
      stream << "\\documentclass[a4paper,10pt]{article}" << "\n";
      stream << "\\usepackage[margin=0.5in]{geometry}" << "\n";
      stream << "\\title{" << title << "Documentation}" << "\n";
      stream << "\\author{" << authorList << "}" << "\n"; 
      stream << "\\begin{document}" << "\n";
      stream << "\\maketitle" << "\n\n";
      stream << "\\section{Introduction}" << "\n";
      stream << "Introduction goes here";   
    }
    else if(statusMode == HTML){ 
      stream << "<!DOCTYPE html><html><head><style>\n";
    }
  }

  void StatusDisplay::ReportStyle(std::ostream & stream) const 
  {
    if (statusMode == HTML) {
      std::string head_color = "lightblue";
      std::string cell_color = "lightgreen";
      std::string err_color = "#FB412d"; //red
      std::string warn_color = "#FFFF00"; //yellow                          
         
      std::string null_color = "lightgrey";
      stream << "table { float: left; margin: 10px;}\n"; //Allows multiple tables on the same line with spacing
      stream << "th { font-size: smaller; background-color:" << head_color << ";}\n"; // Sets header font size and background color
      stream << "th.name {font-size: 20px; }\n";  // Increases the font size of the cell containing the name of the tables
      stream << "td { background-color:" << null_color << "; text-align: right;}\n"; // Sets the background color of null cells to grey
      stream << "td.nonerror { background-color:" << cell_color << ";}\n" ; // sets the background color of data cells
 
      stream << "td.warning { background-color:" << warn_color << ";}\n"; // sets the background color of error cells
      stream << "td.error { background-color:" << err_color << ";}\n"; // sets the background color of error cells 
      stream << "td.null { background-color:" << null_color << ";} </style></head><body>\n"; // sets the background color of null cells

    }
  }

  void StatusDisplay::ReportBody(size_t level, std::ostream & stream, std::string const & singleTable)
  {
    //Clear any entries
    tables.clear();
    
    //Call the function that processes the derived class address table
    this->Process(singleTable);
    printf("Process done\n");
    // Now output the content, looping over the tables
    // Eventually calls one of PrintLaTeX(), PrintHTML() or Print() for each table
    for(std::map<std::string,StatusDisplayMatrix>::iterator itTable = tables.begin();
	itTable != tables.end();
	itTable++){
      itTable->second.Render(stream,level,statusMode);
    }
    //Clean up tables for next time
    tables.clear();
  }

  void StatusDisplay::ReportTrailer(std::ostream & stream) const
  {
    // Now any post-amble
    if (statusMode == LATEX) {
      stream << "\\section{Version}" << "\n";
      stream << "Using SW version: " << version << ".\n";
      stream << "\\end{document}\n";
    } else if(statusMode == HTML){
      //Display the svn version for this release
      stream << "<table><tr><td>SW:</td><td>" << version << "</td></tr></table>\n";
      stream << "</body></html>\n";
    } else if (statusMode == TEXT) {
      stream << "SW VER: " << version << "\n";
    }  
  }

  std::string StatusDisplay::ReportHeader() const {
    std::stringstream str;
    ReportHeader(str);
    return str.str();
  }

  std::string StatusDisplay::ReportStyle() const {
    std::stringstream str;
    ReportStyle(str);
    return str.str();
  }

  std::string StatusDisplay::ReportBody(size_t level, std::string const & singleTable)  {
    std::stringstream str;
    ReportBody(level,str,singleTable);
    return str.str();
  }

  std::string StatusDisplay::ReportTrailer() const{
    std::stringstream str;
    ReportTrailer(str);
    return str.str();
  }
  
  void StatusDisplay::Report(size_t level,std::ostream & stream,std::string const & singleTable)
  {
    ReportHeader(stream);
    ReportStyle(stream);
    ReportBody(level,stream,singleTable);
    ReportTrailer(stream);
  }

  std::string StatusDisplay::ReportBare(size_t level,std::string const & singleTable) {
    std::stringstream str;
    StatusMode saveMode = statusMode;    
    statusMode = BAREHTML;
    Report( level, str, singleTable);
    statusMode = saveMode;
    return str.str();
  }

  const StatusDisplayMatrix* StatusDisplay::GetTable(const std::string & table) const {
    if (tables.find(table) == tables.end()) {
      BUException::BAD_VALUE e;
      char buffer[50];
      snprintf(buffer,49,"Table %s not found\n",table.c_str());
      e.Append(buffer);
      throw e;
    }
    return &tables.at(table);
  }

  std::vector<std::string> StatusDisplay::GetTableList() const {
    std::vector<std::string> tableList;
    for (std::map<std::string,StatusDisplayMatrix>::const_iterator it = tables.begin(); it != tables.end(); it++) {
      tableList.push_back(it->first);
    }
    return tableList;
  }

  std::vector<std::string> StatusDisplay::GetTableRows(std::string const & table) const {
    return GetTable(table)->GetTableRows();
  }

  std::vector<std::string> StatusDisplay::GetTableColumns(std::string const & table) const {
    return GetTable(table)->GetTableColumns();
  }

//  const StatusDisplayCell* StatusDisplay::GetStatusDisplayCell(const std::string& table, const std::string& row, const std::string& col) const {
//    return GetTable(table)->GetStatusDisplayCell(row,col);
//  }

  void StatusDisplay::SetOutputMode(StatusMode mode) { statusMode = mode;}
  /*! Get output mode */
  StatusMode StatusDisplay::GetOutputMode() const { return statusMode;}
  /*! Select "bare HTML" output mode */
  void StatusDisplay::SetBareHTML(){statusMode = BAREHTML;}
  /*! Unselect "bare HTML" output tmode */
  void StatusDisplay::UnsetBareHTML(){statusMode = TEXT;}
  /*! Select HTML output mode */
  void StatusDisplay::SetHTML(){statusMode = HTML;}
  /*! Unselect HTML output mode */
  void StatusDisplay::UnsetHTML(){statusMode = TEXT;}
  /*! Select LaTeX source output mode */
  void StatusDisplay::SetLaTeX() {statusMode = LATEX;}
  /*! Unselect LaTeX source output mode */
  void StatusDisplay::UnsetLaTeX() {statusMode = TEXT;}
  /*! Unselect GRAPHITE output mode */
  void StatusDisplay::SetGraphite(){statusMode = GRAPHITE;}
  /*! Unselect GRAPHITE output mode */
  void StatusDisplay::UnsetGraphite(){statusMode = TEXT;}
  

  
}
