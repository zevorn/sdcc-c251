# 技术资料与编译器内部原理

本章概述 SDCC 的优化与内部数据流，供希望理解生成代码、诊断后端问题或移植新
目标的读者参考。内部接口会随开发演进，不构成稳定的用户 API；源码和 regression
test 始终比本章摘要更权威。

## 优化

SDCC 的优化分布在前端规范化、中间代码（iCode）变换、控制流分析、寄存器分配、
目标 code generation 和 peephole 多个阶段。某条 C 语句对应的最终指令变化，往往
不是单一 pass 的结果。

### 公共子表达式消除

若两个表达式计算相同且中间没有改变其 operand，后一次可复用先前结果。alias、
`volatile` 访问、函数副作用和目标 address space 都会限制这种复用。

### 死代码消除

结果不再被使用且没有 observable effect 的计算可以删除。不可达 basic block 也可
移除。对 memory-mapped I/O 的访问必须用正确的 `volatile` 或目标 SFR 声明，否则
源程序没有告诉编译器该访问可被外部观察。

### copy propagation

形如 `a = b` 的 copy 会被传播到后续使用点，从而删除临时值并暴露新的常量折叠
机会。传播必须在值未被重定义且 alias 安全时进行。

### 循环优化

loop-invariant computation 可以移到循环外；induction variable 能被简化或合并；
循环条件可依据已知范围折叠。小型 8 位处理器上，减少一次循环体指令与减少额外
寄存器压力之间需要权衡。

倒序循环有时可把比较改成更便宜的递减和非零分支，但只在不改变迭代顺序可观察
效果时成立。指针越界、signed overflow 和 `volatile` 会影响合法性。

### 代数化简与常量折叠

恒等式、常量 operand 以及目标已有的便宜 instruction 可用来简化表达式。整数宽度、
promotion、signedness 与 overflow 规则不能在变换中丢失。例如 unsigned arithmetic
按模运算，signed overflow 则属于 undefined behavior。

### `switch` 语句

后端可根据 case 数量和密度选择顺序比较、binary decision tree 或 jump table。
最短代码与最快代码并不总是一致，`--opt-code-size` 和 `--opt-code-speed` 会影响取舍。

### shift、rotate 与字节重排

常数 shift 可以展开，跨字节 shift 可利用 carry chain；某些 rotate、nibble swap、
byte swap、取 bit、取 high byte/high word 的常见表达式会匹配目标 instruction。
这些优化要求精确区分逻辑右移与算术右移，并维护多字节对象的 endian layout。

MCS-251 ABI revision 2 采用 CPU 原生的 big-endian object layout。原生 WR/DR
register tuple 也由最低编号 register 保存最高有效字节，因此经过 alignment、
address space、`volatile` 与 aliasing 检查后，可以直接选用 word/dword
instruction。独立的 MCS-51 后端仍遵循其既有 little-endian layout。

## cyclomatic complexity

cyclomatic complexity 反映 control-flow graph 中独立路径的数量，可帮助发现需要
拆分或加强测试的函数。它不是代码质量的单一评分：interrupt handler、parser 和
状态机本来就可能有较多分支。对编译器而言，复杂 CFG 还会增加 data-flow 与
register allocation 的工作量。

## 为新处理器移植 SDCC

一个可维护的后端不只是 instruction printer。至少需要明确并实现：

1. target option、predefined macro 与 target feature；
2. C type、pointer、address space 和 endian representation；
3. calling convention、stack frame、return value 与 interrupt ABI；
4. assembler/linker dialect、relocation 与 object/output format；
5. startup、运行库、device header 和默认 memory map；
6. instruction selection、cost model、register allocation 与 peephole rule；
7. simulator/硬件运行测试和既有后端防回归测试。

先冻结 ABI，再逐步改善代码质量。若调用约定和 object layout 尚未稳定，peephole
产生的局部好结果也无法构成可互操作的工具链。MCS-251 后端因此用独立的 ABI
revision、library directory 和 conformance test 约束变更。

## 编译流水线

