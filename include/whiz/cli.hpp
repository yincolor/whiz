#pragma once

#include <optional>
#include <string>

namespace whiz
{
    struct cli_options
    {
        std::string project_dir;         // 项目目录参数（可为空，表示当前目录）
        bool has_project_dir{false};

        bool show_help{false};
        bool show_version{false};

        bool devtools{false};
        bool has_devtools{false};

        std::string url;                  // --url 直接加载 URL
        bool has_url{false};

        std::string title;                // --title 覆盖
        bool has_title{false};

        int width{-1};                    // --width 覆盖
        bool has_width{false};
        int height{-1};                   // --height 覆盖
        bool has_height{false};

        bool no_web_security{false};      // --no-web-security
        bool show{false};                 // --show 立即显示
        bool verbose{false};              // --verbose

        bool init_mode{false};            // whiz init 子命令
        bool init_yes{false};             // -y / --yes：跳过询问，使用默认值

        bool force_url{false};            // 内部：仅当 --url 提供时为 true
    };

    struct cli_error
    {
        int exit_code;
        std::string message;
    };

    // 解析命令行参数。出错时返回 cli_error（exit_code 1），
    // 或返回需要立即退出的标志（help/version）。
    cli_error parse_cli(int argc, char **argv, cli_options &out);

    // 帮助与版本文案
    std::string help_text();
    std::string version_text();
} // namespace whiz
