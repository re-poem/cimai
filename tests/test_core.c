// test_core.c —— cimai 核心行为单元测试
// 覆盖：元数据解析、BPM/拍子、TAP/HOLD/SLIDE/TOUCH、修饰符、EACH、伪EACH、双押、时间推进等。
// 期望值均按 wiki（simai語の譜面書式）与当前实现语义手算得出。
#include "test.h"

// ---------- 元数据 ----------
static void test_metadata(void)
{
	// 全字段 + 多难度 + 未知命令
	SimaiFile *f = parse_file_cstr(
		"&title=T&artist=A&des=D&first=0.01"
		"&lv_1=11&des_1=DD&lv_5=14&des_5=EE&inote_1=(120){4}1,2,"
		"&wave=WAV&");
	CHECK_SV(f->title, "T");
	CHECK_SV(f->artist, "A");
	CHECK_SV(f->des, "D");
	CHECK_NEAR(f->offset, 0.01, 1e-9);
	CHECK(f->charts[EASY] != NULL);
	CHECK_SV(f->charts[EASY]->level, "11");
	CHECK_SV(f->charts[EASY]->des, "DD");
	CHECK_SV(f->charts[EASY]->fumen, "(120){4}1,2,");
	CHECK(f->charts[MASTER] != NULL);
	CHECK_SV(f->charts[MASTER]->level, "14");
	CHECK(f->charts[BASIC] == NULL); // 未声明难度不创建
	CHECK_EQ_INT(f->commands.count, 1);
	CHECK_SV(f->commands.items[0].key, "wave");
	CHECK_SV(f->commands.items[0].value, "WAV");
	cimai_file_free(f);
	free(f);

	// 行尾换行应被 trim
	f = parse_file_cstr("&title=Test\n&artist=Artist\n&first=1.5\n");
	CHECK_SV(f->title, "Test");
	CHECK_SV(f->artist, "Artist");
	CHECK_NEAR(f->offset, 1.5, 1e-9);
	cimai_file_free(f);
	free(f);

	// 最后一项没有以 & 结尾（不应丢失）
	f = parse_file_cstr("&title=X&artist=Y");
	CHECK_SV(f->title, "X");
	CHECK_SV(f->artist, "Y");
	cimai_file_free(f);
	free(f);

	// 空格容忍
	f = parse_file_cstr("& title =  T2  & lv_2 = 4 &");
	CHECK_SV(f->title, "T2");
	CHECK(f->charts[BASIC] != NULL);
	CHECK_SV(f->charts[BASIC]->level, "4");
	cimai_file_free(f);
	free(f);

	// first 解析失败时保持 0
	f = parse_file_cstr("&first=abc&title=F&");
	CHECK_NEAR(f->offset, 0.0, 1e-9);
	cimai_file_free(f);
	free(f);
}

