// test_golden.c —— golden 测试
// 1) 参考谱面截取的"全集语法片段"：完整 dump 与 golden 文件逐字节对比
// 2) 完整参考谱面（tests/data/hello_2025_maidata.txt）：结构统计 + 关键时间点
//
// 运行方式：
//   test_golden           正常测试（对比 golden 文件）
//   test_golden --dump    输出完整参考谱面的 dump 到 stdout（用于生成/检查 golden）
#include "test.h"
#include <stdio.h>
#include <stdlib.h>

#define DATA_DIR TEST_DATA_DIR

// 参考谱面前 10 行：覆盖 bpm/拍子、双押、滑条EACH、HOLD EACH、空逗号、
// 无星滑条(?)、EX、tap 连打、BPM 切换。期望值按 wiki 语义手算。
static const char *EXCERPT =
	"(253.125){1},\n"
	"{8}78,12,5-2[4:1]/6-1[4:1],3h[4:1]/4h[4:1],,,5?-2[0##0]/6?-1[0##0],\n"
	"1,1,78,56,34,12,78,56,34,\n"
	"{8}21,87,4-7[4:1]/3-8[4:1],6h[4:1]/5h[4:1],,,4?-7[0##0]/3?-8[0##0],\n"
	"8,8,21,43,65,87,21,43,65,\n"
	"(1012.5){12}\n"
	"1x/1,,7,7,7,,4x/4,,8,8,8,,2x/2,,5,5,5,,3x/3,,6,6,6,,";

static const char *note_type_chars = "?THSXU"; // NONE TAP HOLD SLIDE TOUCH TOUCHHOLD

static void render_note(char *out, size_t cap, const SimaiNote *n)
{
	int w = snprintf(out, cap, "%c%c%d",
		note_type_chars[n->type],
		n->touch_area ? n->touch_area : '-',
		n->start_pos);
	if (n->type == HOLD || n->type == SLIDE || n->type == TOUCHHOLD)
		w += snprintf(out + w, cap - w, ":%.6f", n->duration);
	if (n->type == SLIDE)
		w += snprintf(out + w, cap - w, ":%.6f(" SV_Fmt ")",
			n->slide_shoot_delay, SV_Arg(n->slide_content));
	// 标志顺序固定：e,b,x,m,s,r,f,n,i
	if (n->is_each) out[w++] = 'e';
	if (n->is_break) out[w++] = 'b';
	if (n->is_ex) out[w++] = 'x';
	if (n->is_mine) out[w++] = 'm';
	if (n->is_star) out[w++] = 's';
	if (n->is_star_fake_rotate) out[w++] = 'r';
	if (n->is_hanabi) out[w++] = 'f';
	if (n->is_slide_no_star_fade) out[w++] = 'n';
	if (n->is_ignore_sv) out[w++] = 'i';
	out[w] = '\0';
}

static void render_timing(char *out, size_t cap, size_t idx, const SimaiTiming *t)
{
	int w = snprintf(out, cap, "T%zu t=%.6f bpm=%.4f hs=%.2f sv=%.2f n=%zu |",
		idx, t->time, t->bpm, t->hspeed, t->sveloc, t->notes.count);
	for (size_t i = 0; i < t->notes.count && w < (int)cap; i++)
	{
		char nb[192];
		render_note(nb, sizeof nb, &t->notes.items[i]);
		w += snprintf(out + w, cap - w, " %s", nb);
	}
}

static size_t dump_chart(char *out, size_t cap, const SimaiChart *c)
{
	size_t w = 0;
	for (size_t i = 0; i < c->timings.count && w < cap; i++)
	{
		char line[1024];
		render_timing(line, sizeof line, i, &c->timings.items[i]);
		w += snprintf(out + w, cap - w, "%s\n", line);
	}
	return w;
}

