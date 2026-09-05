# 顶层 CMake 项目文件，在此处执行全局配置
cmake_minimum_required(VERSION 3.21)

# 如果支持，请为 MSVC 编译器启用热重载
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("cimai"
    LANGUAGES C
    VERSION 0.0.1
)

# 仅为 MSVC 使用 /utf-8
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 库内容
add_library (cimai SHARED
    "src/misc/thirdparty_impl.c"
    "src/meta_parser.c"
    "src/chart_parser.c"
    "src/parser.c"
)

target_include_directories(cimai
    PUBLIC "include"
)

# C# Wrapper 
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CSHARP_RID "win-x64")
    else()
        set(CSHARP_RID "win-x86")
    endif()
elseif(APPLE)
    if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        set(CSHARP_RID "osx-arm64")
    else()
        set(CSHARP_RID "osx-x64")
    endif()
else()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)")
        set(CSHARP_RID "linux-arm64")
    else()
        set(CSHARP_RID "linux-x64")
    endif()
endif()

set(CSHARP_NATIVE_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/bindings/CSharp/Cimai/runtimes/${CSHARP_RID}/native")

# 修复 Ninja Multi-Config 会产生 Debug/Release 子目录的问题，让输出直接放置于 native 目录
foreach(CONFIG_SUFFIX IN ITEMS "" _DEBUG _RELEASE _MINSIZEREL _RELWITHDEBINFO)
    set_target_properties(cimai PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY${CONFIG_SUFFIX} "${CSHARP_NATIVE_OUTPUT}"
        LIBRARY_OUTPUT_DIRECTORY${CONFIG_SUFFIX} "${CSHARP_NATIVE_OUTPUT}"
        ARCHIVE_OUTPUT_DIRECTORY${CONFIG_SUFFIX} "${CSHARP_NATIVE_OUTPUT}"
    )
endforeach()