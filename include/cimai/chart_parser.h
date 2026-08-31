#pragma once
#include <cimai/export.h>
#include <cimai/simai_types.h>

#ifdef __cplusplus
extern "C" {
#endif

	CIMAI_API void cimai_parse_chart(SimaiChart *chart);
	CIMAI_API void cimai_chart_free(SimaiChart **chart);

#ifdef __cplusplus
}
#endif
