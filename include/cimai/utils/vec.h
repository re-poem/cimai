#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


#define DEFINE_VEC(T, name) \
    typedef struct {        \
        T *data;            \
        size_t len, cap;    \
    } name

static void *vec_grow_(void *data, size_t *cap, size_t elem)
{
	size_t nc = *cap ? *cap * 2 : 16;
	void *p = realloc(data, (size_t)nc * elem);
	if (!p) abort();
	*cap = nc;
	return p;
}

#define vec_push(v, ...) do {                                            \
    if ((v)->len == (v)->cap)                                            \
        (v)->data = vec_grow_((v)->data, &(v)->cap, sizeof *(v)->data);  \
    (v)->data[(v)->len++] = (__VA_ARGS__);                               \
} while (0)

#define vec_free(v) (free((v)->data), (v)->data = NULL, (v)->len = (v)->cap = 0)