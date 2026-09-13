using System;
using System.Text;

namespace Cimai;

// generator emit：
//   - VALUE_STRUCT_TYPES（SimaiNote/SimaiTiming/SimaiCommand）：readonly struct
//   - REFERENCE_CLASS_TYPES（SimaiChart/SimaiFile）：sealed unsafe partial class

public sealed unsafe partial class SimaiChart
{
    public static SimaiChart Parse(string source) => Parse(Encoding.UTF8.GetBytes(source));

    public static SimaiChart Parse(byte[] source)
    {
        var chart = new SimaiChart();
        chart.Init(source);
        return chart;
    }

    internal void Init(byte[] source)
    {
        fixed (Native.SimaiChart* p = &_native)
        fixed (byte* sp = source)
        {
            p->fumen = new Native.String_View { count = (nuint)source.Length, data = sp };
            Native.Methods.cimai_parse_chart(p);
        }
    }
}

public sealed unsafe partial class SimaiFile
{
    public static SimaiFile Parse(string source) => Parse(Encoding.UTF8.GetBytes(source));

    public static SimaiFile Parse(byte[] source)
    {
        var file = new SimaiFile();
        file.Init(source);
        return file;
    }

    internal void Init(byte[] source)
    {
        fixed (Native.SimaiFile* p = &_native)
        fixed (byte* sp = source)
        {
            var text = new Native.String_View { count = (nuint)source.Length, data = sp };
            Native.Methods.cimai_parse(&text, p);
        }
    }
}