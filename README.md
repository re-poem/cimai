# cimai

## Build

use **Visual Studio** pls.

## Test

use **MajdataX** pls.

## Usage

```c
#include <cimai/cimai.h>

// ---- metadata + charts ----
const char *text = "&title=T&inote_1=(180){4}1,2,";
String_View sv = sv_from_cstr(text);

SimaiFile file = { 0 };
cimai_parse(&sv, &file);
SimaiChart *chart = file.charts[EASY];
// chart->timings.items[i].time / .notes ...
cimai_file_free(&file); // free charts and commands


// ---- chart only ----
const char *text = "(180){4}1,2-4[4:1],,,";
String_View sv = sv_from_cstr(text);

SimaiChart chart = { 0 };
cimai_parse_chart(&sv, &chart);
// chart.timings.items[i].time / .notes ...
cimai_chart_free(&&chart);
```

## Language Bindings

### C#

C# 绑定由 MSBuild + ClangSharp 构建。

> CMake 目前直接把原生库输出到 `bindings/CSharp/Cimai/runtimes/<rid>/native/`供 C# 侧打包。

目录 `bindings/CSharp/`：

- `Cimai/Native.g.cs` —— ClangSharp 自动生成的 P/Invoke
- `Cimai/Native.cs` / `Wrapper.cs` —— 手写部分
- `Cimai.Generator/` —— 不需要手写封装的部分的自动封装

```sh
# 首次使用：恢复 ClangSharp（.config/dotnet-tools.json）
dotnet tool restore
dotnet build bindings/CSharp/Cimai.csproj
```

用法示例：

```csharp
using Cimai;

// metadata + charts
using var file = SimaiFile.Parse("&title=Hello&inote_1=(180){4}1,2,");
Console.WriteLine(file.Title);  // "Hello"
var timings = file.Charts[(int)SimaiDifficulty.EASY].Timings;

// chart only
using var chart = Cimai.SimaiChart.Parse("(180){4}1,2v2[8:1]m,,,");
var timings = chart.Timings;


foreach (var t in timings)
    Console.WriteLine($"{t.Time}, {t.Notes.Length} notes");

```

### Unity

Cimai 同时以 [UPM 包](https://docs.unity3d.com/Manual/cus-layout.html)（`com.re-poem.cimai`）的形式发布到孤儿分支 `upm`。

**通过 UPM 安装（推荐）：** Unity Editor → Window → Package Manager → `+` → **Add package by git URL**：

| 用途 | URL |
|---|---|
| 最新版 | `https://github.com/re-poem/cimai.git#upm` |
| 锁定到 v1.2.3 | `https://github.com/re-poem/cimai.git#upm-v1.2.3` |

也可直接编辑项目的 `Packages/manifest.json`：

```jsonc
{
  "dependencies": {
    "com.re-poem.cimai": "https://github.com/re-poem/cimai.git#upm-v1.2.3"
  }
}
```

**要求 Unity 2022.3 或更高版本** —— Unity 在 2022.2 才正式支持 [UPM 包内携带 Roslyn source generator](https://docs.unity3d.com/Manual/roslyn-analyzers.html)。`SimaiDifficulty` 等枚举副本、struct PascalCase 包装器、fixed buffer 包装都由包里 `SourceGenerators~/` 子目录里的 `Cimai.Generator` 在 Unity 导入包时**现场编译**后挂到 `Cimai.Runtime` 的编译流程上 —— 不是预生成的代码，源里改动即生效。

原生库（`cimai.dll` / `libcimai.so` / `libcimai.dylib`）位于 NuGet 风格的 `runtimes/<rid>/native/` 目录下，Unity 2022.2+ 会自动按平台识别。

<br/>
<br/>

---

<p align="center">
Contributions welcome.⭐ If it helps, consider starring the repo.
</p>
