using System;
using System.Text;

namespace Cimai;

public unsafe partial struct SimaiFile : IDisposable
{
    public static SimaiFile Parse(string fumen) => Parse(Encoding.UTF8.GetBytes(fumen));
    public static SimaiFile Parse(byte[] source)
    {
        var file = stackalloc Native.SimaiFile[1];
        fixed (byte* p = source)
        {
            var text = new Native.String_View
            {
                count = (nuint)source.Length,
                data = p,
            };
            Native.Methods.cimai_parse(&text, file);
        }
        return new SimaiFile(*file);
    }

    public void Dispose()
    {
        fixed (SimaiFile* p = &this)
        {
            Native.Methods.cimai_file_free((Native.SimaiFile*)p);
        }
    }
}

public unsafe partial struct SimaiChart : IDisposable
{
    public static SimaiChart Parse(string fumen) => Parse(Encoding.UTF8.GetBytes(fumen));
    public static SimaiChart Parse(byte[] source)
    {
        var chart = stackalloc Native.SimaiChart[1];
        fixed (byte* p = source)
        {
            chart->fumen = new Native.String_View
            {
                count = (nuint)source.Length,
                data = p,
            };
            Native.Methods.cimai_parse_chart(chart);
        }
        return new SimaiChart(*chart);
    }

    public void Dispose()
    {
        fixed (SimaiChart* self = &this)
        {
            var p = (Native.SimaiChart*)self;
            Native.Methods.cimai_chart_free(&p);
        }
    }
}