#include <cimai/meta_parser.h>
#include <cimai/simai_types.h>
#include <cimai/utils/tools.h>

#include <thirdparty/sv.h>

#include <stddef.h>
#include <stdlib.h>


static inline int key_get_diff(String_View key, String_View *name)
{
	String_View part;

	if (!sv_try_chop_by_delim(&key, '_', &part))
		return -1;

	if (key.count != 1)
		return -1;

	int diff = key.data[0] - '0' - 1;

	if (diff < 0 || diff >= DIFFICULTY_COUNT)
		return -1;

	*name = part;
	return diff;
}

static void parse_metadata_pair(String_View pair, SimaiFile *file)
{
	String_View key = { 0 };
	if (!sv_try_chop_by_delim(&pair, '=', &key))
		return;

	// key/value 都可能带空格，value 还可能带行尾换行，都 trim 掉
	key = sv_trim(key);
	pair = sv_trim(pair);

	if (sv_eq(key, SV("title")))
		file->title = pair;
	else if (sv_eq(key, SV("artist")))
		file->artist = pair;
	else if (sv_eq(key, SV("des")))
		file->des = pair;
	else if (sv_eq(key, SV("first")))
		sv_try_parse_float(pair, &file->offset);
	else
	{
		String_View name = { 0 };
		int diff = key_get_diff(key, &name);
		if (diff >= 0)
		{
			SimaiChart *chart = file->charts[diff];
			if (chart == NULL)
			{
				chart = calloc(1, sizeof(*chart));
				if (chart == NULL) return; // 程序级错误就直接下一个
				file->charts[diff] = chart;
			}

			if (sv_eq(name, SV("des")))
				chart->des = pair;
			else if (sv_eq(name, SV("lv")))
				chart->level = pair;
			else if (sv_eq(name, SV("inote")))
				chart->fumen = pair;
			else
				goto undefined_command;
		}
		else
			goto undefined_command;

		return;

	undefined_command:
		{
			SimaiCommand command = {
				.key = key,
				.value = pair
			};
			da_append(&file->commands, command);
		}
	}
}

// 错误静默处理
void cimai_parse_metadata(String_View *text, SimaiFile *file)
{
	String_View pair = { 0 };
	while (sv_try_chop_by_delim(text, '&', &pair))
		parse_metadata_pair(pair, file);

	// sv_try_chop_by_delim 只在找到 '&' 时才消费；末尾若没有以 '&' 结尾，
	// 最后一段会留在 *text 里，这里补处理，避免最后一个元数据丢失。
	if (text->count > 0)
		parse_metadata_pair(*text, file);
}