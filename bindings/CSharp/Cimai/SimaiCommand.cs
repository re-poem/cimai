using System;
using System.Runtime.InteropServices;

namespace Cimai;

[StructLayout(LayoutKind.Sequential)]
public readonly struct SimaiCommand(Native.SimaiCommand native)
{
    private readonly Native.SimaiCommand _native = native;

    public static implicit operator SimaiCommand(Cimai.Native.SimaiCommand native) => new(native);
    public static implicit operator Cimai.Native.SimaiCommand(SimaiCommand wrapper) => wrapper._native;

    public unsafe ReadOnlySpan<byte> Key =>
        new(_native.key.data, checked((int)_native.key.count));
    public unsafe ReadOnlySpan<byte> Value =>
        new(_native.value.data, checked((int)_native.value.count));
}
