# 使用 SDCC

## 标准符合性

SDCC 支持 ISO C90/C89、C95、C99、C11、C17、C23 的不同子集，并持续跟进后续标准
草案。嵌入式目标的资源约束、地址空间和 ABI 会带来实现定义行为。选择语言方言：

```sh
sdcc --std=c11 -mmcs51 main.c
sdcc --std=c23 -mmcs251 main.c
```

使用某个 `--std` 不表示目标后端已经实现该标准的每个可选特性。可移植程序还应避免
依赖 plain `char` 符号性、整数宽度、位域布局、指针编码、浮点格式和未对齐访问等
实现细节。需要依赖时，应使用静态断言、目标宏和回归测试明确约束。

## 编译流程

常用阶段选项：

| 选项 | 行为 |
|---|---|
| `-E` | 只预处理 |
| `--syntax-only` | 只检查语法和语义 |
| `-S` | 生成汇编，不汇编、不链接 |
| `-c` | 生成 `.rel` 目标文件，不链接 |
| 无阶段选项 | 预处理、编译、汇编并链接 |
| `-o FILE` | 指定输出文件 |
| `-V` | 显示子命令 |
| `--verbose` | 输出更完整的驱动信息 |

单文件工程：

```sh
sdcc -mmcs51 --model-small -o firmware.ihx main.c
```

多文件工程应分别编译再统一链接：

```sh
sdcc -mmcs51 --model-small -c main.c
sdcc -mmcs51 --model-small -c uart.c
sdcc -mmcs51 --model-small -o firmware.ihx main.rel uart.rel
```

所有目标文件必须使用相同目标和兼容 ABI。仅凭链接器没有报错，不能证明混合对象是
正确的。

## Intel HEX 后处理

SDCC 多数微控制器后端输出 Intel HEX。`packihx` 可以合并和规范化记录；`makebin`
可生成原始二进制。二进制格式会丢失稀疏地址信息，转换前必须确定填充值、起始地址
和最大尺寸。MCS-251 固件使用 24 位绝对地址，不能用只处理 16 位地址的脚本截断。

## 附加库

用 `sdar` 创建和管理 ASxxxx 库：

```sh
sdar -rc libdrivers.lib uart.rel gpio.rel
sdranlib libdrivers.lib
sdcc -mmcs51 main.rel -L. -ldrivers
```

库成员仍受目标、模型和 ABI 约束。项目应把库的构建选项作为产物身份的一部分，而
不是只用同一个文件名覆盖不同模型。

## 命令行选项

### 语言和预处理

- `--std=<dialect>` 选择语言方言；
- `-DNAME[=VALUE]` 定义宏；
- `-U NAME` 取消宏；
- `-I PATH` 增加头文件目录；
- `--nostdinc` 禁止默认头文件路径；
- `-Wp OPTION` 把选项传给预处理器。

### 目标选择

目标使用 `-m<target>` 选择，例如 `-mmcs51`、`-mmcs251`、`-mz80`、`-mstm8`。
`sdcc --version` 会列出当前构建包含的后端。目标选项必须出现在编译和链接每一步。

### 优化

- `--opt-code-size` 倾向代码尺寸；
- `--opt-code-speed` 倾向运行速度；
- `--max-allocs-per-node` 控制寄存器分配搜索开销；
- `--peep-file` 增加 peephole 规则；
- `--no-peep`、`--no-reg-params` 等选项用于诊断或特定 ABI 需求。

优化不会改变 C 语言允许的可观察行为，但未定义行为、错误的 `volatile` 声明、数据
竞争或不完整的内联汇编约束常在优化后暴露。遇到只在优化版本出现的问题，应先验证
程序是否符合语言和目标规则，再定位编译器缺陷。

### 调试和诊断

- `--debug` 生成 CDB/ADB 调试信息；
- `--cyclomatic` 报告函数圈复杂度；
- `--fverbose-asm` 在汇编中保留更多说明；
- `--print-search-dirs`、`-V` 和 map 文件用于检查工具及库搜索；
- `--out-fmt-ihx` 等选项选择输出格式，支持情况依目标而异。

### 链接

- `-L PATH` 增加库目录；
- `-l NAME` 链接库；
- `-Wl OPTION` 把参数传给链接器；
- `--code-loc`、`--code-size`、`--xram-loc`、`--xram-size` 和 `--iram-size`
  描述目标内存布局。

包含空格的链接器参数应整体引用：

```sh
sdcc -mmcs251 '-Wl-b GSINIT0=0xfc2800' main.c
```

## 环境变量

SDCC 可通过环境变量覆盖可执行文件、头文件和库的搜索路径。环境变量名称和优先级
可能随平台变化，应以 `sdcc --help` 和 `-V` 输出为准。CI 和可复现构建应尽量使用
显式命令行与安装前缀，避免继承用户 shell 中不可见的配置。
