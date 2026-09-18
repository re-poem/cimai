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

// 释放 file 持有的所有资源：每个 chart 的内部资源 + chart 结构体本身
void cimai_file_release(SimaiFile *file)
{
	if (file == NULL)
		return;
	for (size_t i = 0; i < DIFFICULTY_COUNT; i++)
	{
		SimaiChart *chart = file->charts[i];
		if (chart == NULL)
			continue;
		cimai_chart_release(chart);
		free(chart);
		file->charts[i] = NULL;
	}
	free(file->commands.items);
	file->commands = (SimaiCommandList){ 0 };
}