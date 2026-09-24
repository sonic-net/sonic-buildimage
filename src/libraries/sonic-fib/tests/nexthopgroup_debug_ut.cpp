#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <gtest/gtest.h>
#include <arpa/inet.h>

#include <iostream>
#include <thread>

#include <unistd.h>

#include "src/c-api/nexthopgroup_capi.h"
#include "src/nexthopgroup_debug.h"
#include <array> // for std::array
#include <syslog.h>

using namespace std;
using namespace fib;

// Test C++ API registration and logging
TEST(NextHopGroupDEBUG_API, register) {

    setLogLevel(fib::LogLevel::DEBUG);
    FIB_LOG(fib::LogLevel::DEBUG, "Current log level: %d", static_cast<int>(getLogLevel()));
    ASSERT_EQ(getLogLevel(), fib::LogLevel::DEBUG); // Default is  DEBUG

    setLogLevel(fib::LogLevel::INFO);
    FIB_LOG(fib::LogLevel::INFO, "Current log level after setting: %d", static_cast<int>(getLogLevel()));
    ASSERT_EQ(getLogLevel(), fib::LogLevel::INFO); // Change to INFO
    registerLogCallback([](fib::LogLevel lvl, const char* file, int line,
                            const char* func, const char* format, va_list args) {
        const char* level_str = "DEBUG";
        switch (lvl) {
            case fib::LogLevel::INFO:  level_str = "INFO";  break;
            case fib::LogLevel::WARN:  level_str = "WARN";  break;
            case fib::LogLevel::ERROR: level_str = "ERROR"; break;
            default: break;
        }

        if (static_cast<int>(lvl) < static_cast<int>(getLogLevel())) {
            return; // Skip messages below current log level
        }
        // Format the variadic arguments into a buffer
        std::array<char, 1024> buf;
        va_list args_copy;
        va_copy(args_copy, args);
        int len = vsnprintf(buf.data(), buf.size(), format, args_copy);
        va_end(args_copy);
        if (len > 0 && len < static_cast<int>(buf.size())) {
            cout << "[" << level_str << "] " << file << ":" << line << " " << func << " - " << buf.data() << endl;
        }
    });

    FIB_LOG(fib::LogLevel::DEBUG, "Test log with registered callback - DEBUG level");
    FIB_LOG(fib::LogLevel::INFO, "Test log with registered callback - INFO level");
}

// Ensure C linkage for the callback function to match C ABI expectations
extern "C" {

/* Map fib-sonic levels  to syslog priorities used by FRR */
static int fib_level_to_syslog(int level)
{
    switch (level) {
    case 0: return LOG_DEBUG;    // DEBUG
    case 1: return LOG_INFO;     // INFO
    case 2: return LOG_WARNING;  // WARN
    case 3: return LOG_ERR;      // ERROR
    default: return LOG_DEBUG;
    }
}
static void frr_log_forwarder(int level,
                              const char *file,
                              int line,
                              const char *func,
                              const char *fmt,
                              va_list args)
{
    int syslog_prio = fib_level_to_syslog(level);

    int current_log_level = -1;
    try {
        current_log_level = fib_frr_get_log_level();
    } catch (...) {
        current_log_level = -1;  // fallback on error
    }
    if (level < current_log_level) {
        return; // Skip messages below current log level
    }
    fprintf(stderr, "[LVL=%d %s:%d %s] ", syslog_prio, file, line, func);
    // Core: print formatted message to stderr using va_list
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");  // Add newline (vfprintf does not add one automatically)
}

} // extern "C"
// Test C API registration and logging
TEST(NextHopGroupDEBUG_CAPI, register_c) { 
    fib_frr_set_log_level(0); // Set back to DEBUG
    LogLevel level = getLogLevel();
    FIB_LOG(fib::LogLevel::DEBUG, "Current log level: %d", static_cast<int>(level));
    ASSERT_EQ(getLogLevel(), fib::LogLevel::DEBUG); // Default is  DEBUG
    fib_frr_register_callback(nullptr); // Unregister callback to test default logging
    FIB_LOG(fib::LogLevel::DEBUG, "Test log with default logger - DEBUG level");
    FIB_LOG(fib::LogLevel::INFO, "Test log with default logger - INFO level");

    fib_frr_set_log_level(1); // INFO
    level = getLogLevel();
    FIB_LOG(fib::LogLevel::INFO, "Current log level after setting: %d", static_cast<int>(level));
    ASSERT_EQ(getLogLevel(), fib::LogLevel::INFO); // Change to INFO

    fib_frr_register_callback(frr_log_forwarder); // Register callback to test default logging
    FIB_LOG(fib::LogLevel::DEBUG, "Test log with C callback logger - DEBUG level");
    FIB_LOG(fib::LogLevel::INFO, "Test log with C callback logger - INFO level");
}