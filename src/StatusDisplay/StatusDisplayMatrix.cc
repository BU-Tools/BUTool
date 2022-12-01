#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm> //std::max
#include <iomanip> //for std::setw
#include <ctype.h> // for isDigit()

//For PRI macros
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <StatusDisplay/StatusDisplayMatrix.hh>
#include <StatusDisplay/StatusDisplayCell.hh>

#include <BUTool/ToolException.hh>

#include <string>

namespace BUTool{

  using boost::algorithm::iequals;

  //=============================================================================
  //===== StatusDisplayMatrix Class
  //=============================================================================

  void StatusDisplayMatrix::Clear()
  {
    //Clear the name
    name.clear();
    //Clear the maps
    rowColMap.clear();
    colRowMap.clear();
    rowName.clear();
    colName.clear();
    //Deallocate the data
    for(std::map<std::string,StatusDisplayCell*>::iterator itCell = cell.begin();
	itCell != cell.end();
	itCell++){
      if(itCell->second != NULL){
	delete itCell->second;
	itCell->second = NULL;
      }
    }
    //clear the vector
    cell.clear();
    rowColMap.clear();
    colRowMap.clear();
    rowName.clear();
    colName.clear();
  }

  std::vector<std::string> StatusDisplayMatrix::GetTableRows() const {
    std::vector<std::string> rows;
    for (std::map<std::string,std::map<std::string, StatusDisplayCell *> >::const_iterator it = rowColMap.begin(); 
	 it != rowColMap.end(); it++) {
      rows.push_back(it->first);
    }
    return rows;
  }

  std::vector<std::string> StatusDisplayMatrix::GetTableColumns() const {
    std::vector<std::string> cols;
    for (std::map<std::string,std::map<std::string, StatusDisplayCell *> >::const_iterator it = colRowMap.begin();
	 it != colRowMap.end();it++) {
      cols.push_back(it->first);
    }
    return cols;
  }

  const StatusDisplayCell* StatusDisplayMatrix::GetCell(const std::string & row, const std::string & col) const {
    if (rowColMap.find(row) == rowColMap.end() || colRowMap.find(col) == colRowMap.end()) {
      BUException::BAD_VALUE e;
      
      e.Append("No cell in (\"");
      e.Append(row.c_str());
      e.Append("\",\"");
      e.Append(col.c_str());
      e.Append("\") position\n");
      throw e;
    }
    return rowColMap.at(row).at(col);
  }

  void StatusDisplayMatrix::Add(std::string registerName, RegisterHelperIO* regIO)
  // Add the register with registerName into a StatusDisplayMatrix table instance.
  {
    // Find the table name for this register. If we can't find one, throw an exception
    std::string tableName = regIO->GetRegParameterValue(registerName, "Table");
    
    CheckName(NameBuilder(tableName,registerName,"Table"));

    // Determine row and column
    std::string row = ParseRowOrCol(regIO,registerName,"Row");
    std::string col = ParseRowOrCol(regIO,registerName,"Column");

    // Ignore registers with "_HI", as the RegisterHelperIO class handles
    // the merging of _LO and _HI registers into a single value.
    if (registerName.find("_HI") == (registerName.size()-3)) {
      return;
    }

    // Create a new StatusDisplayCell instance for this entry and add it to the table
    StatusDisplayCell * ptrCell = new StatusDisplayCell;
    cell[registerName] = ptrCell;

    // Setup the StatusDisplayCell instance for this register
    ptrCell->Setup(regIO,registerName,row,col);

    // Setup should have thrown if something bad happened, so we are safe to update the search maps
    rowColMap[row][col] = ptrCell;
    colRowMap[col][row] = ptrCell;
  }

