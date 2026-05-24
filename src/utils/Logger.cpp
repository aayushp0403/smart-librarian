#include "utils/Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace sl {
namespace utils {

// Static member definition — must exist in exactly one .cpp file.
// This is a common source of linker errors for beginners:
// declaring static members in the header is not enough.
LogLevel Logger::s_level = LogLevel::Debug;

void Logger::setLevel(LogLevel level)
{
    s_level = level;
}

void Logger::log(LogLevel level, std::string_view message)
{
    if (static_cast<int>(level) < static_cast<int>(s_level)) {
        return;
    }

    // Get current time for the log prefix
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%H:%M:%S");

    const char* levelStr = "";
    switch (level) {
        case LogLevel::Debug:   levelStr = "DEBUG  "; break;
        case LogLevel::Info:    levelStr = "INFO   "; break;
        case LogLevel::Warning: levelStr = "WARNING"; break;
        case LogLevel::Error:   levelStr = "ERROR  "; break;
    }

    std::cout << "[" << oss.str() << "] ["
              << levelStr << "] "
              << message << "\n";
}

void Logger::debug  (std::string_view msg) { log(LogLevel::Debug,   msg); }
void Logger::info   (std::string_view msg) { log(LogLevel::Info,    msg); }
void Logger::warning(std::string_view msg) { log(LogLevel::Warning, msg); }
void Logger::error  (std::string_view msg) { log(LogLevel::Error,   msg); }

} // namespace utils
} // namespace sl