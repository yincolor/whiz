#include "test_framework.hpp"
#include "whiz/manifest.hpp"

using namespace whiz;

TEST(manifest_basic)
{
    const char *json = R"({
        "name": "my-app",
        "version": "1.2.3",
        "main": "index.html",
        "webSecurity": false,
        "window": { "title": "T", "width": 100, "height": 200 }
    })";

    manifest m;
    auto err = parse_manifest(json, {}, {}, m);
    test::check(err.exit_code == 0, "解析应成功");
    test::check(m.name == "my-app", "name");
    test::check(m.version == "1.2.3", "version");
    test::check(m.main == "index.html", "main");
    test::check(m.web_security == false, "webSecurity=false");
    test::check(m.window.title == "T", "window.title");
    test::check(m.window.width == 100, "window.width");
    test::check(m.window.height == 200, "window.height");
}

TEST(manifest_defaults)
{
    const char *json = R"({ "name": "only-name" })";

    manifest m;
    auto err = parse_manifest(json, {}, {}, m);
    test::check(err.exit_code == 0, "解析应成功");
    test::check(m.main == "index.html", "main 默认 index.html");
    test::check(m.web_security == true, "webSecurity 默认 true");
    test::check(m.window.width == 1024, "width 默认 1024");
    test::check(m.window.height == 768, "height 默认 768");
    test::check(m.window.title == "only-name", "title 兜底为 name");
}

TEST(manifest_parse_error)
{
    manifest m;
    auto err = parse_manifest("{ not valid json", {}, {}, m);
    test::check(err.exit_code == 3, "非法 JSON 返回 exit_code 3");
}

TEST(manifest_expose_whitelist)
{
    const char *json = R"({ "whiz": { "expose": ["dialog", "window"] } })";

    manifest m;
    parse_manifest(json, {}, {}, m);
    test::check(m.expose("dialog"), "白名单含 dialog");
    test::check(m.expose("window"), "白名单含 window");
    test::check(!m.expose("app"), "白名单不含 app");
}

TEST(manifest_expose_all_when_empty)
{
    manifest m;
    test::check(m.expose_all(), "空 expose 表示全量");
    test::check(m.expose("anything"), "全量时任意模块可用");
}
