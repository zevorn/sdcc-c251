# MCS-251 ABI revision 1

本文冻结 `sdcc -mmcs251` 生成的 object 与 calling convention。它描述本仓库实现
的 SDCC 后端；它不是 Arm/Keil MCS251 ABI，也不宣称能够与 OMF-251 object 或
library 互操作。

## 兼容性标识

| 项目 | revision 1 定义 |
|---|---|
| target option | `-mmcs251` |
| predefined target macro | `__SDCC_mcs251` |
| ABI revision | `1` |
| object format | SDCC ASxxxx `.rel`，最终输出 Intel HEX |
| 默认 instruction map | MCS-251 Source mode |
| direct call / tail call | `ECALL` / `EJMP` |
| 普通函数返回 | `ERET` |

不得把采用不同 MCS251 ABI revision 构建的 object 链接到一起。MCS251 运行库也按
本 ABI 单独构建。本目标不提供旧的 `-mc251` 选项，也不提供 `__SDCC_c251`
compatibility macro。

## 标量表示

| C type | 大小 |
|---|---:|
| `char` / `_Bool` | 1 byte |
| `short` / `int` | 2 bytes |
| `long` / `float` | 4 bytes |
| `long long` / `double` | 8 bytes |
| `__bit` | 1 bit |

revision 1 采用 MCS-251 原生的 big-endian scalar layout：most-significant byte
位于最低 memory address。scalar 放入架构 WR 或 DR register tuple 时，同样由最低
编号的 byte register 保存最高有效字节。例如 WR6 的 R6 保存 bit `15:8`，R7 保存
bit `7:0`。`-mmcs251` 只提供这一种 object layout，不设 little-endian MCS251
模式；独立的 `-mmcs51` target 仍沿用既有的 little-endian ABI。

## 指针表示

| pointer kind | 大小 | 表示 |
|---|---:|---|
| `__data` / `__idata` near pointer | 1 byte | page-zero byte address |
| `__pdata` pointer | 1 byte | 当前 `MXAX:P2` MOVX page 内的 offset |
| `__xdata` / `__far` pointer | 3 bytes | flat 24-bit address |
| `__code` pointer | 3 bytes | flat 24-bit address |
| unqualified/generic pointer | 3 bytes | flat 24-bit address |
| function pointer | 3 bytes | flat 24-bit code address |

三字节 pointer object 在递增地址上依次保存地址 bit `23:16`、`15:8` 和 `7:0`。
revision 1 的 generic pointer 没有 address-space tag。pointer arithmetic 会把
carry 传播到全部 24 位，包括跨越 64 KiB boundary。generic pointer 不能表示独立
的 direct SFR window；SFR 仍通过 `__sfr` declaration 和 direct addressing 访问。

把 `__pdata` pointer 转为 flat pointer 时，会快照其 byte offset、P2 和 STC
`MXAX` region byte，组成数值地址 `MXAX:P2:offset`；对应的三字节 object
representation 仍然高字节在前。转换结果因此继续指向与 `MOVX @Ri` 相同的 byte；
转换后改变任一 page register，不会让所得 flat pointer 改指别处。

DPX 是 flat pointer 的标准 hardware address register。DPL、DPH 和 DPXL 分别保存
bit `7:0`、`15:8` 和 `23:16`。indirect function call 把零扩展的 24 位目标载入
DR28，再执行 `ECALL @DR28`。

## 参数与返回值

revision 1 扩展 SDCC MCS-51 calling convention，而不采用 Keil register allocator。

- 第一个 scalar argument 和 scalar return value 使用 SDCC 常规 return register：
  byte 用 DPL，word 用 DPL/DPH，四字节 scalar 用 DPL/DPH/B/A。这里按逻辑上的
  least-significant byte 到 most-significant byte 列出寄存器；若按高位在前书写，
  word 是 DPH:DPL，四字节 scalar 是 A:B:DPH:DPL。
- 主 register slot 中传递或返回三字节 pointer 时，按逻辑低位到高位使用
  DPL/DPH/B；若按高位在前书写，则为 B:DPH:DPL。
- scalar 分配到原生 WR 或 DR register tuple 时，遵循架构 big-endian 顺序：最低
  编号的 register 保存最高有效字节，最高编号的 register 保存最低有效字节。
- 其余 non-reentrant argument 使用 SDCC 可 overlay 的 parameter area。
- reentrant stack argument 使用向高地址增长的 hardware stack。
- bit result 与 MCS-51 后端相同，通过 carry 返回。
- 大型 aggregate return 使用 SDCC hidden-result-pointer convention。

默认保持 SDCC static/overlay convention。因此，如果 indirect call 的 parameter
无法全部放入 register argument slot，其 function-pointer type 与 callee 必须声明
`__reentrant`；indirect caller 无法命名 callee 专属的 overlay area。`--stack-auto`
使所有普通函数采用 reentrant stack convention，从而消除这一限制。同一程序的
所有 translation unit 与运行库必须采用相同选择。除默认 model library 外，还会
安装匹配的 `mcs251-small-stack-auto` 与 `mcs251-large-stack-auto`。

