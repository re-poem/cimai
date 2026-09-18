using System;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.CompilerServices;
using System.Text;

namespace Cimai;

public sealed unsafe class SimaiChart : IDisposable
{
    private Native.SimaiChart _native;
    public bool IsDisposed { get; private set; }

    public ReadOnlySpan<byte> Level
    {
        get
        {
            ThrowIfDisposed();
            return new ReadOnlySpan<byte>(_native.level.data, checked((int)_native.level.count));
        }
    }

    public ReadOnlySpan<byte> Des
    {
        get
        {
            ThrowIfDisposed();
            return new ReadOnlySpan<byte>(_native.des.data, checked((int)_native.des.count));
        }
    }

    public ReadOnlySpan<byte> Fumen
    {
        get
        {
            ThrowIfDisposed();
            return new ReadOnlySpan<byte>(_native.fumen.data, checked((int)_native.fumen.count));
        }
    }

    public ReadOnlySpan<SimaiTiming> Timings
    {
        get
        {
            ThrowIfDisposed();
            return new ReadOnlySpan<SimaiTiming>(_native.timings.items, checked((int)_native.timings.count));
        }
    }



    public static readonly SimaiChart Empty = new();

    internal SimaiChart() => _native = default;
    // 用于从file中摘除chart
    internal SimaiChart(Native.SimaiChart native) => _native = native;
    public static SimaiChart Parse(string source) => Parse(Encoding.UTF8.GetBytes(source));
    public static SimaiChart Parse(byte[] source)
    {
        var chart = new SimaiChart();
        fixed (Native.SimaiChart* p = &chart._native)
        fixed (byte* sp = source)
        {
            p->fumen = new Native.String_View { count = (nuint)source.Length, data = sp };
            Native.Methods.cimai_parse_chart(p);
        }
        return chart;
    }



    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private void ThrowIfDisposed()
    {
        if (IsDisposed)
            ThrowDisposed();
    }

    [DoesNotReturn]
    private static void ThrowDisposed()
    {
        throw new ObjectDisposedException(nameof(SimaiChart));
    }

    private void Free()
    {
        fixed (Native.SimaiChart* p = &_native)
        {
            Native.Methods.cimai_chart_release(p);
        }
        _native = default;
    }

    public void Dispose()
    {
        if (IsDisposed) return;
        IsDisposed = true;
        Free();
        GC.SuppressFinalize(this);
    }

    ~SimaiChart() => Free();
}