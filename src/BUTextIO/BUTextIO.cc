#include <BUTextIO/BUTextIO.hh>

BUTextIO::BUTextIO() {
    // initialize with three controllers (INFO, DEBUG, ERROR)
    // prob a better way of doing this
    controllers.push_back(BUTextController(&std::cout));
    controllers.push_back(BUTextController(&std::cout));
    controllers.push_back(BUTextController(&std::cerr));
}

void BUTextIO::AddOutputStream(Level::level level, std::ostream *os) {
    controllers[level].AddOutputStream(os);
}

void BUTextIO::AddPreamble(Level::level level, std::ostream* os, std::string preambleText, bool timestamp) {
    controllers[level].AddPreamble(os, preambleText, timestamp);
}

void BUTextIO::RemoveOutputStream(Level::level level, std::ostream* os) {
    controllers[level].RemoveOutputStream(os);
}

void BUTextIO::ResetStreams(Level::level level) {
    controllers[level].ResetStreams();
}

void BUTextIO::Print(Level::level level, const char *fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    printer Printer = printerHelper(fmt, argp);
    va_end(argp);

    controllers[level].Print(Printer);

    // after Print call, free storage allocated to the char buffer
    // vasprintf provides a ptr to malloc'd storage - must be free'd by caller
    free(Printer.s);
}