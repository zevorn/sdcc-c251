# Zephyr 与 GNU C 兼容性评估

评估日期：2026-08-02

本章将 C frontend 兼容性与构建 Zephyr 所需的 ABI、目标文件格式和架构移植工作
分开说明。首个可复现目标确定为采用默认 C17 配置的 Zephyr 4.4。

## 结论

SDCC 已能为 MCS-51 和 MCS-251 接受严格的 ISO C17 源码。当前下游还支持
`--std=gnu11` 和 `--std=gnu17`，提供常用 GNU keyword alias 与 attribute syntax，
实现了 statement expression、类型兼容性查询和常量表达式查询，并能保留 GNU
分支预期提示；MCS-51 与 MCS-251 还提供了 GNU bit-counting builtin family，以及
type-generic 的加法、减法和乘法 overflow builtin，并完整支持 signed/unsigned
`int`、`long` 与 `long long` 对应的 18 个 typed variant。common frontend 现有的
GNU 子集仍不足以编译 Zephyr。

支持这两个 language mode 并不等于支持 Zephyr。stock Zephyr 与现行 MCS-251 ABI
以及 ASxxxx `.rel`/`.ihx` 构建流程同样不兼容。完整的前置条件包括：

1. C17，以及 Zephyr 实际使用的 GNU C 子集；
2. 单独选择的 ABI，其中 `int` 为 32 bit，C pointer representation 也为 32 bit；
3. ELF32 relocatable object、archive、named section、symbol、relocation 和最终 ELF
   executable；
4. Zephyr 下游中的 SDCC toolchain 定义，以及 MCS-251 architecture、SoC 和 board
   port。

MCS-51 和 MCS-251 的默认语言行为与 ABI 均不得改变。

## 当前 SDCC 的能力边界

common frontend 在 [`SDCCmain.c`](../../../src/SDCCmain.c) 中把 `c17`、
`sdcc17` 和 `gnu17` 作为 C17 修订版本名称接受；`sdcpp` 会在 C17 与 GNU17
模式下于 [`libcpp/init.cc`](../../../support/cpp/libcpp/init.cc) 中把
`__STDC_VERSION__` 定义为 `201710L`。GNU11 同样会选择 C11 基础版本，并定义
`201112L`。

直接使用当前 MCS-251 编译器探测，可得到以下基线：

| 语法或选项 | 当前结果 |
|---|---|
| `--std=c17`、`--std=sdcc17` | 接受 |
| `--std=gnu11`、`--std=gnu17` | 接受；启用已经过测试的 GNU 子集 |
| `__typeof(...)`、`__typeof__(...)` | 接受，但 expression 仍有限制 |
| `__builtin_types_compatible_p(type1, type2)` | GNU11/GNU17 接受；结果为整数常量表达式 |
| `__builtin_expect(expression, expected)` | GNU11/GNU17 接受；返回 expression 转换为 `long` 后的值；expected 为常量时提供 90/10 分支提示 |
| `__builtin_constant_p(expression)` | GNU11/GNU17 接受；保守地折叠为零或一，不会求值 operand |
| `__builtin_clz*`、`__builtin_ctz*`、`__builtin_popcount*`、`__builtin_ffs*` | MCS-51/MCS-251 的 GNU11/GNU17 接受；常量参数由 frontend 折叠，动态参数调用与 ABI 匹配的 runtime helper |
| `__builtin_add_overflow`、`__builtin_sub_overflow`、`__builtin_mul_overflow` | MCS-51/MCS-251 的 GNU11/GNU17 接受；执行 type-generic 的精确运算，写回截断结果并报告 overflow |
| typed `sadd`/`uadd`/`ssub`/`usub`/`smul`/`umul` overflow family | MCS-51/MCS-251 的 GNU11/GNU17 完整支持 `int`、`long` 与 `long long` 共 18 个 variant |
| `__has_builtin(name)` | 所有模式均报告 common builtin，GNU11/GNU17 另报告 GNU-only builtin；未知名称为零 |
| `__builtin_unreachable()` | 所有模式均接受；不生成外部调用 |
| target data-model macro | 预定义 MCS-51/MCS-251 的大小、类型、范围、常量和 byte order，并抑制 host ABI macro |
| 基本形式 `__asm__("instruction")` | 接受 |
| 带 operand、constraint 和 clobber 的 extended asm | 拒绝 |
| statement expression `({ ... })` | GNU11/GNU17 接受；最后一个 expression statement 决定结果的值和类型 |
| `__auto_type` | GNU11/GNU17 接受；必须有且仅有一个 identifier declarator 和 initializer，由 initializer 推断类型且只求值一次 |
| `__extension__` | 拒绝 |
| nested function 和 computed `goto` | 拒绝 |
| `case low ... high` | 仅在 C2y extension mode 下接受 |
| 常见位置的 GCC `__attribute__((...))` | 可以解析；多数名称目前只接受语法 |