  void StatusDisplayMatrix::CheckName(std::string const & newTableName)
  {
    //Check that this name isn't empty
    if(newTableName.empty()){
      BUException::BAD_VALUE e;
      std::string error("Bad table name \""); error+=newTableName ;error+="\"\n";
      e.Append(error);
      throw e;
    }

    //Strip the leading digits off of the table name
    int loc = newTableName.find("_");
    bool shouldStrip = true;
    for (int i = 0; i < loc; i++) {
      if (!isdigit(newTableName[i])) {
        shouldStrip = false;
        break;
      }
    }
    std::string modName;
    if (shouldStrip) {
      modName = newTableName.substr(loc+1);
    }
    else {
      modName = newTableName;
    }
    
    //Setup table name if not yet set
    if(name.empty()){
      name = modName;
    }else if(!iequals(modName,name)){
      BUException::BAD_VALUE e;
      std::string error("Tried adding entry of table \"");
      error += modName + " to table " + name;
      e.Append(error.c_str());
      throw e;
    }
  }

  void StatusDisplayMatrix::CheckForInvalidCharacter(std::string const & name, char const & invalidChar) const{
    /*
    Function to check if there is an invalid character in the given string
    Throws a BUException if there is one.
    */
    for (size_t iChar = 0; iChar < name.size(); iChar++) {
      if (name[iChar] == invalidChar) {
        BUException::BAD_MARKUP_NAME e;	    
        std::string error("Spaces are not allowed in markup ");
        error += name;
        e.Append(error.c_str());
        throw e;
      } 
    }
  }

  std::string StatusDisplayMatrix::BuildNameWithSingleUnderscore(std::string const & markup,
                  std::vector<std::string> const & parsedName,
                  std::string const & parameterName) const{
    /* 
    Build a row or column name, using a single underscore ('_') character as a special token.
    A double underscore ('__') is treated as a literal underscore, and printed as '_'.
    
    If a non-digit char is following the underscore, that single underscore char will also be treated as literal.

    Example: If we had a register with the name A.B.C.D, the following would be true:
    - If "Row=_3" is set, the row name would be "C"
    - If "Row=_3_4" is set, the row name would be "C D"
    - If "Row=ROW__1" is set, the row name would be "ROW_1"
    - If "Row=ROW_A" is set, the row name would be "ROW_A"
    */

    // The name we're going to return
    std::string result;

    // Spaces in markup are not allowed
    CheckForInvalidCharacter(markup, ' ');
    
    for(size_t iChar = 0; iChar < markup.size(); iChar++){
      /*
       * Check if markup[iChar] is a special parse character ('_')
       * If it is, we'll check the character next to it. There are three possibilities:
       *   1. It is an integer -> That will point to the portion of the register name we want to use
       *   2. It is another underscore -> The two underscores will be treated as a literal, single underscore 
       *   3. It is a non-digit char -> The underscore will be treated as a literal, single underscore
       * 
       * The function will throw a BAD_MARKUP_NAME exception, if the integer specified after the underscore
       * does not point to a valid location in the parsed register name.
       */
      if ((markup[iChar] == STATUS_DISPLAY_PARAMETER_PARSE_TOKEN)) {
        // Check the next character, make sure that it is within range
        // This essentially means that the last character in a valid markup CANNOT be an underscore.
        if (!(iChar + 1 < markup.size())) {
          BUException::BAD_MARKUP_NAME e;	    
          std::string error("Bad " + parameterName + " name for ");
          error += parsedName[0] + " (" + markup + ")";
          error += " -> Last character cannot be an underscore!"; 
          e.Append(error.c_str());
          throw e;
        }

        // Check if the next character is an integer
        if (isdigit(markup[iChar+1])) {
          std::string positionStr;
          positionStr.push_back(markup[iChar+1]);
          size_t position = std::stoi(positionStr);
         
          // Add whitespace if we need it
          if (!result.empty()) {
            result += " ";
          }

          // Position out of bounds of the parsed name size
          // In a valid markup name, this should never happen, throw BAD_MARKUP_NAME
          if (!(position < parsedName.size())) {
            BUException::BAD_MARKUP_NAME e;	    
            std::string error("Bad " + parameterName + " name for ");
            error += parsedName[0] + " (" + markup + ")";
            error += " -> Position " + positionStr + " is out of bounds!"; 
            e.Append(error.c_str());
            throw e;
          }
          result.append(parsedName[position]);
          // Do not process the next character again
          iChar++;
        }

        // Check if the next character is another underscore
        else if (markup[iChar+1] == STATUS_DISPLAY_PARAMETER_PARSE_TOKEN) {
          result.push_back(STATUS_DISPLAY_PARAMETER_PARSE_TOKEN);
          // Do not process the next character again
          iChar++;
        }

        // Any other possibility means that the next character is a non-digit char
        // In this case, treat this underscore as a literal and move on to the next char
        else {
          result.push_back(markup[iChar]);
        }

      }
      // Normal character, just add to the resulting name
      else {
        result.push_back(markup[iChar]);
      }
    }
    return result;
  }

