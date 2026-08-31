using System;
using System.ComponentModel.Design;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;

namespace Cimai
{
    /// <summary>
    /// 已解析的 Simai 文件。
    /// 持有输入文本与原生 SimaiFile 的生命周期，使用完请 Dispose()。
    /// </summary>
    public unsafe class SimaiFile : IDisposable
    {
        private readonly Native.SimaiFile _native;
        private SimaiFile(string simai)
        {
            var source = Encoding.UTF8.GetBytes(simai);
            fixed (byte* p = source)
            fixed (Native.SimaiFile* file = &_native)
            {
                var text = new Native.String_View
                {
                    count = (nuint)source.Length,
                    data = p,
                };
                Native.Methods.cimai_parse(&text, file);

                // source还没被移走时先把数据留下来
                Title = Helper.ReadSV(_native.title) ?? string.Empty;
                Artist = Helper.ReadSV(_native.artist) ?? string.Empty;
                Designer = Helper.ReadSV(_native.des) ?? string.Empty;
                Offset = _native.offset;

                var diffCount = (int)Native.SimaiDifficulty.DIFFICULTY_COUNT;
                Charts = new SimaiChart[diffCount];
                for (var i = 0; i < diffCount; i++)
                {
                    Charts[i] = new(_native.charts[i]);
                }
                var cmdCount = _native.commands.count;
                Commands = new SimaiCommand[cmdCount];
                for (nuint i = 0; i < cmdCount; i++)
                {
                    Commands[i] = new SimaiCommand(_native.commands.items[i]);
                }
            }
        }
        public static SimaiFile Parse(string simai) => new(simai);

        public void Dispose()
        {
            fixed (Native.SimaiFile* file = &_native)
                Native.Methods.cimai_file_free(file);
            GC.SuppressFinalize(this);
        }
        ~SimaiFile() => Dispose();




        public readonly string Title;
        public readonly string Artist;
        public readonly string Designer;
        public readonly float Offset;

        public readonly SimaiChart[] Charts;
        public readonly SimaiCommand[] Commands;
    }

    public sealed unsafe class SimaiChart : IDisposable
    {
        private readonly Native.SimaiChart* _native;
        private SimaiChart(string simai)
        {
            var source = Encoding.UTF8.GetBytes(simai);
            fixed (byte* p = source)
            {
                var text = new Native.String_View
                {
                    count = (nuint)source.Length,
                    data = p,
                };
                _native = (Native.SimaiChart*)Marshal.AllocHGlobal(sizeof(Native.SimaiChart));
                _native->fumen = text;
                Native.Methods.cimai_parse_chart(_native);

                Level = string.Empty;
                Designer = string.Empty;
                Fumen = simai;
            }
        }
        public static SimaiChart Parse(string simai) => new(simai);

        public SimaiChart(Native.SimaiChart* native)
        {
            _native = native;
            Level = Helper.ReadSV(_native->level) ?? string.Empty;
            Designer = Helper.ReadSV(_native->des) ?? string.Empty;
            Fumen = Helper.ReadSV(_native->fumen) ?? string.Empty;
        }

        public void Dispose()
        {
            fixed (Native.SimaiChart** chart = &_native)
                Native.Methods.cimai_chart_free(chart);
            GC.SuppressFinalize(this);
        }
        ~SimaiChart() => Dispose();



        public string Level;
        public string Designer;
        public string Fumen;


        // 请不要调用String_View content！！！！！
        public ReadOnlySpan<Native.SimaiTiming> Timings =>
            new(_native->timings.items, checked((int)_native->timings.count));
    }

    public readonly struct SimaiCommand
    {
        internal SimaiCommand(Native.SimaiCommand native)
        {
            Key = Helper.ReadSV(native.key) ?? string.Empty;
            Value = Helper.ReadSV(native.value) ?? string.Empty;
        }
        public readonly string Key;
        public readonly string Value;
    }
}
