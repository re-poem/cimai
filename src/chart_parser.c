#include <cimai/chart_parser.h>
#include <cimai/simai_types.h>
#include <cimai/utils/tools.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define IS_NOTE_CHAR(c) ((c) >= '1' && (c) <= '8')
#define IS_TOUCH_AREA_CHAR(c) ((c) >= 'A' && (c) <= 'E')
// 通常来讲ABC判断是冗余的所以放后边
#define IS_SLIDE_CODE_CHAR(c) ((c) == 'P' || (c) == 'Q' || (c) == 'K' || (c) == 'A' || (c) == 'B' || (c) == 'C')

typedef struct
{
	double time;
	float bpm;
	float beats;
	float hspeed;
	float sveloc;
	uint8_t sign_num;
	uint8_t sign_den;
	// 每拍秒数 = 60/bpm * 4/beats，bpm/beats 变化时更新，热路径只做加法
	double seconds_per_beat;
} Context;


typedef enum
{
	CIMAI_NOTE_ERR_COMMA_NOT_FOUND = -3,
	CIMAI_NOTE_ERR_NOT_A_NOTE = -2,
	CIMAI_NOTE_ERR_DURATION_NOT_CLOSED = -1,
	CIMAI_NOTE_ERR_INVALID = 0,
	CIMAI_NOTE_SUCCESS = 1,
	CIMAI_NOTE_FAKE_EACH = 2
} ParseNoteRetCode;



// 允许数字前后带空白，避免因空格被 sv_try_parse_float 拒绝
static inline bool parse_float_trimmed(String_View sv, float *out)
{
	return sv_try_parse_float(sv_trim(sv), out);
}
// 把当前 timing 的 notes 复制进 arena，让每个 timing 拥有独立且稳定的数组，
// 避免所有 timing 共享同一个 notes.items，后续 realloc 移动后导致悬垂。
static inline SimaiNoteList notes_into_arena(Arena *arena, SimaiNoteList *notes)
{
	SimaiNoteList owned = { 0 };
	size_t count = notes->count;
	if (count > 0)
	{
		size_t bytes = count * sizeof(SimaiNote);
		SimaiNote *items = arena_alloc(arena, bytes);
		memcpy(items, notes->items, bytes);
		owned = (SimaiNoteList){
			.items = items,
			.count = count,
			.capacity = count,
		};
	}
	notes->count = 0; // 复用 scratch 缓冲
	return owned;
}
// 单个 timing 内滑条路径暂存上限；正常谱面远小于此
#define SLIDE_CONTENT_MAX 256
// 向 slide_content 暂存缓冲追加一个字符，超限则忽略（避免越界写）
static inline void slide_content_add(char *buf, int *len, char c)
{
	if (*len < SLIDE_CONTENT_MAX)
		buf[(*len)++] = c;
}


static int skip_comment(String_View *text, Context *ctx)
{
	if (sv_starts_with(*text, SV("||")))
	{
		sv_chop_left(text, 2);
		String_View content = sv_chop_by_delim(text, '\n');

		if (sv_starts_with(content, SV("s")))
		{
			sv_chop_left(&content, 1);

			String_View num_sv;
			if (!sv_try_chop_by_delim(&content, '/', &num_sv))
				return 0;

			uint8_t num, den;
			if (!sv_try_parse_uint8(num_sv, &num) ||
				!sv_try_parse_uint8(content, &den))
				return 0;

			ctx->sign_num = num;
			ctx->sign_den = den;
		}
		return 1;
	}
	else
		return 0;
}
static int parse_bpm(String_View *text, Context *ctx)
{
	sv_chop_left(text, 1);

	String_View chunk;
	if (!sv_try_chop_by_delim(text, ')', &chunk))
		return 0;

	float value;
	if (!parse_float_trimmed(chunk, &value))
		return 0;

	ctx->bpm = value;
	ctx->seconds_per_beat = 60.0 / ctx->bpm * 4.0 / ctx->beats;

	return 1;
}
static int parse_beats(String_View *text, Context *ctx)
{
	sv_chop_left(text, 1);

	String_View chunk;
	if (!sv_try_chop_by_delim(text, '}', &chunk))
		return 0;

	float value;
	if (!parse_float_trimmed(chunk, &value))
		return 0;

	ctx->beats = value;
	ctx->seconds_per_beat = 60.0 / ctx->bpm * 4.0 / ctx->beats;

	return 1;
}
static int parse_settings(String_View *text, Context *ctx)
{
	sv_chop_left(text, 1); // 跳过 '<'

	String_View pair;
	if (!sv_try_chop_by_delim(text, '>', &pair))
		return 0;

	String_View key;
	if (!sv_try_chop_by_delim(&pair, '*', &key))
		return 0;
	key = sv_trim(key);

	float value;
	if (!parse_float_trimmed(pair, &value))
		return 0;

	if (sv_eq(key, SV("HS")))
	{
		ctx->hspeed = value;
	}
	else if (sv_eq(key, SV("SV")))
	{
		ctx->sveloc = value;
	}
	else
		return 0;

	return 1;
}



