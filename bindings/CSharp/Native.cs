using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Cimai.Native
{
    // ClangSharp 生成的代码用 [NativeTypeName("...")] 记录原生 C 类型名，
    // 这是一个纯文档性 attribute，本身没有运行时行为，这里手动补一个等价的。
    [AttributeUsage(AttributeTargets.All)]
    internal sealed class NativeTypeNameAttribute(string name) : Attribute { }

    // 下面三个 POD 类型定义在 thirdparty/sv.h 与 thirdparty/arena.h 里。
    // 它们故意不参与 ClangSharp 生成（避免把一整套 sv_*/arena_* 函数带进绑定），
    // 这里手写等价的位布局，保证 Native.g.cs 里生成的各结构体按位兼容。
    //
    // String_View:  { size_t count; const char *data; }
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct String_View
    {
        public nuint count;
        public byte* data;
    }

    // struct Region { Region* next; size_t count; size_t capacity; uintptr_t data[1]; }
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct Region
    {
        public Region* next;
        public nuint count;
        public nuint capacity;
        public nuint data; // uintptr_t data[1]
    }

    // typedef struct { Region *begin, *end; } Arena;
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct Arena
    {
        public Region* begin;
        public Region* end;
    }

    public unsafe partial struct SimaiTiming
    {
        public readonly ReadOnlySpan<SimaiNote> Notes =>
            new(notes.items, checked((int)notes.count));
    }
    public unsafe partial struct SimaiNote
    {
        // SimaiNote.slide_content已明确在_arena内，生命周期跟随SimaiChart，可以直接访问
        public readonly ReadOnlySpan<byte> ContentUtf8 =>
            new(slide_content.data, checked((int)slide_content.count));
    }
}