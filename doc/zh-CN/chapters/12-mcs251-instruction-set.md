# MCS-251 指令集支持

本章说明“支持全部 MCS-251 指令”在当前后端中的准确含义。汇编器覆盖率、编译器
instruction selection 和硬件验证是三个不同层次，不能用其中一个代替另外两个。

## 当前结论

对于本仓库依据 Intel 和 STC 指令表维护的 MCS-251 CPU instruction set，`sdas251`
汇编器当前覆盖：

- 65/65 个指令族；
- 269/269 个已记录的合法 operand form；
- 每个形式的 Source 和 Binary 两套 opcode map；
- encoding matrix 中跳过数为 0；
- 24 位 relocation 和 Intel HEX 输出；
- 对代表性非法 operand、地址和范围输入给出诊断。

因此，如果问题是“汇编器是否支持全部指令”，答案是**是**：当前经审阅的 ISA
matrix 中，每个指令族和合法 operand form 都能被汇编，并会在两套 opcode map 下与
golden bytes 逐字节比较。

这不等于 C 编译器会主动生成全部 269 种形式。C 后端要根据类型、ABI、register
allocation 和 cost model 选择指令；不适合作为编译器输出的形式仍可用于 inline
assembly 或独立汇编，并在汇编器层完成验证。编译器当前承诺的范围见后文。

## ISA matrix

规范的机器可读清单位于
[`sdas/as251/tests/instruction-forms.tsv`](../../../sdas/as251/tests/instruction-forms.tsv)，
其中 `reference` 列把每个形式对应到 Intel 指令表。指令族清单位于
[`instruction-families.txt`](../../../sdas/as251/tests/instruction-families.txt)。

| 类别 | 指令族数 | 合法形式数 |
|---|---:|---:|
| 算术和比较 | 10 | 71 |
| 逻辑和 bit 操作 | 6 | 67 |
| 移位、循环移位和 swap | 8 | 11 |
| 数据传送和栈 | 10 | 78 |
| 条件控制流 | 17 | 24 |
| 跳转、调用和返回 | 11 | 15 |
| 其他 | 3 | 3 |
| **合计** | **65** | **269** |

全部指令族及其已测试形式数如下：

- 算术和比较：`ADD`（16）、`ADDC`（4）、`SUB`（12）、`SUBB`（4）、`CMP`
  （13）、`INC`（8）、`DEC`（7）、`MUL`（3）、`DIV`（3）、`DA`（1）。
- 逻辑和 bit 操作：`ANL`（20）、`ORL`（20）、`XRL`（16）、`CLR`（4）、`CPL`
  （4）、`SETB`（3）。
- 移位、循环移位和 swap：`RL`（1）、`RLC`（1）、`RR`（1）、`RRC`（1）、`SLL`
  （2）、`SRA`（2）、`SRL`（2）、`SWAP`（1）。
- 数据传送和栈：`MOV`（55）、`MOVC`（2）、`MOVH`（1）、`MOVS`（1）、`MOVX`
  （4）、`MOVZ`（1）、`XCH`（3）、`XCHD`（1）、`PUSH`（6）、`POP`（4）。
- 条件控制流：`CJNE`（4）、`DJNZ`（2）、`JB`（2）、`JBC`（2）、`JNB`（2）、
  `JC`（1）、`JNC`（1）、`JZ`（1）、`JNZ`（1）、`JE`（1）、`JNE`（1）、`JG`
  （1）、`JLE`（1）、`JSG`（1）、`JSGE`（1）、`JSL`（1）、`JSLE`（1）。
- 跳转、调用和返回：`ACALL`（1）、`AJMP`（1）、`LCALL`（2）、`LJMP`（2）、
  `ECALL`（2）、`EJMP`（2）、`JMP`（1）、`SJMP`（1）、`RET`（1）、`RETI`
  （1）、`ERET`（1）。
- 其他：`NOP`（1）、`ESC`（1）、`TRAP`（1）。

`CALL` 和 `PUSHW` 是方便书写的 alias，并非额外的架构指令族，因此不计入 65 个
指令族。

## 汇编器正向和反向测试

在配置完成的 build 目录中运行完整汇编器门禁：

```sh
make -C sdas/as251 check
```

该目标执行五类彼此独立的检查：

1. 汇编已经审阅的 golden-byte source；
2. 在 Source 和 Binary mode 下汇编全部 269 个合法形式，逐一比较输出字节；
3. 检查生成的 matrix、指令族 manifest 和汇编器 mnemonic table 是否一致；
4. 检查跨目标文件的 24 位 relocation 和 Intel HEX 地址；
5. 拒绝越界的 branch/page/region、非法 register number/width、indirect width、
   displacement width、increment step、bit number 和 push operand，并要求出现诊断。

反向测试覆盖每类受约束 operand 的代表性错误，但不宣称枚举了所有可能的畸形源码
字符串。

## 编译器生成的指令

编译器门禁为：

```sh
make -C src/mcs251 check
make -C support/valdiag test-mcs251
```

它在 small、large、small stack-auto 和 large stack-auto 四种配置下编译后端与运行库。
当前 code generation 断言覆盖：

- 原生带步长 `INC`/`DEC`；
- 原生 `MOVS`/`MOVZ` byte extension 与 `MOVH` high-word replacement；
- 对齐 register tuple 上的原生 32 位 `ADD DR,DR`、`SUB DR,DR`，并通过跨字节
  carry 与 borrow 运行测题；
- 16 位 `ANL`、`ORL`、`XRL`，以及部分 32 位 `XRL`；
- 16 位 immediate `CMP`；
- 合法的 DPX、SPX addressing，并排除 unsupported operand form；
- 24 位 `ECALL`、`EJMP`、indirect call 和 `ERET`；
- extended interrupt vector；
- 平坦 24 位 data、code 和 function pointer relocation；
- 符合 memory model 的 spill placement 和 16 位 SPX frame；
- 共享编译器路径下保持 MCS-51 pointer layout 不变。

validation-diagnostics suite 还会在四种配置下运行 C 语言正向和反向用例。这些测试
确定了当前承诺的基本编译优化范围，但不表示每种合法汇编形式都必须成为 optimization
target，也不表示每个 C 表达式都已经获得最优 instruction selection。

## 边界和已知限制

- 未限定地址空间的三字节 pointer 是没有 address-space tag 的平坦地址，不能表示
  独立的 direct SFR window；把 `__sfr` 的地址转换成普通 pointer 会按设计报错。
- 原生 32 位 register 加减目前要求两个对齐的 DR tuple，且 destructive result 已经
  alias 某个合法 input。memory operand 与任意交叠 tuple 继续使用经过测试的逐字节
  fallback。
- DSP32、TFPU、MDU32 和芯片外设是由寄存器控制的 STC SoC 功能，不是 MCS-251
  CPU opcode，因此不计入 65 个指令族。
- golden bytes 来自已经审阅的指令表，这并不会让 QEMU 成为独立的硬件真值；
  发布验收仍应增加实机差分测试。
- QEMU UART smoke test 只证明代表性编译产物能够执行和通信，不能证明 cycle
  accuracy、外设完整性或 instruction selection 已达到最优。

简而言之：相对于当前维护的官方 ISA matrix，汇编器覆盖已经完整；编译器 instruction
selection 和硬件验证则有更窄、并且明确写出的测试范围。