// ---------- 基础：TAP 与时间推进 ----------
static void test_tap_timing(void)
{
	// (120){4}：每拍 60/120*4/4 = 0.5s
	SimaiChart *c = parse_chart_cstr("(120){4}1,2,3,");
	CHECK_EQ_INT(c->timings.count, 3);
	CHECK_NEAR(c->timings.items[0].time, 0.0, 1e-9);
	CHECK_NEAR(c->timings.items[1].time, 0.5, 1e-9);
	CHECK_NEAR(c->timings.items[2].time, 1.0, 1e-9);
	for (int i = 0; i < 3; i++)
	{
		CHECK_EQ_INT(c->timings.items[i].notes.count, 1);
		CHECK_EQ_INT(c->timings.items[i].notes.items[0].type, TAP);
		CHECK_EQ_INT(c->timings.items[i].notes.items[0].start_pos, i + 1);
	}
	cimai_chart_free(&c);

	// 拍子变化：{8} = 八分音符 = 0.25s；{1} = 全音符 = 2s
	c = parse_chart_cstr("(120){8}1,");
	CHECK_NEAR(c->timings.items[0].time, 0.0, 1e-9);
	CHECK_EQ_INT(c->timings.count, 1);
	cimai_chart_free(&c);
	c = parse_chart_cstr("(120){1}1,");
	CHECK_EQ_INT(c->timings.count, 1);
	cimai_chart_free(&c);

	// 谱面中间换 BPM：换拍后下一拍按新 BPM 推进
	c = parse_chart_cstr("(120){4}1,(240)2,");
	CHECK_NEAR(c->timings.items[0].time, 0.0, 1e-9);
	CHECK_NEAR(c->timings.items[1].time, 0.5, 1e-9);
	cimai_chart_free(&c);

	// 空逗号：每个逗号占一拍，推进时间（不产生 timing）
	c = parse_chart_cstr("(120){4}1,,2,");
	CHECK_EQ_INT(c->timings.count, 2);
	CHECK_NEAR(c->timings.items[0].time, 0.0, 1e-9);
	CHECK_NEAR(c->timings.items[1].time, 1.0, 1e-9);
	cimai_chart_free(&c);

	// 空白/换行应被跳过
	c = parse_chart_cstr("(120){4}1, 2,\n3,");
	CHECK_EQ_INT(c->timings.count, 3);
	CHECK_NEAR(c->timings.items[2].time, 1.0, 1e-9);
	cimai_chart_free(&c);

	// E 结尾不产生 timing、不崩溃
	c = parse_chart_cstr("(120){4}1,E");
	CHECK_EQ_INT(c->timings.count, 1);
	cimai_chart_free(&c);
}

// ---------- 双押与 EACH ----------
static void test_each_double(void)
{
	printf("  [double tap]\n"); 	// 连写双押 "12,"
	SimaiChart *c = parse_chart_cstr("(120){4}12,");
	CHECK_EQ_INT(c->timings.count, 1);
	CHECK_EQ_INT(c->timings.items[0].notes.count, 2);
	CHECK_EQ_INT(c->timings.items[0].notes.items[0].type, TAP);
	CHECK_EQ_INT(c->timings.items[0].notes.items[0].start_pos, 1);
	CHECK_EQ_INT(c->timings.items[0].notes.items[1].start_pos, 2);
	CHECK(c->timings.items[0].notes.items[0].is_each);
	CHECK(c->timings.items[0].notes.items[1].is_each);
	cimai_chart_free(&c);

	printf("  [each slash]\n"); 	// "/" 分隔 EACH：非 TAP 混合时仍为 each
	c = parse_chart_cstr("(120){4}1/8h[2:1],");
	CHECK_EQ_INT(c->timings.items[0].notes.count, 2);
	CHECK_EQ_INT(c->timings.items[0].notes.items[0].type, TAP);
	CHECK_EQ_INT(c->timings.items[0].notes.items[1].type, HOLD);
	CHECK(c->timings.items[0].notes.items[0].is_each);
	CHECK(c->timings.items[0].notes.items[1].is_each);
	cimai_chart_free(&c);

	printf("  [each mine]\n"); 	// 雷点不参与 each 计数：1 个非雷音符 + 1 个雷 → 不是 each
	c = parse_chart_cstr("(120){4}1/2m,");
	CHECK(!c->timings.items[0].notes.items[0].is_each);
	CHECK(!c->timings.items[0].notes.items[1].is_each);
	CHECK(c->timings.items[0].notes.items[1].is_mine);
	cimai_chart_free(&c);

	printf("  [fake each]\n"); 	// 伪EACH `：两组并入同一 timing，时间偏移 1/128 拍（60/bpm*4/128）
	c = parse_chart_cstr("(120){4}1`2,");
	CHECK_EQ_INT(c->timings.count, 1);
	CHECK_EQ_INT(c->timings.items[0].notes.count, 2);
	CHECK_NEAR(c->timings.items[0].time, 60.0 / 120 * 4.0 / 128, 1e-9);
	CHECK_EQ_INT(c->timings.items[0].notes.items[0].start_pos, 1);
	CHECK_EQ_INT(c->timings.items[0].notes.items[1].start_pos, 2);
	cimai_chart_free(&c);

	printf("  [fake each advance]\n"); 	// 伪EACH 组本身占一个逗号：结束后推进一拍
	c = parse_chart_cstr("(120){4}1`2,3,");
	CHECK_EQ_INT(c->timings.count, 2);
	CHECK_NEAR(c->timings.items[0].time, 60.0 / 120 * 4.0 / 128, 1e-9);
	CHECK_NEAR(c->timings.items[1].time, 0.5, 1e-9);
	CHECK_EQ_INT(c->timings.items[1].notes.items[0].start_pos, 3);
	cimai_chart_free(&c);
}

