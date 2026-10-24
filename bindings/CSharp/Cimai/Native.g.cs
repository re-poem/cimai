using System.Runtime.InteropServices;

namespace Cimai.Native
{
    public enum SimaiNoteType
    {
        NONE,
        TAP,
        HOLD,
        SLIDE,
        TOUCH,
        TOUCHHOLD,
    }

    public enum SimaiNotePos
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
    }

    public enum SlideType
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
    }

    public partial struct SlidePart
    {
        public SlideType type;

        public SimaiNotePos end;
    }

    public unsafe partial struct SlidePart_Vec
    {
        public SlidePart* data;

        [NativeTypeName("size_t")]
        public nuint len;

        [NativeTypeName("size_t")]
        public nuint cap;
    }

    public partial struct SimaiNote
    {
        public double duration;

        public SimaiNoteType type;

        public SimaiNotePos pos;

        [NativeTypeName("_Bool")]
        public byte is_each;

        [NativeTypeName("_Bool")]
        public byte is_break;

        [NativeTypeName("_Bool")]
        public byte is_ex;

        [NativeTypeName("_Bool")]
        public byte is_mine;

        [NativeTypeName("_Bool")]
        public byte is_ignore_sv;

        [NativeTypeName("_Bool")]
        public byte is_star;

        [NativeTypeName("_Bool")]
        public byte is_star_fake_rotate;

        [NativeTypeName("_Bool")]
        public byte is_slide_no_star_fade;

        public double slide_shoot_delay;

        [NativeTypeName("size_t")]
        public nuint slide_parts_first;

        [NativeTypeName("size_t")]
        public nuint slide_parts_count;

        [NativeTypeName("_Bool")]
        public byte is_hanabi;

        [NativeTypeName("_Bool")]
        public byte can_be_folded;
    }

    public unsafe partial struct SimaiNote_Vec
    {
        public SimaiNote* data;

        [NativeTypeName("size_t")]
        public nuint len;

        [NativeTypeName("size_t")]
        public nuint cap;
    }

    public partial struct SimaiTiming
    {
        public double time;

        public float bpm;

        public float hspeed;

        public float sveloc;

        [NativeTypeName("uint8_t")]
        public byte sign_num;

        [NativeTypeName("uint8_t")]
        public byte sign_den;

        public String_View content;

        [NativeTypeName("size_t")]
        public nuint fumen_pos;

        [NativeTypeName("size_t")]
        public nuint notes_first;

        [NativeTypeName("size_t")]
        public nuint notes_count;
    }

    public unsafe partial struct SimaiTiming_Vec
    {
        public SimaiTiming* data;

        [NativeTypeName("size_t")]
        public nuint len;

        [NativeTypeName("size_t")]
        public nuint cap;
    }

    public enum SimaiDifficulty
    {
        EASY,
        BASIC,
        ADVANCED,
        EXPERT,
        MASTER,
        REMASTER,
        ORIGINAL,
        DIFFICULTY_COUNT,
    }

    public partial struct SimaiChart
    {
        public String_View level;

        public String_View des;

        public String_View fumen;

        public SimaiTiming_Vec timings;

        public SimaiNote_Vec notes;

        public SlidePart_Vec slide_parts;
    }

    public partial struct SimaiCommand
    {
        public String_View key;

        public String_View value;
    }

    public unsafe partial struct SimaiCommand_Vec
    {
        public SimaiCommand* data;

        [NativeTypeName("size_t")]
        public nuint len;

        [NativeTypeName("size_t")]
        public nuint cap;
    }

    public partial struct SimaiFile
    {
        public String_View title;

        public String_View artist;

        public String_View des;

        public float offset;

        [NativeTypeName("SimaiChart *[7]")]
        public _charts_e__FixedBuffer charts;

        public SimaiCommand_Vec commands;

        public unsafe partial struct _charts_e__FixedBuffer
        {
            public SimaiChart* e0;
            public SimaiChart* e1;
            public SimaiChart* e2;
            public SimaiChart* e3;
            public SimaiChart* e4;
            public SimaiChart* e5;
            public SimaiChart* e6;

            public ref SimaiChart* this[int index]
            {
                get
                {
                    fixed (SimaiChart** pThis = &e0)
                    {
                        return ref pThis[index];
                    }
                }
            }
        }
    }

    public static unsafe partial class Methods
    {
        [DllImport("cimai", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void cimai_parse(String_View* text, SimaiFile* file);

        [DllImport("cimai", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void cimai_file_release(SimaiFile* file);

        [DllImport("cimai", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void cimai_parse_chart(SimaiChart* chart);

        [DllImport("cimai", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void cimai_chart_release(SimaiChart* chart);

        [DllImport("cimai", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void cimai_parse_metadata(String_View* text, SimaiFile* file);

        [DllImport("cimai", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void* cimai_malloc([NativeTypeName("size_t")] nuint size);

        [DllImport("cimai", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void* cimai_calloc([NativeTypeName("size_t")] nuint count, [NativeTypeName("size_t")] nuint size);

        [DllImport("cimai", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void* cimai_realloc(void* ptr, [NativeTypeName("size_t")] nuint size);

        [DllImport("cimai", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void cimai_free(void* ptr);
    }
}