可选的 [`gcc_attr.h`](../../../device/include/gcc_attr.h) 会在非 C23 模式下删除 GCC
attribute，或在 C23 模式下把它的语法改写为 C23 attribute。它不会因此实现 GCC
attribute 的 placement、linkage、layout 或 calling semantics。目前 frontend 只对
`nodiscard`、`maybe_unused`、`deprecated` 和 `fallthrough` 赋予完整语义。

MCS-251 输出由 `sdas251` 和 `sdld` 处理，而不是 GAS 和 GNU ld。即使 parser 接受
GCC 的 inline assembly 写法，其中的汇编文本仍必须使用 ASxxxx syntax。

bit-counting family 覆盖 `int`、`long` 与 `long long` 三组 spelling。动态 operand
只求值一次；`__builtin_ffs*` 的 operand 为零时返回零，GNU C 则没有定义
`__builtin_clz*` 和 `__builtin_ctz*` 对零的结果。测试覆盖 constant expression、
错误参数个数，以及 MCS-51/MCS-251 QEMU 上的动态执行；其中包含 stack-auto 和
MCS-251 的四种 memory-model 组合。

type-generic overflow builtin 接受两个不超过 64 bit 的整数 operand，以及一个指向
可写 standard integer object 的 pointer。它们按照无限精度计算 mathematical
result，将 result type 对应的低位写回，并返回 `_Bool` 表示精确值能否由 result
type 表示。三个 argument 均只求值一次。共享的 reentrant runtime helper 遵循所选
target 的 byte order：MCS-51 沿用 little-endian representation，原生 MCS-251 则
按 big-endian 写回。测试覆盖正常运算、mixed signedness、wraparound、非法类型、
非法参数个数和单次求值，并在两个 QEMU target、stack-auto 以及 MCS-251 的四种
memory-model 组合上运行。

typed overflow 集合由 signed/unsigned 的 add、subtract 和 multiply family 组成，
每组均提供 `int`、`long` 与 `long long` 形式，例如
`__builtin_uadd_overflow`、`__builtin_ssubl_overflow` 和
`__builtin_smulll_overflow`。执行精确运算前，前两个 argument 会先按照 builtin
声明的类型完成转换；第三个 argument 必须指向完全相同且可写的 standard integer
type。编译器会检查 signedness、width、qualifier、enum type 与 code-memory
pointer。实现复用 reentrant 的 type-generic helper，并保证每个 argument 只求值
一次。静态测试覆盖全部 18 个名称、两种 GNU mode、stack-auto、strict mode 隔离
与非法 signature；runtime boundary test 则覆盖两个 QEMU target 和 MCS-251 的全部
memory-model 组合。

