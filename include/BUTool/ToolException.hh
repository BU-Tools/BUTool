#ifndef __TOOLEXCEPTION_HH__
#define __TOOLEXCEPTION_HH__ 1

#include <BUException/ExceptionBase.hh>

namespace BUException{           
  //Exceptions for Tool
  ExceptionClassGenerator(COMMAND_LIST_ERROR,"Error in Command Data Structure class\n")
  ExceptionClassGenerator(DEVICE_CREATION_ERROR,"Error in device constructor\n")
  ExceptionClassGenerator(CREATOR_UNREGISTERED,"Device factory asked to create unregistered class\n Did you forget to call add_lib or set $BUTOOL_AUTOLOAD_LIBRARY_LIST?\n")
  ExceptionClassGenerator(REG_READ_DENIED,"Read access denied\n")
  ExceptionClassGenerator(REG_WRITE_DENIED,"Write access denied\n")
  ExceptionClassGenerator(BAD_REG_NAME,"Register name not found\n")
  ExceptionClassGenerator(BAD_VALUE,"Bad value\n")
  ExceptionClassGenerator(BAD_MARKUP_NAME,"Bad markup name\n")
  ExceptionClassGenerator(BUS_ERROR,"Bus error encountered during a register read/write\n")
  // BUTextIO exception
  ExceptionClassGenerator(TEXTIO_BAD_INIT, "BUTool::RegisterHelper TextIO pointer is NULL\n Did you forget to call SetupTextIO() in the device's constructor?\n")
  // RegisterHelper exceptions
  ExceptionClassGenerator(FORMATTING_NOT_IMPLEMENTED, "BUTool::RegisterHelper: The formatting specified is not implemented\n")
  ExceptionClassGenerator(FUNCTION_NOT_IMPLEMENTED, "Feature not implmented\n")
}




#endif