```text
C source
   │
   ▼
preprocessor → parser/type checking → AST
   │
   ▼
iCode → CFG/data-flow → target-independent optimization
   │
   ▼
register allocation → target code generation → peephole
   │
   ▼
assembly → relocatable object → linker → HEX/ELF/binary
```

### 前端与 AST

预处理器处理 macro、include 与 conditional compilation；parser 建立 abstract syntax
tree（AST），完成 type checking、usual conversion 和 SDCC extension 的语义检查。
目标后端提供 type size、address space 和部分 cost/feature 信息，但不能绕过 C 语言
语义。

### iCode

AST 被降为较简单的 three-address 风格 iCode。临时值、label、call、load/store 和
branch 在这一层显式出现。许多 target-independent optimization 在 iCode 上工作，
因此后端 bug 与公共优化 bug 的分界通常可通过 intermediate dump 判断。

编译器提供多个 intermediate dump 选项。具体名称以 `sdcc --help` 为准；诊断时应
保存源文件、完整命令行以及各阶段 dump，比较错误最早在哪一层出现。

### basic block 与 CFG

leader 划分出 basic block：block 内控制流顺序执行，只在末尾分叉。每个 block 都有
successor 和 predecessor；由此形成 control-flow graph（CFG）。

dominance 表示从入口到某 block 的每条路径都经过另一个 block。dominator、loop
header 与 back edge 用于识别 natural loop 和决定合法的 code motion。异常控制流、
computed jump、函数调用副作用和 interrupt 可观察状态会使分析更保守。

### liveness 与寄存器分配

若某值在程序点之后还会被读取，它在那里是 live。liveness 由 CFG 上的 data-flow
迭代求得，形成 live range。相互重叠的 live range 不能占用同一物理 register；后端
根据 register class、calling convention、instruction constraint 与 cost 分配，必要
时 spill 到 memory 或 stack。

小型 CPU 常有隐含 register、成对 register 或只能访问特定 address space 的操作。
register allocator 与 code generator 必须共同维护这些 constraint。MCS-251 新增
R0–R31、WR/DR、DPX/SPX 和 extended addressing，但不能因此改变 `-mmcs51` 的
register set 或 ABI。

### code generation 与 peephole

code generator 把已分配的 iCode 变成目标 assembly，处理 prologue/epilogue、参数、
branch、literal pool 或 helper call。随后 peephole optimizer 在局部 instruction
窗口内匹配等价但更短或更快的序列。

peephole rule 必须写清 flag、register、memory 和 control-flow 前提。对 MCS-251，
还要区分 Source mode 与 Binary mode opcode map、16/24 位地址、signed indexed
displacement 及三字节 `ECALL` return frame。一条规则在 MCS-251 合法，不代表可放
进共享 MCS-51 rule；目标边界必须可测试。

## 诊断生成代码

遇到错误时，从最小 C 程序开始，依次检查：

1. preprocessed source 是否符合预期；
2. iCode 中的 type、address space 与 control flow 是否正确；
3. register allocation 后 live value 是否被错误覆盖；
4. assembly 是否遵守 ABI 与 instruction encoding；
5. relocation/link map 是否把 symbol 放到预期地址；
6. simulator trace 中首个错误 machine state 位于何处。

仅比较最终 UART 文本通常太晚。把首个错误缩到某个 iCode、instruction 或
relocation，才能判断修复应位于公共前端、目标后端、assembler/linker 还是 QEMU。

## 致谢与索引

SDCC 是许多维护者、贡献者、测试者和用户长期合作的成果。各源码文件、release
note 与版本控制历史保留了更精确的作者和贡献记录。本中文版本沿用英文手册的主题
结构，并补入本分支 MCS-251 实现；如译文与源码行为发生偏差，请以测试和当前源码
为准并提交修正。

PDF/HTML 输出的目录提供按主题导航；术语和 option 可通过文本搜索定位。命令行
option、C keyword、register 与 symbol 保留英文拼写，以便与源码、diagnostic 和
官方 instruction manual 一一对应。