预处理器的数据模型取决于所选择的 SDCC target，而不是运行 SDCC 的 host machine。
MCS-51 声明现有的 little-endian ABI，MCS-251 则声明原生 big-endian ABI。两者均为
16-bit `int`、32-bit `long`、64-bit `long long` 和 3-byte generic pointer。driver
还提供 portable compiler abstraction header 所需的 GCC 风格 exact、least、fast、
pointer、maximum integer type macro 与 constant macro。测试会分别为两个 target
读取真实预处理器宏集，并编译实际使用这些定义的 C expression。

## Zephyr 对 C frontend 的要求

Zephyr 4.4 默认选择 C17。Kconfig 也允许启用 GNU extension，compiler abstraction
则要求把 language mode 正确映射到 `-std=`。参见官方
[Zephyr Kconfig](https://github.com/zephyrproject-rtos/zephyr/blob/v4.4.0/Kconfig.zephyr)
和 [GCC compiler properties](https://github.com/zephyrproject-rtos/zephyr/blob/v4.4.0/cmake/compiler/gcc/compiler_flags.cmake)。

即使应用代码只使用 standard C，Zephyr common header 仍需要下列能力，或者需要
compiler-specific 的等价实现：

- 已在 GNU11/GNU17 实现的 `__typeof__`、`_Generic`、statement expression
  和 `__auto_type`，以及仍待实现的 variadic macro comma elision；
- 已在 GNU11/GNU17 实现的 `__builtin_types_compatible_p`、
  `__builtin_expect` 与 `__builtin_constant_p`，以及所有模式均已实现的
  `__builtin_unreachable`；MCS target 的 `clz`/`ctz`/`popcount`/`ffs` family
  以及 type-generic add/subtract/multiply overflow builtin 和全部 18 个 typed
  variant，也已在 GNU11/GNU17 实现；Zephyr 若用到 predicate overflow builtin，
  仍需继续补齐；
- `section`、`used`、`weak`、`packed`、`aligned`、`always_inline`、`noinline`、
  `noreturn`、`alias` 等 attribute semantics；
- context switch 和 interrupt code 使用的 compiler barrier 与 target operation；
- global、weak 和 absolute assembly symbol。

具体定义见 Zephyr 官方
[`toolchain/gcc.h`](https://github.com/zephyrproject-rtos/zephyr/blob/v4.4.0/include/zephyr/toolchain/gcc.h)。
并非所有 GCC extension 都必须逐字实现。Zephyr 允许 custom compiler header，因而
可以用 SDCC-specific 的等价实现替代 extended asm 或某个 builtin，只要所需语义
保持一致。不能通过定义 `__GNUC__` 强迫 Zephyr 包含 `gcc.h`；这种做法会虚假声称
SDCC 提供了大量并不存在的 GCC semantics。

## ABI 边界

当前 MCS-251 后端在 [`mcs51/main.c`](../../../src/mcs51/main.c) 中定义了以下 C
类型大小：

| 类型 | 常规 MCS-251 ABI |
|---|---:|
| `char` | 1 byte |
| `short` | 2 bytes |
| `int` | 2 bytes |
| `long` | 4 bytes |
| generic/data/code pointer | 3 bytes |

Zephyr 的 public kernel header 断言：`int32_t` 与 `int` 大小相同，`int64_t` 与
`long long` 大小相同，`intptr_t` 与 `long` 大小相同。参见
[`kernel.h`](https://github.com/zephyrproject-rtos/zephyr/blob/v4.4.0/include/zephyr/kernel.h)。
它的 minimal libc 也以 32-bit `int` data model 为前提。

建议增加一个显式选择的 Zephyr ABI。现行 16-bit `int` ABI 继续作为默认值；
Zephyr ABI 把 `int` 设为 32 bit，并用 32-bit C pointer representation 保存 24-bit
hardware address。pointer 的未使用 bit 必须写入规范并纳入测试。这个 ABI 还需要
独立的 runtime library、calling-convention test 和 QEMU test。

若改为让 stock Zephyr 支持 16-bit `int` 和 3-byte pointer，就必须同时修改 generic
kernel、libc assumption 和 post-link tool；这条路线范围更大，也更难长期维护。

## 目标文件格式与 linker 边界

Zephyr 会无条件构建 offsets object，再从中提取 absolute symbol。生成脚本使用
`pyelftools` 读取 object，后续阶段也会检查 ELF section 和 symbol。参见
[`gen_offset_header.py`](https://github.com/zephyrproject-rtos/zephyr/blob/v4.4.0/scripts/build/gen_offset_header.py)
以及官方 [top-level build](https://github.com/zephyrproject-rtos/zephyr/blob/v4.4.0/CMakeLists.txt)。

因此，只有最终 Intel HEX image 并不够。MCS-251 toolchain 必须提供：

- ELF32 relocatable object 与 static archive；
- target relocation record，以及稳定的 ELF machine/ABI 定义；
- local、global、weak 和 absolute symbol；
- 任意命名的 code、data、read-only、BSS 与 metadata section；
- linker-script placement、section retention 和生成的 boundary symbol；
- 可继续转换为 QEMU 所需 Intel HEX 的最终 ELF executable。

在构建末端加一层 `.ihx` 到 ELF 的 wrapper，无法恢复 relocation、已丢弃的 symbol
和 input-section identity。assembler/linker 必须原生支持 ELF，或在 link 之前完成
无损转换。

## Zephyr 接入边界

Zephyr 官方 [custom toolchain guide](https://docs.zephyrproject.org/latest/develop/toolchains/custom_cmake.html)
允许 out-of-tree toolchain variant 提供自己的 compiler、linker、bintools 和
`toolchain/other.h`。这是 SDCC 正确的接入点，可以避免把 SDCC 伪装成 GCC。

stock Zephyr 中没有 MCS-51 或 MCS-251 architecture。官方
[architecture porting guide](https://docs.zephyrproject.org/latest/hardware/porting/arch.html)
要求实现 early boot、interrupt entry/exit、context switching、thread frame、
atomic 与 IRQ primitive、system timer、linker integration 和 stack alignment。
architecture port 完成后，还需要 STC32G144K246 SoC/board 定义，以及用于测试输出的
最小 UART driver。

## 实现门槛

按以下顺序推进，可以让失败原因保持清晰，并保护 MCS-51：

1. 持续维护 `gnu11` 与 `gnu17` 的 frontend 正向和反向测试。mode 必须设置正确的
   `__STDC_VERSION__`，并且只启用已经实现的 extension。
2. 继续在 common frontend 中实现所需的 keyword alias、expression、attribute 和
   builtin。MCS-51 与 MCS-251 共用 parser，因此两者必须运行相同的 probe。
3. 增加 opt-in Zephyr ABI，并构建独立的 MCS-251 runtime library。现有 default-ABI
   regression input 的输出应保持 byte-for-byte 一致。
4. 增加 ELF32 assembly/link 支持，并覆盖 Zephyr 使用的每种 relocation、symbol
   binding 和 section-placement rule。
5. 在 Zephyr 下游增加 out-of-tree SDCC toolchain，以及 MCS-251
   architecture/SoC/board port。
6. 依次编译和运行 freestanding C、Zephyr `hello_world`、interrupt/timer smoke、
   cooperative thread、preemptive thread 和 synchronization primitive。
7. 每次 CI 变更都运行完整 MCS-51 regression，以及 MCS-51/MCS-251 QEMU test。使用
   QEMU TCG `icount` 获得确定性 timeout，并通过 UART 输出测试结果；QEMU 只负责
   执行，不充当测试 oracle。

接受 `gnu17` mode 只表示提供了经过测试的兼容子集，并不代表已经完整兼容 GCC。
只有从 stock baseline 编译、链接 Zephyr image，并在 MCS-251 QEMU machine 上
执行成功，才能声称实现了 Zephyr 支持。
