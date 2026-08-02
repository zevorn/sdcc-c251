# MCS-251 内存架构与 Keil MCS251 内存模型研究

本笔记给 SDCC 的 MCS251 后端实现提供一条可审计的资料链。结论按三层分开：

1. Intel 文档定义 MCS-251 架构事实；
2. Arm/Keil 文档定义 MCS251 编译器自己的语言扩展、内存模型和 ABI；
3. STC 文档定义 STC32G144K246 这一派生芯片的物理资源、地址映射和推荐配置。

因此，Keil 的默认放置和调用约定不是处理器硬件要求，STC 的物理映射也不能反推为所有 MCS-251 芯片的通用映射。本文只使用厂商发布或厂商托管的资料，不以博客、论坛转述或反编译结果补空白。资料核验日期为 2026-08-01。

## 一手资料和页码

### Arm/Keil

- [User's Guides for Keil MCS251 Development Tools](https://support.arm.com/documentation/101647/0560/)，Arm 文档号 101647，v5.60，revision 01，标为 Non-Confidential，发布日期 2022-07-03。下载包 `Keil_C251_Docs.zip` 的 SHA-256 为 `88fb380aeadaa6162d7d53ac4f553e92238bac8c85314aa7b0f0741b379f08fc`。包内是 CHM/HTML 帮助而不是 PDF，所以没有“印刷页/PDF 页”；下文以精确的帮助主题标题和 URL 定位。
- 主要 C 编译器主题：[Memory Models](https://www.keil.com/support/man/docs/c251/c251_le_memmodels.asp)、[Implicit Memory Types](https://www.keil.com/support/man/docs/c251/c251_le_impmemtypes.asp)、[Memory Types](https://www.keil.com/support/man/docs/c251/c251_le_memtypes.asp)、[Memory Type in Pointer Declarations](https://www.keil.com/support/man/docs/c251/c251_le_memtypepointers.asp)、[Pointer Conversions](https://www.keil.com/support/man/docs/c251/c251_le_conversionpointers.asp)、[HPTR](https://www.keil.com/support/man/docs/c251/c251_hptr.asp) 和 [ROM](https://www.keil.com/support/man/docs/c251/c251_rom.asp)。
- 对象布局主题：[2-Byte Scalars](https://www.keil.com/support/man/docs/c251/c251_ap_2bytescalar.asp)、[4-Byte Scalars](https://www.keil.com/support/man/docs/c251/c251_ap_4bytescalar.asp) 和 [E. Byte Ordering](https://www.keil.com/support/man/docs/c251/c251_xe.asp)。
- ABI 与栈主题：[Passing in Registers](https://www.keil.com/support/man/docs/c251/c251_ap_parampassreg.asp)、[Passing in Memory](https://www.keil.com/support/man/docs/c251/c251_ap_parampassmem.asp)、[Function Return Values](https://www.keil.com/support/man/docs/c251/c251_ap_funcret.asp)、[Register Usage](https://www.keil.com/support/man/docs/c251/c251_ap_regusage.asp)、[Reentrant Functions](https://www.keil.com/support/man/docs/c251/c251_le_reentrantfuncs.asp)、[Hardware Stack](https://www.keil.com/support/man/docs/c251/c251_ap_hardwarestack.asp) 和 L251 [Overlaying Data Memory](https://www.keil.com/support/man/docs/l251/l251_in_overlaying.asp)。
- 指令模式主题：[MODBIN](https://www.keil.com/support/man/docs/c251/c251_modbin.asp)、A251 [MODSRC](https://www.keil.com/support/man/docs/a251/a251_modsrc.asp) 和 µVision [Target Options for MCS251](https://www.keil.com/support/man/docs/uv4cl/uv4cl_dg_target251.asp)。

### Intel

- [8XMCS251SA, 8XMCS251SB, 8XMCS251SP, 8XMCS251SQ Embedded Microcontroller User's Manual](https://www.keil.com/dd/docs/datashts/intel/8xmcs251sx_um.pdf)，Intel Corporation，May 1996，共 458 个 PDF 页面；PDF 元数据标题为 *Intel 8xMCS251Sx User's Manual*。文件 SHA-256 为 `bffda43f128d415a07c596e4720e8731d57ab9ffa6284ebecea1f0a90c8f35fd`。这是 Intel 原始手册，由 Keil/Arm 官方站点托管。
- 本文逐页读取并渲染核验了：Chapter 3 §§3.1–3.3.2.3，印刷 3-1–3-15 / PDF 43–57；Chapter 4 §§4.6–4.8，印刷 4-13–4-16 / PDF 77–80；Chapter 5 §§5.1–5.2.3，印刷 5-1–5-2 / PDF 83–84；Chapter 5 §§5.5.1–5.6，印刷 5-13–5-16 / PDF 95–98。

### STC

- [STC32G144K246 / STC32G96K246 / STC32G18K64 单片机原理及应用](https://www.stcaimcu.com/data/download/Datasheet/STC32G144K246.pdf)，资料更新日期 2026-07-15，共 1321 个 PDF 页面；封面明确标注“初稿框架，逐章校核及详细测试中”。文件 SHA-256 为 `da7bf04ee6163300ae5140cf201b3c161626338547ce719385c0af9c83c1f6cd`。入口页为 [STC32XL 产品与下载页](https://www.stcai.com/cp_stc32xl)。
- 本文逐页读取并渲染核验了：Chapter 12 开篇与 §12.1，印刷 265–266 / PDF 299–300；§§2.14–2.15.5，印刷 74–81 / PDF 108–115；§§12.2–12.2.12，印刷 268–278 / PDF 302–312；§§12.3.1–12.3.2，印刷 279–281 / PDF 313–315；§§12.4.1–12.4.5，印刷 287–290 / PDF 321–324；Appendix A §§A.1.1–A.1.3，印刷 1172–1174 / PDF 1206–1208；调用/返回指令条目，印刷 1208–1210、1220、1249 / PDF 1242–1244、1254、1283。

### Infineon 的资料边界

[Infineon 的安全控制器产品页](https://www.infineon.com/products/security-smart-card-solutions/security-controllers/serial-interface-security-controllers)确认其产品组合仍包括“基于 MCS-251 指令集的 16 位架构”。但在当前公开文档库中没有找到可用来定义通用 MCS-251 内存架构、Keil MCS251 内存模型或公开 ABI 的 Infineon 技术手册。其安全控制器还可能带 MMU、缓存和未公开扩展，不能拿产品简介代替 Intel 架构手册。本文因此不从 Infineon 资料引入任何地址、指针或 ABI 规则，也不拿无关的 Infineon F²MC/C166 文档类比。

## 架构基线：一个线性的 16 MiB 空间

Intel Chapter 3 §3.1，印刷 3-1–3-2 / PDF 43–44，定义了三个互相独立的地址空间：

- 16 MiB 的 memory address space，地址 `0x000000`–`0xFFFFFF`；
- 512 B 的 SFR space，地址 `S:000`–`S:1FF`；
- 64 B 的 register file。

16 MiB 空间是**非分段、线性**空间。手册写作中的 `00:`、`01:` … `FF:` 只是把地址按 64 KiB 分区显示的方便记法，不存在相应的段寄存器。编译器中的 `near`、`far`、`huge` 是访问方式和指针算术策略，不是硬件把空间切成了 C 语言意义上的段。

Intel §3.1.1，印刷 3-2–3-5 / PDF 44–47，给出 MCS-51 兼容映射：

| 8051 视图 | MCS-251 线性地址/独立空间 | 复位后的兼容机制 |
|---|---|---|
| 64 KiB code | `FF:0000`–`FF:FFFF` | 16 位 code 地址映射到 region `FF:` |
| 64 KiB MOVX xdata | 默认 `01:0000`–`01:FFFF` | `DPXL` 复位为 `01h`，并可软件改映射 |
| 256 B internal data | `00:0000`–`00:00FF` | 保持直接/间接寻址语义 |
| SFR `80h`–`FFh` | 独立的 `S:080`–`S:0FF` | 仍由直接 SFR 寻址访问 |
| register banks | 独立 register file 的兼容映射 | 保持传统寄存器组行为 |

这里有两个容易混淆的限定：

- Intel 手册中的 8XMCS251Sx **具体器件**只实现了 `00:`、`01:`、`FE:`、`FF:` 四个 memory regions；这不是 16 MiB 架构上限，更不是 STC 器件映射。
- `code`、`xdata` 等 8051 兼容视图仍是 64 KiB 窗口；它们与可由 24 位地址到达的整个线性空间同时存在。

## Keil MCS251 的数据内存模型

[Memory Models](https://www.keil.com/support/man/docs/c251/c251_le_memmodels.asp) 说明，内存模型只替没有显式限定符的对象选择默认存储类，并决定未落入寄存器的参数/自动变量放在哪里；显式限定符优先。v5.60 的 [Implicit Memory Types](https://www.keil.com/support/man/docs/c251/c251_le_impmemtypes.asp) 表如下：

| Keil data model | 参数/自动变量默认 | 全局/静态对象默认 | 常量默认 | 未限定 generic pointer |
|---|---|---|---|---:|
| TINY | `data` | `data` | `near` | `near *`, 2 B |
| XTINY | `near` | `near` | `near` | `near *`, 2 B |
| SMALL | `data` | `data` | `code` | `far *`, 4 B |
| XSMALL | `near` | `near` | `code` | `far *`, 4 B |
| LARGE | `xdata` | `xdata` | `code` | `far *`, 4 B |

`near` 对象由 linker class `EDATA` 放在 region `00:`；`data` 只是其中低 128 B 的直接寻址子集。`xdata` 是 64 KiB 的 MOVX/DPTR 兼容窗口。`HOLD` 还可按对象大小把全局对象改放到别的 memory type，所以表格描述的是默认规则，不是最终链接布局。

特别要澄清“huge model”：

- Keil MCS251 **没有第六个叫 HUGE 的数据内存模型**；数据模型只有上表五种。
- `huge` 是一种数据/指针 memory type；`HPTR` 可在 SMALL、XSMALL、LARGE 中把默认 `far` 指针改为 `huge`。
- `ROM(HUGE)` 是另一条轴上的**代码大小模型**，控制函数远近、调用和返回。它不会自动把普通数据对象放成 `huge`。

因此，“small/large/huge 默认对象放置”必须写成：SMALL 的普通数据默认 `data`，LARGE 默认 `xdata`；不存在 HUGE 数据模型的默认对象放置，只有 `huge` 显式对象/指针和 `ROM(HUGE)` 代码策略。

## 内存类型、指针宽度和可表达范围

Keil v5.60 的 [Memory Types](https://www.keil.com/support/man/docs/c251/c251_le_memtypes.asp) 和 [Memory Type in Pointer Declarations](https://www.keil.com/support/man/docs/c251/c251_le_memtypepointers.asp) 给出：

| 类型 | 指针宽度 | 范围/寻址 | 关键限制 |
|---|---:|---|---|
| `data *` | 8 bit | region `00:` 的低 128 B，直接寻址 | 只能表示 `0x00`–`0x7F` |
| `idata *` | 16 bit | 兼容的 256 B 间接数据区 | 文档建议需要时也可用 `near *` |
| `near *` | 16 bit | `00:0000`–`00:FFFF`，`@WRj`/`dir16` | 对象不能跨 `00:FFFF` |
| `xdata *` | 16 bit | 64 KiB MOVX/DPTR 窗口 | region 由目标/寄存器映射决定 |
| `code *` | 16 bit | 64 KiB 兼容 code 窗口，MOVC | 只读；Keil 建议新 MCS251 代码用 `const near/far` |
| `far *` | 32 bit | 24 位地址、整个 16 MiB，`@DRk` | 只做 16 位指针算术；单对象 ≤64 KiB 且不能跨 64 KiB 边界 |
| `huge *` | 32 bit | 24 位地址、整个 16 MiB，`@DRk` | 32 位算术；对象可跨 64 KiB 边界，最大受 16 MiB 可用空间约束 |
| generic pointer | 16 或 32 bit | TINY/XTINY 为 `near *`；其余为 `far *` | `HPTR` 可把后三种模型的默认改为 `huge *` |
| near function pointer | 16 bit | 当前 64 KiB region | 近调用语义 |
| far function pointer | 32 bit | 整个 16 MiB | 远调用语义 |

MCS-251 线性地址值只有 24 个有效位，具体器件对外引出的地址线还可能更少；但 Keil 把 `far`/`huge` 指针表示为 4 B。这是一项 Keil ABI 选择，不是硬件强制 C 指针必须为 4 B。`far` 与 `huge` 的主要差异也不是可达地址，而是指针运算和单对象能否跨 64 KiB 边界。

TINY/XTINY 的 generic pointer 只有 2 B，不能到达 `0x010000` 以上；需要访问整个空间时必须显式使用 `far`/`huge`。指针变量自身的存储位置与它指向的 memory type 是两件事，例如 `char near * data p` 表示 `p` 本身在 `data`，所指对象在 `near`。

### Keil 的指针转换规则

[Pointer Conversions](https://www.keil.com/support/man/docs/c251/c251_le_conversionpointers.asp) 定义的是 Keil 前端/ABI 语义，主要规则为：

- `far -> code/near/xdata` 保留低 16 位；`far -> data/idata` 保留低 8 位。
- `near -> far` 把高字置 `0x0000`，保留 16 位偏移；`data/idata -> near` 对 8 位地址零扩展。
- `code -> far` 使用 code base 的高字，默认 region 为 `FF:`；`xdata -> far` 使用 xdata base，默认 region 为 `01:`。
- `near -> data/idata` 截断成低 8 位。
- memory-specific pointer 可按函数原型的形参类型转换为 generic pointer。在 TINY/XTINY 中，即使调用库函数，generic pointer 仍不能凭空到达 region `00:` 以外。

这些高位补值是 Keil 和目标启动/链接配置的组合，不应硬编码成 MCS-251 架构常量。SDCC 若使用 24 位 flat pointer，可定义自己的零扩展、地址空间标记和诊断规则。

## 代码大小模型与调用/返回宽度

[ROM Compiler Directive](https://www.keil.com/support/man/docs/c251/c251_rom.asp) 的默认是 `ROM(LARGE)`。它和数据内存模型正交：

| Keil code ROM model | 典型直接跳转/调用 | 范围 |
|---|---|---|
| SMALL | `AJMP` / `ACALL` | 整个程序 2 KiB |
| MEDIUM | 模块内 `AJMP` / `ACALL`，模块外 `LJMP` / `LCALL` | 单段 2 KiB，总计 64 KiB |
| COMPACT | `LCALL`，函数内可用 `AJMP` | 单函数 2 KiB，总计 64 KiB |
| LARGE | `LJMP` / `LCALL` | 函数和总计均不超过 64 KiB |
| HUGE | `far` 函数用 `ECALL` / `ERET`；显式 `near` 函数仍可近调用 | 整个 16 MiB，单函数仍 ≤64 KiB |

硬件规则来自 Intel Chapter 5：

- §5.5.1，印刷 5-13 / PDF 95：`addr11` 位于当前 2 KiB 页；`addr16` 位于当前 64 KiB region；`addr24` 可到整个 16 MiB。
- §5.5.4，印刷 5-15–5-16 / PDF 97–98：`ACALL`/`LCALL` 压入 16 位返回地址，`RET` 弹出 2 B；`ECALL` 压入 24 位返回地址，`ERET` 弹出 3 B。
- §4.8，印刷 4-16 / PDF 80：两字节中断帧压 2 B PC，只能返回 `FF:`；四字节中断帧压 PSW1 和 3 B PC，`RETI` 可返回任意 region。

STC 当前手册的指令条目也独立确认：`ECALL` 压 3 B PC 且 `SP += 3`（印刷 1208–1209 / PDF 1242–1243），`ERET` 弹 3 B 且 `SP -= 3`（印刷 1210 / PDF 1244），`LCALL` 压 2 B（印刷 1220 / PDF 1254），`RET` 弹 2 B、`RETI` 按中断帧模式恢复（印刷 1249 / PDF 1283）。`ERET` 条目的一句中文枚举误写成四个序号，但其 operation、SP 增量和 Intel 手册都一致证明是 3 B。

所以 `LCALL` 配 `ERET` 或 `ECALL` 配 `RET` 都会破坏栈；函数指针、尾调用、独立编译单元与链接器松弛必须保持调用/返回种类一致。Keil 的模型选择是一种编译策略，硬件的 2 B/3 B 返回地址才是不可违反的架构事实。

## Keil MCS251 参数、返回值和寄存器约定

以下是 Keil ABI 的描述，用于理解现有 MCS251 生态和设计测试，不自动成为 SDCC ABI。

### 参数

[Passing in Registers](https://www.keil.com/support/man/docs/c251/c251_ap_parampassreg.asp) 说最多有 9 个参数占用 `R11` 与 `R0`–`R7`，但能否入寄存器还取决于参数顺序和宽度。第一参数的典型位置为：

| 第一参数类型 | Keil 寄存器 |
|---|---|
| `char` 或 1 B pointer | `R11` |
| `int` 或 2 B pointer | `WR6` |
| `long`、`float` 或 4 B pointer | `DR4` |
| `double` | `DR0` + `DR4` |

后续可用 `R7`…`R0`、`WR4`…`WR0`、`DR0`/`DR4`，具体表应以该帮助主题为准；这不是简单的“前 N 个机器字参数”。C51 兼容的 3 B pointer 另有 `R1/R2/R3` 规则。

[Passing in Memory](https://www.keil.com/support/man/docs/c251/c251_ap_parampassmem.asp) 说明，即使参数实际从寄存器传递，编译器仍为它分配固定参数段。寄存器放不下的参数进入按 data model 选择的 `DATA`、`EDATA` 或 `XDATA` 固定区，bit 参数进入独立 bit 区。符号可采用 `?function?BYTE`、`?function?BIT` 一类 Keil/L251 命名。

### 返回值与保存规则

[Function Return Values](https://www.keil.com/support/man/docs/c251/c251_ap_funcret.asp) 给出：

| 返回类型 | Keil 寄存器 |
|---|---|
| `bit` | Carry |
| `char` / 1 B pointer | `R11` |
| `int` / 2 B pointer | `WR6` |
| `long` / `float` | `DR4` |
| `double` | `DR0` + `DR4` |
| 4 B pointer | `DR0` |
| 1–8 B struct | `R0`–`R7`，低有效字节落在 `R7` 端 |
| >8 B struct | 由 `DR12` 传入隐藏结果地址 |

[Register Usage](https://www.keil.com/support/man/docs/c251/c251_ap_regusage.asp) 将 `R0`–`R11`、`WR16`–`WR30`、PSW、PSW1、DPTR 列为可被被调函数破坏；`R12`–`R15` 由被调函数保存；`DR60` 是硬件栈指针并应保持栈纪律。这套分工以及 Keil 的标量字节序、对象格式、段名必须一起匹配，不能只复制一张寄存器表就声称 ABI 兼容。

`PARM251` 是 Keil 默认参数规则；`PARM51` 用于在源码边界与旧 Keil C51 对接。这是 Keil 私有互操作模式，不是通用 MCS-251 ABI。

## auto、overlay、reentrant 与扩展栈

Keil 的普通 block-local 默认为 `auto`，但“自动存储期”不等于“必然在运行时栈上”：

- 默认函数是非重入的。参数与自动变量通常分配到固定的、可 overlay 的静态内存，访问快但同一函数不能安全递归或重入。
- L251 根据调用树把不会同时存活的固定参数/局部段覆盖在同一地址。[Overlaying Data Memory](https://www.keil.com/support/man/docs/l251/l251_in_overlaying.asp) 和 [The Call Tree](https://www.keil.com/support/man/docs/l251/l251_ol_calltree.asp) 要求对函数指针、由中断到达的函数以及汇编隐藏调用补充调用关系；`NOOVERLAY` 可全局关闭，但通常应修正调用树。
- `reentrant` 函数把未入寄存器的参数和自动变量放到 251 hardware stack，因而可递归/重入；不允许 bit 参数或 bit local，访问也较慢。`FUNCTIONS` 可批量施加这一属性。
- 函数级 `small`/`large` 还会选择相应的静态参数区，或为 reentrant 函数选择不同的栈帧访问方案。

Intel §3.3.2.3，印刷 3-15 / PDF 57，定义 `DR60 = SPX`，其中 `SPH:SP` 是低 16 位；栈向高地址增长并位于 region `00:`。`ACALL`/`LCALL`/`ECALL`、`RET`/`ERET`/`RETI` 和 PUSH/POP 都使用它。它不是传统 8051 的 8 位小栈，但实际可用容量仍由芯片在 region `00:` 实现的 RAM 决定。

Keil [Hardware Stack](https://www.keil.com/support/man/docs/c251/c251_ap_hardwarestack.asp) 说明 START251 初始化并预留该栈，reentrant 参数/局部和返回地址共用它。栈大小不能只按 C 局部变量相加，还要包含 2 B/3 B 返回地址、保存寄存器、PUSH/POP、中断嵌套和最深调用链。

## Source mode 与 Binary mode

名称描述的是**操作码页选择**，不是 C ABI、目标文件格式或“输出是否为二进制文件”。Intel §4.6，印刷 4-13–4-15 / PDF 77–79，给出的硬件规则是：Area I 在两种模式下都不带前缀，下表只列出会交换位置的 Area II/III。

| CPU mode | 不带 `0xA5` 的冲突页 | 带 `0xA5` ESC 的冲突页 |
|---|---|---|
| Binary | 传统 MCS-51 area II | 新 MCS-251 area III |
| Source | 新 MCS-251 area III | 传统 MCS-51 area II |

Binary mode 的价值是让已有的、未修改的 MCS-51 机器码按原编码运行；Source mode 更适合大量使用 251 新指令的程序。STC Appendix A §A.1.1，印刷 1172 / PDF 1206，以及 §A.1.3，印刷 1174 / PDF 1208，确认 STC32G144K246 实现同样的 `0xA5` 互换规则；§A.1.2，印刷 1172–1173 / PDF 1206–1207，又明确 `@WRj` 是 `0x0000`–`0xFFFF`，`@DRk` 是 `0x000000`–`0xFFFFFF`，`addr24` 用于 `ECALL`/`EJMP`。

Keil MCS251 编译器默认生成 Source/native 代码；`MODBIN` 生成 Binary-mode 编码。即便选 `MODBIN`，MCS251 仍可能通过 ESC 使用新 251 指令，所以其产物不能因此宣称为普通 8051 兼容代码。A251 的 `MODSRC`/mode 标记还要进入目标文件，让 linker/debugger 检查模块模式是否一致。

另一个易混点是 MCS251 的 `SRC` directive：它只要求编译器输出汇编源文件，与 CPU Source opcode mode 无关。

## STC32G144K246 的具体映射

STC 的映射是目标配置，不是 MCS-251 通则。

### RAM 与线性地址

STC §12.2，印刷 268 / PDF 302，给出 16 KiB `edata` 和 128 KiB 片内 `xdata`；`edata` 单周期且可作栈，`xdata` 通常需 2–3 个时钟。§12.2.2，印刷 271 / PDF 305，说明低 256 B 仍兼容 8051：低 128 B 可直接/间接寻址，高 128 B 只能间接寻址，SFR 是重叠编号但独立的直接寻址空间。硬件栈逻辑上可在 region `00:` 使用 16 位 SP，K246 实际只实现 16 KiB `edata`。

Chapter 12 的地址图，印刷 265 / PDF 299，给出最完整的实际映射；§12.2.12，印刷 278 / PDF 312，给出较粗的汇总表：

| 线性范围 | STC32G144K246 用途 |
|---|---|
| `00:0000`–`00:3FFF` | 16 KiB 片内 `edata`，也是实际可用栈 RAM |
| `00:4000`–`00:FFFF` | 保留；SP 的硬件地址宽度虽允许，K246 并未安装 RAM |
| `01:0000`–`02:FFFF` | 128 KiB 片内 `xdata` |
| `03:0000`–`03:0FFF` | 4 KiB RAM 的 data alias；行为受 `RAMEXE` 控制 |
| `03:1000`–`7D:FFFF` | 保留数据区 |
| `7E:0000`–`7E:FFFF` | XFR，需 `EAXFR` |
| `7F:0000`–`7F:FFFF` | 外部 `xdata`，需 `EXTRAM` |
| `80:0000`–`80:0FFF` | 4 KiB RAM code，需 `RAMEXE` 才可执行 |
| `80:1000`–`FB:FFFF` | 保留 code 区 |
| `FC:0000`–`FC:27FF` | system code |
| `FC:2800`–`FE:FFFF` | `ecode` |
| `FF:0000`–`FF:FFFF` | 兼容 `code` |

§12.3.1，印刷 279–280 / PDF 313–314，补充 `80:0000`–`80:0FFF` 的别名行为：`RAMEXE=0` 时它以 data alias `03:0000`–`03:0FFF` 读写、不可执行；`RAMEXE=1` 时从 `80:` 读/执行，而 data alias 写入被阻止。§12.2.12 把 `03:0000`–`7D:FFFF` 整段粗略标成保留，漏掉了地址图和下一节明确给出的 4 KiB alias，应视为当前草稿的汇总表遗漏。§12.3.2，印刷 281 / PDF 315，说明 `far` 使用 24 位绝对地址，但单对象仍不能超过或跨越 64 KiB。

### STC 对 Keil 工程的建议

STC §2.15.2，印刷 76–78 / PDF 110–112，以及 §12.2.1，印刷 269–270 / PDF 303–304，推荐 `XSmall`：普通变量进入快速的 16 KiB `edata`，generic pointer 仍是 4 B，可访问位于 `FC:2800`–`FF:FFFF` 的程序内容。其当前界面表为：

| model | 默认变量 | STC 表中的默认常量 | 默认指针 |
|---|---|---|---:|
| Tiny | `data` | `near` | 2 B |
| XTiny | `edata` | `near` | 2 B |
| Small | `data` | `far` | 4 B |
| XSmall | `edata` | `far` | 4 B |
| Large | `xdata` | `far` | 4 B |

这里的 `edata` 对应 Keil 帮助中的 `near`/EDATA linker class；“默认常量 near/far”是 STC 当前目标界面的说明，而 Arm v5.60 的通用 [Implicit Memory Types](https://www.keil.com/support/man/docs/c251/c251_le_impmemtypes.asp) 对 SMALL/XSMALL/LARGE 写的是 `code`。这是版本/目标界面术语和目标配置差异，不能静默混成一张所谓通用 Keil 表。实现 SDCC 时应以自己的选项、地址空间和链接脚本明确规定默认值。

STC §2.15.3，印刷 79 / PDF 113，建议超过 64 KiB 程序选 Keil Code ROM Size `Huge`，并再次把它与 Memory Model 分成两个独立下拉项。§2.15.1，印刷 76 / PDF 110，推荐 Source (251 Native) 和 4 Byte Interrupt Frame Size。

## 文档内部冲突和采用原则

官方帮助也有遗留文字，不能把任意单句直接冻结成 ABI：

- [Pointer Conversions](https://www.keil.com/support/man/docs/c251/c251_le_conversionpointers.asp) 页末示例注释把 generic pointer 写成 3 B、near pointer 写成 1 B，但同页正文和规范性的 pointer-size 表明确是 2/4 B generic、2 B near。应采表格，并把示例注释视为陈旧残留。
- [REGPARMS](https://www.keil.com/support/man/docs/c251/c251_regparms.asp) 概述说“最多三个参数”，而详细的 [Passing in Registers](https://www.keil.com/support/man/docs/c251/c251_ap_parampassreg.asp) 表说最多 9 个。实现互操作前必须用编译器产物和调用测试验证，不能仅凭概述。
- [Calculating Stack Size](https://www.keil.com/support/man/docs/c251/c251_ap_stacksize.asp) 写默认 100 bytes；[Startup Code](https://www.keil.com/support/man/docs/c251/c251_ap_startup.asp) 则写 START251 的默认 `STACKSIZE EQU 100H`，即 256 B。SDCC 不应继承任一值，应由链接布局、目标 RAM 和最坏调用深度决定。
- STC 当前表的常量 `near/far` 与 Arm v5.60 通用表的 `near/code` 不同；前者是 K246 项目建议，后者是编译器帮助中的通用默认。
- Keil 个别网页仍以 64 KiB “program memory/code”描述 `code` 类型，而 `ROM(HUGE)` 和 Intel 架构明确支持 16 MiB。前者指兼容 `code` class/16 位窗口，不是架构上限。

采用顺序应为：硬件编码和状态转换听 Intel；K246 的实际映射、容量和控制位听当前 STC 手册；Keil 的语言与 ABI 事实以 v5.60 规范表/详细主题为先，并用实物编译测试消除冲突。

## 与当前 SDCC MCS251 ABI revision 2 的逐项对照

本节对照仓库中的 [SDCC MCS251 ABI revision 2](abi.md)。它只审查书面约定与官方事实是否相容，不代表这里重新验证了编译器实现。Keil 一栏是差异参照，不是兼容目标。

| 项目 | 当前 SDCC rev2 | Intel / Keil / STC 参照 | 结论与约束 |
|---|---|---|---|
| 兼容身份 | `-mmcs251`，ABI revision 2，ASxxxx `.rel` + Intel HEX；明确不是 Keil ABI | Keil 使用自己的对象、段、库和调试生态 | 边界正确；必须继续拒绝不同 ABI revision 或 Keil 对象的无声明混链 |
| opcode mode | 默认 Source mode；启动环境须保证 CPU 处于 Source | Intel 定义两种 opcode map；Keil 默认 Source；STC 推荐 Source | 架构一致；编译器/assembler mode 标记、启动寄存器和 QEMU 初态必须端到端一致，不能只改指令编码 |
| 标量字节序 | 采用 MCS-251 原生 big-endian 对象与 WR/DR register tuple 布局；不提供 little-endian 模式 | Intel Chapter 5 §5.2.1.1，印刷 5-2 / PDF 84，规定 word/dword 在 memory 和 register file 中为 big-endian；Keil 的 [2 B](https://www.keil.com/support/man/docs/c251/c251_ap_2bytescalar.asp) / [4 B scalar](https://www.keil.com/support/man/docs/c251/c251_ap_4bytescalar.asp) 也是高字节先存 | 与架构和 Keil 的标量端序一致；pointer size、calling convention 与 object format 仍是独立 ABI |
| data model | `--model-small` 把普通对象放 page-zero edata；`--model-large` 放 XSEG | Keil 有 TINY/XTINY/SMALL/XSMALL/LARGE 五种，并把 code ROM model 单列 | 名称相似但语义不兼容；SDCC 文档和诊断必须始终使用自己的定义，不应承诺 Keil model 等价 |
| `__data` / `__idata` pointer | 二者都是 1 B page-zero 地址 | Keil `data *` 为 1 B，`idata *` 为 2 B | `__idata` 明确不兼容 Keil；这只约束显式 near pointer，不限制使用 16 位 SPX 和 3 B flat address 的栈对象 |
| `__xdata` / `__far` pointer | 3 B flat 24-bit | Keil `xdata *` 为 2 B；`far *` 为 4 B且只做 16 位算术 | 表示和语义都不兼容；SDCC `__far` 可跨 64 KiB 的行为更接近 Keil `huge`，但仍不是 Keil `huge` ABI |
| `__code` pointer | 3 B flat 24-bit | Keil `code *` 为 2 B；全空间常量通常用 4 B `far`/`huge` pointer | 不兼容，但能自然表达 K246 的 `FC:`–`FF:` flash；转换、relocation 和只读约束需由 SDCC 自己定义 |
| generic pointer | 3 B flat、无 address-space tag、24 位进位 | Keil 在 TINY/XTINY 为 2 B near，其余通常为 4 B far，可由 `HPTR` 改 huge | 不兼容；无 tag 意味着 generic pointer 不能表达独立 SFR space，这与 rev2 的 `__sfr` 专用访问规则一致 |
| function pointer | 3 B；装入 `DR28` 后 `ECALL @DR28` | 硬件允许 24 位间接调用；Keil near function pointer 2 B、far function pointer 4 B | 硬件有效、Keil ABI 不兼容；所有间接调用必须配 `ERET` callee |
| direct/tail call | 一律 `ECALL` / `EJMP`，普通函数一律 `ERET` | Intel 允许 24 位全空间调用；Keil 只在 `ROM(HUGE)` 的 far 路径这样做 | 是保守且可链接的独立 ABI 选择，牺牲近调用尺寸换取跨模块位置无关；后续 relaxation 必须同时证明 callee return kind |
| 参数与返回值 | 首个 scalar/return 用 DPL/DPH/B/A；3 B pointer 用 DPL/DPH/B；bit 用 Carry | Keil 用 R11、WR6、DR4/DR0 等，4 B pointer return 在 DR0 | 明确不兼容；不得把 Keil 优化规则局部移植进 rev2 而不升 ABI revision |
| 非重入参数/overlay | 后续非重入参数用 SDCC overlayable parameter areas | Keil/L251 也会静态分配和 overlay，但段名、调用图元数据和布局不同 | 只能借鉴活跃期分析思想，不能共享固定参数符号或链接器元数据 |
| reentrant/extended stack | `__reentrant` 参数走向上增长的 hardware stack；`--stack-auto` 可把全部普通自动对象和非寄存器参数切到同一 ABI；`SPX` 由 startup 初始化；scalar slot 用 signed `@SPX+dis16`，栈对象地址物化为 region `00:` 的 3 B flat pointer | Intel 的 SPX/DR60 允许 region `00:` 的 16 位栈；STC K246 实装 16 KiB edata；Keil reentrant 也用该栈 | 已能跨越任意 256 B 页边界；default 与 stack-auto 库必须匹配，后者提供 small/large 独立库目录；实际容量由芯片 RAM、链接布局、调用深度和中断嵌套共同约束，K246 上限不能超过其 16 KiB edata |
| return frame / `setjmp` | 所有 C call 按 3 B return PC；15 B `jmp_buf` 保存 2 B SPX、3 B PC、2 B 结果传递区和 R0–R7 | `ECALL`/`ERET` 的硬件帧正是 3 B | 已用四种 model/stack-auto 真实运行库把 SPX 设为 `0x0120`，验证 `longjmp` 恢复完整 SPX、3 B PC、16 位结果和 live register value |
| interrupt frame | 采用目标的 4 B interrupt frame，`RETI` 返回 | Intel 4 B frame = PSW1 + 3 B PC；STC §2.15.1 推荐勾选 4 Byte Interrupt Frame Size | 与 K246 一致；仍需确认真实 CPUMODE/中断配置寄存器初态，不能从 Keil UI 反推位极性 |
| XSEG 默认位置 | `0x010000`，允许 board 用 `--xram-loc` 改写 | Intel `DPXL` reset 为 `01h`；K246 的片内 xdata 从 `01:0000` 开始 | 对 K246 是合理默认，不应升级为所有 MCS-251 芯片的 ABI 常量 |
| SFR | 不通过 flat generic pointer，到 `__sfr`/`__sbit` 专用寻址 | Intel SFR 是独立 512 B space，不属于 16 MiB memory | 架构一致；避免把 `0x000080` 与 `S:080` 合并别名 |

最重要的结论是：rev2 选择了“3 B flat pointer + 原生 big-endian layout + 全远调用”，而 Keil 选择“2/4 B 分类型 pointer + big-endian layout + ROM model 决定调用距离”。两者的标量端序相同，但 pointer representation、calling convention 与 object format 不同，因而不会自然互操作。

## SDCC 可以借鉴什么

可以独立实现并写入 SDCC 自己规范的内容：

- Intel 公开架构规定的 24 位线性地址、独立 SFR/register spaces、Source/Binary opcode map、`@WRj`/`@DRk` 可达范围以及 2 B/3 B 返回地址。
- 近/远访问代价不同的优化思想：可把确定在 region `00:` 的对象/指针用 16 位 WR 寻址，把完整线性地址用 DPX/DR 寻址。
- 把“数据默认放置策略”和“代码调用距离策略”分离。rev2 中不能只把 `ECALL` 缩成 `LCALL`；近调用优化必须通过显式 near-function ABI、thunk，或能同时证明并改写 call/return 两端的全程序优化完成。
- 为跨 `0x??FFFF -> 0x??+1:0000` 的 pointer arithmetic、截断/扩展、direct/indirect call、2 B/3 B return frame 建立运行时测试。
- K246 target/board 层采用 STC 的实际 `edata`、`xdata`、XFR、RAM-code 和 flash 映射，而不是照抄 Intel 8XMCS251Sx 的四-region 实现。
- 只有在调用图完整且对函数指针、中断、汇编边界保守时，才考虑 overlay 优化；否则自动变量放栈或不覆盖更安全。

## SDCC 不应复制或宣称兼容什么

- 不复制 Keil CHM 的文字、表格排版、示例源码或专有库；本文只做必要的事实性概括。v5.60 包没有给这些内容附开放源码许可证。
- 当前 SDCC ABI 不应把 Keil OMF251 对象格式、linker class/segment 命名、`?function?BYTE`/`?function?BIT` 固定参数符号、overlay 元数据或库 ABI 当作默认目标。若未来明确追求互操作，应另起 ABI/对象模式，依公开接口资料独立规范化、做授权审查和一致性测试，不复用 Keil 的实现或受保护资产。
- 不因采用相似的 `near`/`far`/`huge` 名称、寄存器或调用指令，就声称与 Keil binary ABI、object format、library 或 debugger 兼容。
- 不把 `PARM51`、Keil 的 register allocation、struct return、caller/callee-save 等零散规则混入 SDCC 现有 ABI。SDCC 的大端布局独立遵循 MCS-251 架构，并不表示兼容 Keil。若未来提供 Keil-compatible ABI，它必须是显式的独立模式，有完整对象布局、varargs、aggregate、bit、函数指针和跨模块测试。
- 不把 “Binary mode” 当作 C ABI 兼容承诺。它只说明 opcode 页；Keil/SDCC 生成的 Binary-mode C 程序仍可使用 251 指令，也仍有各自的调用和对象格式。
- 不把 STC 的 K246 映射或推荐的 XSmall/Huge 选项宣称为所有 80MCS251 实现的默认值。

SDCC 当前应把自己的 ABI 文档作为唯一承诺来源。若其 pointer width、字节序、参数寄存器或 direct-call 策略不同于 Keil，这本身没有问题；关键是明确版本、拒绝混链、由编译器/汇编器/linker/QEMU 的端到端测试共同约束。
