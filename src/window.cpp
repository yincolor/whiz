#include "whiz/window.hpp"
#include "whiz/logger.hpp"

#include <saucer/smartview.hpp>
#include <saucer/icon.hpp>
#include <saucer/modules/desktop.hpp>

#include <nlohmann/json.hpp>

#include <array>
#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#if defined(SAUCER_WEBKITGTK)
    #include <saucer/modules/stable/webkitgtk.hpp>
    #include <webkit/webkit.h>
#endif

namespace whiz
{
    namespace
    {
        constexpr int k_exit_saucer = 5;

        namespace picker = saucer::modules::picker;

        // 平台标识
        std::string platform_name()
        {
#if defined(_WIN32)
            return "win32";
#elif defined(__APPLE__)
            return "darwin";
#else
            return "linux";
#endif
        }

        // 用户数据目录：<用户家目录>/.local/share/whiz/<应用名>。
        // 跨平台一致：POSIX 取 $HOME，Windows 取 %USERPROFILE%，统一拼接 .local/share/whiz。
        // 返回空路径表示无法定位主目录（此时回退到 saucer 默认行为）。
        fs::path user_data_dir(const std::string &app_name)
        {
            const char *home = nullptr;
#if defined(_WIN32)
            home = std::getenv("USERPROFILE");
#else
            home = std::getenv("HOME");
#endif
            if (home == nullptr || *home == '\0')
            {
                return {};
            }
            return fs::path(home) / ".local" / "share" / "whiz" / app_name;
        }

        // 解析十六进制颜色 "#RRGGBB" -> saucer::color (RGBA)
        saucer::color parse_color(const std::string &hex)
        {
            saucer::color c{0xFF, 0xFF, 0xFF, 0xFF};
            std::string s = hex;
            if (s.starts_with('#'))
            {
                s.erase(s.begin());
            }
            if (s.size() == 6)
            {
                auto hex_byte = [](char hi, char lo) -> std::uint8_t {
                    const std::array<char, 2> digits{hi, lo};
                    unsigned value = 0;
                    std::from_chars(digits.data(), digits.data() + digits.size(), value, 16);
                    return static_cast<std::uint8_t>(value);
                };
                c = {hex_byte(s[0], s[1]), hex_byte(s[2], s[3]), hex_byte(s[4], s[5]), 0xFF};
            }
            return c;
        }

        // 读取脚本文件内容
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

        // 定位桥接脚本（优先编译目录，其次源码目录）
        std::string locate_bridge_script()
        {
            std::vector<std::string> candidates = {
                "scripts/bridge.js",
                "../scripts/bridge.js",
                "../../scripts/bridge.js",
            };
            for (const auto &c : candidates)
            {
                std::string content = read_file(c);
                if (!content.empty())
                {
                    return content;
                }
            }
            return {};
        }

        // Electron 风格对话框选项 -> saucer-desktop 的 picker 配置
        struct dialog_request
        {
            saucer::modules::picker::type type{saucer::modules::picker::type::file};
            saucer::modules::picker::options options{};
        };

        dialog_request parse_dialog_options(const std::string &json_text, bool is_save)
        {
            using enum picker::type;

            dialog_request req;
            req.type = is_save ? save : file;

            if (json_text.empty())
            {
                return req;
            }

            nlohmann::json root;
            try
            {
                root = nlohmann::json::parse(json_text);
            }
            catch (const nlohmann::json::exception &)
            {
                return req;
            }

            if (!root.is_object())
            {
                return req;
            }

            if (auto it = root.find("defaultPath"); it != root.end() && it->is_string())
            {
                req.options.initial = fs::path(it->get<std::string>());
            }

            if (!is_save && root.contains("properties") && root["properties"].is_array())
            {
                for (const auto &prop : root["properties"])
                {
                    if (!prop.is_string())
                    {
                        continue;
                    }

                    const std::string value = prop.get<std::string>();
                    if (value == "openDirectory")
                    {
                        req.type = folder;
                    }
                    else if (value == "multiSelections" && req.type != folder)
                    {
                        req.type = files;
                    }
                }
            }

            if (root.contains("filters") && root["filters"].is_array())
            {
                req.options.filters.clear();
                for (const auto &filter : root["filters"])
                {
                    if (!filter.is_object() || !filter.contains("extensions") || !filter["extensions"].is_array())
                    {
                        continue;
                    }

                    for (const auto &ext : filter["extensions"])
                    {
                        if (!ext.is_string())
                        {
                            continue;
                        }

                        std::string pattern = ext.get<std::string>();
                        if (!pattern.empty() && pattern.front() != '.')
                        {
                            pattern.insert(pattern.begin(), '.');
                        }
                        req.options.filters.insert(std::format("*{}", pattern));
                    }
                }
            }

            if (req.options.filters.empty())
            {
                req.options.filters.insert("*");
            }

            return req;
        }

