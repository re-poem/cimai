using System;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.CompilerServices;
using System.Text;

namespace Cimai;

public sealed unsafe class SimaiFile : IDisposable
{
    private Native.SimaiFile _native;
    private readonly SimaiChart?[] _charts = new SimaiChart[(int)SimaiDifficulty.DIFFICULTY_COUNT];

    public bool IsDisposed { get; private set; }


    public ReadOnlySpan<byte> Title
    {
        get
        {
            ThrowIfDisposed();
            return new ReadOnlySpan<byte>(_native.title.data, checked((int)_native.title.count));
        }
    }

    public ReadOnlySpan<byte> Artist
    {
        get
        {
            ThrowIfDisposed();
            return new ReadOnlySpan<byte>(_native.artist.data, checked((int)_native.artist.count));
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

    public float Offset
    {
        get
        {
            ThrowIfDisposed();
            return _native.offset;
        }
    }

    public ReadOnlySpan<SimaiChart?> Charts
    {
        get
        {
            ThrowIfDisposed();
            return _charts;
        }
    }

    public ReadOnlySpan<SimaiCommand> Commands
    {
        get
        {
            ThrowIfDisposed();
            return new ReadOnlySpan<SimaiCommand>(_native.commands.items, checked((int)_native.commands.count));
        }
    }


    public static readonly SimaiFile Empty = new();


    public static SimaiFile Parse(string source) => Parse(Encoding.UTF8.GetBytes(source));
    public static SimaiFile Parse(byte[] source)
    {
        var file = new SimaiFile();
        fixed (Native.SimaiFile* p = &file._native)
        fixed (byte* sp = source)
        {
            var text = new Native.String_View { count = (nuint)source.Length, data = sp };
            Native.Methods.cimai_parse(&text, p);
        }

        for (var i = 0; i < (int)SimaiDifficulty.DIFFICULTY_COUNT; i++)
        {
            var chart = file._native.charts[i];
            if (chart != null)
            {
                // 从file摘除，由托管端管理
                file._charts[i] = new SimaiChart(*chart);
                Native.Methods.cimai_free(chart);
                file._native.charts[i] = null;
            }
        }
        return file;
    }

    public static SimaiFile ParseMetadata(string source) => ParseMetadata(Encoding.UTF8.GetBytes(source));
    public static SimaiFile ParseMetadata(byte[] source)
    {
        var file = new SimaiFile();
        fixed (Native.SimaiFile* p = &file._native)
        fixed (byte* sp = source)
        {
            var text = new Native.String_View { count = (nuint)source.Length, data = sp };
            Native.Methods.cimai_parse_metadata(&text, p);
        }
        return file;
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
        throw new ObjectDisposedException(nameof(SimaiFile));
    }

    private void Free()
    {
        fixed (Native.SimaiFile* p = &_native)
        {
            Native.Methods.cimai_file_release(p);
            for (var i = 0; i < (int)SimaiDifficulty.DIFFICULTY_COUNT; i++)
            {
                _charts[i]?.Dispose();
                _charts[i] = null;
            }
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

    ~SimaiFile() => Free();
}