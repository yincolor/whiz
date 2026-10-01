#include "whiz/cli.hpp"

#include <charconv>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

namespace whiz
{
    namespace
    {
        constexpr int k_exit_ok       = 0;
        constexpr int k_exit_bad_args = 1;

        constexpr const char *k_version = "0.1.0";

        // 尝试解析非负整数参数，失败返回 false
        bool parse_int_arg(std::string_view text, int &out)
        {
            if (text.empty())
            {
                return false;
            }

            int value = 0;
            const char *begin = text.data();
            const char *end   = begin + text.size();
            auto [ptr, ec]    = std::from_chars(begin, end, value);

            if (ec != std::errc{} || ptr != end || value < 0)
            {
                return false;
            }

            out = value;
            return true;
        }
    } // namespace

    std::string help_text()
    {
        return
            "用法: whiz [选项] <项目目录>\n"
            "\n"
            "将指定 Web 项目目录（含 package.json 与入口 HTML）加载为原生桌面应用。\n"
            "省略 <项目目录> 时使用当前工作目录。\n"
            "\n"
            "选项:\n"
            "  -h, --help              打印帮助并退出\n"
            "  -v, --version           打印版本号并退出\n"
            "  -d, --devtools          启动时打开开发者工具\n"
            "      --url <url>         直接加载指定 URL，跳过 package.json 解析\n"
            "      --title <text>      覆盖窗口标题\n"
            "      --width <n>         覆盖窗口宽度（像素）\n"
            "      --height <n>        覆盖窗口高度（像素）\n"
            "      --no-web-security   强制解除跨域限制（等价 webSecurity: false）\n"
            "      --show              立即显示窗口，不等 ready 事件\n"
            "      --verbose           输出调试日志\n"
            "\n"
            "子命令:\n"
            "  init                    在当前目录交互式创建 package.json\n"
            "      --yes, -y           配合 init 使用：跳过询问，使用默认值\n";
    }

    std::string version_text()
    {
        return std::format("whiz {}", k_version);
    }

    cli_error parse_cli(int argc, char **argv, cli_options &out)
    {
        out = cli_options{};

        const std::span args{argv, static_cast<std::size_t>(argc)};
        std::size_t i = 1;

        auto need_value = [&](std::string_view flag, std::string &dst) -> cli_error {
            if (i + 1 >= args.size())
            {
                return {k_exit_bad_args, std::format("{} 需要一个参数", flag)};
            }
            dst = args[++i];
            return {k_exit_ok, ""};
        };

        for (; i < args.size(); ++i)
        {
            const std::string_view arg = args[i];

            if (arg == "--help" || arg == "-h")
            {
                out.show_help = true;
                return {k_exit_ok, ""};
            }
            if (arg == "--version" || arg == "-v")
            {
                out.show_version = true;
                return {k_exit_ok, ""};
            }
            if (arg == "--devtools" || arg == "-d")
            {
                out.devtools = true;
                out.has_devtools = true;
                continue;
            }
            if (arg == "--no-web-security")
            {
                out.no_web_security = true;
                continue;
            }
            if (arg == "--show")
            {
                out.show = true;
                continue;
            }
            if (arg == "--verbose")
            {
                out.verbose = true;
                continue;
            }
            if (arg == "--yes" || arg == "-y")
            {
                out.init_yes = true;
                continue;
            }
            if (arg == "--url")
            {
                std::string v;
                if (auto e = need_value("--url", v); e.exit_code != k_exit_ok) return e;
                out.url = v;
                out.has_url = true;
                continue;
            }
            if (arg == "--title")
            {
                std::string v;
                if (auto e = need_value("--title", v); e.exit_code != k_exit_ok) return e;
                out.title = v;
                out.has_title = true;
                continue;
            }
            if (arg == "--width")
            {
                std::string v;
                if (auto e = need_value("--width", v); e.exit_code != k_exit_ok) return e;
                if (!parse_int_arg(v, out.width))
                {
                    return {k_exit_bad_args, "--width 需要非负整数"};
                }
                out.has_width = true;
                continue;
            }
            if (arg == "--height")
            {
                std::string v;
                if (auto e = need_value("--height", v); e.exit_code != k_exit_ok) return e;
                if (!parse_int_arg(v, out.height))
                {
                    return {k_exit_bad_args, "--height 需要非负整数"};
                }
                out.has_height = true;
                continue;
            }

            // 未知选项
            if (!arg.empty() && arg.front() == '-')
            {
                return {k_exit_bad_args, std::format("未知选项: {}", arg)};
            }

            // 位置参数：init 子命令或项目目录
            if (arg == "init" && !out.has_project_dir)
            {
                out.init_mode = true;
                continue;
            }
            if (out.init_mode)
            {
                return {k_exit_bad_args, std::format("init 不接受额外的参数: {}", arg)};
            }
            if (out.has_project_dir)
            {
                return {k_exit_bad_args, std::format("只能指定一个项目目录: {}", arg)};
            }
            out.project_dir = arg;
            out.has_project_dir = true;
        }

        return {k_exit_ok, ""};
    }
} // namespace whiz
