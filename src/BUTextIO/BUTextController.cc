#include <BUTextIO/BUTextController.hh>

struct alreadyInVector {
    // struct that takes the place of the unary function in find_if call within AddOutputStream
    std::ostream* toBeAdded;
    alreadyInVector(std::ostream* os) : toBeAdded(os) {}
    bool operator()(std::ostream* alreadyAdded) {
        return (alreadyAdded == toBeAdded);
    }
};

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
    // only add ostream pointer if said pointer is not already in the streams vector
    if (std::find_if(streams.begin(), streams.end(), alreadyInVector(os)) != streams.end()) {
        return;
    }
    else {
        streams.push_back(os);
    }
}

void BUTextController::RemoveOutputStream(std::ostream *os) {
    std::vector<std::ostream*>::iterator it;
    for (it=streams.begin(); it!=streams.end();) {
        if (*it == os) {
            it = streams.erase(it);
        }
        else {
            ++it;
        }
    }
}

void BUTextController::ResetStreams() {
    streams.clear();
}