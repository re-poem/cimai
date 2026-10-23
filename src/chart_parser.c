#include <cimai/chart_parser.h>
#include <cimai/simai_types.h>
#include <cimai/utils/vec.h>
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

static SlideType translate_circle(char shape, int from, int to)
{
	if (shape == '^')
	{
		int cw = (to - from + 8) % 8;
		if (cw == 0 || cw == 4)
			return SLIDE_NONE;
		return cw < 4 ? CIRCLE_R : CIRCLE_L;
	}
	bool lower_half = from >= 3 && from <= 6;
	return (shape == '<') != lower_half ? CIRCLE_L : CIRCLE_R;
}
// 返回环上的位置编号
static int note_pos_offset(SimaiNotePos node)
{
	if (node >= BTN1 && node <= BTN8) return node - BTN1 + 1;
	if (node >= A1 && node <= A8) return node - A1 + 1;
	if (node >= B1 && node <= B8) return node - B1 + 1;
	if (node >= D1 && node <= D8) return node - D1 + 1;
	if (node >= E1 && node <= E8) return node - E1 + 1;
	return 0;
}
static bool get_note_pos(String_View text, size_t *cursor, SimaiNotePos *end)
{
	size_t pos = *cursor;
	if (pos >= text.count) return false;
	char c = text.data[pos++];
	SimaiNotePos node;
	if (IS_NOTE_CHAR(c))
		node = (SimaiNotePos)(BTN1 + c - '1');
	else if (c == 'C')
	{
		node = C1;
		if (pos < text.count && isdigit((unsigned char)text.data[pos]))
		{
			char digit = text.data[pos++];
			if (digit != '1' && digit != '2') return false;
			node = digit == '1' ? C1 : C2;
		}
	}
	else
	{
		if (!IS_TOUCH_AREA_CHAR(c) || pos >= text.count || !IS_NOTE_CHAR(text.data[pos]))
			return false;
		int digit = text.data[pos++] - '1';
		SimaiNotePos base = c == 'A' ? A1 : c == 'B' ? B1 : c == 'D' ? D1 : E1;
		node = (SimaiNotePos)(base + digit);
	}
	*end = node;
	*cursor = pos;
	return true;
}