  std::string StatusDisplayMatrix::BuildNameWithMultipleUnderscores(std::string const & markup,
              std::vector<std::string> const & parsedName,
              std::string const & parameterName) const {
    /* 
    Build a row or column name, using multiple underscore ('_') characters as a special token
    Determines the row and column name according to the following:
    
    - Single underscore: Treated as a literal underscore
    - Double underscore ("__N"): Get the Nth portion of the register name (count from LEFT)
    - Triple underscore ("___N"): Get the Nth portion of the register name (count from RIGHT)
    */
    std::string ret;
    
    // Spaces are not allowed
    CheckForInvalidCharacter(markup, ' ');
    
    size_t iChar = 0;
    for(;iChar < markup.size();iChar++){
      //look for the next special char (which is a run of two, a run of three is a reverse token)
      if((markup[iChar] == STATUS_DISPLAY_PARAMETER_PARSE_TOKEN) && (iChar+1 < markup.size())) {
        // Is the next character a special token?
        if(markup[iChar+1] != STATUS_DISPLAY_PARAMETER_PARSE_TOKEN){
          // Just append to the return value and continue the for loop
          // as there are no more special characters
          ret.push_back(markup[iChar]);	    	    
          continue;
        }
        // We have a second special character
        else {
          iChar++;
        }

        // This boolean specifies whether we want to count in reverse or not
        // The value is True if three adjacent special characters ('_') are specified in markup
        bool reverseToken = false;
        // Start parsing a special
        iChar++;
        if(iChar < markup.size()){
          // Check for third special character
          if(markup[iChar] == STATUS_DISPLAY_PARAMETER_PARSE_TOKEN){
            // Three special characters in a row, set reverseToken = true
            reverseToken = true;
            iChar++;
          }
          // Find number
          std::string position;
          for(;iChar < markup.size();iChar++){
            // A valid markup will have a digit after the special characters
            // Check if this is the case, otherwise we'll raise an exception later
            if(!isdigit(markup[iChar])){
              iChar--;
              break;
            } 
            else { position.push_back(markup[iChar]); }
          }
          // If we found a valid position, figure out the name from the register name
          if(!position.empty()){
            int pos = std::stoi(position);
            // Recompute the position if this is a reverse token.
            if(reverseToken){
              pos = parsedName.size() - pos;
            }
            if(pos > int(parsedName.size())){
              BUException::BAD_VALUE e;	    
              std::string error("Bad " + parameterName + " name for ");
              error += parsedName[0] + " with token " + std::to_string(pos) + " from markup " + markup;
              e.Append(error.c_str());
              throw e;
            }
            if(!ret.empty()){
              // Add whitespace if we need it
              ret += " ";
            }
            ret.append(parsedName[pos]);
          }else{
            BUException::BAD_VALUE e;	    
            std::string error("Bad " + parameterName + " name for ");
            error += parsedName[0] + " from markup " + markup;
            e.Append(error.c_str());
            throw e;
          }
        }
      }
      else {
        // No special character, so just copy the string
        ret.push_back(markup[iChar]);
      }
    }
    return ret;
  }

