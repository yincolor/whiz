#include "whiz/paths.hpp"

#include <format>
#include <system_error>

namespace whiz
{
    namespace
    {
        constexpr int k_exit_bad_dir = 2;
    }

    std::optional<resolved_paths> resolve_project_path(const std::string &input)
    {
        resolved_paths out;
        if (auto err = resolve_project_path_or_error(input, out); err.exit_code != 0)
        {
            return std::nullopt;
        }
        return out;
    }

    path_error resolve_project_path_or_error(const std::string &input, resolved_paths &out)
    {
        fs::path p = input.empty() ? fs::path(".") : fs::path(input);

        std::error_code ec;
        fs::path abs = fs::absolute(p, ec);
        if (ec)
        {
            return {k_exit_bad_dir, std::format("项目目录不可访问: {}", input)};
        }

        bool is_file = false;
        bool is_dir = false;
        if (!fs::exists(abs, ec) || ec)
        {
            return {k_exit_bad_dir, std::format("项目目录不存在: {}", input)};
        }
        is_file = fs::is_regular_file(abs, ec);
        is_dir = fs::is_directory(abs, ec);

        if (is_file)
        {
            // 可能是 package.json 文件路径
            if (abs.filename() == "package.json")
            {
                fs::path root = abs.parent_path();
                out.project_root = root;
                out.package_json = abs;
                out.has_package_json = true;
                return {};
            }
            return {k_exit_bad_dir, std::format("期望目录或 package.json 文件路径: {}", input)};
        }

        if (!is_dir)
        {
            return {k_exit_bad_dir, std::format("项目目录不可读: {}", input)};
        }

        out.project_root = abs;
        fs::path pkg = abs / "package.json";
        if (fs::exists(pkg, ec) && fs::is_regular_file(pkg, ec))
        {
            out.package_json = pkg;
            out.has_package_json = true;
        }
        else
        {
            out.has_package_json = false;
        }
        return {};
    }

    std::optional<fs::path> resolve_entry_html(const fs::path &project_root, const std::string &main_field)
    {
        std::error_code ec;

        // 1. 若 main 指向 .js，直接失败（由调用方报错）
        // 2. 若 main 指向 .html / .htm 文件，使用它
        // 3. 若 main 指向目录，追加 index.html
        // 4. 否则回退 index.html / index.htm / main.html

        if (!main_field.empty())
        {
            fs::path candidate = fs::path(main_field);
            if (candidate.is_absolute())
            {
                // 绝对路径直接使用
                if (fs::exists(candidate, ec) && fs::is_regular_file(candidate, ec))
                {
                    return candidate;
                }
            }
            else
            {
                fs::path full = project_root / candidate;
                if (fs::is_directory(full, ec))
                {
                    full = full / "index.html";
                }
                if (fs::exists(full, ec) && fs::is_regular_file(full, ec))
                {
                    return full;
                }
            }
        }

        // 回退查找
        for (const char *name : {"index.html", "index.htm", "main.html"})
        {
            fs::path full = project_root / name;
            if (fs::exists(full, ec) && fs::is_regular_file(full, ec))
            {
                return full;
            }
        }

        return std::nullopt;
    }
} // namespace whiz
