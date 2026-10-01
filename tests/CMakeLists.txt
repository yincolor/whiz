# 单元测试：覆盖 manifest / cli / paths 的纯逻辑（不依赖 saucer，无需开窗口）

add_executable(whiz_tests
    test_main.cpp
    test_manifest.cpp
    test_cli.cpp
    test_init.cpp
    ${CMAKE_SOURCE_DIR}/src/manifest.cpp
    ${CMAKE_SOURCE_DIR}/src/cli.cpp
    ${CMAKE_SOURCE_DIR}/src/init.cpp
    ${CMAKE_SOURCE_DIR}/src/paths.cpp
    ${CMAKE_SOURCE_DIR}/src/logger.cpp
)

target_include_directories(whiz_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(whiz_tests PRIVATE
    nlohmann_json::nlohmann_json
)

target_compile_features(whiz_tests PRIVATE cxx_std_23)

add_test(NAME whiz_tests COMMAND whiz_tests)