        std::string dialog_open_result(saucer::modules::desktop &desktop, const dialog_request &req)
        {
            using enum picker::type;

            nlohmann::json result;
            result["canceled"]  = true;
            result["filePaths"] = nlohmann::json::array();

            switch (req.type)
            {
            case file:
            {
                if (auto picked = desktop.pick<picker::type::file>(req.options))
                {
                    result["canceled"] = false;
                    result["filePaths"].push_back(picked->string());
                }
                break;
            }
            case files:
            {
                if (auto picked = desktop.pick<picker::type::files>(req.options))
                {
                    std::vector<std::string> paths;
                    paths.reserve(picked->size());
                    for (const auto &path : *picked)
                    {
                        paths.emplace_back(path.string());
                    }

                    result["canceled"]  = false;
                    result["filePaths"] = paths;
                }
                break;
            }
            case folder:
            {
                if (auto picked = desktop.pick<picker::type::folder>(req.options))
                {
                    result["canceled"] = false;
                    result["filePaths"].push_back(picked->string());
                }
                break;
            }
            default:
                break;
            }

            return result.dump();
        }

        std::string dialog_save_result(saucer::modules::desktop &desktop, const dialog_request &req)
        {
            nlohmann::json result;
            result["canceled"] = true;
            result["filePath"] = "";

            if (auto picked = desktop.pick<picker::type::save>(req.options))
            {
                result["canceled"] = false;
                result["filePath"] = picked->string();
            }

            return result.dump();
        }

#if defined(SAUCER_WEBKITGTK)
        void set_gtk_window_icon(GtkWindow *gtk_window, const std::string &icon_path)
        {
            auto *surface = gtk_native_get_surface(GTK_NATIVE(gtk_window));
            if (!surface)
            {
                logger::warn("窗口图标设置失败：无法获取 GdkSurface");
                return;
            }
            logger::debug(std::format("获取到 GdkSurface: {}", static_cast<const void *>(surface)));

            auto *toplevel = GDK_TOPLEVEL(surface);
            if (!toplevel)
            {
                logger::warn(std::format("窗口图标设置失败：GdkSurface 不是 GdkToplevel: {}",
                                         static_cast<const void *>(surface)));
                return;
            }
            logger::debug(std::format("获取到 GdkToplevel: {}", static_cast<const void *>(toplevel)));

            GError *error = nullptr;
            GdkTexture *texture = gdk_texture_new_from_filename(icon_path.c_str(), &error);
            if (!texture)
            {
                logger::warn(std::format("无法加载窗口图标 {}: {}", icon_path,
                                         error ? error->message : "未知错误（无 GError 信息）"));
                if (error)
                {
                    g_error_free(error);
                }
                return;
            }

            logger::debug(std::format("已加载窗口图标纹理: {}x{}",
                                      gdk_texture_get_width(texture), gdk_texture_get_height(texture)));

            GList *icons = g_list_append(nullptr, texture);
            gdk_toplevel_set_icon_list(toplevel, icons);
            g_list_free(icons);
            g_object_unref(texture);
            logger::debug("已通过 gdk_toplevel_set_icon_list 设置窗口图标");
        }