// ---------- 修饰符 ----------
static void test_modifiers(void)
{
	// b / x / bx 顺序任意
	SimaiChart *c = parse_chart_cstr("(120){4}1b,1x,1bx,1xb,");
	CHECK(c->timings.items[0].notes.items[0].is_break);
	CHECK(!c->timings.items[0].notes.items[0].is_ex);
	CHECK(c->timings.items[1].notes.items[0].is_ex);
	CHECK(!c->timings.items[1].notes.items[0].is_break);
	CHECK(c->timings.items[2].notes.items[0].is_break && c->timings.items[2].notes.items[0].is_ex);
	CHECK(c->timings.items[3].notes.items[0].is_break && c->timings.items[3].notes.items[0].is_ex);
	cimai_chart_free(&c);

	// m / c
	c = parse_chart_cstr("(120){4}1m,1c,");
	CHECK(c->timings.items[0].notes.items[0].is_mine);
	CHECK(c->timings.items[1].notes.items[0].is_ignore_sv);
	cimai_chart_free(&c);

	// $ 星形 / $$ 旋转星形
	c = parse_chart_cstr("(120){4}1$,1$$,");
	CHECK(c->timings.items[0].notes.items[0].is_star);
	CHECK(!c->timings.items[0].notes.items[0].is_star_fake_rotate);
	CHECK(c->timings.items[1].notes.items[0].is_star);
	CHECK(c->timings.items[1].notes.items[0].is_star_fake_rotate);
	cimai_chart_free(&c);
}

// ---------- HOLD ----------
static void test_hold(void)
{
	// [4:1] = 60/bpm*4/4*1
	SimaiChart *c = parse_chart_cstr("(120){4}1h[4:1],");
	CHECK_EQ_INT(c->timings.items[0].notes.items[0].type, HOLD);
	CHECK_NEAR(c->timings.items[0].notes.items[0].duration, 0.5, 1e-6);
	cimai_chart_free(&c);

	// [#秒] 绝对时长
	c = parse_chart_cstr("(120){4}1h[#0.25],");
	CHECK_NEAR(c->timings.items[0].notes.items[0].duration, 0.25, 1e-6);
	cimai_chart_free(&c);

	// [BPM#α:β] 指定 BPM
	c = parse_chart_cstr("(120){4}1h[240#4:1],");
	CHECK_NEAR(c->timings.items[0].notes.items[0].duration, 60.0 / 240 * 4 / 4, 1e-6);
	cimai_chart_free(&c);

	// EX-HOLD、BREAK-HOLD 顺序任意
	c = parse_chart_cstr("(120){4}3hx[4:1],5bh[4:1],7bxh[4:1],");
	CHECK(c->timings.items[0].notes.items[0].is_ex);
	CHECK(c->timings.items[1].notes.items[0].is_break);
	CHECK(c->timings.items[2].notes.items[0].is_break && c->timings.items[2].notes.items[0].is_ex);
	cimai_chart_free(&c);

	// 疑似TAP：无时长 hold（3h,）
	c = parse_chart_cstr("(120){4}3h,");
	CHECK_EQ_INT(c->timings.items[0].notes.items[0].type, HOLD);
	CHECK_NEAR(c->timings.items[0].notes.items[0].duration, 0.0, 1e-9);
	cimai_chart_free(&c);
}

