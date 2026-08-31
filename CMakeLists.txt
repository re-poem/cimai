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
# dllexport
target_compile_definitions(cimai
    PRIVATE CIMAI_BUILD
)

# 测试
enable_testing()
add_subdirectory(tests)