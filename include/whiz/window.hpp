#pragma once

#include "whiz/manifest.hpp"

#include <string>

namespace whiz
{
    struct window_error
    {
        int exit_code;
        std::string message;
    };

    // 构建并运行原生窗口。阻塞直到窗口关闭或应用退出。
    // cfg 为合并了命令行覆盖后的最终配置。
    // force_url 非空时直接加载该 URL；否则加载 cfg.entry_html。
    window_error run_window(const manifest &cfg, const std::string &force_url);
} // namespace whiz
