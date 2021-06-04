#include <BUTextIO/BUTextController.hh>

BUTextController::BUTextController(std::ostream *os) {
    streams.push_back(os);
}

// deprecated - use Print(printer p)
void BUTextController::Print(const char *fmt, ...) {
    // wrapper around printerHelper to allow variable arg forwarding
    va_list argp;
    va_start(argp, fmt);
    printer Printer = printerHelper(fmt, argp);
    va_end(argp);

    std::vector<std::ostream*>::iterator it;
    for (it = streams.begin(); it != streams.end(); ++it) {
        *(*it) << Printer;
    }
}

void BUTextController::Print(printer a) {
    std::vector<std::ostream*>::iterator it;
    for (it = streams.begin(); it != streams.end(); it++) {
        *(*it) << a;
    }
}

void BUTextController::AddOutputStream(std::ostream *os) {
    streams.push_back(os);
}

void BUTextController::ResetStreams() {
    std::vector<std::ostream*>::iterator it;
    for (it = streams.begin(); it != streams.end(); it++) {
        delete (*it);
    }
    streams.clear();
}