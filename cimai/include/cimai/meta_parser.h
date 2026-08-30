#pragma once
#include <cimai/simai_types.h>
#include <thirdparty/sv.h>

#ifdef __cplusplus
extern "C" {
#endif

void cimai_parse_metadata(String_View *text, SimaiFile *file);

#ifdef __cplusplus
}
#endif
