#include "whiz/cli.hpp"
#include "whiz/init.hpp"
#include "whiz/logger.hpp"
#include "whiz/manifest.hpp"
#include "whiz/paths.hpp"
#include "whiz/window.hpp"

#include <format>
#include <fstream>
#include <iostream>
#include <print>
#include <sstream>
#include <string>

namespace
{
    // 退出码约定
    constexpr int k_exit_ok          = 0;
    constexpr int k_exit_bad_args    = 1;
    constexpr int k_exit_bad_dir     = 2;
    constexpr int k_exit_bad_manifest = 3;
    constexpr int k_exit_no_entry    = 4;
    constexpr int k_exit_saucer      = 5;

    std::string read_file(const std::string &path)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in)
        {
            return {};
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

    // 将命令行覆盖项合并进 manifest
    void apply_cli_overrides(const whiz::cli_options &opts, whiz::manifest &m)
    {
        if (opts.has_title)
        {
            m.window.title = opts.title;
        }
        if (opts.has_width && opts.width > 0)
        {
            m.window.width = opts.width;
        }
        if (opts.has_height && opts.height > 0)
        {
            m.window.height = opts.height;
        }
        if (opts.no_web_security)
        {
            m.web_security = false;
        }
        if (opts.has_devtools)
        {
            m.whiz.dev_tools = opts.devtools;
        }
        if (opts.show)
        {
            m.window.show = true;
        }
    }
} // namespace

int main(int argc, char **argv)
{
    using namespace whiz;

    // 1. 解析命令行
    cli_options opts;
    if (auto err = parse_cli(argc, argv, opts); err.exit_code != k_exit_ok)
    {
        std::println(std::cerr, "whiz: {}", err.message);
        if (err.exit_code == k_exit_bad_args)
        {
            std::println(std::cerr, "尝试 'whiz --help' 获取帮助。");
        }
        return err.exit_code;
    }

    if (opts.show_help)
    {
        std::print("{}", help_text());
        return k_exit_ok;
    }
    if (opts.show_version)
    {
        std::println("{}", version_text());
        return k_exit_ok;
    }

    logger::set_verbose(opts.verbose);

    // 2. init 子命令：在当前目录交互式创建 package.json
    if (opts.init_mode)
    {
        return run_init(fs::current_path(), opts.init_yes);
    }

    // 3. 若提供 --url，跳过 package.json 解析，直接加载 URL
    if (opts.has_url)
    {
        logger::info(std::format("使用 --url 直接加载: {}", opts.url));
        manifest direct;
        apply_cli_overrides(opts, direct);
        if (auto err = run_window(direct, opts.url); err.exit_code != k_exit_ok)
        {
            logger::error(err.message);
            return err.exit_code;
        }
        return k_exit_ok;
    }

    // 4. 解析项目目录
    std::string project_arg = opts.has_project_dir ? opts.project_dir : ".";
    resolved_paths paths;
    if (auto err = resolve_project_path_or_error(project_arg, paths); err.exit_code != k_exit_ok)
    {
        logger::error(err.message);
        return err.exit_code;
    }

    logger::debug(std::format("项目根目录: {}", paths.project_root.string()));

    // 5. 读取并解析 package.json（若存在）
    manifest m;
    m.project_root = paths.project_root;
    m.package_json_path = paths.package_json;

    if (paths.has_package_json)
    {
        std::string text = read_file(paths.package_json.string());
        if (text.empty() && !paths.package_json.empty())
        {
            // 文件存在但读取失败
            logger::error(std::format("无法读取 package.json: {}", paths.package_json.string()));
            return k_exit_bad_manifest;
        }
        if (auto err = parse_manifest(text, paths.package_json, paths.project_root, m); err.exit_code != k_exit_ok)
        {
            logger::error(err.message);
            return err.exit_code;
        }
        if (!m.has_name || m.name.empty())
        {
            logger::error("package.json 缺少 name 属性（或 name 为空/不是字符串）");
            return k_exit_bad_manifest;
        }
        if (m.name == "." || m.name == ".." ||
            m.name.find('/') != std::string::npos || m.name.find('\\') != std::string::npos)
        {
            logger::error(std::format("package.json 的 name 属性非法: {}", m.name));
            return k_exit_bad_manifest;
        }
        if (m.name == "whiz-app")
        {
            logger::error("package.json 的 name 不能使用保留名 \"whiz-app\"（与 --url 直连模式的默认名冲突）");
            return k_exit_bad_manifest;
        }
        logger::debug(std::format("解析 package.json: {} v{}", m.name, m.version));
    }
    else
    {
        // 无 package.json：直接报错退出，不再回退到默认配置。
        logger::error(std::format("未找到 package.json: {}", paths.project_root.string()));
        return k_exit_bad_manifest;
    }

    // 6. 解析入口 HTML
    auto entry = resolve_entry_html(paths.project_root, m.main);
    if (!entry)
    {
        logger::error(std::format("无法定位入口 HTML（main={}），已依次尝试 index.html / index.htm / main.html", m.main));
        return k_exit_no_entry;
    }
    m.entry_html = *entry;
    logger::debug(std::format("入口 HTML: {}", m.entry_html.string()));

    // 若 main 指向 .js 文件，直接报错（不提供 Node 运行时）
    if (m.has_main)
    {
        std::string ext = m.entry_html.extension().string();
        // 注意：resolve_entry_html 已确保是 .html/.htm；此处防御
        if (ext == ".js")
        {
            logger::error("main 指向 .js 文件，whiz 不提供 Node 运行时，请改用 HTML 入口并通过 <script> 引入脚本");
            return k_exit_no_entry;
        }
    }

    // 7. 应用命令行覆盖
    apply_cli_overrides(opts, m);

    // 8. 运行窗口
    if (auto err = run_window(m, ""); err.exit_code != k_exit_ok)
    {
        logger::error(err.message);
        return err.exit_code;
    }

    return k_exit_ok;
}