// 解析两个逗号间所组成的一个timing，跳转到下一个timing开头（即下一个 , 或 ` 后）（遇到错误时恒跳转到下一个 , 防止越界访问）
static ParseNoteRetCode parse_timing(
	const Context ctx, String_View *text,
	SimaiNote_Vec *notes, SlidePart_Vec *slide_parts)
{
	const size_t notes_first = notes->len;
	const size_t slide_parts_first = slide_parts->len;

	// 连写双押
	if (text->count >= 3 &&
		IS_NOTE_CHAR(text->data[0]) &&
		IS_NOTE_CHAR(text->data[1]) &&
		text->data[2] == ',')
	{
		SimaiNote a = { .type = TAP, .pos = (SimaiNotePos)(BTN1 + text->data[0] - '1') };
		SimaiNote b = { .type = TAP, .pos = (SimaiNotePos)(BTN1 + text->data[1] - '1') };
		vec_push(notes, a, b);
		sv_chop_left(text, 3); // 吃掉 "XY,"
		return CIMAI_NOTE_SUCCESS;
	}

	ParseNoteRetCode error = CIMAI_NOTE_ERR_COMMA_NOT_FOUND;

	size_t i = 0;
	SimaiNote note = { 0 };

	// folding 判断
	const char *note_start = text->data;
	String_View last_note = { 0 };

	// note 属性记录
	bool is_tap_head_slide = false;
	bool is_slide_no_star_fade = false;
	double hold_slide_duration = NAN;
	bool invalid_slide = false;

	// 提交 note 宏
#define EMIT_NOTE \
	do { \
		if (note.type != SLIDE || invalid_slide) \
		{ \
			if (note.slide_parts_count) \
				slide_parts->len = note.slide_parts_first; \
			note.slide_parts_first = note.slide_parts_count = 0; \
		} \
		if (note.type != NONE && !invalid_slide) \
		{ \
			if (note.type == SLIDE && !isnan(hold_slide_duration)) \
				note.slide_shoot_delay = hold_slide_duration; \
			vec_push(notes, note); \
		} \
		note = (SimaiNote){ 0 }; \
		invalid_slide = false; \
		hold_slide_duration = NAN; \
	} while (0)




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
				error = CIMAI_NOTE_ERR_DURATION_NOT_CLOSED;
				goto fail;
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
				error = CIMAI_NOTE_ERR_DURATION_NOT_CLOSED;
				goto fail;
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

			// 标准 slide：形状后立即读取终点，大 V 额外读取折返点。
		case '-':
		case '^':
		case '<':
		case '>':
		case 'v':
		case 'V':
		case 'p':
		case 'q':
		case 's':
		case 'z':
		case 'w':
		case 'A':
		case 'B':
		case 'C':
		case 'P':
		case 'Q':
		case 'K':
		{
			char type = text->data[i];
			// 音符开头的 A/B/C 是 TOUCH 区域；路径中的 A/B/C 才是 CMD
			if (IS_TOUCH_AREA_CHAR(type) && note.type == NONE)
				goto note_head;

			int from = note.slide_parts_count
				? note_pos_offset(slide_parts->data[note.slide_parts_first + note.slide_parts_count - 1].end)
				: note_pos_offset(note.pos);
			if (note.type != SLIDE)
			{
				// 强制star的note依然强制，强制tap-head的依然tap-head，也就是说1$$@-4 == 1-4
				note.is_star = is_tap_head_slide && !note.is_star ? false : true;
				double head_duration = (note.type == HOLD || note.type == TOUCHHOLD)
					? note.duration : NAN;
				SimaiNotePos pos = note.pos;
				EMIT_NOTE; // 星星头
				hold_slide_duration = head_duration;
				note.pos = pos;
				note.is_slide_no_star_fade = is_slide_no_star_fade;
			}
			note.type = SLIDE;


			// pp qq
			bool doubled = (type == 'p' || type == 'q') &&
				i + 1 < text->count && text->data[i + 1] == type;
			if (doubled) i++;
			// V
			int mid = 0;
			if (type == 'V')
			{
				if (i + 1 >= text->count || !IS_NOTE_CHAR(text->data[i + 1]))
				{
					invalid_slide = true;
					break;
				}
				mid = text->data[++i] - '0';
			}

			SlidePart part = { .end = NOTE_POS_NONE };
			size_t cursor = i + 1;
			if (!get_note_pos(*text, &cursor, &part.end))
			{
				invalid_slide = true;
				break;
			}
			i = cursor - 1; // 外层 for 还会推进一次，保留下一个未消费字符。
			int to = note_pos_offset(part.end);

			switch (type)
			{
			case '-': part.type = STRAIGHT; break;
			case '^':
			case '<':
			case '>': part.type = translate_circle(type, from, to); break;
			case 'v': part.type = CORNER; break;
			case 'V':
			{
				int offset = (mid - from + 8) % 8;
				if (offset == 6) part.type = SKIP_L;
				else if (offset == 2) part.type = SKIP_R;
				break;
			}
			case 'p': part.type = doubled ? BEND_L : CURVE_L; break;
			case 'q': part.type = doubled ? BEND_R : CURVE_R; break;
			case 's': part.type = THUNDER_L; break;
			case 'z': part.type = THUNDER_R; break;
			case 'w': part.type = FAN; break;
			case 'A': part.type = A_CMD; break;
			case 'B': part.type = B_CMD; break;
			case 'C': part.type = C_CMD; break;
			case 'P': part.type = P_CMD; break;
			case 'Q': part.type = Q_CMD; break;
			case 'K': part.type = K_CMD; break;
			}

			bool needs_from = type == '<' || type == '>' || type == '^' || type == 'V';
			if (part.type == SLIDE_NONE || (needs_from && from == 0) || (type == '^' && to == 0))
				invalid_slide = true;
			if (invalid_slide) break;

			if (note.slide_parts_count == 0)
				note.slide_parts_first = slide_parts->len;
			vec_push(slide_parts, part);
			note.slide_parts_count++;
			break;
		}




		case '*':
			if (note.type == SLIDE)
			{
				SimaiNotePos pos = note.pos;
				EMIT_NOTE;
				note.pos = pos;
			}
			break;



		case '/':
			String_View cur_note = sv_from_parts(note_start, text->data + i - note_start);
			if (notes->len > notes_first && sv_eq(last_note, cur_note))
			{
				notes->data[notes->len - 1].can_be_folded = true;
			}
			last_note = cur_note;

			EMIT_NOTE;
			break;


		case '`':
			EMIT_NOTE;
			sv_chop_left(text, i + 1);
			return CIMAI_NOTE_FAKE_EACH;
		case ',':
			EMIT_NOTE;
			sv_chop_left(text, i + 1);
			return CIMAI_NOTE_SUCCESS;

		default:
		note_head:
			// 音符头
			if (note.type == NONE &&
				(IS_NOTE_CHAR(text->data[i]) || IS_TOUCH_AREA_CHAR(text->data[i])))
			{
				size_t cursor = i;
				SimaiNotePos pos;
				if (!get_note_pos(*text, &cursor, &pos))
					break;
				note_start = text->data + i;
				note.pos = pos;
				note.type = pos >= BTN1 && pos <= BTN8 ? TAP : TOUCH;
				i = cursor - 1;
			}
			break;
		}
	}

