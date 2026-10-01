#include "whiz/init.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <expected>
#include <format>
#include <fstream>
#include <iostream>
#include <print>
#include <string>
#include <system_error>

namespace whiz
{
    namespace
    {
        using json = nlohmann::ordered_json;

        std::string trim(std::string s)
        {
            auto not_space = [](unsigned char c) { return !std::isspace(c); };
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
            s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
            return s;
        }

        std::string lower(std::string s)
        {
            std::ranges::transform(s, s.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return s;
        }

        std::vector<std::string> split_list(const std::string &text)
        {
            std::vector<std::string> out;
            std::size_t start = 0;
            while (start <= text.size())
            {
                std::size_t pos = text.find(',', start);
                std::string token = trim(text.substr(
                    start, pos == std::string::npos ? std::string::npos : pos - start));
                if (!token.empty())
                {
                    out.push_back(std::move(token));
                }
                if (pos == std::string::npos)
                {
                    break;
                }
                start = pos + 1;
            }
            return out;
        }

        std::expected<std::string, std::string> default_project_name(const fs::path &dir)
        {
            const std::string raw = dir.filename().string();
            if (raw.empty() || raw == "." || raw == "..")
            {
                return std::unexpected{std::format(
                    "无法从目录名推断项目名：目录 '{}' 没有可用名称（不能为空、'.' 或 '..'），请在名称明确的目录中运行 whiz init",
                    dir.string())};
            }

            std::string s = lower(trim(raw));
            std::string out;
            bool last_dash = false;
            for (unsigned char c : s)
            {
                if (std::isalnum(c) || c == '_' || c == '-')
                {
                    out += static_cast<char>(c);
                    last_dash = (c == '-');
                }
                else if (c == '.')
                {
                    out += '.';
                    last_dash = false;
                }
                else if (!out.empty() && !last_dash)
                {
                    out += '-';
                    last_dash = true;
                }
            }

            while (!out.empty() && (out.front() == '-' || out.front() == '.'))
            {
                out.erase(out.begin());
            }
            while (!out.empty() && (out.back() == '-' || out.back() == '.'))
            {
                out.pop_back();
            }

            if (out.empty())
            {
                return std::unexpected{std::format(
                    "目录名 '{}' 不包含可用于项目名的字符（仅支持字母、数字、_、-、.），请修改目录名后重试",
                    raw)};
            }
            if (out == "whiz-app")
            {
                return std::unexpected{std::format(
                    "目录名 '{}' 推断出的项目名 'whiz-app' 是保留名（与 --url 直连模式的默认名冲突），请更换目录名后重试",
                    raw)};
            }

            return out;
        }

        std::string prompt_string(const std::string &label, const std::string &def)
        {
            if (def.empty())
            {
                std::print("{}: ", label);
            }
            else
            {
                std::print("{}: ({}) ", label, def);
            }

            std::string line;
            std::getline(std::cin, line);
            line = trim(std::move(line));
            return line.empty() ? def : line;
        }

        bool prompt_bool(const std::string &label, bool def)
        {
            while (true)
            {
                std::print("{}: ({}) ", label, def ? "Y/n" : "y/N");
                std::string line;
                std::getline(std::cin, line);
                line = lower(trim(std::move(line)));

                if (line.empty())
                {
                    return def;
                }
                if (line == "y" || line == "yes")
                {
                    return true;
                }
                if (line == "n" || line == "no")
                {
                    return false;
                }

                std::println("  请输入 y 或 n");
            }
        }

        std::vector<std::string> prompt_list(const std::string &label, const std::string &def)
        {
            std::string hint = def.empty() ? "留空 = 全量" : def;
            std::print("{}: ({}) ", label, hint);

            std::string line;
            std::getline(std::cin, line);
            line = trim(std::move(line));

            return line.empty() ? split_list(def) : split_list(line);
        }

        init_config collect_config_interactive(init_config cfg)
        {
            std::println("此命令将引导你创建一个 whiz 的 package.json 文件。");
            std::println("它只覆盖 whiz 会读取的字段，括号中为默认值，直接回车表示使用默认值。");
            std::println("按 Ctrl+C 可随时退出。");
            std::println("");

            cfg.name = prompt_string("package name", cfg.name);
            cfg.version = prompt_string("version", cfg.version);
            cfg.description = prompt_string("description", cfg.description);
            cfg.license = prompt_string("license", cfg.license);
            cfg.main = prompt_string("entry HTML file (main)", cfg.main);
            cfg.web_security = prompt_bool("webSecurity", cfg.web_security);

            std::println("");
            std::println("whiz 扩展配置 (whiz):");
            cfg.dev_tools = prompt_bool("devTools", cfg.dev_tools);
            cfg.user_agent = prompt_string("userAgent", cfg.user_agent);
            cfg.background_color = prompt_string("backgroundColor", cfg.background_color);
            cfg.expose = prompt_list("expose（逗号分隔，留空 = 全量）", "");

            return cfg;
        }
    } // namespace

