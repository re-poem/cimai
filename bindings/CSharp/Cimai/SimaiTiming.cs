using System;
using System.Runtime.InteropServices;

namespace Cimai;

[StructLayout(LayoutKind.Sequential)]
public readonly struct SimaiTiming(Native.SimaiTiming native)
{
    private readonly Native.SimaiTiming _native = native;

    public static implicit operator SimaiTiming(Native.SimaiTiming native) => new(native);
    public static implicit operator Native.SimaiTiming(SimaiTiming wrapper) => wrapper._native;

    public double Time => _native.time;
    public float Bpm => _native.bpm;
    public float HSpeed => _native.hspeed;
    public float SVeloc => _native.sveloc;
    public nuint FumenPos => _native.fumen_pos;
    public byte SignNum => _native.sign_num;
    public byte SignDen => _native.sign_den;
    public unsafe ReadOnlySpan<SimaiNote> Notes =>
        new(_native.notes.items, checked((int)_native.notes.count));
}