所有 direct C call 都使用 24 位 return frame（`ECALL`/`ERET`），即使 caller 与
callee 恰好位于同一 64 KiB region。这样 separately compiled object 与 linker
placement 不依赖 code region boundary。explicit assembly 仍可用 `LCALL`/`LJMP`；
如果目标不在 next instruction 所在 region，linker 会拒绝该跳转。

## 栈与启动

MCS251 startup module 把 SPX 初始化为 `__start__stack - 1`。hardware stack 向高
地址增长。call 保存三字节 return address；配置的 call overhead 已计入
pre-increment push behavior。

revision 1 对 stack-resident automatic variable、spill 和 reentrant argument 使用
完整的 16 位 hardware SPX。scalar stack slot 使用 MCS251 signed `@SPX+dis16`
addressing form。取得 stack object 地址时，会在 region `00:` 中物化一个三字节
flat pointer，因此 frame 跨过 256-byte boundary 后，array 与 aggregate 仍然可寻址。

架构栈可覆盖 region `00:`，但可用容量取决于设备和 linker layout。例如
STC32G144K246 在 `00:0000`–`00:3fff` 实现 16 KiB edata；针对该设备的构建必须
预留 startup stack，并确保最深 call、interrupt 与 reentrant-frame 嵌套不会越界。

所有 MCS251 data model 中，`jmp_buf` 都是 15 字节：两个 big-endian byte 保存
SPX，三个 big-endian byte 保存完整 `ECALL` return PC，另两个 big-endian
private scratch byte 把规范化后的 `longjmp` result 传给 naked restore helper，
最后八个 byte 保存 R0–R7。`setjmp` 在禁止中断时快照 frame 和当前可分配的通用
寄存器；`longjmp` 重建三字节 frame，恢复 SPX、R0–R7 与原 interrupt-enable
state，并返回请求的非零值。

QEMU conformance image 会在 `setjmp` 前把 SPX 设为 `0x0120`，随后以 small 与
large、default 与 stack-auto 的四种 library 组合，验证 SPX、`0x1234` 结果以及
live register value 均已恢复。

## memory space 与 memory model

language address space 保持其 SDCC 含义：

- `__data` 和 `__idata` 选择 page-zero edata/direct storage；
- `__xdata` / `__far` 通过 DPX 选择 flat MCS251 data address space；
- `__code` 选择 flat code address space，对 C 程序只读；
- unqualified pointer 使用 flat 24-bit generic representation；
- `__sfr` 与 `__sbit` 使用 direct SFR/bit addressing，不能由 generic pointer
  conversion 到达。

SDCC model option 是 allocation policy，不是 processor manual 中 memory class 的
别名：

- `--model-small` 把普通 object 放入 page-zero edata，尽可能用 direct byte
  addressing；
- `--model-large` 把普通 object 放入 XSEG，通过 24 位 DPX pointer 访问；
- `--stack-auto` 与上述 data model 正交，把 automatic object 和 non-register
  parameter 放在 16 位 SPX stack 上；不使用它时，普通 non-reentrant function
  保留 SDCC 可 overlay 的 static allocation，显式 `__reentrant` function 始终使用
  SPX。

MCS251 默认把 XSEG 放在 `0x010000`。这可防止 flat generic/far pointer 与
page-zero edata alias，并匹配 QEMU machine 使用的 STC32G on-chip XRAM window。
memory map 不同的 board 可以用 `--xram-loc` 覆盖。QEMU integration test 把 code
放在 `0xff0000`，访问 `0x010000` 以上的 xdata，并验证 pointer arithmetic 能从
`0x01ffff` 跨到 `0x020000`。

## 中断函数

MCS251 interrupt function 使用目标的 extended control-flow sequence，并以 `RETI`
返回。STC32G machine 使用文档规定的四字节 interrupt frame。application startup
必须让处理器保持 Source mode；编译器不会改写 `AUXR2.CPUMODE`。

interrupt routine 与普通函数共享数据时，仍要遵循 C 的 `volatile` 与 atomicity
规则。还要把 hardware 保存的 frame、compiler prologue 和最深 nested call 一并
计入 SPX 容量。

## 必需的 conformance test

任何影响 ABI 的修改都必须提升 revision，并至少覆盖：

1. 位于 64 KiB 以上的 independent-unit direct/indirect call；
2. 所有 pointer size 与 high-byte relocation；
3. 64 KiB 以上以及跨 region boundary 的 far/generic load/store；
4. `atof`、`bsearch` 等消费 pointer 的运行库函数；
5. 三字节 return frame、SPX 高于 `0x00ff` 时的 `setjmp`/`longjmp`；
6. small/large 与各自 `--stack-auto` library 的完整运行；
7. `-mmcs51` object layout、generic-pointer tag 和 QEMU 行为保持不变。

测试应同时包含 compiler regression、assembler/linker relocation、运行库和 QEMU
end-to-end image。若 QEMU trace 的首个错误状态违反 instruction manual，而生成的
encoding 与 ABI 正确，应把它记录为 emulator 问题，不应通过改变 SDCC ABI 绕过。