static int parse_duration(
	String_View content, const int hash_count, const Context ctx,
	double *duration, double *shoot_delay)
{
	switch (hash_count)
	{
	case 0: // [8:1]
	{
		String_View div_sv;
		if (!sv_try_chop_by_delim(&content, ':', &div_sv))
			return 0;

		float div, beats;
		if (!parse_float_trimmed(div_sv, &div) ||
			!parse_float_trimmed(content, &beats))
			return 0;

		*duration = 60 / ctx.bpm * 4 / div * beats;
		*shoot_delay = 60 / ctx.bpm * 4 / ctx.sign_den;
		break;
	}
	case 1: // [180#8:1] [180#0.2]   [#0.2]<-(hold only but it is also compatible with slide)
	{
		String_View bpm_sv;
		if (!sv_try_chop_by_delim(&content, '#', &bpm_sv))
			return 0;

		bool is_hold_absolute_duration = bpm_sv.count == 0;

		float bpm = 0;
		if (!is_hold_absolute_duration &&
			!parse_float_trimmed(bpm_sv, &bpm))
			return 0;


		String_View div_sv;
		if (!is_hold_absolute_duration &&
			sv_try_chop_by_delim(&content, ':', &div_sv))
		{
			float div, beats;
			if (!parse_float_trimmed(div_sv, &div) ||
				!parse_float_trimmed(content, &beats))
				return 0;

			*duration = 60 / bpm * 4 / div * beats;
			*shoot_delay = 60 / bpm * 4 / ctx.sign_den;
		}
		else
		{
			float absolute_duration;
			if (!parse_float_trimmed(content, &absolute_duration))
				return 0;

			*duration = absolute_duration;
			*shoot_delay = 60 / (is_hold_absolute_duration ? ctx.bpm : bpm) * 4 / ctx.sign_den;
		}
		break;
	}
	case 2: // [0.2##8:1] [0.2##0.2]
	{
		String_View absolute_delay_sv;
		if (!sv_try_chop_by_delim(&content, '#', &absolute_delay_sv))
			return 0;
		if (content.data[0] != '#')
			return 0;

		float absolute_delay;
		if (!parse_float_trimmed(absolute_delay_sv, &absolute_delay))
			return 0;

		sv_chop_left(&content, 1);


		String_View div_sv;
		if (sv_try_chop_by_delim(&content, ':', &div_sv))
		{
			float div, beats;
			if (!parse_float_trimmed(div_sv, &div) ||
				!parse_float_trimmed(content, &beats))
				return 0;

			*duration = 60 / ctx.bpm * 4 / div * beats;
			*shoot_delay = absolute_delay;
		}
		else
		{
			float absolute_duration;
			if (!parse_float_trimmed(content, &absolute_duration))
				return 0;

			*duration = absolute_duration;
			*shoot_delay = absolute_delay;
		}
		break;
	}
	case 3: // [0.2##180#8:1]
	{
		String_View absolute_delay_sv;
		if (!sv_try_chop_by_delim(&content, '#', &absolute_delay_sv))
			return 0;
		if (content.data[0] != '#')
			return 0;

		float absolute_delay;
		if (!parse_float_trimmed(absolute_delay_sv, &absolute_delay))
			return 0;

		sv_chop_left(&content, 1);


		String_View bpm_sv;
		if (!sv_try_chop_by_delim(&content, '#', &bpm_sv))
			return 0;

		float bpm;
		if (!parse_float_trimmed(bpm_sv, &bpm))
			return 0;


		String_View div_sv;
		if (!sv_try_chop_by_delim(&content, ':', &div_sv))
			return 0;

		float div, beats;
		if (!parse_float_trimmed(div_sv, &div) ||
			!parse_float_trimmed(content, &beats))
			return 0;

		*duration = 60 / bpm * 4 / div * beats;
		*shoot_delay = absolute_delay;
		break;
	}
	default:
		return 0;
	}
	return 1;
}
// 解析两个逗号间所组成的一个timing，跳转到下一个timing开头（即下一个 , 或 ` 后）（遇到错误时恒跳转到下一个 , 防止越界访问）
static ParseNoteRetCode parse_timing(
	const Context ctx, String_View *text,
	SimaiNoteList *notes, Arena *_arena)
{

#define EMIT_NOTE \
	do { \
		if (note.type != NONE) \
		{ \
			da_append(notes, note); \
			note = (SimaiNote){ 0 }; \
		} \
	} while (0)

#define EMIT_SLIDE_CONTENT \
	do { \
		size_t size = slide_content_len * sizeof(*slide_content); \
		char *data = arena_alloc(_arena, size); \
		memcpy(data, slide_content, size); \
		note.slide_content = (String_View){ \
			.data = data, \
			.count = slide_content_len \
		}; \
	} while (0)


	// 连写双押
	if (text->count >= 3 &&
		IS_NOTE_CHAR(text->data[0]) &&
		IS_NOTE_CHAR(text->data[1]) &&
		text->data[2] == ',')
	{
		SimaiNote a = { .type = TAP, .start_pos = text->data[0] - '0' };
		SimaiNote b = { .type = TAP, .start_pos = text->data[1] - '0' };
		da_append(notes, a);
		da_append(notes, b);
		sv_chop_left(text, 3); // 吃掉 "XY,"
		return CIMAI_NOTE_SUCCESS;
	}

	SimaiNote note = { 0 };
	const char *note_start = text->data;
	String_View last_note = { 0 };

	size_t i = 0;

	bool is_touch_start = false;
	bool is_tap_head_slide = false;
	bool is_slide_no_star_fade = false;
	double hold_slide_duration = NAN;
	// 滑条路径暂存
	char slide_content[SLIDE_CONTENT_MAX];
	int slide_content_len = 0;

	for (; i < text->count; i++)
	{
		switch (text->data[i])
		{
		case 'h':
			if (note.type == TAP)
				note.type = HOLD;
			else if (note.type == TOUCH)
				note.type = TOUCHHOLD;

			break;

		case '[':
			if (text->count <= i + 1)
			{
				sv_chop_by_delim(text, ',');
				return CIMAI_NOTE_ERR_DURATION_NOT_CLOSED;
			}

			i++;
			size_t start = i;
			size_t end = -1;
			int hash_count = 0;
			for (; i < text->count; i++)
			{
				if (text->data[i] == '#') hash_count++;
				if (text->data[i] == ']')
				{
					end = i;
					break;
				}
			}
			if (end == -1)
			{
				sv_chop_by_delim(text, ',');
				return CIMAI_NOTE_ERR_DURATION_NOT_CLOSED;
			}

			String_View dc = {
				.data = text->data + start,
				.count = end - start
			};
			double duration = 0, shoot_delay = 0;
			if (!parse_duration(dc, hash_count, ctx, &duration, &shoot_delay))
			{
				duration = 0;
				shoot_delay = 0;
			}

			note.duration += duration;
			// 应该没傻逼分段时间slide还乱指定启动拍。。
			note.slide_shoot_delay = shoot_delay;

			break;

		case 'b':
			note.is_break = true;
			break;
		case 'x':
			note.is_ex = true;
			break;
		case 'm':
			note.is_mine = true;
			break;
		case 'c':
			note.is_ignore_sv = true;
			break;

		case '$':
			note.is_star = true;
			if (text->count > i + 1 && text->data[i + 1] == '$')
			{
				note.is_star_fake_rotate = true;
				i++;
			}
			break;

		case '?':
			note.type = NONE;
			break;
		case '!':
			is_slide_no_star_fade = true;
			break;
		case '@':
			is_tap_head_slide = true;
			break;

		case 'f':
			note.is_hanabi = true;
			break;


			// slides
		case '-':
		case '^':
		case 'v':
		case '<':
		case '>':
		case 'V':
		case 'p':
		case 'q':
		case 's':
		case 'z':
		case 'w':
			// 理论上，对于一个合法的slide-code，在K之前的参数全部给予头，K之后全部给予尾是正确的
		case 'K':
			if (note.type != SLIDE)
			{
				// 强制star的note依然强制，强制tap-head的依然tap-head，也就是说1$$@-4 == 1-4
				note.is_star = is_tap_head_slide && !note.is_star ? false : true;
				if (note.type == HOLD || note.type == TOUCHHOLD)
				{
					hold_slide_duration = note.duration;
				}
				EMIT_NOTE; // 星星头
				note.is_slide_no_star_fade = is_slide_no_star_fade;
			}
			note.type = SLIDE;
			// 已是 SLIDE 则继续累积路径；连结滑条整体是一条 SLIDE
			slide_content_add(slide_content, &slide_content_len, text->data[i]);
			break;




		case '*':
			if (note.type == SLIDE)
			{
				if (!isnan(hold_slide_duration))
				{
					note.slide_shoot_delay = hold_slide_duration;
				}
				EMIT_SLIDE_CONTENT;
				EMIT_NOTE;

				if (is_touch_start) slide_content_len = 2;
				else slide_content_len = 1;
			}
			break;



		case '/':
			String_View cur_note = sv_from_parts(note_start, text->data + i - note_start);
			if (sv_eq(last_note, cur_note))
			{
				notes->items[notes->count - 1].can_be_folded = true;
			}
			last_note = cur_note;

			if (note.type == SLIDE)
			{
				if (!isnan(hold_slide_duration))
				{
					note.slide_shoot_delay = hold_slide_duration;
					hold_slide_duration = NAN;
				}
				EMIT_SLIDE_CONTENT;
			}
			EMIT_NOTE;
			slide_content_len = 0;
			break;


		case '`':
			if (note.type == SLIDE)
			{
				if (!isnan(hold_slide_duration))
				{
					note.slide_shoot_delay = hold_slide_duration;
					hold_slide_duration = NAN;
				}
				EMIT_SLIDE_CONTENT;
			}
			EMIT_NOTE;
			sv_chop_left(text, i + 1);
			return CIMAI_NOTE_FAKE_EACH;
		case ',':
			if (note.type == SLIDE)
			{
				if (!isnan(hold_slide_duration))
				{
					note.slide_shoot_delay = hold_slide_duration;
					hold_slide_duration = NAN;
				}
				EMIT_SLIDE_CONTENT;
			}
			EMIT_NOTE;
			sv_chop_left(text, i + 1);
			return CIMAI_NOTE_SUCCESS;

		default:
			// 音符头
			if (note.type == NONE &&
				(IS_NOTE_CHAR(text->data[i]) || IS_TOUCH_AREA_CHAR(text->data[i])))
			{
				if (IS_NOTE_CHAR(text->data[i]))
				{
					note.type = TAP;
					note.start_pos = text->data[i] - '0';
					is_touch_start = false;
					slide_content_add(slide_content, &slide_content_len, text->data[i]);
				}
				else if (text->data[i] == 'C')
				{
					note.touch_area = 'C';
					note.type = TOUCH;
					note.start_pos = 1; // actually not used
					is_touch_start = true;
					slide_content_add(slide_content, &slide_content_len, text->data[i]);
					if (i + 1 < text->count && (text->data[i + 1] == '1' || text->data[i + 1] == '2'))
						i++;
				}
				else if (i + 1 < text->count && IS_NOTE_CHAR(text->data[i + 1]))
				{
					note.touch_area = text->data[i];
					note.type = TOUCH;
					note.start_pos = text->data[i + 1] - '0';
					is_touch_start = true;
					slide_content_add(slide_content, &slide_content_len, text->data[i]);
					slide_content_add(slide_content, &slide_content_len, text->data[i + 1]);
					i++;
				}
				else
				{
					slide_content_add(slide_content, &slide_content_len, text->data[i]);
				}
				note_start = text->data + i;
			}
			// Slide Content
			else if (IS_NOTE_CHAR(text->data[i]) ||
				IS_TOUCH_AREA_CHAR(text->data[i]) ||
				IS_SLIDE_CODE_CHAR(text->data[i]))
			{
				slide_content_add(slide_content, &slide_content_len, text->data[i]);
			}
			break;
		}
	}

	sv_chop_by_delim(text, ',');
	return CIMAI_NOTE_ERR_COMMA_NOT_FOUND;
}



