#pragma once
#include <cimai/export.h>
#include <cimai/simai_types.h>

#ifdef __cplusplus
extern "C" {
#endif

	CIMAI_API void cimai_parse_chart(SimaiChart *chart);
	// 释放 chart 内部由 cimai_parse_chart 分配的资源（timings.items、_arena），
	// 但不释放 chart 结构体本身——chart 由调用方持有/释放。
	CIMAI_API void cimai_chart_release(SimaiChart *chart);

#ifdef __cplusplus
}
#endif