  std::string StatusDisplayMatrix::NameBuilder(std::string const & markup,
                  std::string const & registerName,
                  std::string const & parameterName) const{
    /*
    Main name builder function to determine row and column names for StatusDisplayMatrix tables.
    Row and column names are inferred from the "Row" and "Column" parameters in the address table.

    If SD_USE_NEW_PARSER is defined via a compile-time flag, this function will call the more recent
    version of the name parser. Otherwise, it will call the older version of the name parser, where only
    a single underscore ('_') in the beginning is treated as a special character.
    */

    // Create a tokenized register name, that is, the register name split by '.' character
    boost::tokenizer<boost::char_separator<char> > tokenizedName(registerName,
				boost::char_separator<char>("."));
    
    // parsedName vector will hold the full register name at index 0
    // and the ith component of the tokenized name at index i
    std::vector<std::string> parsedName;
    parsedName.push_back(registerName); // for _0
    for(auto itTok = tokenizedName.begin(); itTok!=tokenizedName.end(); itTok++) {
      parsedName.push_back(*itTok); // for _N
    }
    
    #ifdef SD_USE_NEW_PARSER
    return BuildNameWithMultipleUnderscores(markup, parsedName, parameterName);
    #endif

    return BuildNameWithSingleUnderscore(markup, parsedName, parameterName);
  }

  std::string StatusDisplayMatrix::ParseRowOrCol(RegisterHelperIO* regIO,
              std::string const & registerName,
              std::string const & parameterName) const
  {
    /*
    Function that returns the row (parameterName="row") or column name
    (parameterName="column") for a given named register.
    */
    std::string markup;
    try {
      markup = regIO->GetRegParameterValue(registerName, parameterName);
    } catch (BUException::BAD_VALUE & e) {
      // Missing row or column, append information to the error message
      std::string error("Missing ");
      error += parameterName;
      error += " for ";
      error += registerName;
      e.Append(error.c_str());
      throw e; 
    }

    boost::to_upper(markup);
    // Determine the row or column name from the markup and the register name
    std::string newName = NameBuilder(markup, registerName, parameterName);

    // If there is a "_LO" or "_HI" in the row/column name, drop it
    if (newName.find("_LO") == newName.size()-3) {
      newName = newName.substr(0, newName.find("_LO"));
    }
    else if (newName.find("_HI") == newName.size()-3) {
      newName = newName.substr(0, newName.find("_HI"));
    }

    return newName;
  }

