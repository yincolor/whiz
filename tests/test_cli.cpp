#include "test_framework.hpp"
#include "whiz/cli.hpp"

using namespace whiz;

TEST(cli_help)
{
    const char *argv[] = {"whiz", "--help"};
    cli_options opt;
    auto err = parse_cli(2, const_cast<char **>(argv), opt);
    test::check(err.exit_code == 0, "--help 无错误");
    test::check(opt.show_help, "show_help 为 true");
}

TEST(cli_version_short)
{
    const char *argv[] = {"whiz", "-v"};
    cli_options opt;
    parse_cli(2, const_cast<char **>(argv), opt);
    test::check(opt.show_version, "-v 触发 version");
}

TEST(cli_project_dir)
{
    const char *argv[] = {"whiz", "./my-app"};
    cli_options opt;
    auto err = parse_cli(2, const_cast<char **>(argv), opt);
    test::check(err.exit_code == 0, "解析成功");
    test::check(opt.project_dir == "./my-app", "项目目录");
    test::check(opt.has_project_dir, "has_project_dir");
}

TEST(cli_overrides)
{
    const char *argv[] = {"whiz", "--title", "Hello", "--width", "640", "--height", "480", "--no-web-security", "--verbose", "."};
    cli_options opt;
    auto err = parse_cli(10, const_cast<char **>(argv), opt);
    test::check(err.exit_code == 0, "解析成功");
    test::check(opt.title == "Hello", "title");
    test::check(opt.width == 640, "width");
    test::check(opt.height == 480, "height");
    test::check(opt.no_web_security, "no_web_security");
    test::check(opt.verbose, "verbose");
}

TEST(cli_unknown_option)
{
    const char *argv[] = {"whiz", "--bogus"};
    cli_options opt;
    auto err = parse_cli(2, const_cast<char **>(argv), opt);
    test::check(err.exit_code == 1, "未知选项 exit_code 1");
}

TEST(cli_missing_value)
{
    const char *argv[] = {"whiz", "--title"};
    cli_options opt;
    auto err = parse_cli(2, const_cast<char **>(argv), opt);
    test::check(err.exit_code == 1, "缺少参数 exit_code 1");
}

TEST(cli_init)
{
    const char *argv[] = {"whiz", "init"};
    cli_options opt;
    auto err = parse_cli(2, const_cast<char **>(argv), opt);
    test::check(err.exit_code == 0, "init 无错误");
    test::check(opt.init_mode, "init_mode 为 true");
    test::check(!opt.has_project_dir, "init 不占用 project_dir");
}

TEST(cli_init_yes)
{
    const char *argv[] = {"whiz", "init", "-y"};
    cli_options opt;
    auto err = parse_cli(3, const_cast<char **>(argv), opt);
    test::check(err.exit_code == 0, "init -y 无错误");
    test::check(opt.init_mode, "init_mode 为 true");
    test::check(opt.init_yes, "init_yes 为 true");
}

TEST(cli_init_reject_extra_arg)
{
    const char *argv[] = {"whiz", "init", "./foo"};
    cli_options opt;
    auto err = parse_cli(3, const_cast<char **>(argv), opt);
    test::check(err.exit_code == 1, "init 带额外参数应报错");
}
