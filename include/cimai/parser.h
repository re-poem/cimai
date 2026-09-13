#pragma once
#include <cimai/export.h>
#include <thirdparty/sv.h>
#include <cimai/simai_types.h>

#ifdef __cplusplus
extern "C" {
#endif

	CIMAI_API void cimai_parse(String_View *text, SimaiFile *file);
	// 释放 file 内部资源：所有 chart 内部资源 + chart 结构体（file 持有） + commands。
	// 不释放 file 结构体本身——由调用方持有/释放。
	CIMAI_API void cimai_file_release(SimaiFile *file);

#ifdef __cplusplus
}
#endif