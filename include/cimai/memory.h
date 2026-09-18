#pragma once

#include <cimai/export.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

	CIMAI_API void *cimai_malloc(size_t size);
	CIMAI_API void *cimai_calloc(size_t count, size_t size);
	CIMAI_API void *cimai_realloc(void *ptr, size_t size);
	CIMAI_API void  cimai_free(void *ptr);

#ifdef __cplusplus
}
#endif