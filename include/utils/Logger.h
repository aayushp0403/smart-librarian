#pragma once

#include <string>
#include <string_view>

/**
 * Logger.h — Minimal structured logger
 *
 * Design:
 *   Static interface for simplicity at this stage.
 *   Later we'll make this a proper singleton with
 *   configurable sinks (file, stdout, UI console).
 *
 * string_view usage:
 *   We accept string_view instead of const string&
 *   because it can bind to string literals, std::string,
 *   and char* without copies. Zero overhead for the caller.
 */

namespace sl {
namespace utils {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger
{
public:
    static void debug  (std::string_view message);
    static void info   (std::string_view message);
    static void warning(std::string_view message);
    static void error  (std::string_view message);

    static void setLevel(LogLevel level);

private:
    static LogLevel s_level;

    static void log(LogLevel level, std::string_view message);
};

} // namespace utils
} // namespace sl