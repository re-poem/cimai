#pragma once

#define da_append(list_p , value) \
	do {\
		if ((list_p)->count >= (list_p)->capacity) {\
			if ((list_p)->capacity == 0) (list_p)->capacity = 256;\
			else (list_p)->capacity *= 2;\
			void *new_items = realloc((list_p)->items , (list_p)->capacity * sizeof(*(list_p)->items)); \
			if (new_items == NULL) abort(); \
			(list_p)->items = new_items;\
		}\
		(list_p)->items[(list_p)->count++] = (value);\
	} while (0)