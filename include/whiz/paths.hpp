#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace whiz
{
    namespace fs = std::filesystem;

    // 项目目录定位结果。path 是最终确定的项目根目录，
    // 用于解析 package.json 与入口 HTML。
    struct resolved_paths
    {
        fs::path project_root;     // 项目根目录（绝对路径）
        fs::path package_json;     // package.json 路径（若存在）
        bool has_package_json{false};
    };

    // 将用户传入的项目目录参数（可为目录 / package.json 路径 / 空）解析为规范路径。
    // 解析失败时返回错误信息。
    struct path_error
    {
        int exit_code;
        std::string message;
    };

    // 返回值：成功则 optional 有效，失败则 path_error 有效。
    std::optional<resolved_paths> resolve_project_path(const std::string &input);
    path_error resolve_project_path_or_error(const std::string &input, resolved_paths &out);

    // 从项目根目录解析入口 HTML 的绝对路径。
    // main_field 来自 package.json 的 "main"（可为空）。
    // 返回空 optional 表示无法定位入口。
    std::optional<fs::path> resolve_entry_html(const fs::path &project_root, const std::string &main_field);
} // namespace whiz