fail:
	// 回滚本组追加的范围，保留此前 timing 已提交的音符和路径。
	notes->len = notes_first;
	slide_parts->len = slide_parts_first;
	sv_chop_by_delim(text, ',');
	return error;
#undef EMIT_NOTE
}


static void calc_each(SimaiNote_Vec *notes, size_t first)
{
	int note_count = 0;
	int slide_count = 0;

	for (size_t i = first; i < notes->len; i++)
	{
		SimaiNote o = notes->data[i];
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

	for (size_t i = first; i < notes->len; i++)
	{
		SimaiNote *o = &notes->data[i];
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

	SimaiTiming_Vec *timings = &chart->timings;

	SimaiNote_Vec *notes = &chart->notes;
	SlidePart_Vec *slide_parts = &chart->slide_parts;

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
					// 伪双的每组独立提交范围，依次晚一个 128 分音符。
					size_t fake_count = 0;
					for (;;)
					{
						size_t notes_first = notes->len;
						ParseNoteRetCode ret = parse_timing(ctx, &text, notes, slide_parts);
						if (ret <= 0)
						{
							// 消费了逗号的错误组仍占一拍；末尾未闭合组不推进。
							if (text.data > chart_start && text.data[-1] == ',')
								ctx.time += ctx.seconds_per_beat;
							break;
						}

						calc_each(notes, notes_first);
						SimaiTiming timing = {
							.time = ctx.time + fake_count * (60 / ctx.bpm * 4 / 128),
							.bpm = ctx.bpm,
							.hspeed = ctx.hspeed,
							.sveloc = ctx.sveloc,
							.sign_num = ctx.sign_num,
							.sign_den = ctx.sign_den,
							.content = sv_from_parts(last_timing_end, text.data - last_timing_end),
							.fumen_pos = text.data - chart_start,
							.notes_first = notes_first,
							.notes_count = notes->len - notes_first,
						};
						last_timing_end = text.data;
						vec_push(timings, timing);

						if (ret != CIMAI_NOTE_FAKE_EACH)
						{
							ctx.time += ctx.seconds_per_beat;
							break;
						}
						fake_count++;
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

}

// 释放 chart 内部由 cimai_parse_chart 分配的资源。
void cimai_chart_release(SimaiChart *chart)
{
	if (chart == NULL)
		return;
	vec_free(&chart->slide_parts);
	vec_free(&chart->notes);
	vec_free(&chart->timings);
}