        void on_window_realize(GtkWidget *widget, gpointer user_data)
        {
            auto *icon_path = static_cast<std::string *>(user_data);
            if (!icon_path)
            {
                logger::warn("窗口图标设置失败：realize 回调缺少图标路径");
                return;
            }

            set_gtk_window_icon(GTK_WINDOW(widget), *icon_path);
        }
#endif

        void apply_window_icon(saucer::window &webview, const fs::path &icon_path)
        {
            logger::debug(std::format("尝试设置窗口图标: {}", icon_path.string()));

            std::error_code ec;
            if (!fs::exists(icon_path, ec))
            {
                logger::warn(std::format("窗口图标文件不存在: {}", icon_path.string()));
                return;
            }
            if (ec)
            {
                logger::warn(std::format("检查窗口图标文件失败: {} ({})", icon_path.string(), ec.message()));
                return;
            }
            if (!fs::is_regular_file(icon_path, ec))
            {
                logger::warn(std::format("窗口图标路径不是普通文件: {}", icon_path.string()));
                return;
            }

#if defined(SAUCER_WEBKITGTK)
            // saucer 在 GTK 后端的 window::set_icon 是空实现，
            // 这里直接通过 GTK4 的 GdkToplevel 设置窗口图标。
            // 注意：必须等 GTK 窗口 realize（底层 GdkSurface/XID 创建完成）后再设置，
            // 否则 gdk_toplevel_set_icon_list 会因 surface 未就绪而被忽略。
            auto *gtk_window = webview.native<true>().window;
            if (!gtk_window)
            {
                logger::warn("窗口图标设置失败：无法获取 GtkWindow");
                return;
            }
            logger::debug(std::format("获取到 GtkWindow: {}", static_cast<const void *>(gtk_window)));

            if (gtk_widget_get_realized(GTK_WIDGET(gtk_window)))
            {
                set_gtk_window_icon(gtk_window, icon_path.string());
            }
            else
            {
                g_signal_connect_data(GTK_WIDGET(gtk_window), "realize", G_CALLBACK(+on_window_realize),
                                      new std::string(icon_path.string()),
                                      [](gpointer data, GClosure *) { delete static_cast<std::string *>(data); },
                                      G_CONNECT_AFTER);
                logger::debug("窗口尚未 realize，已注册 realize 信号，待 surface 创建后设置图标");
            }
#else
            if (auto icon = saucer::icon::from(icon_path))
            {
                webview.set_icon(*icon);
                logger::debug("已通过 saucer::window::set_icon 设置窗口图标");
            }
            else
            {
                logger::warn(std::format("无法加载窗口图标: {}", icon_path.string()));
            }
#endif
        }
    } // namespace

