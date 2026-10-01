#pragma once

// 极简测试框架（避免引入额外依赖）
#include <functional>
#include <string>
#include <vector>

namespace test
{
    struct test_case
    {
        std::string name;
        std::function<void()> fn;
    };

    std::vector<test_case> &registry();
    int &failures();
    void check(bool cond, const std::string &msg);

    struct registrar
    {
        registrar(const std::string &name, std::function<void()> fn);
    };
} // namespace test

#define TEST(name)                                           \
    static void test_##name();                               \
    static ::test::registrar reg_##name(#name, test_##name); \
    static void test_##name()
