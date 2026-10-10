# 顶层 CMake 项目文件，在此处执行全局配置
cmake_minimum_required(VERSION 3.21)

# 如果支持，请为 MSVC 编译器启用热重载
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("cimai"
    LANGUAGES C
)

# 仅为 MSVC 使用 /utf-8
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 库内容
add_library (cimai SHARED
    "src/misc/thirdparty_impl.c"
    "src/memory.c"
    "src/meta_parser.c"
    "src/chart_parser.c"
    "src/parser.c"
)

target_include_directories(cimai
    PUBLIC "include"
)

# C# Wrapper / NuGet runtimes RID 推导
# 优先识别 Android NDK（CMAKE_SYSTEM_NAME=Android + ANDROID_ABI），
# 再通过 CMAKE_SYSTEM_NAME 和 CMAKE_OSX_SYSROOT 识别 iOS，最后回退到桌面三元组。
if(CMAKE_SYSTEM_NAME STREQUAL "Android" AND DEFINED ANDROID_ABI)
    if(ANDROID_ABI STREQUAL "arm64-v8a")
        set(CSHARP_RID "android-arm64")
    elseif(ANDROID_ABI STREQUAL "x86_64")
        set(CSHARP_RID "android-x64")
    elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
        set(CSHARP_RID "android-arm")
    elseif(ANDROID_ABI STREQUAL "x86")
        set(CSHARP_RID "android-x86")
    else()
        message(FATAL_ERROR "Unsupported ANDROID_ABI: ${ANDROID_ABI}")
    endif()
elseif(APPLE)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        if(CMAKE_OSX_SYSROOT MATCHES "(^iphoneos$|/iPhoneOS[^/]*\\.sdk$)")
            if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)")
                set(CSHARP_RID "ios-arm64")
            else()
                message(FATAL_ERROR "iOS device build must target arm64")
            endif()
        elseif(CMAKE_OSX_SYSROOT MATCHES "(^iphonesimulator$|/iPhoneSimulator[^/]*\\.sdk$)")
            if(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
                set(CSHARP_RID "iossimulator-x64")
            elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
                set(CSHARP_RID "iossimulator-arm64")
            else()
                message(FATAL_ERROR "Unsupported iOS simulator architecture: ${CMAKE_OSX_ARCHITECTURES}")
            endif()
        else()
            message(FATAL_ERROR "Unsupported iOS SDK: ${CMAKE_OSX_SYSROOT}")
        endif()
    else()
        # 桌面 macOS 分支
        if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
            set(CSHARP_RID "osx-arm64")
        else()
            set(CSHARP_RID "osx-x64")
        endif()
    endif()
elseif(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CSHARP_RID "win-x64")
    else()
        set(CSHARP_RID "win-x86")
    endif()
else()
    # Linux / 其他 Unix
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)")
        set(CSHARP_RID "linux-arm64")
    else()
        set(CSHARP_RID "linux-x64")
    endif()
endif()

message(STATUS "NuGet RID for C# wrapper: ${CSHARP_RID}")

set(CSHARP_NATIVE_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/bindings/CSharp/Cimai/runtimes/${CSHARP_RID}/native")

# 修复 Ninja Multi-Config 会产生 Debug/Release 子目录的问题，让输出直接放置于 native 目录
foreach(CONFIG_SUFFIX IN ITEMS "" _DEBUG _RELEASE _MINSIZEREL _RELWITHDEBINFO)
    set_target_properties(cimai PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY${CONFIG_SUFFIX} "${CSHARP_NATIVE_OUTPUT}"
        LIBRARY_OUTPUT_DIRECTORY${CONFIG_SUFFIX} "${CSHARP_NATIVE_OUTPUT}"
        ARCHIVE_OUTPUT_DIRECTORY${CONFIG_SUFFIX} "${CSHARP_NATIVE_OUTPUT}"
    )
endforeach()