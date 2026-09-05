# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
cmake_minimum_required(VERSION 3.12)

# 如果支持，请为 MSVC 编译器启用热重载
if(POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# 使用无签名UTF-8
add_compile_options(/utf-8)

project("cimai"
	LANGUAGES C
	VERSION 0.0.1
)

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
# 根据平台自动指定 C# 绑定的 runtimes 输出目录
if(WIN32)
    set(CSHARP_RID "win-$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>")
elseif(APPLE)
    set(CSHARP_RID "osx-$<IF:$<EQUAL:${CMAKE_HOST_SYSTEM_PROCESSOR},arm64>,arm64,x64>")
else()
    set(CSHARP_RID "linux-x64")
endif()

set(CSHARP_NATIVE_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/bindings/CSharp/Cimai/runtimes/${CSHARP_RID}/native")

# 将动态库输出复制到对应的 runtimes 目录
set_target_properties(cimai PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CSHARP_NATIVE_OUTPUT}"
    LIBRARY_OUTPUT_DIRECTORY "${CSHARP_NATIVE_OUTPUT}"
)