    window_error run_window(const manifest &cfg, const std::string &force_url)
    {
        try
        {
            // 1. 初始化 saucer application
            auto app = saucer::application::init({
                .id   = cfg.name.empty() ? "whiz.app" : std::format("whiz.{}", cfg.name),
                .argc = std::nullopt,
                .argv = std::nullopt,
            });
            if (!app)
            {
                return {k_exit_saucer, "saucer 初始化失败：无法创建 application"};
            }

            // 2. 添加 desktop 模块（支撑 window.whiz.dialog）
            auto &desktop = app->add_module<saucer::modules::desktop>();

            // 3. 计算持久化存储目录并创建 smartview
            fs::path storage = user_data_dir(cfg.name.empty() ? "whiz-app" : cfg.name);
            if (!storage.empty())
            {
                std::error_code ec;
                fs::create_directories(storage, ec);
                if (!ec)
                {
                    logger::debug(std::format("用户数据目录: {}", storage.string()));
                }
                else
                {
                    logger::warn(std::format("无法创建用户数据目录: {}", storage.string()));
                    storage.clear();
                }
            }

            // storage_path 语义因后端而异：WebKitGTK 期望一个 SQLite 数据库
            // “文件”路径，而 WebView2 / Qt 则将其当作目录使用。
            fs::path storage_path = storage;
#if defined(SAUCER_WEBKITGTK)
            if (!storage_path.empty())
            {
                storage_path /= "cookies.sqlite";
            }
#endif

            saucer::smartview webview{{
                .application  = app,
                .storage_path = storage_path,
                .user_agent   = {},
                .browser_flags = {},
            }};

            // 4. 应用窗口配置
            webview.set_title(cfg.window.title);
            webview.set_size(cfg.window.width, cfg.window.height);

            if (cfg.window.min_width > 0 || cfg.window.min_height > 0)
            {
                webview.set_min_size(cfg.window.min_width > 0 ? cfg.window.min_width : 0,
                                     cfg.window.min_height > 0 ? cfg.window.min_height : 0);
            }
            if (cfg.window.max_width > 0 || cfg.window.max_height > 0)
            {
                webview.set_max_size(cfg.window.max_width > 0 ? cfg.window.max_width : 0,
                                     cfg.window.max_height > 0 ? cfg.window.max_height : 0);
            }

            if (!cfg.window.icon.empty())
            {
                fs::path icon_path = cfg.window.icon;
                if (icon_path.is_relative())
                {
                    icon_path = cfg.project_root / icon_path;
                }

                apply_window_icon(webview, icon_path);
            }
            else
            {
                logger::debug("package.json 未配置 window.icon，使用系统默认图标");
            }

            webview.set_resizable(cfg.window.resizable);

            if (cfg.window.frameless)
            {
                webview.set_decorations(false);
            }

            if (cfg.window.always_on_top)
            {
                webview.set_always_on_top(true);
            }

            if (cfg.window.fullscreen)
            {
                webview.set_maximized(true);
                logger::warn("fullscreen 降级为最大化窗口（saucer 无原生 fullscreen API）");
            }

            webview.set_background(parse_color(cfg.whiz.background_color));

            // user agent 覆盖
            if (!cfg.whiz.user_agent.empty())
            {
                logger::warn("userAgent 覆盖暂未生效（saucer 需在 preferences 中设置）");
            }

            // 5. webSecurity 处理
            if (!cfg.web_security)
            {
#if defined(SAUCER_WEBKITGTK)
                auto native = webview.native<true>();
                if (auto *wv = native.webview)
                {
                    auto *settings = webkit_web_view_get_settings(wv);
                    webkit_settings_set_allow_file_access_from_file_urls(settings, true);
                    webkit_settings_set_allow_universal_access_from_file_urls(settings, true);
                    logger::debug("已解除同源策略（WebKitGTK）");
                }
#else
                logger::warn("webSecurity=false 在当前平台暂未实现，保持默认安全策略");
#endif
            }

            // 6. devtools
            if (cfg.whiz.dev_tools)
            {
                webview.set_dev_tools(true);
            }

            // 7. 注入 window.whiz 桥接脚本（DOM ready 之前）
            std::string bridge = locate_bridge_script();
            if (bridge.empty())
            {
                logger::warn("未找到 scripts/bridge.js，window.whiz 命名空间不可用");
            }
            else
            {
                // 替换版本与平台占位符
                auto replace = [](std::string &s, const std::string &from, const std::string &to) {
                    size_t pos = 0;
                    while ((pos = s.find(from, pos)) != std::string::npos)
                    {
                        s.replace(pos, from.size(), to);
                        pos += to.size();
                    }
                };
                replace(bridge, "__WHIZ_VERSION__", "0.1.0");
                replace(bridge, "__WHIZ_PLATFORM__", platform_name());

                webview.inject(saucer::script{
                    .code = bridge,
                    .time = saucer::load_time::creation,
                });
                logger::debug("已注入 window.whiz 桥接脚本");
            }

            // 8. 暴露原生函数
            // app
            webview.expose("whiz_app_quit", [app]() -> std::expected<void, std::string> {
                app->quit();
                return {};
            });

            webview.expose("whiz_app_get_path", [](const std::string &name) -> std::expected<std::string, std::string> {
                if (name == "home")
                {
                    return fs::path(std::getenv("HOME") ? std::getenv("HOME") : ".").string();
                }
                if (name == "temp" || name == "tmp")
                {
                    return fs::temp_directory_path().string();
                }
                if (name == "cwd")
                {
                    return fs::current_path().string();
                }
                return std::unexpected{std::format("unknown path name: {}", name)};
            });

            // window
            webview.expose("whiz_window_set_title", [&webview](const std::string &text) -> std::expected<void, std::string> {
                webview.set_title(text);
                return {};
            });

            webview.expose("whiz_window_get_title", [&webview]() -> std::expected<std::string, std::string> {
                return webview.title();
            });

            webview.expose("whiz_window_set_size", [&webview](int w, int h) -> std::expected<void, std::string> {
                webview.set_size(w, h);
                return {};
            });

            webview.expose("whiz_window_get_size", [&webview]() -> std::expected<std::array<int, 2>, std::string> {
                auto [w, h] = webview.size();
                return std::array<int, 2>{w, h};
            });

            webview.expose("whiz_window_minimize", [&webview]() -> std::expected<void, std::string> {
                webview.set_minimized(true);
                return {};
            });

            webview.expose("whiz_window_maximize", [&webview]() -> std::expected<void, std::string> {
                webview.set_maximized(true);
                return {};
            });

            webview.expose("whiz_window_unmaximize", [&webview]() -> std::expected<void, std::string> {
                webview.set_maximized(false);
                return {};
            });

            webview.expose("whiz_window_set_always_on_top", [&webview](bool v) -> std::expected<void, std::string> {
                webview.set_always_on_top(v);
                return {};
            });

            webview.expose("whiz_window_focus", [&webview]() -> std::expected<void, std::string> {
                webview.focus();
                return {};
            });

            webview.expose("whiz_window_close", [&webview]() -> std::expected<void, std::string> {
                webview.close();
                return {};
            });

            // 暂未实现：set_position / get_position / center / blur / set_fullscreen
            webview.expose("whiz_window_set_position", [](int, int) -> std::expected<void, std::string> {
                return std::unexpected{std::string("setPosition 暂未实现")};
            });
            webview.expose("whiz_window_get_position", []() -> std::expected<std::array<int, 2>, std::string> {
                return std::array<int, 2>{-1, -1};
            });
            webview.expose("whiz_window_center", []() -> std::expected<void, std::string> {
                return std::unexpected{std::string("center 暂未实现")};
            });
            webview.expose("whiz_window_set_fullscreen", [&webview](bool v) -> std::expected<void, std::string> {
                webview.set_maximized(v);
                return {};
            });
            webview.expose("whiz_window_blur", []() -> std::expected<void, std::string> {
                return std::unexpected{std::string("blur 暂未实现")};
            });

            // dialog
            webview.expose("whiz_dialog_open",
                           [&desktop](const std::string &options) -> std::expected<std::string, std::string> {
                return dialog_open_result(desktop, parse_dialog_options(options, false));
            });

            webview.expose("whiz_dialog_save",
                           [&desktop](const std::string &options) -> std::expected<std::string, std::string> {
                return dialog_save_result(desktop, parse_dialog_options(options, true));
            });

            // 9. 监听 ready -> show（无闪烁启动）
            bool auto_show = !cfg.window.show;
            if (auto_show)
            {
                webview.on<saucer::web_event::dom_ready>([&webview]() {
                    webview.show();
                });
            }

            // 10. 加载页面
            if (!force_url.empty())
            {
                webview.set_url(force_url);
            }
            else
            {
                webview.set_file(cfg.entry_html);
            }

            if (cfg.window.show)
            {
                webview.show();
            }

            // 11. 进入事件循环
            app->run();

            return {0, ""};
        }
        catch (const std::exception &e)
        {
            return {k_exit_saucer, std::format("saucer 运行异常: {}", e.what())};
        }
    }
} // namespace whiz
