// MAIN.CPP
//
// Entry point for the program that the Nvidia Jetson TK1 will be excuting at
// boot time.

#include "Config.h"
#include "RemoteTransmitter.h"
#include "PapasVision.h"

#ifdef __GLIBCXX__
#include <cxxabi.h> // A GCC-specific function useful for demangling std::type_info.name() strings.
#endif // #ifdef __GLIBCXX__

#include <exception>
#include <algorithm>
#include <iterator>
#include <iomanip>
#include <iostream>
#include <thread>
#include <string>
#include <vector>

#include <opencv2/core/core.hpp>

namespace p = boost::program_options;
using std::ostream_iterator;
using std::setprecision;
using std::exception;
using std::string;
using std::vector;
using std::copy;
using std::cout;
using std::cerr;
using namespace std::this_thread;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace robot;

void mainLoop(const Config& config);

int main() {
    cv::Mat matrix;
    cout << "OpenCV version " << CV_VERSION << " ready!\n";

    try {
        Config config;

        cout << "RoboRIO IP address list from config file \""
             << config.path() << "\": ";

        vector<string> address_list = config.robotAddresses();
        copy(address_list.begin(), address_list.end(), ostream_iterator<string>(cout, ", "));
        cout << "\n";

        mainLoop(config);

    } catch(const exception& e) {

        string exceptionTypeName = typeid(e).name();

#ifdef __GLIBCXX__
        // Translates a mangled g++ type name into something a human being can
        // read.  See
        // https://gcc.gnu.org/onlinedocs/libstdc++/manual/ext_demangling.html
        // for more information.  Other compilers don't seem to do this, so we
        // could leave exceptionTypeName as-is.

        int status;
        char* realname;
        realname = abi::__cxa_demangle(exceptionTypeName.c_str(), 0, 0, &status);
        exceptionTypeName = string(realname);
        free(realname);
#endif // #ifdef __GLIBCXX__

        // Catch-all for any exceptions thrown in the program.
        cerr << "\n\n*** Abnormal termination due to uncaught "
             << exceptionTypeName << " exception.***\n\n";
        cerr << "Exception message: \"" << e.what() << "\"\n";
        return 1;

    }
}


// =========================================================================
// Runs the camera code, constructs messages from it, and transmits those
// messages remotely as long as there are messages to transmit and something
// weird doesn't happen.


void mainLoop(const Config& config) {

    // The RemoteTransmitter will shut the thread down when it goes out of scope.
    RemoteTransmitter transmitter(config);
    PapasVision papasVision(config, 0.0, true);
    auto start = high_resolution_clock::now();
    bool done = false;

    while (!done) {

        cout << "\r";
        papasVision.findGoal(1); // Sample images are ./samples/{1..8}.png

        // For now, let's run everything for ten seconds.
        double elapsedSeconds = duration<double>(high_resolution_clock::now() - start).count();
        if (elapsedSeconds >= 10.0) {
            done = true;
        } else {
            cerr << "\rWaiting for " << setprecision(2) << (10.0 - elapsedSeconds) << " seconds...";
        }

        // Ensure that the log messages aren't too spammy.
        std::this_thread::sleep_for(0.1s);
    }
    cout << "\n";

}