// 与 golden 文件逐字节对比；不一致时把实际输出写到 actual_path 便于排查
static int compare_dump(const char *dump, const char *golden_path, const char *actual_path)
{
	FILE *g = fopen(golden_path, "rb");
	if (!g)
	{
		FILE *a = fopen(actual_path, "wb");
		if (a) { fwrite(dump, 1, strlen(dump), a); fclose(a); }
		printf("golden missing: %s\n(actual written to %s)\n", golden_path, actual_path);
		return 1;
	}
	fseek(g, 0, SEEK_END);
	long gl = ftell(g);
	fseek(g, 0, SEEK_SET);
	char *golden = (char *)malloc((size_t)gl + 1);
	size_t rd = fread(golden, 1, (size_t)gl, g);
	golden[rd] = '\0';
	fclose(g);

	int fail = strcmp(dump, golden) != 0;
	if (fail)
	{
		FILE *a = fopen(actual_path, "wb");
		if (a) { fwrite(dump, 1, strlen(dump), a); fclose(a); }
		// 打印第一个差异行
		const char *d = dump, *gd = golden;
		int line = 1;
		while (*d && *gd && *d == *gd) { if (*d == '\n') line++; d++; gd++; }
		printf("GOLDEN MISMATCH at line %d\n  actual : %s\n  golden : %s\n",
			line, *d ? d : "(end)", *gd ? gd : "(end)");
		printf("full actual dump written to %s\n", actual_path);
	}
	free(golden);
	return fail;
}

static void test_excerpt_golden(void)
{
	char path[1024], actual[1024];
	snprintf(path, sizeof path, "%s/golden_excerpt.txt", DATA_DIR);
	snprintf(actual, sizeof actual, "%s/excerpt_actual.txt", DATA_DIR);

	SimaiChart *c = parse_chart_cstr(EXCERPT);
	char *dump = (char *)malloc(1 << 20);
	size_t n = dump_chart(dump, 1 << 20, c);
	dump[n] = '\0';
	CHECK_MSG(compare_dump(dump, path, actual) == 0, "excerpt dump 与 golden 一致");
	free(dump);
	cimai_chart_free(&c);
}

static void test_full_chart(void)
{
	char path[1024];
	snprintf(path, sizeof path, "%s/hello_2025_maidata.txt", DATA_DIR);
	FILE *fp = fopen(path, "rb");
	CHECK_MSG(fp != NULL, "参考谱面文件可读");
	if (!fp)
		return;

	fseek(fp, 0, SEEK_END);
	long len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *buf = (char *)malloc((size_t)len + 1);
	if (fread(buf, 1, (size_t)len, fp) != (size_t)len) { fclose(fp); free(buf); return; }
	buf[len] = '\0';
	fclose(fp);

	String_View sv = { (size_t)len, buf };
	SimaiFile file = { 0 };
	cimai_parse(&sv, &file);

	// 元数据
	CHECK_SV(file.title, "请设置标题");
	CHECK_SV(file.artist, "请设置艺术家");
	CHECK_SV(file.des, "请设置做谱人");
	CHECK_NEAR(file.offset, 0.01, 1e-6);
	CHECK(file.charts[EASY] != NULL);
	CHECK_SV(file.charts[EASY]->level, "18+");

	SimaiChart *ch = file.charts[EASY];
	CHECK(ch->timings.count > 0);

	// 统计（打印供核对；断言值由捕获后固化）
	size_t total_notes = 0, taps = 0, holds = 0, slides = 0, touches = 0, touchholds = 0;
	for (size_t i = 0; i < ch->timings.count; i++)
	{
		for (size_t j = 0; j < ch->timings.items[i].notes.count; j++)
		{
			const SimaiNote *n = &ch->timings.items[i].notes.items[j];
			total_notes++;
			switch (n->type)
			{
			case TAP: taps++; break;
			case HOLD: holds++; break;
			case SLIDE: slides++; break;
			case TOUCH: touches++; break;
			case TOUCHHOLD: touchholds++; break;
			default: break;
			}
		}
	}
	printf("[stats] timings=%zu notes=%zu tap=%zu hold=%zu slide=%zu touch=%zu touchhold=%zu\n",
		ch->timings.count, total_notes, taps, holds, slides, touches, touchholds);
	printf("[stats] first_time=%.6f last_time=%.6f\n",
		ch->timings.items[0].time, ch->timings.items[ch->timings.count - 1].time);

	// 结构统计（2025adx 参考谱面，捕获后固化）
	CHECK_EQ_INT(ch->timings.count, 9046);
	CHECK_EQ_INT(total_notes, 11621);
	CHECK_EQ_INT(taps, 9728);
	CHECK_EQ_INT(holds, 122);
	CHECK_EQ_INT(slides, 1206);
	CHECK_EQ_INT(touches, 556);
	CHECK_EQ_INT(touchholds, 9);
	CHECK_NEAR(ch->timings.items[0].time, 0.948148, 1e-6);
	CHECK_NEAR(ch->timings.items[ch->timings.count - 1].time, 222.786927, 1e-3);

	// 时间单调不减
	for (size_t i = 1; i < ch->timings.count; i++)
		CHECK(ch->timings.items[i].time >= ch->timings.items[i - 1].time - 1e-9);

	// 开头与截取片段一致（前 4 个 timing 的手算值）
	CHECK_NEAR(ch->timings.items[0].time, 0.9481481481481481, 1e-6);
	CHECK_EQ_INT(ch->timings.items[0].notes.count, 2);
	CHECK_NEAR(ch->timings.items[1].time, 1.0666666666666667, 1e-6);
	CHECK_NEAR(ch->timings.items[2].time, 1.1851851851851851, 1e-6);
	CHECK_EQ_INT(ch->timings.items[2].notes.count, 4);
	CHECK_NEAR(ch->timings.items[3].time, 1.3037037037037036, 1e-6);
	CHECK_EQ_INT(ch->timings.items[3].notes.count, 2);

	cimai_file_free(&file);
	free(buf);
}

