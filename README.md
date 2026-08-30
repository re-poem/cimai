# cimai

## 构建

```sh
cmake --preset windows        # 生成 VS 工程（out/build/windows）
cmake --build out/build/windows --config Debug
```

## 测试

```sh
ctest --test-dir out/build/windows -C Debug --output-on-failure
```
测试代码完全Vibe而成，~~我觉得是时候得有人做simai-full-test了（~~

测试套件位于 `tests/`：

- `test_core` —— 元数据、BPM/拍子、TAP/HOLD/SLIDE/TOUCH、修饰符、EACH、伪EACH、
  同拍双押、HS/SV、内存生命周期等单元测试（期望值按 simai 官方谱面书式手算）。
- `test_golden` —— golden 测试：
  - 参考谱面前 10 行的完整解析 dump 与 `tests/data/golden_excerpt.txt` 逐字节对比；
  - 完整参考谱面（`tests/data/hello_2025_maidata.txt`，改编自 "Hello (BPM) 2025"）
    的元数据、结构统计（timings/各音符类型计数）与关键时间点断言。

重新生成 golden 文件（仅在有意变更解析行为后执行）：

```sh
out/build/windows/tests/Debug/test_golden.exe --gen-golden
```

## 使用

```c
#include <cimai/cimai.h>

const char *text = "&title=T&inote_1=(180){4}1,2,";
String_View sv = sv_from_cstr(text);
SimaiFile file = { 0 };
cimai_parse(&sv, &file);          // 解析元数据 + 全部难度谱面
SimaiChart *ch = file.charts[EASY];
// ch->timings.items[i].time / .notes ...
cimai_file_free(&file);           // 释放 charts 与命令表（charts 指针被置 NULL）
```