    std::string generate_package_json(const init_config &cfg)
    {
        json root;
        root["name"] = cfg.name;
        root["version"] = cfg.version;
        root["main"] = cfg.main;
        root["webSecurity"] = cfg.web_security;
        root["license"] = cfg.license;

        if (!cfg.description.empty())
        {
            root["description"] = cfg.description;
        }

        json window = json::object();
        if (!cfg.title.empty() && cfg.title != cfg.name)
        {
            window["title"] = cfg.title;
        }
        if (cfg.width != 1024) window["width"] = cfg.width;
        if (cfg.height != 768) window["height"] = cfg.height;
        if (cfg.min_width != 0) window["minWidth"] = cfg.min_width;
        if (cfg.min_height != 0) window["minHeight"] = cfg.min_height;
        if (cfg.max_width != 0) window["maxWidth"] = cfg.max_width;
        if (cfg.max_height != 0) window["maxHeight"] = cfg.max_height;
        if (cfg.x != -1) window["x"] = cfg.x;
        if (cfg.y != -1) window["y"] = cfg.y;
        if (!cfg.resizable) window["resizable"] = false;
        if (cfg.frameless) window["frameless"] = true;
        if (cfg.transparent) window["transparent"] = true;
        if (cfg.always_on_top) window["alwaysOnTop"] = true;
        if (cfg.fullscreen) window["fullscreen"] = true;
        if (cfg.show) window["show"] = true;
        if (!cfg.center) window["center"] = false;
        if (!cfg.icon.empty()) window["icon"] = cfg.icon;
        if (!window.empty())
        {
            root["window"] = window;
        }

        json whiz = json::object();
        if (cfg.dev_tools) whiz["devTools"] = true;
        if (!cfg.user_agent.empty()) whiz["userAgent"] = cfg.user_agent;
        if (cfg.background_color != "#FFFFFF") whiz["backgroundColor"] = cfg.background_color;
        if (!cfg.expose.empty()) whiz["expose"] = cfg.expose;
        if (!whiz.empty())
        {
            root["whiz"] = whiz;
        }

        return root.dump(2);
    }

    int run_init(const fs::path &target_dir, bool yes)
    {
        fs::path pkg = target_dir / "package.json";

        std::error_code ec;
        if (fs::exists(pkg, ec))
        {
            bool overwrite = yes;
            if (!yes)
            {
                std::print("package.json 已存在，是否覆盖? (y/N) ");
                std::string line;
                std::getline(std::cin, line);
                line = lower(trim(std::move(line)));
                overwrite = (line == "y" || line == "yes");
            }
            if (!overwrite)
            {
                std::println(std::cerr, "已取消，未写入 package.json");
                return 1;
            }
        }
        else if (ec)
        {
            std::println(std::cerr, "无法访问 {}: {}", pkg.string(), ec.message());
            return 1;
        }

        init_config cfg;
        auto derived_name = default_project_name(target_dir);
        if (!derived_name)
        {
            std::println(std::cerr, "{}", derived_name.error());
            return 1;
        }
        cfg.name = derived_name.value();

        if (!yes)
        {
            cfg = collect_config_interactive(std::move(cfg));
        }

        std::string text = generate_package_json(cfg);

        if (!yes)
        {
            std::println("");
            std::println("即将写入 {}:", pkg.string());
            std::println("");
            std::println("{}", text);
            std::println("");
            if (!prompt_bool("是否确认写入?", true))
            {
                std::println("已取消，未写入 package.json");
                return 1;
            }
        }

        std::ofstream out(pkg, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            std::println(std::cerr, "无法写入 package.json: {}", pkg.string());
            return 1;
        }

        out << text << "\n";
        if (!out)
        {
            std::println(std::cerr, "写入 package.json 失败: {}", pkg.string());
            return 1;
        }

        std::println("已创建 {}", pkg.string());
        return 0;
    }
} // namespace whiz
