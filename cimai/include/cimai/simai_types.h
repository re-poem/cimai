#pragma once

#include <thirdparty/arena.h>
#include <thirdparty/sv.h>

#include <stddef.h>
#include <stdint.h>


typedef enum
{
	NONE,
	TAP,
	HOLD,
	SLIDE,
	TOUCH,
	TOUCHHOLD
} SimaiNoteType;

// 默认初始化为全0即可
typedef struct
{
	SimaiNoteType type;

	int8_t start_pos;
	char touch_area;

	double duration;

	bool is_each;

	bool is_break;
	bool is_ex;
	bool is_mine;
	bool is_ignore_sv;

	// tap
	bool is_star;
	bool is_star_fake_rotate;
	// 这两个在parser中计算是复杂且不值得的
	//bool is_star_double;
	//float star_rotate_speed;
	//slide
	bool is_slide_no_star_fade;
	double slide_shoot_delay;
	String_View slide_content;
	//touch
	bool is_hanabi;
} SimaiNote;
typedef struct
{
	SimaiNote *items;
	size_t count;
	size_t capacity;
} SimaiNoteList;

typedef struct
{
	double time;
	float bpm;
	float hspeed;
	float sveloc;
	String_View content;
	size_t fumen_pos;
	uint8_t sign_num;
	uint8_t sign_den;
	SimaiNoteList notes;
} SimaiTiming;
typedef struct
{
	SimaiTiming *items;
	size_t count;
	size_t capacity;
} SimaiTimingList;

typedef enum
{
	EASY,
	BASIC,
	ADVANCED,
	EXPERT,
	MASTER,
	REMASTER,
	ORIGINAL,

	DIFFICULTY_COUNT
} SimaiDifficulty;

typedef struct
{
	String_View level;
	String_View des;
	String_View fumen;

	// 存储notes.slide_content
	Arena _arena;

	SimaiTimingList timings;
} SimaiChart;

typedef struct
{
	String_View key;
	String_View value;
} SimaiCommand;
typedef struct
{
	SimaiCommand *items;
	size_t count;
	size_t capacity;
} SimaiCommandList;

typedef struct
{
	String_View title;
	String_View artist;
	String_View des;
	float offset;
	SimaiChart *charts[DIFFICULTY_COUNT];

	SimaiCommandList commands;
} SimaiFile;