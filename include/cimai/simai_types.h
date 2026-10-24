#pragma once

#include <thirdparty/sv.h>

#include <stddef.h>
#include <stdint.h>

#include <cimai/utils/vec.h>


typedef enum
{
	NONE,
	TAP,
	HOLD,
	SLIDE,
	TOUCH,
	TOUCHHOLD
} SimaiNoteType;

typedef enum
{
	BTN0,
	BTN1,
	BTN2,
	BTN3,
	BTN4,
	BTN5,
	BTN6,
	BTN7,
	BTN8,
	BTN9,

	A1,
	A2,
	A3,
	A4,
	A5,
	A6,
	A7,
	A8,
	B1,
	B2,
	B3,
	B4,
	B5,
	B6,
	B7,
	B8,
	C1,
	C2,
	D1,
	D2,
	D3,
	D4,
	D5,
	D6,
	D7,
	D8,
	E1,
	E2,
	E3,
	E4,
	E5,
	E6,
	E7,
	E8,
} SimaiNotePos;
typedef enum
{
	SLIDE_NONE,

	STRAIGHT,
	CIRCLE_L,
	CIRCLE_R,
	CURVE_L,
	CURVE_R,
	THUNDER_L,
	THUNDER_R,
	CORNER,
	BEND_L,
	BEND_R,
	SKIP_L,
	SKIP_R,
	FAN,

	A_CMD,
	B_CMD,
	C_CMD,
	P_CMD,
	Q_CMD,
	K_CMD,
} SlideType;

typedef struct
{
	SlideType type;
	SimaiNotePos end;
} SlidePart;
DEFINE_VEC(SlidePart, SlidePart_Vec);

// 默认初始化为全0即可，排列顺序尽量可读了
typedef struct
{
	double duration;

	SimaiNoteType type;

	SimaiNotePos pos;

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
	size_t slide_parts_first;
	size_t slide_parts_count;

	//touch
	bool is_hanabi;

	bool can_be_folded;
} SimaiNote;
DEFINE_VEC(SimaiNote, SimaiNote_Vec);

typedef struct
{
	double time;
	float bpm;
	float hspeed;
	float sveloc;
	uint8_t sign_num;
	uint8_t sign_den;
	String_View content;
	size_t fumen_pos;

	size_t notes_first;
	size_t notes_count;
} SimaiTiming;
DEFINE_VEC(SimaiTiming, SimaiTiming_Vec);

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

	SimaiTiming_Vec timings;
	SimaiNote_Vec notes;
	SlidePart_Vec slide_parts;
} SimaiChart;

typedef struct
{
	String_View key;
	String_View value;
} SimaiCommand;
DEFINE_VEC(SimaiCommand, SimaiCommand_Vec);

typedef struct
{
	String_View title;
	String_View artist;
	String_View des;
	float offset;
	SimaiChart *charts[DIFFICULTY_COUNT];

	SimaiCommand_Vec commands;
} SimaiFile;