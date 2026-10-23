using System;

namespace Cimai;

public sealed unsafe class SimaiChart : IDisposable
{
    private Native.SimaiChart _native;
    public bool IsDisposed { get; private set; }

    public ReadOnlySpan<byte> Level { get { ThrowIfDisposed(); return new(_native.level.data, checked((int)_native.level.count)); } }
    public ReadOnlySpan<byte> Des { get { ThrowIfDisposed(); return new(_native.des.data, checked((int)_native.des.count)); } }
    public ReadOnlySpan<byte> Fumen { get { ThrowIfDisposed(); return new(_native.fumen.data, checked((int)_native.fumen.count)); } }
    public ReadOnlySpan<SimaiTiming> Timings { get { ThrowIfDisposed(); return new((SimaiTiming*)_native.timings.data, checked((int)_native.timings.len)); } }
    public ReadOnlySpan<SimaiNote> Notes { get { ThrowIfDisposed(); return new((SimaiNote*)_native.notes.data, checked((int)_native.notes.len)); } }
    public ReadOnlySpan<SlidePart> SlideParts { get { ThrowIfDisposed(); return new((SlidePart*)_native.slide_parts.data, checked((int)_native.slide_parts.len)); } }

    public static readonly SimaiChart Empty = new();

    internal SimaiChart() => _native = default;
    internal SimaiChart(Native.SimaiChart native) => _native = native;

    internal void ThrowIfDisposed()
    {
        if (IsDisposed) throw new ObjectDisposedException(nameof(SimaiChart));
    }

    // 调用方必须保证 source 在 chart 及其返回的 span 使用期间固定且不变。
    public static SimaiChart Parse(byte* source, int length)
    {
        var chart = new SimaiChart();
        try
        {
            fixed (Native.SimaiChart* p = &chart._native)
            {
                p->fumen = new Native.String_View { count = (nuint)length, data = source };
                Native.Methods.cimai_parse_chart(p);
            }
            return chart;
        }
        catch
        {
            chart.Dispose();
            throw;
        }
    }

    private void Free()
    {
        fixed (Native.SimaiChart* p = &_native) Native.Methods.cimai_chart_release(p);
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
