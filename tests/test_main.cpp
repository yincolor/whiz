#include "test_framework.hpp"

#include <iostream>

namespace test
{
    std::vector<test_case> &registry()
    {
        static std::vector<test_case> tests;
        return tests;
    }

    int &failures()
    {
        static int count = 0;
        return count;
    }

    registrar::registrar(const std::string &name, std::function<void()> fn)
    {
        registry().push_back({name, std::move(fn)});
    }

    void check(bool cond, const std::string &msg)
    {
        if (!cond)
        {
            ++failures();
            std::cerr << "  [FAIL] " << msg << "\n";
        }
    }
} // namespace test

int main()
{
    int passed = 0;
    for (const auto &t : test::registry())
    {
        int before = test::failures();
        t.fn();
        if (test::failures() == before)
        {
            std::cout << "[PASS] " << t.name << "\n";
            ++passed;
        }
        else
        {
            std::cout << "[FAIL] " << t.name << "\n";
        }
    }

    std::cout << "\n" << passed << "/" << test::registry().size() << " tests passed\n";
    return passed == static_cast<int>(test::registry().size()) ? 0 : 1;
}
