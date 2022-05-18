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
      char buffer[50];
      snprintf(buffer,49,"No cell in (\"%s\",\"%s\") position\n",row.c_str(),col.c_str());
      e.Append(buffer);
      throw e;
    }
    return rowColMap.at(row).at(col);
  }

  void StatusDisplayMatrix::Add(std::string registerName, RegisterHelperIO* regIO)
  // Add the register with registerName into a StatusDisplayMatrix table instance.
  {
    // Find the table name for this register. If we can't find one, throw an exception
    std::string tableName;
    try{
      tableName = regIO->GetRegParameterValue(registerName, "Table");
    }catch (BUException::BAD_REG_NAME & e){
      BUException::BAD_VALUE e2;
      char tmp[256];
      snprintf(tmp, 255, "Missing Table value for %s \n",registerName.c_str());
      e2.Append(tmp);
      throw e2;
    }
    
    CheckName(NameBuilder(tableName,registerName));

    // Determine row and column
    std::string row = ParseRowOrCol(regIO,registerName,"Row");
    std::string col = ParseRowOrCol(regIO,registerName,"Column");

    int bitShift = 0;

    // Check if registerName contains a "_HI" or a "_LO"    
    if((registerName.find("_LO") == (registerName.size()-3)) ||
       (registerName.find("_HI") == (registerName.size()-3))) {
      // Search for an existing base register name
      std::string baseRegisterName;
      if(registerName.find("_LO") == (registerName.size()-3)) {
        baseRegisterName = registerName.substr(0,registerName.find("_LO"));
        bitShift = 0;
      }
      if(registerName.find("_HI") == (registerName.size()-3)) {
        baseRegisterName = registerName.substr(0,registerName.find("_HI"));
        bitShift = 32;
        std::string LO_address(baseRegisterName);
        LO_address.append("_LO");
        // Check if the LO word has already been placed
        if (cell.find(LO_address) != cell.end()) {
          uint32_t mask = cell.at(LO_address)->GetMask();
          while ( (mask & 0x1) == 0) {
            mask >>= 1;
          }
          int count = 0;
          while ( (mask & 0x1) == 1) {
            count++;
            mask >>= 1;
          }
          bitShift = count; //Set bitShift to be the number of bits in the LO word
        }
      // If LO word hasn't been placed into the table yet, assume 32
      }
      std::map<std::string,StatusDisplayCell*>::iterator itCell;
      if(((itCell = cell.find(baseRegisterName)) != cell.end()) ||  //Base address exists already
        ((itCell = cell.find(baseRegisterName+std::string("_HI"))) != cell.end()) || //Hi address exists already
        ((itCell = cell.find(baseRegisterName+std::string("_LO"))) != cell.end())) { //Low address exists already
        
        if(iequals(itCell->second->GetRow(),row) && 
          iequals(itCell->second->GetCol(),col)) {
          // We want to combine these entries so we need to rename the old one
          StatusDisplayCell * ptrCell = itCell->second;
          cell.erase(ptrCell->GetAddress());
          cell[baseRegisterName] = ptrCell;
          ptrCell->SetAddress(baseRegisterName);	  
          registerName=baseRegisterName;

        }
      }
    }

    // Get description,format,rule, and statuslevel
    std::string description;
    try {
      description = regIO->GetRegParameterValue(registerName, "Description");
    } catch (BUException::BAD_REG_NAME & e) {
      description = std::string("");
    }
    
    std::string statusLevel;
    try {
      statusLevel = regIO->GetRegParameterValue(registerName, "Status");
    } catch (BUException::BAD_REG_NAME & e) {
      statusLevel = std::string("");
    }
    
    std::string rule;
    try {
      rule = regIO->GetRegParameterValue(registerName, "Show");
    } catch (BUException::BAD_REG_NAME & e) {
      rule = std::string("");
    }
    
    std::string format;
    try {
      format = regIO->GetRegParameterValue(registerName, "Format");
    } catch (BUException::BAD_REG_NAME & e) {
      format = STATUS_DISPLAY_DEFAULT_FORMAT;
    }

    boost::to_upper(rule);

    // Determine if this register is "enabled" to be shown
    bool enabled=true;
    try {
      // True if string isn't equal to "0"
      enabled=regIO->GetRegParameterValue(registerName, "Enabled").compare("0");
    } catch (BUException::BAD_REG_NAME & e) {
      enabled=true;
    }
 
    StatusDisplayCell * ptrCell;
    // Add or append this entry
    if(cell.find(registerName) == cell.end()) {
      ptrCell = new StatusDisplayCell;
      cell[registerName] = ptrCell;
    }
    else {
      ptrCell = cell[registerName];
    }
    ptrCell->Setup(registerName,description,row,col,format,rule,statusLevel,enabled);
    // Read the value if it is as non-zero status level
    // A status level of zero is for write only registers
    if(ptrCell->DisplayLevel() > 0){
      uint32_t value;
      value = regIO->ReadRegister(registerName);
      ptrCell->Fill(value,bitShift);
    }
    //Setup should have thrown if something bad happened, so we are safe to update the search maps
    rowColMap[row][col] = ptrCell;
    colRowMap[col][row] = ptrCell;

    uint32_t valueMask = regIO->GetRegMask(registerName); 
    ptrCell->SetMask(valueMask);

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


  std::string StatusDisplayMatrix::NameBuilder(std::string const & markup,std::string const & name) const{
    //use a boost tokenizer to split up the name
    //this won't actually split up the string until we call .begin() on it
    boost::tokenizer<boost::char_separator<char> > tokenizedName(name,
				boost::char_separator<char>("."));
    std::vector<std::string> positionNames;

    std::string ret;
    size_t iChar = 0;
    for(;iChar < markup.size();iChar++){
      //look for the next special char (which is a run of two, a run of three is a reverse token)
      if(markup[iChar] == STATUS_DISPLAY_PARAMETER_PARSE_TOKEN){
	if(iChar+1 < markup.size()){
	  if(markup[iChar+1] != STATUS_DISPLAY_PARAMETER_PARSE_TOKEN){
	    //there was only one parse token special character, so this is just a normal _
	    //but " " is not allowed
	    if(markup[iChar] == ' '){
	      BUException::BAD_VALUE e;	    
	      std::string error("Spaces are not allowed in markup ");
	      error += markup;
	      e.Append(error.c_str());
	      throw e;
	    }
	    ret.push_back(markup[iChar]);	    	    
	    continue;
	  }else{
	    iChar++;
	  }
	}

	bool reverseToken = false;
	//start parsing a special
	iChar++;
	if(iChar < markup.size()){
	  //check for special character
	  if(markup[iChar] == STATUS_DISPLAY_PARAMETER_PARSE_TOKEN){
	    reverseToken = true;
	    iChar++;
	  }
	  //find number
	  std::string position;
	  for(;iChar < markup.size();iChar++){
	    if(!isdigit(markup[iChar])){
	      iChar--;
	      break;
	    }else{
	      position.push_back(markup[iChar]);
	    }
	  }
	  if(position.size() > 0){
	    int pos = std::stoi(position);
	    //build the parsed vector of position names if we haven't
	    if(positionNames.size() == 0){
	      positionNames.push_back(name); // for _0
	      auto itTok = tokenizedName.begin();
	      for(;itTok!=tokenizedName.end();itTok++){
		positionNames.push_back(*itTok); // for _n
	      }
	    }
	    //Recompute the position if this is a reverse token.
	    if(reverseToken){
	      pos = positionNames.size() - pos;
	    }
	    if(pos > int(positionNames.size())){
	      BUException::BAD_VALUE e;	    
	      std::string error("Bad markup name for ");
	      error += name + " with token " + std::to_string(pos) + " from markup " + markup;
	      e.Append(error.c_str());
	      throw e;
	    }
	    if(ret.size()){
	      //add whitespace if we need it
	      ret += " ";
	    }
	    ret.append(positionNames[pos]);
	  }else{
	    BUException::BAD_VALUE e;	    
	    std::string error("Bad markup for ");
	    error += name + " from markup " + markup;
	    e.Append(error.c_str());
	    throw e;
	  }
	}
      }else{
	//no special character, so just copy the string
	if(markup[iChar] == ' '){
	  BUException::BAD_VALUE e;	    
	  std::string error("Spaces are not allowed in name ");
	  error += "\""+markup+"\"";
	  e.Append(error.c_str());
	  throw e;
	}
	ret.push_back(markup[iChar]);
      }
    }
    return ret;
  }

  std::string StatusDisplayMatrix::ParseRowOrCol(RegisterHelperIO* regIO,
              std::string const & addressBase,
              std::string const & parameterName) const
  {
    // Get row or column name, as specified by the parameterName argument
    std::string newName;
    try {
      newName = regIO->GetRegParameterValue(addressBase, parameterName);
    } catch (BUException::BAD_REG_NAME & e) {
      // Missing row or column
      BUException::BAD_VALUE e;
      std::string error("Missing ");
      error += parameterName;
      error += " for ";
      error += addressBase;
      e.Append(error.c_str());
      throw e; 
    }

    boost::to_upper(newName);
    newName = NameBuilder(newName, addressBase);

    return newName;
  }

  // render one table
  void StatusDisplayMatrix::Render(std::ostream & stream,int status,StatusMode statusMode) const
  {
    bool forceDisplay = (status >= 99) ? true : false;

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
	  if( rowKill[itColCell->first])
	    // yes, delete entry in rowDisp if any
	    rowDisp.erase(itColCell->first);
	  else 
	    // nope, mark it for display
	    rowDisp[itColCell->first] = true;
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
    int headerColWidth = 16;
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
	    std::string tdClass = "";
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
    std::string headerSuffix = "";
    std::string cols = "";
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
      char s_mask[10];

      while (thisRow.find("_") != std::string::npos) {
	thisRow = thisRow.replace(thisRow.find("_"),1," ");
      }

      for(std::vector<std::string>::iterator itCol = modColName.begin(); itCol != modColName.end(); itCol++) {
	std::string addr;

	if (true) {
	  if (rowColMap.at(*itRow).find(*itCol) == rowColMap.at(*itRow).end()) {
	    addr = " ";
	    *s_mask = '\0';
	  }
	  else {
	    addr = rowColMap.at(*itRow).at(*itCol)->GetAddress();
	    uint32_t mask = rowColMap.at(*itRow).at(*itCol)->GetMask();
	    snprintf( s_mask, 10, "0x%x", mask );

	    if (addr.find(".") != std::string::npos) {
	      addr = addr.substr(addr.find_last_of(".")+1);
	    }
	    while (addr.find("_") != std::string::npos) {
	      addr = addr.replace(addr.find("_"),1," ");
	    }
	  }


	  thisRow = thisRow + " & " + s_mask + "/" + addr;
	} else {
	  thisRow = thisRow + " & " + " ";
	}
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
	    if(itMap->second->Display(status,forceDisplay)){	      
	      stream << name << "." << itRow->first << "." << colName[iCol] << " "
		     << itMap->second->Print(0,true) 
		     << " " << time_now << "\n";
	    }
	  }
	}
      }
    }
  }
 
}
