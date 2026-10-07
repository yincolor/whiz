# whiz_embed_resources：将指定目录下的所有文件嵌入目标，并注册到 embedded://root/ 协议。
#
# 用法：
#   whiz_embed_resources(<target> WEB_ROOT <dir>)
#
# 会在构建时扫描 <dir>，生成一个 C++ 源文件加入 <target>，通过
# whiz::detail::register_embedded_resource 把内容注册为嵌入式资源。

function(whiz_embed_resources target)
    cmake_parse_arguments(WHIZ_EMBED "" "WEB_ROOT" "" ${ARGN})

    if(NOT WHIZ_EMBED_WEB_ROOT)
        message(FATAL_ERROR "whiz_embed_resources: 必须指定 WEB_ROOT")
    endif()

    get_filename_component(web_root "${WHIZ_EMBED_WEB_ROOT}" ABSOLUTE)
    if(NOT EXISTS "${web_root}")
        message(FATAL_ERROR "whiz_embed_resources: WEB_ROOT 不存在: ${web_root}")
    endif()

    file(GLOB_RECURSE whiz_embed_files CONFIGURE_DEPENDS "${web_root}/*")

    set(gen_dir "${CMAKE_CURRENT_BINARY_DIR}/whiz_embedded/${target}")
    file(MAKE_DIRECTORY "${gen_dir}")
    set(gen_cpp "${gen_dir}/resources.cpp")

    set(declarations "")
    set(registrations "")
    set(index 0)

    foreach(file IN LISTS whiz_embed_files)
        if(IS_DIRECTORY "${file}")
            continue()
        endif()

        file(RELATIVE_PATH rel "${web_root}" "${file}")
        string(REPLACE "\\" "/" rel "${rel}")

        file(READ "${file}" content HEX)
        string(REGEX MATCHALL ".." bytes "${content}")
        list(TRANSFORM bytes PREPEND "0x")
        list(JOIN bytes ", " bytes_str)

        string(APPEND declarations "static const unsigned char whiz_data_${index}[] = {${bytes_str}};\n")
        string(APPEND registrations
               "    whiz::detail::register_embedded_resource(\"${rel}\", std::string(reinterpret_cast<const char *>(whiz_data_${index}), sizeof(whiz_data_${index})));\n")

        math(EXPR index "${index} + 1")
    endforeach()

    if(index EQUAL 0)
        # 空目录：生成空的注册函数即可
        set(registrations "    (void)0;\n")
    endif()

    file(WRITE "${gen_cpp}"
"// 由 whiz_embed_resources 自动生成，请勿手动修改。\n"
"#include <whiz/detail/embedded.hpp>\n"
"#include <string>\n"
"\n"
"namespace\n"
"{\n"
"${declarations}"
"void whiz_embed_register_all()\n"
"{\n"
"${registrations}"
"}\n"
"\n"
"struct WhizEmbedInitializer\n"
"{\n"
"    WhizEmbedInitializer()\n"
"    {\n"
"        whiz_embed_register_all();\n"
"    }\n"
"};\n"
"\n"
"static WhizEmbedInitializer g_whiz_embed_initializer;\n"
"} // namespace\n"
    )

    target_sources(${target} PRIVATE "${gen_cpp}")
    target_include_directories(${target} PRIVATE "${web_root}")
endfunction()
