#pragma once

#include <string>
#include <string_view>

namespace whiz
{
    enum class log_level
    {
        error,
        warn,
        info,
        debug,
    };

    class logger
    {
      public:
        static void set_verbose(bool enabled);
        static bool verbose();

        static void error(std::string_view msg);
        static void warn(std::string_view msg);
        static void info(std::string_view msg);
        static void debug(std::string_view msg);

      private:
        static void write(log_level level, std::string_view msg);
    };
} // namespace whiz