int main(int argc, char **argv)
{
	if (argc > 1 && strcmp(argv[1], "--gen-golden") == 0)
	{
		// 由 C 程序生成 golden 文件（保证与 dump 完全一致，含 LF 行尾）
		SimaiChart *c = parse_chart_cstr(EXCERPT);
		char *dump = (char *)malloc(1 << 20);
		size_t n = dump_chart(dump, 1 << 20, c);
		dump[n] = '\0';
		char path[1024];
		snprintf(path, sizeof path, "%s/golden_excerpt.txt", DATA_DIR);
		FILE *f = fopen(path, "wb");
		if (!f) { printf("cannot write %s\n", path); return 1; }
		fwrite(dump, 1, n, f);
		fclose(f);
		cimai_chart_free(&c);
		free(dump);
		printf("wrote %s (%zu bytes)\n", path, n);
		return 0;
	}

	if (argc > 1 && strcmp(argv[1], "--dump") == 0)
	{
		// 输出完整参考谱面的 dump 到 stdout
		char path[1024];
		snprintf(path, sizeof path, "%s/hello_2025_maidata.txt", DATA_DIR);
		FILE *fp = fopen(path, "rb");
		if (!fp) { printf("cannot open %s\n", path); return 1; }
		fseek(fp, 0, SEEK_END);
		long len = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		char *buf = (char *)malloc((size_t)len + 1);
		fread(buf, 1, (size_t)len, fp);
		buf[len] = '\0';
		fclose(fp);

		String_View sv = { (size_t)len, buf };
		SimaiFile file = { 0 };
		cimai_parse(&sv, &file);
		SimaiChart *ch = file.charts[EASY];
		char *dump = (char *)malloc(64 << 20);
		size_t n = dump_chart(dump, 64 << 20, ch);
		dump[n] = '\0';
		fwrite(dump, 1, n, stdout);
		cimai_file_free(&file);
		free(buf);
		free(dump);
		return 0;
	}

	test_excerpt_golden();
	test_full_chart();
	return TEST_RESULT();
}
