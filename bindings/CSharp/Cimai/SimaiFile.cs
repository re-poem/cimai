using System;

namespace Cimai;

public sealed unsafe class SimaiFile : IDisposable
{
    private Native.SimaiFile _native;
    private readonly SimaiChart?[] _charts = new SimaiChart[(int)SimaiDifficulty.DIFFICULTY_COUNT];

    public bool IsDisposed { get; private set; }
    public ReadOnlySpan<byte> Title { get { ThrowIfDisposed(); return new(_native.title.data, checked((int)_native.title.count)); } }
    public ReadOnlySpan<byte> Artist { get { ThrowIfDisposed(); return new(_native.artist.data, checked((int)_native.artist.count)); } }
    public ReadOnlySpan<byte> Des { get { ThrowIfDisposed(); return new(_native.des.data, checked((int)_native.des.count)); } }
    public float Offset { get { ThrowIfDisposed(); return _native.offset; } }
    public ReadOnlySpan<SimaiChart?> Charts { get { ThrowIfDisposed(); return _charts; } }
    public ReadOnlySpan<SimaiCommand> Commands { get { ThrowIfDisposed(); return new((SimaiCommand*)_native.commands.data, checked((int)_native.commands.len)); } }

    public static readonly SimaiFile Empty = new();

    internal SimaiFile() => _native = default;

    public void DetachChart(SimaiDifficulty difficulty)
    {
        ThrowIfDisposed();
        _charts[(int)difficulty] = null;
    }

    private void ThrowIfDisposed()
    {
        if (IsDisposed) throw new ObjectDisposedException(nameof(SimaiFile));
    }

    // 调用方必须保证 source 在 file、其 charts 及返回 spans 使用期间固定且不变。
    public static SimaiFile Parse(byte* source, int length)
    {
        var file = new SimaiFile();
        try
        {
            var text = new Native.String_View { count = (nuint)length, data = source };
            fixed (Native.SimaiFile* p = &file._native) Native.Methods.cimai_parse(&text, p);
            for (var i = 0; i < file._charts.Length; i++)
            {
                var chart = file._native.charts[i];
                if (chart == null) continue;
                file._charts[i] = new SimaiChart(*chart);
                file._native.charts[i] = null;
                Native.Methods.cimai_free(chart);
            }
            return file;
        }
        catch
        {
            file.Dispose();
            throw;
        }
    }

    private void Free(bool disposing)
    {
        fixed (Native.SimaiFile* p = &_native)
        {
            Native.Methods.cimai_file_release(p);
            for (var i = 0; i < _charts.Length; i++)
            {
                if (disposing) _charts[i]?.Dispose();
                _charts[i] = null;
            }
        }
        _native = default;
    }

    public void Dispose()
    {
        if (IsDisposed) return;
        IsDisposed = true;
        Free(true);
        GC.SuppressFinalize(this);
    }

    ~SimaiFile() => Free(false);
}