  // render one table
  void StatusDisplayMatrix::Render(std::ostream & stream,int status,StatusMode statusMode) const
  {
    bool forceDisplay = (status >= 99); //NOLINT

    //==========================================================================
    //Rebuild our col/row map since we added something new
    //==========================================================================
    std::map<std::string,std::map<std::string,StatusDisplayCell *> >::const_iterator it;
    rowName.clear();
    for(it = rowColMap.begin();it != rowColMap.end();it++){      
      rowName.push_back(it->first);
    }
    colName.clear();
    for(it = colRowMap.begin();it != colRowMap.end();it++){
      colName.push_back(it->first);
    }	    


    //==========================================================================
    //Determine the widths of each column, decide if a column should be printed,
    // and decide if a row should be printed
    //==========================================================================
    std::vector<int> colWidth(colName.size(),-1);
    bool printTable = false;
    // map entry defined to display this row, passed on to print functions
    std::map<std::string,bool> rowDisp; 
    // map entry "true" to kill this row, used only locally
    std::map<std::string,bool> rowKill;
    
    for(size_t iCol = 0;iCol < colName.size();iCol++){
      //Get a vector for this row
      const std::map<std::string,StatusDisplayCell*> & inColumn = colRowMap.at(colName[iCol]);
      for(std::map<std::string,StatusDisplayCell*>::const_iterator itColCell = inColumn.begin();
	  itColCell != inColumn.end();
	  itColCell++){
	//Check if we should display this cell
	if(itColCell->second->Display(status,forceDisplay)){
	  //This entry will be printed, 
	  //update the table,row, and column display variables
	  printTable = true;
	  // check if this row is already marked to be suppressed
	  if( rowKill[itColCell->first]) {
	    // yes, delete entry in rowDisp if any
	    rowDisp.erase(itColCell->first);
	  } else {
	    // nope, mark it for display
	    rowDisp[itColCell->first] = true;
	  }
	  // deal with the width
	  int width = itColCell->second->Print(-1).size();
	  if(width > colWidth[iCol]){
	    colWidth[iCol]=width;
	  }
	}
	// now check if this cell should cause the row to be suppressed
	if( itColCell->second->SuppressRow( forceDisplay) || rowKill[itColCell->first] ) {
	  rowKill[itColCell->first] = true;
	  rowDisp.erase(itColCell->first);
	}
      }
    }

    if(!printTable){
      return;
    }
    
    //Determine the width of the row header
    int headerColWidth = 16; //NOLINT
    if(name.size() > size_t(headerColWidth)){
      headerColWidth = name.size();
    } 
    for(std::map<std::string,bool>::iterator itRowName = rowDisp.begin();
	itRowName != rowDisp.end();
	itRowName++){
      if(itRowName->first.size() > size_t(headerColWidth)){
	headerColWidth = itRowName->first.size();
      }
    }

        
    //Print out the data
    if (statusMode == LATEX) {
      PrintLaTeX(stream);
    }else if(statusMode == HTML || statusMode == BAREHTML){
      PrintHTML(stream,status,forceDisplay,headerColWidth,rowDisp,colWidth);
    }else if (statusMode == GRAPHITE){
      PrintGraphite(stream,status,forceDisplay,headerColWidth,rowDisp,colWidth);
    }else{
      Print(stream,status,forceDisplay,headerColWidth,rowDisp,colWidth);
    }
  }
  void StatusDisplayMatrix::Print(std::ostream & stream,
			       int status,
			       bool forceDisplay,
			       int headerColWidth,
			       std::map<std::string,bool> & rowDisp,
			       std::vector<int> & colWidth) const
  {
    //=====================
    //Print the header
    //=====================
    //Print the rowName
    stream << std::right << std::setw(headerColWidth+1) << name << "|";	
    for(size_t iCol = 0; iCol < colWidth.size();iCol++){
      if(colWidth[iCol] > 0){
	size_t columnPrintWidth = std::max(colWidth[iCol],int(colName[iCol].size()));
	stream<< std::right  
	      << std::setw(columnPrintWidth+1) 
	      << colName[iCol] << "|";
      }	  
    }
    //Draw line
    stream << std::endl << std::right << std::setw(headerColWidth+2) << "--|" << std::setfill('-');
    for(size_t iCol = 0; iCol < colWidth.size();iCol++){
      if(colWidth[iCol] > 0){
	size_t columnPrintWidth = std::max(colWidth[iCol],int(colName[iCol].size()));
	stream<< std::right  
	      << std::setw(columnPrintWidth+2) 
	      << "|";
      }	  
    }
    stream << std::setfill(' ') << std::endl;
    
    //=====================
    //Print the data
    //=====================
    for(std::map<std::string,bool>::iterator itRow = rowDisp.begin();
	itRow != rowDisp.end();
	itRow++){
      //Print the rowName
      stream << std::right << std::setw(headerColWidth+1) << itRow->first << "|";
      //Print the data
      const std::map<std::string,StatusDisplayCell*> & colMap = rowColMap.at(itRow->first);
      for(size_t iCol = 0; iCol < colName.size();iCol++){	  
	if(colWidth[iCol] > 0){
	  size_t width = std::max(colWidth[iCol],int(colName[iCol].size()));
	  std::map<std::string,StatusDisplayCell*>::const_iterator itMap = colMap.find(colName[iCol]);
	  if((itMap != colMap.end()) &&
	     (itMap->second->Display(status,forceDisplay))){
	    stream << std::right  
		   << std::setw(width+1)
		   << itMap->second->Print(colWidth[iCol]) << "|";	   
	  }else{
	    stream << std::right 
		   << std::setw(width+2) 
		   << " |";
	  }
	}
      }
      stream << std::endl;
    }

    //=====================
    //Print the trailer
    //=====================
    stream << std::endl;
  }
  void StatusDisplayMatrix::PrintHTML(std::ostream & stream,
				      int status,
				      bool forceDisplay,
				      int /*headerColWidth*/,
				      std::map<std::string,bool> & rowDisp,
				      std::vector<int> & colWidth) const
  {
    //=====================
    //Print the header
    //=====================
    //Print the rowName
    stream << "<table border=\"1\" >" << "<tr>" << "<th class=\"name\">" << name << "</th>";    
    for(size_t iCol = 0; iCol < colWidth.size();iCol++){
      if(colWidth[iCol] > 0){
	stream << "<th>" << colName[iCol] << "</th>";
      }	  
    }
    stream << "</tr>\n";   	
    //=====================
    //Print the data
    //=====================
    for(std::map<std::string,bool>::iterator itRow = rowDisp.begin();
	itRow != rowDisp.end();
	itRow++){
      stream << "<tr><th>" << itRow->first << "</th>";
      //Print the data
      const std::map<std::string,StatusDisplayCell*> & colMap = rowColMap.at(itRow->first);
      for(size_t iCol = 0; iCol < colName.size();iCol++){	  
	if(colWidth[iCol] > 0){
	  std::map<std::string,StatusDisplayCell*>::const_iterator itMap = colMap.find(colName[iCol]);
	  if(itMap != colMap.end()){

	    //sets the class for the td element for determining its color
	    std::string tdClass;
	    if((itMap->second->GetDesc().find("error") != std::string::npos)){
	      tdClass = "error";
	    }else if((itMap->second->GetDesc().find("warning") != std::string::npos)){
	      tdClass = "warning" ;
	    }else{
	      tdClass = "nonerror";
	    }
	    tdClass = (itMap->second->Print(colWidth[iCol],true) == "0") ? "null" : tdClass; 

	    if(itMap->second->Display(status,forceDisplay)){
	      stream << "<td title=\"" << itMap->second->GetDesc()  << "\" class=\"" << tdClass << "\">" 
		     << itMap->second->Print(colWidth[iCol],true) << "</td>";
	    }else{
	      //	      stream << "<td title=\"" << itMap->second->GetDesc()  << "\" class=\"" << tdClass << "\">" << " " << "</td>";
	      stream << "<td title=\"" << itMap->second->GetDesc()  << "\">" << " " << "</td>";
	    }
	  }else{
	    stream << "<td>" << " " << "</td>";
	  }
	}
      }
      stream << "</tr>\n";
    }
    //=====================
    //Print the trailer
    //=====================
    stream << "</table>\n";
  }

