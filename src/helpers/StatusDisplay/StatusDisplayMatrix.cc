#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm> //std::max
#include <iomanip> //for std::setw
#include <ctype.h> // for isDigit()

//For PRI macros
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <BUTool/helpers/StatusDisplay/StatusDisplayMatrix.hh>
#include <BUTool/helpers/StatusDisplay/StatusDisplayCell.hh>

#include <BUTool/ToolException.hh>


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

  void StatusDisplayMatrix::Add(std::string address,uint32_t value,uint32_t value_mask, uMap const & parameters)
  {
    uMap::const_iterator itTable= parameters.find("Table");
    if(itTable == parameters.end()){
      BUException::BAD_VALUE e;
      char tmp[256];
      snprintf(tmp, 255, "Missing Table value for %s \n",address.c_str());
      e.Append( tmp);
      throw e;
    }
    CheckName(itTable->second);
    
    //Determine address
    boost::to_upper(address);

    //Check if the rows/columns are the same
    //Determine row and column
    std::string row = ParseRow(parameters,address);
    std::string col = ParseCol(parameters,address);    
    int bitShift = 0;
    
    //Check if address contains a "_HI" or a "_LO"    
    if((address.find("_LO") == (address.size()-3)) ||
       (address.find("_HI") == (address.size()-3))){
      //Search for an existing base address
      std::string baseAddress;
      if(address.find("_LO") == (address.size()-3)){
	baseAddress = address.substr(0,address.find("_LO"));
	bitShift = 0;
      }
      if(address.find("_HI") == (address.size()-3)){
	baseAddress = address.substr(0,address.find("_HI"));
	bitShift = 32;
	std::string LO_address(baseAddress);
	LO_address.append("_LO");
	//Check if the LO word has already been placed
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
	  bitShift = count; //Set bitShift t be the number of bits in the LO word
	}
	//If LO word hasn't been placed into the table yet, assume 32
      }
      std::map<std::string,StatusDisplayCell*>::iterator itCell;
      if(((itCell = cell.find(baseAddress)) != cell.end()) ||  //Base address exists alread
	 ((itCell = cell.find(baseAddress+std::string("_HI"))) != cell.end()) || //Hi address exists alread
	 ((itCell = cell.find(baseAddress+std::string("_LO"))) != cell.end())){ //Low address exists alread
	if(iequals(itCell->second->GetRow(),row) && 
	   iequals(itCell->second->GetCol(),col)){
	  //We want to combine these entries so we need to rename the old one
	  StatusDisplayCell * ptrCell = itCell->second;
	  cell.erase(ptrCell->GetAddress());
	  cell[baseAddress] = ptrCell;
	  ptrCell->SetAddress(baseAddress);	  
	  address=baseAddress;

	}
      }
    }
 
    
    //Get description,format,rule, and statuslevel    
    std::string description = (parameters.find("Description") != parameters.end()) ? parameters.find("Description")->second : std::string("");
    std::string statusLevel = (parameters.find("Status") != parameters.end())      ? parameters.find("Status")->second      : std::string("");
    std::string rule        = (parameters.find("Show") != parameters.end())        ? parameters.find("Show")->second        : std::string(""); 
    std::string format      = (parameters.find("Format") != parameters.end())      ? parameters.find("Format")->second      : STATUS_DISPLAY_DEFAULT_FORMAT; 

    boost::to_upper(rule);

    bool enabled=true;
    if(parameters.find("Enabled") != parameters.end()){
      enabled = parameters.find("Enabled")->second.compare("0"); //True if string isn't equal to "0"
    }

    StatusDisplayCell * ptrCell;
    //Add or append this entry
    if(cell.find(address) == cell.end()){
      ptrCell = new StatusDisplayCell;
      cell[address] = ptrCell;
    }else{
      ptrCell = cell[address];
    }
    ptrCell->Setup(address,description,row,col,format,rule,statusLevel,enabled);
    //Read the value if it is as non-zero status level
    //A status level of zero is for write only registers
    if(ptrCell->DisplayLevel() > 0){
      ptrCell->Fill(value,bitShift);
    }
    //Setup should have thrown if something bad happened, so we are safe to update the search maps
    rowColMap[row][col] = ptrCell;
    colRowMap[col][row] = ptrCell;    
    ptrCell->SetMask(value_mask);
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



  void FindTokenPositions(std::string const & markup, std::vector<size_t> & tokens){
    //Bail when we have no more tokens
    if(STATUS_DISPLAY_PARAMETER_PARSE_TOKEN != markup[0]){
      return;
    }
    for(size_t iChar = 1;iChar < markup.size();iChar++){
      //Search until we get to a non-digit character
      if(!isdigit(markup[iChar]) || (iChar+1 == markup.size())){
	//append this number
	tokens.push_back(strtoul(markup.substr(1,iChar).c_str(),NULL,0));
	if((STATUS_DISPLAY_PARAMETER_PARSE_TOKEN == markup[iChar]) && ((iChar+1) < markup.size())){
	  FindTokenPositions(markup.substr(iChar),tokens);
	}
	break;
      }
    }      
  }

  std::string StatusDisplayMatrix::ParseRow(uMap const & parameters,
					    std::string const & addressBase) const
  {
    uMap::const_iterator rowName = parameters.find("Row");
    std::string newRow;
    //Row
    if(rowName != parameters.end()){
      //Grab the row name and store it
      newRow = rowName->second;
      boost::to_upper(newRow);
      //Check if we have a special character at the beginning to tell us what to use for row
      if((newRow.size() > 1)&&
	 (newRow[0] == STATUS_DISPLAY_PARAMETER_PARSE_TOKEN)){
	//Parse the total token count
	std::vector<size_t> tokensForName;
	FindTokenPositions(newRow,tokensForName);

	

	//Build a BOOST tokenizer for the address name
	//This is for use with undefined rows and columns. 
	//This does not tokenize until .begin() is called
	boost::char_separator<char> sep(".");
	boost::tokenizer<boost::char_separator<char> > tokenizedAddressName(addressBase,sep);
	boost::tokenizer<boost::char_separator<char> >::iterator itTok = tokenizedAddressName.begin();
	std::vector<std::string> tokenNames;
	tokenNames.push_back(addressBase); // for _0
	for(;itTok!=tokenizedAddressName.end();itTok++){
	  tokenNames.push_back(*itTok); // for _n
	}
	newRow.clear();
	for(size_t iToken = 0; iToken < tokensForName.size();iToken++){		  	 
	    //Check that this is a valid value 
	    if(tokensForName[iToken] >= tokenNames.size()){
	      BUException::BAD_VALUE e;	    
	      std::string error("Bad row for ");
	      error += addressBase + " with token " + rowName->second;
	      e.Append(error.c_str());
	      throw e;
	    }
	    
	    if(newRow.size()){
	      //append spaces between items
	      newRow.append(" ");
	    }
	    newRow.append(tokenNames[tokensForName[iToken]]);
	}
      } 
    }else{
      //Missing row
      BUException::BAD_VALUE e;
      std::string error("Missing row for ");
      error += addressBase;
      e.Append(error.c_str());
      throw e;
    }
    return newRow;
  }
  std::string StatusDisplayMatrix::ParseCol(uMap const & parameters,
					 std::string const & addressBase) const
  {    
    uMap::const_iterator colName = parameters.find("Column");
    std::string newCol;
    //Col
    if(colName != parameters.end()){
      //Grab the col name and store it
      newCol = colName->second;
      boost::to_upper(newCol);
      //Check if we have a special character at the beginning to tell us what to use for col
      if((newCol.size() > 1)&&
	 (newCol[0] == STATUS_DISPLAY_PARAMETER_PARSE_TOKEN)){

	std::vector<size_t> tokensForName;
	FindTokenPositions(newCol,tokensForName);

	//Build a BOOST tokenizer for the address name
	//This is for use with undefined cols and columns. 
	//This does not tokenize until .begin() is called
	boost::char_separator<char> sep(".");
	boost::tokenizer<boost::char_separator<char> > tokenizedAddressName(addressBase,sep);
	boost::tokenizer<boost::char_separator<char> >::iterator itTok = tokenizedAddressName.begin();
	std::vector<std::string> tokenNames;
	tokenNames.push_back(addressBase); // for _0
	for(;itTok!=tokenizedAddressName.end();itTok++){
	  tokenNames.push_back(*itTok); // for _n
	}
	newCol.clear();

	for(size_t iToken = 0; iToken < tokensForName.size();iToken++){		  	 
	    if(tokensForName[iToken] >= tokenNames.size()){
	      BUException::BAD_VALUE e;	    
	      std::string error("Bad col for ");
	      error += addressBase + " with token " + colName->second;
	      e.Append(error.c_str());
	      throw e;
	    }
	    
	    if(newCol.size()){
	      //append spaces between items
	      newCol.append(" ");
	    }
	    newCol.append(tokenNames[tokensForName[iToken]]);
	}
      } 
    }else{
      //Missing col
      BUException::BAD_VALUE e;
      std::string error("Missing col for ");
      error += addressBase;
      e.Append(error.c_str());
      throw e;
    }
    return newCol;
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

//    //Shrink the column list to not include redundant columns
//    for (std::vector<std::string>::const_iterator colIt = colName.begin() ; colIt != colName.end(); ++colIt) {
//      if (modColName.empty()) {
//	modColName.push_back(*colIt);
//      } else if (modColName.back().substr(0,3).compare((*colIt).substr(0,3))==0 && ((*colIt).substr(0,3) == "AMC" ||
//										    (*colIt).substr(0,3) == "SFP")) {
//	
//      } else {
//	modColName.push_back(*colIt);
//      }
//    }
	    
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
