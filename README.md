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
<br/>
<br/>

---

<p align="center">
Contributions welcome.⭐ If it helps, consider starring the repo.
</p>
