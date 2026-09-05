#include <cimai/parser.h>
#include <cimai/meta_parser.h>
#include <cimai/chart_parser.h>

#include <stdlib.h>

void cimai_parse(String_View *text, SimaiFile *file)
{
	cimai_parse_metadata(text, file);
	for (size_t i = 0; i < DIFFICULTY_COUNT; i++)
	{
		if (file->charts[i] != NULL)
			cimai_parse_chart(file->charts[i]);
	}
}

void cimai_file_free(SimaiFile *file)
{
	if (file == NULL)
		return;
	for (size_t i = 0; i < DIFFICULTY_COUNT; i++)
		cimai_chart_free(&file->charts[i]);
	free(file->commands.items);
	file->commands = (SimaiCommandList){ 0 };
}