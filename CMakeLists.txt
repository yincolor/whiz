cmake_minimum_required(VERSION 3.25)

project(whiz LANGUAGES CXX VERSION 0.1.0)

# ---------------------------------------------------------------------------
# 构建选项
# ---------------------------------------------------------------------------
option(WHIZ_BUILD_TESTS "Build unit tests" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---------------------------------------------------------------------------
# 第三方依赖（FetchContent / CPM）
#   saucer 与 saucer-desktop 内部通过 CPM 自动拉取其传递依赖，无需手工安装。
# ---------------------------------------------------------------------------
include(FetchContent)

# nlohmann/json：解析 package.json 与桥接消息
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# saucer：跨平台 WebView 窗口（v6.0.0）
FetchContent_Declare(
    saucer
    GIT_REPOSITORY https://github.com/saucer/saucer.git
    GIT_TAG        v6.0.0
)
FetchContent_MakeAvailable(saucer)

# saucer-desktop：对话框等桌面能力（v3.0.0，与 saucer v6.0.0 兼容）
FetchContent_Declare(
    saucer-desktop
    GIT_REPOSITORY https://github.com/saucer/desktop.git
    GIT_TAG        v3.0.0
)
FetchContent_MakeAvailable(saucer-desktop)

# ---------------------------------------------------------------------------
# whiz 可执行文件
# ---------------------------------------------------------------------------
add_executable(whiz
    src/main.cpp
    src/cli.cpp
    src/init.cpp
    src/logger.cpp
    src/manifest.cpp
    src/paths.cpp
    src/window.cpp
)

target_include_directories(whiz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(whiz PRIVATE
    saucer::saucer
    saucer::desktop
    nlohmann_json::nlohmann_json
)

target_compile_features(whiz PRIVATE cxx_std_23)

# 关闭 saucer 内部 -Werror 带来的潜在问题：本目标不带 -Werror
if(MSVC)
    target_compile_options(whiz PRIVATE /W3)
else()
    target_compile_options(whiz PRIVATE -Wall -Wextra)
    # 抑制第三方（saucer 依赖的 eraser 等）在模板实例化时产生的告警
    target_compile_options(whiz PRIVATE -Wno-unused-but-set-variable)
endif()

# ---------------------------------------------------------------------------
# 安装
# ---------------------------------------------------------------------------
install(TARGETS whiz RUNTIME DESTINATION bin)

# ---------------------------------------------------------------------------
# 测试
# ---------------------------------------------------------------------------
if(WHIZ_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
