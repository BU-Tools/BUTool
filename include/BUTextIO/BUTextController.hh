#ifndef __BU_TEXT_CONTROLLER_HH__
#define __BU_TEXT_CONTROLLER_HH__

#include "Preamble.hh"
#include "Print.hh"
#include "PrintLevel.hh"

#include <time.h>   // getTime() - preamble
#include <tuple>
#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>

class BUTextController {
private:
    // tuple contains : ostream pointer, preamble bitmask, preamble string (can be empty)
    std::vector<std::tuple<std::ostream*, Preamble, std::string> > streams;
public:
    BUTextController(std::ostream *os);
    void Print(printer a);
    void AddOutputStream(std::ostream *os);
    void AddPreamble(std::ostream *os, std::string preambleText, bool timestamp);
    void RemoveOutputStream(std::ostream *os);
    void ResetStreams();
};

#endif