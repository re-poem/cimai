#pragma once
// cimai 测试迷你框架：无第三方依赖，ctest 直接驱动。
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include <cimai/cimai.h>

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond) CHECK_MSG(cond, #cond)
#define CHECK_MSG(cond, msg) \
	do { \
		g_checks++; \
		if (!(cond)) { \
			g_failures++; \
			printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, msg); \
		} \
	} while (0)

#define CHECK_EQ_INT(actual, expected) \
	do { \
		long long _a = (long long)(actual), _e = (long long)(expected); \
		g_checks++; \
		if (_a != _e) { \
			g_failures++; \
			printf("FAIL %s:%d  %s == %s  (%lld vs %lld)\n", __FILE__, __LINE__, #actual, #expected, _a, _e); \
		} \
	} while (0)

#define CHECK_NEAR(actual, expected, eps) \
	do { \
		double _a = (double)(actual), _e = (double)(expected); \
		g_checks++; \
		if (fabs(_a - _e) > (eps)) { \
			g_failures++; \
			printf("FAIL %s:%d  %s ~= %s  (%g vs %g)\n", __FILE__, __LINE__, #actual, #expected, _a, _e); \
		} \
	} while (0)

#define CHECK_SV(actual, cstr) \
	do { \
		String_View _a = (actual); \
		const char *_e = (cstr); \
		size_t _el = strlen(_e); \
		g_checks++; \
		if (_a.count != _el || (_el > 0 && memcmp(_a.data, _e, _el) != 0)) { \
			g_failures++; \
			printf("FAIL %s:%d  SV(" SV_Fmt ") != \"%s\"\n", __FILE__, __LINE__, SV_Arg(_a), _e); \
		} \
	} while (0)

#define TEST_RESULT() \
	(printf("%s: %d checks, %d failures\n", __FILE__, g_checks, g_failures), \
	 g_failures == 0 ? 0 : 1)

// 解析一个仅含谱面的字符串字面量（fumen 生命周期覆盖整个测试）
static SimaiChart *parse_chart_cstr(const char *fumen)
{
	SimaiChart *c = (SimaiChart *)calloc(1, sizeof *c);
	c->fumen = sv_from_parts(fumen, strlen(fumen));
	cimai_parse_chart(c);
	return c;
}

// 解析完整 SimaiFile（metadata + 谱面），内部拷贝输入文本
static SimaiFile *parse_file_cstr(const char *text)
{
	SimaiFile *f = (SimaiFile *)calloc(1, sizeof *f);
	size_t len = strlen(text);
	char *buf = (char *)malloc(len + 1);
	memcpy(buf, text, len + 1);
	String_View sv = { len, buf };
	cimai_parse(&sv, f);
	return f;
}