// ---------- SLIDE ----------
static void test_slide(void)
{
	// 基础：1-4[8:1] → 星形TAP + SLIDE，时长 0.25，启动延迟一拍 0.5
	SimaiChart *c = parse_chart_cstr("(120){4}1-4[8:1],");
	CHECK_EQ_INT(c->timings.items[0].notes.count, 2);
	CHECK_EQ_INT(c->timings.items[0].notes.items[0].type, TAP);
	CHECK(c->timings.items[0].notes.items[0].is_star);
	CHECK_EQ_INT(c->timings.items[0].notes.items[1].type, SLIDE);
	CHECK_SV(c->timings.items[0].notes.items[1].slide_content, "1-4");
	CHECK_NEAR(c->timings.items[0].notes.items[1].duration, 0.25, 1e-6);
	CHECK_NEAR(c->timings.items[0].notes.items[1].slide_shoot_delay, 0.5, 1e-6);
	cimai_chart_free(&c);

	// [BPM#α:β]：移动按指定 BPM，等待也按指定 BPM
	c = parse_chart_cstr("(120){4}1-4[160#8:3],");
	CHECK_NEAR(c->timings.items[0].notes.items[1].duration, 60.0 / 160 * 4.0 / 8 * 3, 1e-6);
	CHECK_NEAR(c->timings.items[0].notes.items[1].slide_shoot_delay, 60.0 / 160 * 4.0 / 4, 1e-6);
	cimai_chart_free(&c);

	// [秒##秒] 双绝对时长
	c = parse_chart_cstr("(120){4}1-4[3##1.5],");
	CHECK_NEAR(c->timings.items[0].notes.items[1].duration, 1.5, 1e-6);
	CHECK_NEAR(c->timings.items[0].notes.items[1].slide_shoot_delay, 3.0, 1e-6);
	cimai_chart_free(&c);

	// [秒##BPM#α:β]
	c = parse_chart_cstr("(120){4}1-4[3##160#8:3],");
	CHECK_NEAR(c->timings.items[0].notes.items[1].duration, 60.0 / 160 * 4.0 / 8 * 3, 1e-6);
	CHECK_NEAR(c->timings.items[0].notes.items[1].slide_shoot_delay, 3.0, 1e-6);
	cimai_chart_free(&c);

	// 同始点：1-4[4:1]*-6[8:1] → 两条滑条共享星头，均为 each
	c = parse_chart_cstr("(120){4}1-4[4:1]*-6[8:1],");
	CHECK_EQ_INT(c->timings.items[0].notes.count, 3);
	CHECK_EQ_INT(c->timings.items[0].notes.items[1].type, SLIDE);
	CHECK_SV(c->timings.items[0].notes.items[1].slide_content, "1-4");
	CHECK_EQ_INT(c->timings.items[0].notes.items[2].type, SLIDE);
	CHECK_SV(c->timings.items[0].notes.items[2].slide_content, "1-6");
	CHECK(c->timings.items[0].notes.items[1].is_each);
	CHECK(c->timings.items[0].notes.items[2].is_each);
	cimai_chart_free(&c);

	// 连结滑条：1-4q7-2[1:2] → 单条 SLIDE，路径 "1-4q7-2"，时长全音符2个=4s
	c = parse_chart_cstr("(120){4}1-4q7-2[1:2],");
	CHECK_EQ_INT(c->timings.items[0].notes.count, 2);
	CHECK_EQ_INT(c->timings.items[0].notes.items[1].type, SLIDE);
	CHECK_SV(c->timings.items[0].notes.items[1].slide_content, "1-4q7-2");
	CHECK_NEAR(c->timings.items[0].notes.items[1].duration, 4.0, 1e-6);
	cimai_chart_free(&c);

	// BREAK SLIDE：1-4[4:1]b → b 落在 SLIDE 上
	c = parse_chart_cstr("(120){4}1-4[4:1]b,");
	CHECK(c->timings.items[0].notes.items[1].is_break);
	cimai_chart_free(&c);

	// @：星形TAP 变回普通 TAP
	c = parse_chart_cstr("(120){4}1@-4[4:1],");
	CHECK(!c->timings.items[0].notes.items[0].is_star);
	CHECK_EQ_INT(c->timings.items[0].notes.items[1].type, SLIDE);
	cimai_chart_free(&c);

	// ?：无星形TAP（星头被吞，只剩 SLIDE）
	c = parse_chart_cstr("(120){4}1?-4[4:1],");
	CHECK_EQ_INT(c->timings.items[0].notes.count, 1);
	CHECK_EQ_INT(c->timings.items[0].notes.items[0].type, SLIDE);
	CHECK_SV(c->timings.items[0].notes.items[0].slide_content, "1-4");
	cimai_chart_free(&c);

	// !：无星且不淡入
	c = parse_chart_cstr("(120){4}1!-4[4:1],");
	CHECK_EQ_INT(c->timings.items[0].notes.count, 2);
	CHECK(c->timings.items[0].notes.items[1].is_slide_no_star_fade);
	cimai_chart_free(&c);
}

