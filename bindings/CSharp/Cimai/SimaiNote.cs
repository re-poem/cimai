using System;
using System.Runtime.InteropServices;

namespace Cimai;

[StructLayout(LayoutKind.Sequential)]
public readonly struct SimaiNote(Cimai.Native.SimaiNote native)
{
    private readonly Native.SimaiNote _native = native;

    public static implicit operator SimaiNote(Cimai.Native.SimaiNote native) => new(native);
    public static implicit operator Native.SimaiNote(SimaiNote wrapper) => wrapper._native;

    public SimaiNoteType Type => (SimaiNoteType)_native.type;
    public sbyte StartPos => _native.start_pos;
    public sbyte TouchArea => _native.touch_area;
    public double Duration => _native.duration;
    public bool IsEach => _native.is_each != 0;
    public bool IsBreak => _native.is_break != 0;
    public bool IsEx => _native.is_ex != 0;
    public bool IsMine => _native.is_mine != 0;
    public bool IsIgnoreSV => _native.is_ignore_sv != 0;
    public bool IsStar => _native.is_star != 0;
    public bool IsStarFakeRotate => _native.is_star_fake_rotate != 0;
    public bool IsSlideNoStarFade => _native.is_slide_no_star_fade != 0;
    public double SlideShootDelay => _native.slide_shoot_delay;
    public unsafe ReadOnlySpan<byte> SlideContent =>
        new(_native.slide_content.data, checked((int)_native.slide_content.count));
    public bool IsHanabi => _native.is_hanabi != 0;
    public bool CanBeFolded => _native.can_be_folded != 0;
}
