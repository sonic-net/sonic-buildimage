#include "nexthopgroup_debug.h"
#include <cstdio>
#include <cstdarg>
#include <memory>
#include <array> // for std::array
#include <vector> // for std::vector

using namespace std;
using namespace fib;

namespace {  // Anonymous namespace
struct LoggerState {
    fib::LogCallback callback;
    std::mutex mutex;
    fib::LogLevel level = fib::LogLevel::DEBUG;
};
LoggerState& getState() {
    static LoggerState state;
    return state;
}

// Default fallback: print to stderr
void defaultLog(LogLevel level, const char* file, int line,
                const char* func, const char* format, ...) {
    const char* level_str = "DEBUG";
    switch (level) {
        case LogLevel::INFO:  level_str = "INFO";  break;
        case LogLevel::WARN:  level_str = "WARN";  break;
        case LogLevel::ERROR: level_str = "ERROR"; break;
        default: break;
    }
   // Format the variadic arguments into a buffer
    std::array<char, 1024> buf;
    va_list args;
    va_start(args, format);
    int written = std::vsnprintf(buf.data(), buf.size(), format, args);
    va_end(args);

    if (written < 0) {
        // Formatting error – log minimal fallback
        std::fprintf(stderr, "[%s] %s:%d %s: <format error>\n",
                     level_str, file, line, func);
        return;
    }
    // Handle truncation gracefully (optional but recommended)
    if (static_cast<size_t>(written) >= buf.size()) {
        // Truncated – indicate with ellipsis
        constexpr size_t ellipsis_len = 3;
        if (buf.size() > ellipsis_len) {
            std::fill_n(buf.end() - ellipsis_len - 1, ellipsis_len, '.');
        }
    }

    // Print the final formatted message
    std::fprintf(stderr, "[%s] %s:%d %s: %s\n",
                 level_str, file, line, func, buf.data());
}

} // anonymous namespace

// Public API implementations
// Don't use FIB_LOG here to avoid recursion mutex
void fib::registerLogCallback(LogCallback cb) {
    auto& state = getState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.callback = std::move(cb);
}

void fib::setLogLevel(LogLevel level) {
    auto& state = getState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.level = level;
}

fib::LogLevel fib::getLogLevel() {
    auto& state = getState();
    std::lock_guard<std::mutex> lock(state.mutex);
    return state.level;
}

// Internal logging implementation
void fib::internalLog(LogLevel level, const char* file, int line,
                        const char* func, const char* format, ...) {
    auto& state = getState();
    fib::LogCallback cb;
    va_list args;
    va_start(args, format);
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        if (static_cast<int>(level) < static_cast<int>(state.level) )
            return;
        if (!state.callback) {
            // Use default logger
            cb = defaultLog;
        }  else {
            cb = state.callback;
        }
    }
    if (cb) cb(level, file, line, func, format, args);  // Forward va_list directly
    va_end(args);
}