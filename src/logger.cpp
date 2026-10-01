#include "whiz/logger.hpp"

#include <iostream>
#include <print>

namespace whiz
{
    namespace
    {
        bool g_verbose = false;
    }

    void logger::set_verbose(bool enabled)
    {
        g_verbose = enabled;
    }

    bool logger::verbose()
    {
        return g_verbose;
    }

    void logger::error(std::string_view msg)
    {
        write(log_level::error, msg);
    }

    void logger::warn(std::string_view msg)
    {
        write(log_level::warn, msg);
    }

    void logger::info(std::string_view msg)
    {
        write(log_level::info, msg);
    }

    void logger::debug(std::string_view msg)
    {
        write(log_level::debug, msg);
    }

    void logger::write(log_level level, std::string_view msg)
    {
        // debug 级别仅在 --verbose 时输出
        if (level == log_level::debug && !g_verbose)
        {
            return;
        }

        // info 级别始终输出到 stdout（便于普通运行反馈）
        // error / warn 输出到 stderr
        std::ostream &out = (level == log_level::error || level == log_level::warn) ? std::cerr : std::cout;

        switch (level)
        {
        case log_level::error:
            std::println(out, "[error] {}", msg);
            break;
        case log_level::warn:
            std::println(out, "[warn]  {}", msg);
            break;
        case log_level::info:
            std::println(out, "[info]  {}", msg);
            break;
        case log_level::debug:
            std::println(out, "[debug] {}", msg);
            break;
        }

        // 立即刷新，确保在进程被强制终止（例如测试时 timeout/kill）
        // 时日志也不会丢失，便于排查窗口图标等问题。
        out.flush();
    }
} // namespace whiz