static void calc_each(const SimaiNoteList notes)
{
	int note_count = 0;
	int slide_count = 0;

	for (size_t i = 0; i < notes.count; i++)
	{
		SimaiNote o = notes.items[i];
		if (!o.is_mine)
		{
			if (o.type == SLIDE)
			{
				slide_count++;
			}
			else
			{
				note_count++;
			}
		}
	}

	bool note_each = note_count > 1;
	bool slide_each = slide_count > 1;

	for (size_t i = 0; i < notes.count; i++)
	{
		SimaiNote *o = &notes.items[i];
		if (o->type == SLIDE)
		{
			o->is_each = slide_each;
		}
		else
		{
			o->is_each = note_each;
		}
	}
}

// 错误静默处理
void cimai_parse_chart(SimaiChart *chart)
{
	Context ctx = {
		.time = 0,
		.bpm = NAN,
		.beats = NAN,
		.hspeed = 1,
		.sveloc = 1,
		.sign_num = 4,
		.sign_den = 4,
	};

	String_View text = chart->fumen;

	Arena *_arena = &chart->_arena;
	SimaiTimingList *timings = &chart->timings;

	SimaiTiming timing = { 0 };
	SimaiNoteList notes = { 0 };

	const char *chart_start = text.data;
	const char *last_timing_end = text.data;

	// 已知字符跳转不要在这个循环出现，放到各个函数中保持整洁，只有各个函数未跳过的不认识的才从这里chop_left一下
	while (text.count > 0)
	{
		switch (text.data[0])
		{
		case '|':
			if (!skip_comment(&text, &ctx))
				sv_chop_left(&text, 1);
			break;
		case '(':
			if (!parse_bpm(&text, &ctx))
				sv_chop_left(&text, 1);
			break;
		case '{':
			if (!parse_beats(&text, &ctx))
				sv_chop_left(&text, 1);
			break;
		case '<':
			if (!parse_settings(&text, &ctx))
				sv_chop_left(&text, 1);
			break;
		default:
			if (!isnan(ctx.bpm) && !isnan(ctx.beats)) // 定义了这俩后的note才需要管
			{
				char c = text.data[0];
				if (IS_NOTE_CHAR(c) || IS_TOUCH_AREA_CHAR(c))
				{
					ParseNoteRetCode ret = parse_timing(ctx, &text, &notes, _arena);
					calc_each(notes);
					switch (ret)
					{
					case CIMAI_NOTE_SUCCESS:
						timing.time = ctx.time;
						timing.bpm = ctx.bpm;
						timing.hspeed = ctx.hspeed;
						timing.sveloc = ctx.sveloc;
						size_t count = text.data - last_timing_end;
						timing.content = sv_from_parts(last_timing_end, count);
						last_timing_end = text.data;
						timing.fumen_pos = text.data - chart_start;
						timing.sign_num = ctx.sign_num;
						timing.sign_den = ctx.sign_den;
						timing.notes = notes_into_arena(_arena, &notes);
						ctx.time += ctx.seconds_per_beat;
						da_append(timings, timing);
						break;
					case CIMAI_NOTE_FAKE_EACH:
						int fake_count = 1;
						do
						{
							ret = parse_timing(ctx, &text, &notes, _arena);
							calc_each(notes);
							if (ret <= 0) break;
							timing.time = ctx.time + fake_count * (60 / ctx.bpm * 4 / 128);
							timing.bpm = ctx.bpm;
							timing.hspeed = ctx.hspeed;
							timing.sveloc = ctx.sveloc;
							size_t count = text.data - last_timing_end;
							timing.content = sv_from_parts(last_timing_end, count);
							last_timing_end = text.data;
							timing.fumen_pos = text.data - chart_start;
							timing.sign_num = ctx.sign_num;
							timing.sign_den = ctx.sign_den;
							timing.notes = notes_into_arena(_arena, &notes);
							fake_count++;
							da_append(timings, timing);
						} while (ret == CIMAI_NOTE_FAKE_EACH);
						ctx.time += ctx.seconds_per_beat;
						break;
					default:
						break;
					}
				}
				else if (c == ',')
				{
					// 空逗号
					ctx.time += ctx.seconds_per_beat;
					sv_chop_left(&text, 1);
				}
				else
				{
					// 未定义字符
					sv_chop_left(&text, 1);
				}
			}
			else
			{
				// bpm/beats 未定义，忽略
				sv_chop_left(&text, 1);
			}
			break;
		}
	}

	// 释放 parse_timing 用过的 scratch notes 堆缓冲
	free(notes.items);
}

void cimai_chart_free(SimaiChart **chart_ptr)
{
	if (chart_ptr == NULL || *chart_ptr == NULL)
		return;
	SimaiChart *chart = *chart_ptr;
	free(chart->timings.items);
	arena_free(&chart->_arena);
	free(chart);
	*chart_ptr = NULL;
}