// ---------- TOUCH ----------
static void test_touch(void)
{
	SimaiChart *c = parse_chart_cstr("(120){4}B1,D4,C,Ch[4:1],B7f,");
	CHECK_EQ_INT(c->timings.items[0].notes.items[0].type, TOUCH);
	CHECK_EQ_INT(c->timings.items[0].notes.items[0].touch_area, 'B');
	CHECK_EQ_INT(c->timings.items[0].notes.items[0].start_pos, 1);
	CHECK_EQ_INT(c->timings.items[1].notes.items[0].touch_area, 'D');
	CHECK_EQ_INT(c->timings.items[1].notes.items[0].start_pos, 4);
	CHECK_EQ_INT(c->timings.items[2].notes.items[0].touch_area, 'C'); // 中心无编号
	CHECK_EQ_INT(c->timings.items[3].notes.items[0].type, TOUCHHOLD);
	CHECK_EQ_INT(c->timings.items[3].notes.items[0].touch_area, 'C');
	CHECK_NEAR(c->timings.items[3].notes.items[0].duration, 0.5, 1e-6);
	CHECK(c->timings.items[4].notes.items[0].is_hanabi);
	cimai_chart_free(&c);
}

// ---------- HS / SV ----------
static void test_hs_sv(void)
{
	SimaiChart *c = parse_chart_cstr("(120){4}<HS*2>1,<SV*0.5>2,");
	CHECK_NEAR(c->timings.items[0].hspeed, 2.0, 1e-6); // HS 在首个音符前设置，作用于它
	CHECK_NEAR(c->timings.items[1].sveloc, 0.5, 1e-6);
	CHECK_NEAR(c->timings.items[1].hspeed, 2.0, 1e-6);
	cimai_chart_free(&c);
}

// ---------- 内存生命周期 ----------
static void test_lifetime(void)
{
	SimaiFile *f = parse_file_cstr("&title=T&inote_1=(120){4}1,2,3,");
	CHECK_EQ_INT(f->charts[EASY]->timings.count, 3);
	// chart 释放后指针置空，重复释放安全
	cimai_chart_free(&f->charts[EASY]);
	cimai_chart_free(&f->charts[EASY]);
	CHECK(f->charts[EASY] == NULL);
	cimai_file_free(f);
	cimai_file_free(f); // 重复释放安全
	free(f);
}

int main(void)
{
#define RUN(f) do { printf("## %s\n", #f); fflush(stdout); f(); } while (0)
	RUN(test_metadata);
	RUN(test_tap_timing);
	RUN(test_each_double);
	RUN(test_modifiers);
	RUN(test_hold);
	RUN(test_slide);
	RUN(test_touch);
	RUN(test_hs_sv);
	RUN(test_lifetime);
#undef RUN
	return TEST_RESULT();
}
