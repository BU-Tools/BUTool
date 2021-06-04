#ifndef __BU_TEXT_IO_HH__
#define __BU_TEXT_IO_HH__

#include "BUTextController.hh"

class BUTextIO {
public:
    BUTextIO();
    virtual void AddOutputStream(Level::level level, std::ostream *os);
    virtual void ResetStreams(Level::level level);
    virtual void Print(Level::level level, const char *fmt, ...);
private:
    std::vector<BUTextController> controllers;
};

#endif 
