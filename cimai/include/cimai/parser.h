#pragma once
#include <thirdparty/sv.h>
#include <cimai/simai_types.h>

int cimai_parse(String_View *text, SimaiFile *file);
void cimai_file_free(SimaiFile *file);