  void StatusDisplayMatrix::PrintLaTeX(std::ostream & stream)  const {
    //=====================
    //Print the header
    //=====================
    std::string headerSuffix;
    std::string cols;
    std::vector<std::string> modColName;

	    
    //Remove underscores from column names and create the suffix for the header
    for (std::vector<std::string>::iterator it = modColName.begin(); it != modColName.end(); ++it) {
      headerSuffix = headerSuffix + "|l";
      if (it->find("_") != std::string::npos) {
	cols = cols +" & " + (*it).replace(it->find("_"),1," ");
      } else {
	cols = cols + " & " + *it;
      }
    }

    //Strip underscores off table name
    std::string strippedName = name;
    while (strippedName.find("_") != std::string::npos) {
      strippedName = strippedName.replace(strippedName.find("_"),1," ");
    }


    headerSuffix = headerSuffix + "|l|";
    stream << "\\section{" << strippedName << "}\n";
    stream << "\\begin{center}\n";
    stream << "\\begin{tabular}" <<"{" << headerSuffix << "}" <<"\n";
    stream << "\\hline\n";
    
    //=========================================================================
    //Print the first row, which contains the table name and the column names
    //=========================================================================
    stream << strippedName + cols + " \\\\\n";
    stream << "\\hline\n";

    //========================
    //Print subsequent rows
    //========================
    for (std::vector<std::string>::const_iterator itRow = rowName.begin(); itRow != rowName.end(); itRow++) {
      std::string thisRow = *itRow; 
      size_t const  s_maskSize = 10;
      char s_mask[s_maskSize+1];

      while (thisRow.find("_") != std::string::npos) {
	thisRow = thisRow.replace(thisRow.find("_"),1," ");
      }

      for(std::vector<std::string>::iterator itCol = modColName.begin(); itCol != modColName.end(); itCol++) {
	std::string addr;

	//	if (true) { //WTF? why is this here?
	  if (rowColMap.at(*itRow).find(*itCol) == rowColMap.at(*itRow).end()) {
	    addr = " ";
	    *s_mask = '\0';
	  }
	  else {
	    addr = rowColMap.at(*itRow).at(*itCol)->GetAddress();
	    uint32_t mask = rowColMap.at(*itRow).at(*itCol)->GetMask();
	    snprintf( s_mask, s_maskSize, "0x%x", mask );

	    if (addr.find(".") != std::string::npos) {
	      addr = addr.substr(addr.find_last_of(".")+1);
	    }
	    while (addr.find("_") != std::string::npos) {
	      addr = addr.replace(addr.find("_"),1," ");
	    }
	  }


	  thisRow = thisRow + " & " + s_mask + "/" + addr;
//	} else {
//	  thisRow = thisRow + " & " + " ";
//	}
      }
      
      
      stream << thisRow + "\\\\\n";
      stream << "\\hline\n";
    }
    
    //=====================
    //Print trailer
    //=====================
    stream << "\\end{tabular}\n";
    stream << "\\end{center} \n";
    stream << "Documentation goes here" << "\n\n\n";
  }


  
  void StatusDisplayMatrix::PrintGraphite(std::ostream & stream,
					  int status,
					  bool forceDisplay,
					  int /*headerColWidth*/,
					  std::map<std::string,bool> & rowDisp,
					  std::vector<int> & colWidth) const
  {
    for(std::map<std::string,bool>::iterator itRow = rowDisp.begin();
	itRow != rowDisp.end();
	itRow++){
      //Print the data
      const std::map<std::string,StatusDisplayCell*> & colMap = rowColMap.at(itRow->first);
      for(size_t iCol = 0; iCol < colName.size();iCol++){	  
	if(colWidth[iCol] > 0){
	  std::map<std::string,StatusDisplayCell*>::const_iterator itMap = colMap.find(colName[iCol]);
	  if(itMap != colMap.end()){
	    time_t time_now = time(NULL);
      
      // Build the graphite command for this cell
	    if(itMap->second->Display(status,forceDisplay)){	      
        // Check for spaces in the row and column names
        // If there is a space, replace it with an underscore
        std::string rowForCell = itRow->first;
        std::string colForCell = colName[iCol];

        std::replace(rowForCell.begin(), rowForCell.end(), ' ', '_');
        std::replace(colForCell.begin(), colForCell.end(), ' ', '_');

        // Append table, row and column names to the stream
        stream << name << "." << rowForCell << "." << colForCell;

        // Retrieve the cell value and check if it is a number
        std::string cellValue = itMap->second->Print(0,true);

        bool valueIsNumber = true;
        uint8_t startPosition = 0;

        // Check if this is a number represented in hex notation
        // If that is the case, start checking from starting from position 2
        if (cellValue.compare(0, 2, '0x') == 0) {
          startPosition = 2;
        } 

        for (uint8_t idx=startPosition; idx<cellValue.size(); idx++) {
          // If this if clause is True, we found a non-hex character in the string
          if (isxdigit(cellValue[idx]) == 0) {
            valueIsNumber = false;
            break;
          }
        }

        /* 
         * If the cell value is not a number, append it to the column name (colForCell) 
         * with a dot, and fill the value as 1. This is done for Graphite-related purposes,
         * since it expects all cell values to be numbers.
         */
        if (valueIsNumber) {
          stream << " " << cellValue;
        } else {
          stream << "." << cellValue << " 1";
        }

        // Finally, add the timestamp
        stream << " " << time_now << "\n";
	    }
	  }
	}
      }
    }
  }
 
}
