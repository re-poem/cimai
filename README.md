# cimai



## C API

```c
#include <cimai/cimai.h>

const char *text = "&title=Hello&inote_1=(180){4}1,2,";
String_View input = sv_from_cstr(text);

SimaiFile file = { 0 };
cimai_parse(&input, &file);

SimaiChart *chart = file.charts[EASY];
if (chart != NULL) {
    for (size_t i = 0; i < chart->timings.len; i++) {
        const SimaiTiming *timing = &chart->timings.data[i];
        /* Use timing and chart->notes to read chart data. */
    }
}

cimai_file_release(&file);
```

To parse a single chart, use `cimai_parse_chart`:

```c
SimaiChart chart = { 0 };
chart.fumen = sv_from_cstr("(180){4}1,2-4[4:1],,,");
cimai_parse_chart(&chart);

/* Use chart.timings, chart.notes, and chart.slide_parts. */
cimai_chart_release(&chart);
```

## C# bindings

Restore the tools and build on first use:

```sh
dotnet tool restore
dotnet build bindings/CSharp/Cimai/Cimai.csproj -p:CimaiDesktopOnly=true
```

Basic usage:

```csharp
using Cimai;
using System.Text;

unsafe
{
    var bytes = Encoding.UTF8.GetBytes("&title=Hello&inote_1=(180){4}1,2,");
    fixed (byte* input = bytes)
    using (var file = SimaiFile.Parse(input, (nuint)bytes.Length))
    {
        Console.WriteLine(Encoding.UTF8.GetString(file.Title));
        var easy = file.Charts[(int)SimaiDifficulty.EASY];
        Console.WriteLine(easy?.Timings.Length);
    }
}
```

<br/>
<br/>

---

<p align="center">
Contributions welcome.⭐ If it helps, consider starring the repo.
</p>
