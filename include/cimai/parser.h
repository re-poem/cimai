#pragma once
#include <cimai/export.h>
#include <thirdparty/sv.h>
#include <cimai/simai_types.h>

#ifdef __cplusplus
extern "C" {
#endif

	CIMAI_API int cimai_parse(String_View *text, SimaiFile *file);
	CIMAI_API void cimai_file_free(SimaiFile *file);

#ifdef __cplusplus
}
#endif