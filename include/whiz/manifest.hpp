#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace whiz
{
    namespace fs = std::filesystem;

    struct window_config
    {
        std::string title{"whiz-app"};
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
        bool show{false};        // false 表示等 ready 后再显示
        bool center{true};
        std::string icon;        // 空表示未设置
    };

    struct whiz_config
    {
        bool dev_tools{false};
        std::string user_agent;                     // 空表示使用默认 UA
        std::string background_color{"#FFFFFF"};
        std::vector<std::string> expose;            // 空表示暴露全量模块
    };

    struct manifest
    {
        std::string name{"whiz-app"};
        std::string version{"0.0.0"};
        std::string main{"index.html"};
        bool web_security{true};

        bool has_name{false};
        bool has_version{false};
        bool has_main{false};
        bool has_web_security{false};

        window_config window;
        whiz_config whiz;

        fs::path project_root;                      // 项目根目录
        fs::path package_json_path;                 // package.json 路径（可能为空）
        fs::path entry_html;                        // 解析后的入口 HTML 绝对路径

        // 是否保留 web_security（用于区分用户显式设置 false / 默认 true）
        [[nodiscard]] bool expose_all() const { return whiz.expose.empty(); }
        [[nodiscard]] bool expose(const std::string &module) const;
    };

    // 解析 package.json 内容。project_root 用于后续解析入口。
    // 返回错误信息；成功时 msg 为空。
    struct manifest_error
    {
        int exit_code;
        std::string message;
    };

    // 从 package.json 文本解析 manifest。若 package.json 不存在则使用默认值。
    manifest_error parse_manifest(const std::string &json_text, const fs::path &package_json_path,
                                  const fs::path &project_root, manifest &out);
} // namespace whiz
