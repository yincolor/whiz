#include "whiz/manifest.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>

namespace whiz
{
    namespace
    {
        constexpr int k_exit_bad_manifest = 3;

        using json = nlohmann::json;

        // 安全读取字符串字段
        std::optional<std::string> get_string(const json &obj, const char *key)
        {
            if (!obj.is_object() || !obj.contains(key))
            {
                return std::nullopt;
            }
            const auto &v = obj[key];
            if (v.is_string())
            {
                return v.get<std::string>();
            }
            return std::nullopt;
        }

        // 安全读取整数字段
        std::optional<int> get_int(const json &obj, const char *key)
        {
            if (!obj.is_object() || !obj.contains(key))
            {
                return std::nullopt;
            }
            const auto &v = obj[key];
            if (v.is_number_integer())
            {
                return v.get<int>();
            }
            if (v.is_number_unsigned())
            {
                return static_cast<int>(v.get<unsigned>());
            }
            if (v.is_number_float())
            {
                return static_cast<int>(v.get<double>());
            }
            return std::nullopt;
        }

        // 安全读取布尔字段
        std::optional<bool> get_bool(const json &obj, const char *key)
        {
            if (!obj.is_object() || !obj.contains(key))
            {
                return std::nullopt;
            }
            const auto &v = obj[key];
            if (v.is_boolean())
            {
                return v.get<bool>();
            }
            return std::nullopt;
        }

        void parse_window(const json &win, window_config &cfg)
        {
            if (!win.is_object())
            {
                return;
            }
            if (auto v = get_string(win, "title"); v) cfg.title = *v;
            if (auto v = get_int(win, "width"); v) cfg.width = *v;
            if (auto v = get_int(win, "height"); v) cfg.height = *v;
            if (auto v = get_int(win, "minWidth"); v) cfg.min_width = *v;
            if (auto v = get_int(win, "minHeight"); v) cfg.min_height = *v;
            if (auto v = get_int(win, "maxWidth"); v) cfg.max_width = *v;
            if (auto v = get_int(win, "maxHeight"); v) cfg.max_height = *v;
            if (auto v = get_int(win, "x"); v) cfg.x = *v;
            if (auto v = get_int(win, "y"); v) cfg.y = *v;
            if (auto v = get_bool(win, "resizable"); v) cfg.resizable = *v;
            if (auto v = get_bool(win, "frameless"); v) cfg.frameless = *v;
            if (auto v = get_bool(win, "transparent"); v) cfg.transparent = *v;
            if (auto v = get_bool(win, "alwaysOnTop"); v) cfg.always_on_top = *v;
            if (auto v = get_bool(win, "fullscreen"); v) cfg.fullscreen = *v;
            if (auto v = get_bool(win, "show"); v) cfg.show = *v;
            if (auto v = get_bool(win, "center"); v) cfg.center = *v;
            if (auto v = get_string(win, "icon"); v) cfg.icon = *v;
        }

        void parse_whiz(const json &w, whiz_config &cfg)
        {
            if (!w.is_object())
            {
                return;
            }
            if (auto v = get_bool(w, "devTools"); v) cfg.dev_tools = *v;
            if (auto v = get_string(w, "userAgent"); v) cfg.user_agent = *v;
            if (auto v = get_string(w, "backgroundColor"); v) cfg.background_color = *v;

            if (w.contains("expose") && w["expose"].is_array())
            {
                cfg.expose.clear();
                for (const auto &item : w["expose"])
                {
                    if (item.is_string())
                    {
                        cfg.expose.push_back(item.get<std::string>());
                    }
                }
            }
        }
    } // namespace

    bool manifest::expose(const std::string &module) const
    {
        if (expose_all())
        {
            return true;
        }
        return std::ranges::find(whiz.expose, module) != whiz.expose.end();
    }

    manifest_error parse_manifest(const std::string &json_text, const fs::path &package_json_path,
                                  const fs::path &project_root, manifest &out)
    {
        out.project_root = project_root;
        out.package_json_path = package_json_path;

        if (json_text.empty())
        {
            // 空内容：使用默认值
            return {0, ""};
        }

        json root;
        try
        {
            root = json::parse(json_text);
        }
        catch (const json::parse_error &e)
        {
            return {k_exit_bad_manifest, std::format("package.json 解析失败: {}", e.what())};
        }

        if (!root.is_object())
        {
            return {k_exit_bad_manifest, "package.json 顶层必须是对象"};
        }

        if (auto v = get_string(root, "name"); v)
        {
            out.name = *v;
            out.has_name = true;
        }
        if (auto v = get_string(root, "version"); v)
        {
            out.version = *v;
            out.has_version = true;
        }
        if (auto v = get_string(root, "main"); v)
        {
            out.main = *v;
            out.has_main = true;
        }
        if (auto v = get_bool(root, "webSecurity"); v)
        {
            out.web_security = *v;
            out.has_web_security = true;
        }

        // 窗口标题兜底为 name
        if (out.has_name)
        {
            out.window.title = out.name;
        }

        if (root.contains("window"))
        {
            parse_window(root["window"], out.window);
        }
        if (root.contains("whiz"))
        {
            parse_whiz(root["whiz"], out.whiz);
        }

        // 再次兜底：若 window.title 未单独设置且 name 存在，使用 name
        if (out.has_name && !root.contains("window"))
        {
            out.window.title = out.name;
        }
        else if (out.has_name && root.contains("window"))
        {
            const auto &win = root["window"];
            if (win.is_object() && !win.contains("title"))
            {
                out.window.title = out.name;
            }
        }

        return {0, ""};
    }
} // namespace whiz
