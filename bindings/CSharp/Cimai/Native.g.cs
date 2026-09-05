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

    public partial struct SimaiNote
    {
        public SimaiNoteType type;

        [NativeTypeName("int8_t")]
        public sbyte start_pos;

        [NativeTypeName("char")]
        public sbyte touch_area;

        public double duration;

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

        public String_View slide_content;

        [NativeTypeName("_Bool")]
        public byte is_hanabi;

        [NativeTypeName("_Bool")]
        public byte can_be_folded;
    }

    public unsafe partial struct SimaiNoteList
    {
        public SimaiNote* items;

        [NativeTypeName("size_t")]
        public nuint count;

        [NativeTypeName("size_t")]
        public nuint capacity;
    }

    public partial struct SimaiTiming
    {
        public double time;

        public float bpm;

        public float hspeed;

        public float sveloc;

        public String_View content;

        [NativeTypeName("size_t")]
        public nuint fumen_pos;

        [NativeTypeName("uint8_t")]
        public byte sign_num;

        [NativeTypeName("uint8_t")]
        public byte sign_den;

        public SimaiNoteList notes;
    }

    public unsafe partial struct SimaiTimingList
    {
        public SimaiTiming* items;

        [NativeTypeName("size_t")]
        public nuint count;

        [NativeTypeName("size_t")]
        public nuint capacity;
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

        public Arena _arena;

        public SimaiTimingList timings;
    }

    public partial struct SimaiCommand
    {
        public String_View key;

        public String_View value;
    }

    public unsafe partial struct SimaiCommandList
    {
        public SimaiCommand* items;

        [NativeTypeName("size_t")]
        public nuint count;

        [NativeTypeName("size_t")]
        public nuint capacity;
    }

    public partial struct SimaiFile
    {
        public String_View title;

        public String_View artist;

        public String_View des;

        public float offset;

        [NativeTypeName("SimaiChart *[7]")]
        public _charts_e__FixedBuffer charts;

        public SimaiCommandList commands;

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
        public static extern void cimai_file_free(SimaiFile* file);

        [DllImport("cimai", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void cimai_parse_chart(SimaiChart* chart);

        [DllImport("cimai", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void cimai_chart_free(SimaiChart** chart);

        [DllImport("cimai", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void cimai_parse_metadata(String_View* text, SimaiFile* file);
    }
}
