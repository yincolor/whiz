#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace whiz
{
    namespace fs = std::filesystem;

    // “whiz init” 交互式问卷收集到的配置。
    // 字段默认值对齐 manifest 解析时的默认值，仅当用户修改后才写入 package.json。
    struct init_config
    {
        std::string name;                    // 必填：由 run_init 从目录名推断或交互输入，无默认值
        std::string version{"0.1.0"};
        std::string description;              // 空表示省略
        std::string license{"MIT"};
        std::string main{"index.html"};
        bool web_security{true};

        // window 配置
        std::string title;                    // 空表示使用 name
        int width{1024};
        int height{768};
        int min_width{0};
        int min_height{0};
        int max_width{0};
        int max_height{0};
        int x{-1};
        int y{-1};
        bool resizable{true};
        bool frameless{false};
        bool transparent{false};
        bool always_on_top{false};
        bool fullscreen{false};
        bool show{false};
        bool center{true};
        std::string icon;                     // 空表示省略

        // whiz 扩展配置
        bool dev_tools{false};
        std::string user_agent;               // 空表示省略
        std::string background_color{"#FFFFFF"};
        std::vector<std::string> expose;      // 空表示全量
    };

    // 将 init_config 序列化为 package.json 文本。
    // name / version / main / webSecurity / license 始终输出；
    // 可选字段仅在非默认值（或非空）时输出。
    std::string generate_package_json(const init_config &cfg);

    // 在 target_dir 下执行 “whiz init”。
    // yes 为 true 时跳过交互，直接使用默认值（类似 npm init -y）。
    // 返回进程退出码：0 成功，非 0 失败。
    int run_init(const fs::path &target_dir, bool yes);
} // namespace whiz
