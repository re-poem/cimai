using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Text;

namespace Cimai;

internal class Helper
{
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public unsafe static string? ReadSV(Native.String_View sv)
    {
        if (sv.data == null || sv.count == 0)
            return null;
        return Encoding.UTF8.GetString(sv.data, (int)sv.count);
    }
}
