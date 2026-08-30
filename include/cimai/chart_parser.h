#pragma once
#include <cimai/simai_types.h>

#ifdef __cplusplus
extern "C" {
#endif

void cimai_parse_chart(SimaiChart *chart);
void cimai_chart_free(SimaiChart **chart);

#ifdef __cplusplus
}
#endif
