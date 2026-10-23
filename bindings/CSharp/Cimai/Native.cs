using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Cimai.Native;

// ClangSharp 生成的代码用 [NativeTypeName("...")] 记录原生 C 类型名，
// 这是一个纯文档性 attribute，本身没有运行时行为，这里手动补一个等价的。
[AttributeUsage(AttributeTargets.All)]
#pragma warning disable CS9113 // 参数未读。
internal sealed class NativeTypeNameAttribute(string name) : Attribute { }
#pragma warning restore CS9113 // 参数未读。

// String_View 定义在 thirdparty/sv.h，手写布局以免生成整套 sv_* 函数。
// String_View:  { size_t count; const char *data; }
[StructLayout(LayoutKind.Sequential)]
public unsafe struct String_View
{
    public nuint count;
    public byte* data;
}
