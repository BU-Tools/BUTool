#include <BUTextIO/BUTextController.hh>

inline std::string getTime() {
    char buffer[128];
    time_t unixTime=time(NULL);
    struct tm * timeinfo = localtime(&unixTime);
    strftime(buffer,128,"%F-%T-%Z",timeinfo);
    return std::string(buffer);
}

BUTextController::BUTextController(std::ostream *os) {
    // constructor adds an ostream* without a preamble by default
    streams.push_back(std::make_tuple(os, Preamble::Disabled, ""));
}

void BUTextController::Print(printer a) {
    std::vector<std::tuple<std::ostream*,Preamble,std::string>>::iterator it;
    for (it = streams.begin(); it != streams.end(); ++it) {
        // check bitmask for preamble text and timestamp
        switch (std::get<1>(*it)) {
            case (Preamble::Enabled | Preamble::Timestamp | Preamble::Text):
            {   // timestamp : preamble : text
                *std::get<0>(*it) << getTime() << " : " << a << "\n";
                break;
            }
            case (Preamble::Enabled | Preamble::Timestamp):
            {   // timestamp : text
                *std::get<0>(*it) << getTime() << " : " << a << "\n";
                break;
            }
            case (Preamble::Enabled | Preamble::Text):
            {   // preamble : text
                *std::get<0>(*it) << std::get<2>(*it) << " " << a << "\n";
                break;
            }
            default:
            {   // only display text
                *std::get<0>(*it) << a << "\n";
                break;
            }
        }
    }
}

void BUTextController::AddOutputStream(std::ostream *os) {
    // only add ostream pointer if said pointer is not already in the streams vector
    std::vector<std::tuple<std::ostream*,Preamble,std::string>>::iterator it;
    for (it=streams.begin(); it!=streams.end(); ++it) {
        if (std::get<0>(*it) == os) {
            return;
        }
    }
    // if pointer doesn't already exist in vector, add it (without preamble)
    streams.push_back(std::make_tuple(os, Preamble::Disabled, ""));
}

void BUTextController::AddPreamble(std::ostream *os, std::string preambleText, bool timestamp) {
    // check that input makes sense
    if (preambleText.size()==0 && !timestamp) {
        printf("AddPreamble call must contain preamble text, true timestamp flag, or both\n");
        return;
    }
    // find the desired ostream*, add the preamble text and/or timestamp flag
    std::vector<std::tuple<std::ostream*,Preamble,std::string>>::iterator it;
    for (it=streams.begin(); it!=streams.end(); ++it) {
        if (std::get<0>(*it) == os) {
            // set the Preamble bitmask to enabled
            std::get<1>(*it) = Preamble::Enabled;
            // check the other variables
            if (preambleText.size() != 0) {
                std::get<1>(*it) |= Preamble::Text;
                std::get<2>(*it) = preambleText;
            }
            if (timestamp) {
                std::get<1>(*it) |= Preamble::Timestamp;
            }
        }
    }
}

void BUTextController::RemoveOutputStream(std::ostream *os) {
    std::vector<std::tuple<std::ostream*,Preamble,std::string>>::iterator it;
    for (it=streams.begin(); it!=streams.end();) {
        if (std::get<0>(*it) == os) {
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