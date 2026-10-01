#include "test_framework.hpp"
#include "whiz/init.hpp"

#include <nlohmann/json.hpp>

using namespace whiz;

TEST(init_generate_minimal)
{
    init_config cfg;
    cfg.name = "demo";   // name 无默认值，需显式设置
    std::string text = generate_package_json(cfg);

    auto j = nlohmann::json::parse(text);
    test::check(j["name"] == "demo", "name 显式设置");
    test::check(j["version"] == "0.1.0", "version 默认值");
    test::check(j["main"] == "index.html", "main 默认值");
    test::check(j["webSecurity"] == true, "webSecurity 默认值");
    test::check(j["license"] == "MIT", "license 默认 MIT");
    test::check(!j.contains("window"), "默认不输出 window");
    test::check(!j.contains("whiz"), "默认不输出 whiz");
}

TEST(init_generate_custom)
{
    init_config cfg;
    cfg.name = "demo";
    cfg.version = "2.0.0";
    cfg.description = "A demo app";
    cfg.license = "Apache-2.0";
    cfg.main = "src/index.html";
    cfg.web_security = false;
    cfg.title = "Demo";
    cfg.width = 800;
    cfg.height = 600;
    cfg.dev_tools = true;
    cfg.background_color = "#1e1e1e";
    cfg.expose = {"dialog", "window"};

    auto j = nlohmann::json::parse(generate_package_json(cfg));
    test::check(j["name"] == "demo", "name");
    test::check(j["version"] == "2.0.0", "version");
    test::check(j["description"] == "A demo app", "description");
    test::check(j["license"] == "Apache-2.0", "license");
    test::check(j["main"] == "src/index.html", "main");
    test::check(j["webSecurity"] == false, "webSecurity=false");
    test::check(j["window"]["title"] == "Demo", "window.title");
    test::check(j["window"]["width"] == 800, "window.width");
    test::check(j["window"]["height"] == 600, "window.height");
    test::check(j["whiz"]["devTools"] == true, "whiz.devTools");
    test::check(j["whiz"]["backgroundColor"] == "#1e1e1e", "whiz.backgroundColor");
    test::check(j["whiz"]["expose"].size() == 2, "whiz.expose 大小");
}

TEST(init_generate_window_flags)
{
    init_config cfg;
    cfg.name = "flags";
    cfg.title = "flags";   // 与 name 相同，不输出 title
    cfg.resizable = false;
    cfg.frameless = true;
    cfg.always_on_top = true;
    cfg.show = true;
    cfg.center = false;
    cfg.x = 100;
    cfg.y = 200;

    auto j = nlohmann::json::parse(generate_package_json(cfg));
    test::check(!j["window"].contains("title"), "title 与 name 相同时省略");
    test::check(j["window"]["resizable"] == false, "resizable=false");
    test::check(j["window"]["frameless"] == true, "frameless=true");
    test::check(j["window"]["alwaysOnTop"] == true, "alwaysOnTop=true");
    test::check(j["window"]["show"] == true, "show=true");
    test::check(j["window"]["center"] == false, "center=false");
    test::check(j["window"]["x"] == 100, "x");
    test::check(j["window"]["y"] == 200, "y");
}
