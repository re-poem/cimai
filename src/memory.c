#include <cimai/memory.h>

#include <stdlib.h>

void *cimai_malloc(size_t size)
{
	return malloc(size);
}

void *cimai_calloc(size_t count, size_t size)
{
	return calloc(count, size);
}

void *cimai_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

void cimai_free(void *ptr)
{
	free(ptr);
}