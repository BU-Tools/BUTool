#ifndef __BU_TEXT_IO_HH__
#define __BU_TEXT_IO_HH__

#include <BUTextIO/BUTextController.hh>

class BUTextIO {
public:
    BUTextIO();
    ~BUTextIO() {};
    void AddOutputStream(Level::level level, std::ostream *os);
    void RemoveOutputStream(Level::level level, std::ostream* os);
    void ResetStreams(Level::level level);
    void Print(Level::level level, const char *fmt, ...);
private:
    std::vector<BUTextController> controllers;
};

#endif 
