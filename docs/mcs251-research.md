# SDCC 80MCS251（MCS-251）后端：一手资料、实现边界与验收基线

核验日期：2026-07-31（Asia/Shanghai）

本文是方案审批前的研究基线。它只使用 SDCC、STC、Intel、Arm/Keil 的官方资料，以及本机 `/Users/zevorn/oss/qemu` 的目标实现；本轮没有修改 SDCC 或 QEMU，也没有把 PDF、ZIP、源码压缩包等二进制资料提交到仓库。

## 0. 结论与建议基线

1. **源码基线**：正式版是 SDCC **4.6.0**；官方 `trunk` 在核验时是开发版 **4.6.2**、SVN r16742、Git 镜像提交 `e212e94b189d327f4821026ae8994317dd498442`。实现新后端宜从这个固定的 `trunk` 提交开始，同时保留 4.6.0 构建作为回归参照。
2. **当前没有可复用的 MCS251 后端**：SDCC 现有端口中没有 `mcs251`、`mcs251` 或 `80mcs251`；已撤掉维护的 `xa51` 也不是 80MCS251 后端。工作量包括编译器端口、汇编器/链接器、启动代码、运行库和回归端口，不能只加一张 opcode 表。
3. **ISA 范围**：“所有 MCS251 指令”应定义为 Intel/ STC 指令附录中全部合法的助记符、操作数形式、位宽、寻址方式和两套 opcode 映射；STC32G 默认产物必须使用 **Source（251 Native）** 映射，必要的经典 MCS-51 形式用 `A5/ESC` 前缀。
4. **精确目标机器**：QEMU 机器是 `stc32g144k246-evb`，复位 PC `0xff0000`，用户 Flash `0xfc2800..0xffffff`。Intel HEX 地址按绝对 24 位地址加载；raw 文件从 `0xfc2800` 起装载，复位代码位于文件偏移 `0x2d800`。
5. **Source 模式兼容**：普通 STC32G 手册明确该系列只支持 Source 模式；STC32G144K246 手册则明确该子系列同时支持 Binary/Source，Keil 默认 Source。QEMU 复位为 Source，并把 `AUXR2.CPUMODE=1` 解释为 Binary；这个位的极性是 QEMU 的明确推断，不是 STC 已发布的定义。默认编译产物不应写 CPUMODE。
6. **官方 ABI 存在**：STC 芯片手册没有完整 C ABI，但 Arm 官方发布的 Keil MCS251 v5.60 文档包给出了参数、返回值、易失寄存器、硬件栈、类型和字节序规则。新 SDCC 端口仍需明确选择“Keil 调用约定兼容”或“SDCC 自有 ABI”；仅采用调用约定并不等于兼容 Keil 的 OMF-251 对象文件。
7. **端序**：MCS-251 的普通 16/32 位内存对象和寄存器值为大端，且允许非对齐；指令流里的多字节立即数/地址也是高字节在前。调用压栈和中断帧存在指令规定的顺序，不能用普通对象端序替代。
8. **验收方法**：用官方手册产生独立的机器码黄金向量，再用本地 QEMU 做行为验证；不能让新汇编器和 QEMU 互相充当唯一 oracle。最终门槛应是由 SDCC 编译的 C 程序经 UART 输出 PASS，而不只是手写机器码通过。

## 1. SDCC 官方最新源码

### 1.1 可复现版本

| 用途 | 官方状态（核验时） | 固定标识 | 官方入口 |
|---|---|---|---|
| 稳定发布 | SDCC 4.6.0，发布页时间 2026-06-22 | `sdcc-src-4.6.0.tar.bz2` | [4.6.0 文件页](https://sourceforge.net/projects/sdcc/files/sdcc/4.6.0/) |
| 持续开发 | `.version` 为 4.6.2 | SVN r16742 | [官方 SVN 浏览器](https://sourceforge.net/p/sdcc/code/HEAD/tree/trunk/sdcc/) |
| 官方 Git 镜像 | `trunk` | `e212e94b189d327f4821026ae8994317dd498442` | [Git 镜像页面](https://sourceforge.net/p/sdcc/git-mirror/ref/trunk/) |

稳定版直链：

```text
https://sourceforge.net/projects/sdcc/files/sdcc/4.6.0/sdcc-src-4.6.0.tar.bz2/download
```

本次实际下载结果：`26,552,471` 字节，SHA-256 `5fd6a93e5997ce01756868fe35e441095cfb637894a80c262514a634094973b6`。

开发版固定获取方式：

```sh
git clone --branch trunk --single-branch \
    https://git.code.sf.net/p/sdcc/git-mirror sdcc
cd sdcc
git checkout e212e94b189d327f4821026ae8994317dd498442
```

该 Git 提交的提交信息带有 `git-svn-id: .../trunk/sdcc@16742`，因此能同时固定 Git 和 canonical SVN 基线。以后更新“最新源码”时必须先重新记录提交和 SVN revision，不能浮动依赖 `trunk`。

### 1.2 构建方式

源码包和当前 trunk 都带有生成好的 `configure`。官方 Wiki 的构建流程也是 `configure` 加 GNU make；建议使用独立构建目录：

```sh
mkdir build
cd build
../configure --prefix=/absolute/path/to/sdcc-install
make -j4
make install
```

当前 `configure --help` 提供逐端口的 `--disable-*-port`、`--disable-ucsim`、`--disable-device-lib`、`--disable-sdbinutils`、`--enable-doc` 等选项。开发 MCS251 端口时应保持汇编器、链接器和 device library 开启。修改 `configure.ac` 后需要重新运行 Autoconf；参见 SDCC 官方 [Adding a port](https://sourceforge.net/p/sdcc/wiki/Adding%20a%20port/) 页面。

### 1.3 现状和新端口接线面

当前 `src/` 的目标目录是 `ds390`、`f8`、`hc08`、`mcs51`、`mos6502`、`pdk`、`pic14`、`pic16`、`stm8`、`z80`，没有 MCS251。`doc/README.txt` 只把旧 `xa51` 列为“不再维护”，不存在可恢复的完整 80MCS251 编译器或汇编器。

按照官方移植指南，完整端口至少要接入：

- `src/<port>/` 的端口描述、代码生成和寄存器分配；
- `configure.ac`、顶层/`src` Makefile、`src/port.h`、`src/SDCCmain.c`；
- `sdas/` 下能表达完整 MCS251 指令与重定位的 assembler/linker 支持；
- `device/lib/<port>/` 的 crt、运行库和内存模型支持；
- `support/regression/ports/<port>/` 的模拟器运行配置；
- 用户文档、命令行帮助、目标预定义宏和安装清单。

建议用户接口名为 `-mmcs251`，但这是待批准的项目设计，不是现有 SDCC 接口。

## 2. 官方资料清单与保留策略

### 2.1 STC32/车规产品页的完整资料

入口是 [STC32/车规系列](https://www.stcai.com/cp_stc32xl)。`cp_stc32xl` 是遗留 URL 名，当前页面实际展示 STC32 系列。与 MCS251 实现直接相关的完整文档为：

| 文档 | 页面/文档日期 | 用途 | 官方直链 |
|---|---:|---|---|
| 《STC32G 系列单片机原理及应用》 | 2026-07-08 | 最新通用 STC32G 手册；附录 A 是主 ISA 依据 | [STC32G.pdf](https://www.stcaimcu.com/data/download/Datasheet/STC32G.pdf) |
| 《STC32G144K246/96K246/18K64 单片机原理及应用》 | 2026-07-15 | 与 QEMU 机器同型号；仍标为逐章校核中的初稿框架 | [STC32G144K246.pdf](https://www.stcaimcu.com/data/download/Datasheet/STC32G144K246.pdf) |
| 《STC32G12K128-24A 车规系列技术参考手册》 | 2024-05-30 | 车规子系列交叉核验 | [STC32G12K128-24A.pdf](https://www.stcaimcu.com/data/download/Datasheet/STC32G12K128-24A.pdf) |
| *STC32G Series MCUs Reference Manual* | 页面当前英文资料 | 英文选型/外设；没有完整 ISA 附录 | [STC32G-EN.pdf](https://www.stcaimcu.com/data/download/Datasheet/STC32G-EN.pdf) |
| 《STC32F 系列技术参考手册》 | 2024-02-02 | 较早版本附录 A 的独立交叉核验 | [STC32F.pdf](http://www.stcaimcu.com/data/download/Datasheet/STC32F.pdf) |
| 《STC32G 系列实验指导书》 | 2025-10-21 | Keil/下载/板级实验参考，不是 opcode 规范 | [STC32G-实验指导书.pdf](https://www.stcaimcu.com/data/download/Specification/STC32G-%E5%AE%9E%E9%AA%8C%E6%8C%87%E5%AF%BC%E4%B9%A6.pdf) |

页面另提供分卷下载。STC32G 分卷为 [封面/目录](https://www.stcaimcu.com/data/download/Datasheet/STC32G/STC32G-1-%E5%B0%81%E9%9D%A2%E5%8F%8A%E7%9B%AE%E5%BD%95.pdf)、[Chapter 1–2.20](https://www.stcaimcu.com/data/download/Datasheet/STC32G/STC32G-2-Chapter1-2.1-2.20.pdf)、[Chapter 2.21–3](https://www.stcaimcu.com/data/download/Datasheet/STC32G/STC32G-3-Chapter2.21-2.33-3.pdf)、[Chapter 4–20](https://www.stcaimcu.com/data/download/Datasheet/STC32G/STC32G-4-Chapter4-20.pdf)、[Chapter 21–30](https://www.stcaimcu.com/data/download/Datasheet/STC32G/STC32G-5-Chapter21-30.pdf)、[Chapter 31–39](https://www.stcaimcu.com/data/download/Datasheet/STC32G/STC32G-6-Chapter31-39.pdf) 和 [附录/ISA](https://www.stcaimcu.com/data/download/Datasheet/STC32G/STC32G-7-%E7%9B%AE%E5%BD%95.pdf)。最后一个文件名虽含“目录”，实质是 355 页附录；它比完整手册旧，冲突时以 2026-07-08 完整版为准。

STC32G144K246 分卷为 [第 1 卷](https://www.stcaimcu.com/data/download/Datasheet/STC32G144K246/STC32G144K246-1.pdf)、[第 2 卷](https://www.stcaimcu.com/data/download/Datasheet/STC32G144K246/STC32G144K246-2.pdf)、[第 3 卷](https://www.stcaimcu.com/data/download/Datasheet/STC32G144K246/STC32G144K246-3.pdf)、[第 4 卷](https://www.stcaimcu.com/data/download/Datasheet/STC32G144K246/STC32G144K246-4.pdf)、[第 5 卷/附录](https://www.stcaimcu.com/data/download/Datasheet/STC32G144K246/STC32G144K246-5.pdf)。核验时这些直链均返回有效 PDF。

### 2.2 Intel 原始 MCS-251 资料

STC 官方 [传统 MCU 手册页](https://www.stcai.com/cp_ctmcusc) 重新发布了 Intel 的三份原始文档：

- [8XMCS251SA/SB/SP/SQ User's Manual](https://www.stcaimcu.com/data/download/Datasheet/8xmcs251sx_um.pdf)：架构、寄存器、地址空间、端序、Binary/Source opcode 映射；
- [MCS-251 Microcontroller Family Programmer's Reference Manual](https://www.stcaimcu.com/data/download/Datasheet/8xmcs251_instr.pdf)：Appendix A 的完整指令、操作数和编码表；
- [8XMCS251SA/SB/SP/SQ Data Sheet](https://www.stcaimcu.com/data/download/Datasheet/8xmcs251sx_ds.pdf)：具体 Intel 器件数据手册。

实现 opcode/relocation 时应把 Intel Programmer's Reference 与最新 STC 附录做双重核对；STC 表内存在个别明显笔误，不能只从一份 PDF 自动生成全部 oracle。

### 2.3 Arm/Keil MCS251 官方 ABI 资料

Arm 官方文档条目 [User's Guides for Keil MCS251 Development Tools, v5.60](https://documentation-service.arm.com/documentation/101647/latest) 的 API 返回资源：

```text
https://documentation-service.arm.com/static/635bcfb5c5a70d2cdb15fdb6?token=
```

资源名为 `Keil_C251_Docs.zip`，内含 `c251.chm`、`a251.chm`、`is251.chm`、`l251.chm` 等。ABI 关键页面是 `c251.chm` 中的：

- `c251_ap_parampassreg.htm`（寄存器传参）；
- `c251_ap_parampassmem.htm`（固定内存传参）；
- `c251_ap_funcret.htm`（返回值）；
- `c251_ap_regusage.htm`（寄存器保存规则）；
- `c251_ap_hardwarestack.htm`（硬件栈布局）；
- `c251_le_datatypes.htm`、`c251_le_memtypepointers.htm`（类型和指针）；
- `c251_xe.htm`（字节序）。

### 2.4 已核验文件、大小与 SHA-256

大型资料**不留在仓库**。本轮只在临时目录 `/tmp/sdcc-mcs251-research.uBoBI8/` 下载并解析；该路径不可作为长期依赖。

| 文件 | 字节数 | 页数 | SHA-256 |
|---|---:|---:|---|
| `sdcc-src-4.6.0.tar.bz2` | 26,552,471 | — | `5fd6a93e5997ce01756868fe35e441095cfb637894a80c262514a634094973b6` |
| `STC32G.pdf` | 73,016,510 | 1,986 | `1ef45fb0f67aa88b9c7ce746e7e9d828cd3d1159d1868eb3ac9f0d6e8a0069f0` |
| `STC32G144K246.pdf` | 37,889,850 | 1,321 | `da7bf04ee6163300ae5140cf201b3c161626338547ce719385c0af9c83c1f6cd` |
| `STC32G-7-目录.pdf` | 10,928,248 | 355 | `b3d697a9810ff7b77d924ccf075ba64e642e625daba53d155822bfbb446a78d9` |
| `8xmcs251_instr.pdf` | 478,330 | 139 | `3e9e898af1495ab73f44a77e524803ddaf5892dc7d8349b28f41b1f6f6a9d424` |
| `8xmcs251sx_um.pdf` | 2,225,431 | 458 | `bffda43f128d415a07c596e4720e8731d57ab9ffa6284ebecea1f0a90c8f35fd` |
| `8xmcs251sx_ds.pdf` | 220,739 | 36 | `9b0a0d7db2a574b669a2c0f7f4ba2b502afebb9545216d6ee645e8876e6d8525` |
| `Keil_C251_Docs.zip` | 8,810,621 | — | `88fb380aeadaa6162d7d53ac4f553e92238bac8c85314aa7b0f0741b379f08fc` |

可重复获取核心证据集：

```sh
refdir=/tmp/mcs251-reference
mkdir -p "$refdir"
curl -L --fail -o "$refdir/STC32G.pdf" \
    https://www.stcaimcu.com/data/download/Datasheet/STC32G.pdf
curl -L --fail -o "$refdir/STC32G144K246.pdf" \
    https://www.stcaimcu.com/data/download/Datasheet/STC32G144K246.pdf
curl -L --fail -o "$refdir/8xmcs251_instr.pdf" \
    https://www.stcaimcu.com/data/download/Datasheet/8xmcs251_instr.pdf
curl -L --fail -o "$refdir/8xmcs251sx_um.pdf" \
    https://www.stcaimcu.com/data/download/Datasheet/8xmcs251sx_um.pdf
curl -L --fail -o "$refdir/Keil_C251_Docs.zip" \
    'https://documentation-service.arm.com/static/635bcfb5c5a70d2cdb15fdb6?token='
shasum -a 256 "$refdir"/*
```

若上游替换同名文件，SHA 不匹配应触发人工重新核验页码和编码表，而不是静默接受。

## 3. 可直接落为实现契约的架构事实

### 3.1 Binary、Source 与 `A5/ESC`

Intel 8XMCS251 User's Manual §4.6（印刷页 4-14，PDF 物理页 78）把首字节 opcode 分为三块：

- Area I（经典表的列 0–5）是两种模式共享的无前缀指令；
- Area II 是会与新指令冲突的 MCS-51 指令；
- Area III 是 MCS-251 新指令；
- Binary 模式中 Area II 无前缀、Area III 加 `A5`；Source 模式交换 Area II/III，因此 MCS-251 新指令无前缀、冲突的经典指令加 `A5`；
- `A5` 只切换紧随其后的指令解码映射。未使用 opcode 按 NOP 处理。

同页官方例子：`DEC A` 两种模式都是 `14`；`SUBB A,R4` 在 Source 模式是 `A5 9C`；`SUB R4,R4` 在 Source 模式是 `9C`。完整首字节网格见 Intel Programmer's Reference Appendix A 的 Tables A-6/A-7（PDF 物理页 4–7），以及 STC32G 印刷页 1595–1598（PDF 物理页 1635–1638）；逐操作数编码见 STC32G 印刷页 1599–1680。

STC 的产品差异必须保留：

- STC32G 完整手册 §2.17.4 **印刷页 121 / PDF 物理页 161** 明确写“STC32G 系列目前只支持 Source 模式”，并要求选择 `Source (251 Native)`；
- STC32G144K246 手册 §2.15.1 **印刷页 76 / PDF 物理页 110** 明确写该子系列目前支持 Binary 和 Source 两种模式，Keil 下 Source 是默认选择；
- 两份手册都建议启用 `4 Byte Interrupt Frame Size`。

因此，面向 QEMU/STC32G144K246 的编译器默认值应为 Source；汇编器为了“完整 MCS251”可以提供显式 Binary 模式并测试两种编码，但不能让普通 STC32G 产物依赖 Binary。

附录 A 的 65 个助记符家族如下；验收粒度必须进一步细化到每个合法操作数形式：

```text
ADD ADDC SUB SUBB CMP INC DEC MUL DIV DA
ANL ORL XRL CLR CPL SETB RL RLC RR RRC SLL SRA SRL SWAP
MOV MOVC MOVH MOVS MOVX MOVZ PUSH POP XCH XCHD
ACALL LCALL ECALL RET ERET RETI AJMP LJMP EJMP SJMP JMP
JB JBC JC JE JG JLE JNB JNC JNE JNZ JSG JSGE JSL JSLE JZ
CJNE DJNZ NOP TRAP ESC
```

`TRAP` 的已发布 STC 语义是 NOP。STC 的 MDU32、DSP32、TFPU 是通过 SFR/XFR 命令驱动的片上加速器，不是附录 A 的 MCS251 CPU opcode；它们不属于“全部 MCS251 指令”的第一阶段完成条件。

### 3.2 R/WR/DR 重叠和合法编号

Intel User's Manual §3.3（印刷页 3-10–3-15，PDF 物理页 52–57）定义 40 个寄存器文件位置 `0..31`、`56..63`，`32..55` 保留：

| 记号 | 合法编号 | 重叠关系/限制 |
|---|---|---|
| `R` | `R0..R15` | 单字节；`R0..R7` 来自当前 PSW bank，`R8..R15` 固定可见 |
| `WR` | `WR0,WR2,...,WR30` | `WRn = {Rn,R(n+1)}`；位置 16..31 只能作为 WR/DR 访问 |
| `DR` | `DR0,DR4,...,DR28,DR56,DR60` | `DRn` 覆盖从位置 n 开始的 4 字节；56..63 只能作为 DR 访问 |

多字节寄存器中最低编号字节存 MSB。例如 `WR6` 的 MSB 在 `R6`、LSB 在 `R7`。寄存器分配器必须把 R/WR/DR 当成重叠寄存器类，任何部分写都要正确 kill 相交 live range。

专用重叠：

- `R10 = B`，`R11 = ACC`；
- `DR56 = DPX`，位置 57/58/59 分别是 `DPXL/DPH/DPL`，低 24 位构成扩展数据指针；
- `DR60 = SPX`，位置 62/63 是 `SPH/SP`；PUSH/POP、call/return/RETI 都使用 SPX，**不得把 DR60 当通用寄存器**；
- `R0..R7` 有 4 个可切换 bank；各 bank 同时映射到 `00:0000..00:001f`。

### 3.3 端序、地址宽度和寻址形式

Intel User's Manual §5.2.1.1（印刷页 5-2，PDF 物理页 84）明确：普通 word/dword 在内存和寄存器文件中均为**大端**，MSB 在最低地址/最低寄存器位置，LSB 在最高地址；word/dword 可从任意字节地址开始，不要求 2/4 字节对齐。Arm/Keil 的 `c251_xe.htm` 给出相同 ABI 规则，并指出 LCALL 返回地址压栈顺序是例外。

MCS-251 架构有 24 位 PC 和 16 MiB 线性地址 `00:0000..FF:FFFF`。STC32G 的具体映射把 `00:..7F:` 用作数据范围、`80:..FF:` 用作代码范围，复位入口是 `FF:0000`；这是一种 SoC 映射，不应硬编码进通用 80MCS251 后端的所有层。

主要操作数形式：

| 形式 | 范围/语义 |
|---|---|
| `Rn` | 当前 bank 的 R0–R7 |
| `Rm` | R0–R15 |
| `WRj` | WR0、WR2、…、WR30 |
| `DRk` | DR0、DR4、…、DR28、DR56、DR60 |
| `@Ri` | 经典 R0/R1 的 8 位间接地址 |
| `dir8` | `00:0000..00:007f` 的 data/word，或 SFR 直接地址 |
| `dir16` | `00:0000..00:ffff` 的 byte/word |
| `@WRj` | 16 位间接，只覆盖 region `00:` |
| `@DRk` | 低 24 位间接，可覆盖 16 MiB；DR 的高 8 位必须为 0 |
| `@WRj+dis16` | 16 位有符号位移，在 region `00:` 内回绕 |
| `@DRk+dis24` | DR 的 24 位基址加 16 位有符号位移 |
| `#data8/#data16` | 8/16 位立即数 |
| `#0data16/#1data16` | 32 位零扩展/全 1 高半字扩展常量 |
| `#short` | INC/DEC 编码的 1、2、4 |
| `rel` | 相对下一条指令的 `-128..+127` |
| `addr11` | 与下一条指令同一 2 KiB 块 |
| `addr16` | 与下一条指令同一 64 KiB region |
| `addr24` | 任意 24 位目标；ECALL/EJMP |

指令流内 `data16`、`dir16`、16 位位移和 `addr16/addr24` 都按高字节到低字节编码。汇编器/链接器必须分别实现 rel8、addr11 page、addr16 region、addr24 的范围校验和重定位，不能把 24 位目标截成 16 位。

### 3.4 中断和调用的硬件事实

- `ACALL/AJMP` 的 11 位目标受 2 KiB 块限制；`LCALL/LJMP` 的 16 位目标受当前 64 KiB region 限制；`ECALL/EJMP` 改写完整 24 位 PC。
- `RET` 弹 2 字节返回地址，`ERET` 弹 3 字节。
- STC32G 使用 4 字节中断帧，RETI 恢复 `PSW1 + 24-bit PC`；Intel 对 4 字节帧给出的入栈次序是 `PSW1, PC[23:16], PC[7:0], PC[15:8]`。启动代码和中断序言必须按目标芯片规则测试，不能照搬 8051 的 2 字节帧。
- 复位后 `SP=0x07`。crt 应显式初始化 SPX 到链接脚本预留的 edata 栈范围，并在进入 `main` 前完成 `.data` 复制和 `.bss` 清零。

## 4. Arm/Keil MCS251 ABI：已存在的官方参考

STC 手册只给 Memory Model、指针宽度和若干 Keil 设置；完整调用约定来自 Arm/Keil v5.60 文档包。下面是可用于兼容性测试的官方规则摘要。

### 4.1 类型和传参

Keil MCS251 的 `char/short/int/long` 分别为 1/2/2/4 字节；默认 `double` 可等同 float，启用 FLOAT64 后为 8 字节。内存特定指针有 1、2、4 字节形式；`far/huge` 指针为 4 字节但地址值范围是 24 位。

寄存器传参最多 9 个，按“仍可用且适合该类型的寄存器”分配：

| 类型 | 第 1 候选 | 第 2 候选 | 后续候选 |
|---|---|---|---|
| char / 1-byte pointer | R11 | R7 | R6、R5、R4、R3、R2、R1、R0 |
| int / 2-byte pointer | WR6 | WR4 | WR2、WR0 |
| long / float | DR4 | DR0 | — |
| 4-byte pointer | DR0 | DR4 | — |
| double | DR0 + DR4 | — | — |

放不进寄存器的参数使用固定参数区或硬件栈，取决于 static/reentrant 属性。Keil 默认 static 代码用 `?function?BYTE`、`?function?BIT` 固定区并可 overlay；reentrant 代码把非寄存器参数和自动变量放到向高地址增长的 SPX 硬件栈。若以“Keil ABI 兼容”为目标，这一差异必须明确实现或明确声明不兼容。

### 4.2 返回值和寄存器保存

| 返回类型 | 位置 |
|---|---|
| bit | Carry |
| char / 1-byte pointer | R11 |
| int / 2-byte pointer | WR6（MSB R6，LSB R7） |
| long / float | DR4 |
| 4-byte pointer | DR0（MSB R0，LSB R3） |
| double | DR0 + DR4 |
| 1..8 字节 struct | R0..R7 中以 R7 结束的连续字节 |
| 大于 8 字节 struct | 调用者在 DR12 传隐藏目标地址 |

调用者应假设 `R0..R11`、`WR16..WR30`、`PSW`、`PSW1`、`DPTR` 被调用破坏；使用 `R12..R15` 的被调者必须保存/恢复。`DR60/SPX` 在返回时必须与入口一致。由于寄存器重叠，保存规则要按底层位置集合计算，而不是把 R、WR、DR 当作独立寄存器。

### 4.3 项目必须做出的 ABI 决策

为了最大化现有汇编源码和 Keil 生态兼容性，建议至少让公开 C 函数采用上述 Keil 寄存器参数、返回值、callee-save 和大端布局。但在编码前仍需批准以下选择：

1. 是否复制 Keil 的 static 固定参数区/overlay，还是让 SDCC 默认使用可重入硬件栈；
2. near/far/huge 和 SDCC 现有 `__data/__xdata/__code` 地址空间如何映射；
3. 4 字节指针的高字节表示、函数指针模型、空指针表示；
4. struct/union/bit-field 对齐与打包、varargs、尾调用、名字修饰；
5. 普通函数何时使用 LCALL/RET，何时使用 ECALL/ERET；
6. 是否追求 Keil OMF-251 `.OBJ/.LIB` 互操作。调用约定兼容本身不能证明对象格式和重定位兼容。

任何选择都应写成仓库内版本化 ABI 文档，并用独立编译单元和手写汇编互调测试冻结。

## 5. `/Users/zevorn/oss/qemu` 的实际契约

### 5.1 调查快照和工作树状态

调查基线是分支 `mcs251-target-support`、提交 `213d2d68d7e326980a9c771f160d287488a96011`（2026-07-31）。稳定代码引用可从 [zevorn/qemu 该提交](https://github.com/zevorn/qemu/tree/213d2d68d7e326980a9c771f160d287488a96011) 查看。

本机 QEMU 工作树当时不是 clean：正在把 `target/mcs251`、`hw/mcs251` 等重构到共享的 `mcs51` 路径。因此本文以提交 hash 固定行为，路径说明以该提交为准；不要在 SDCC 工作中顺手修改或清理这个 QEMU 工作树。

本机已有 `/Users/zevorn/oss/qemu/builds/build-mcs251/qemu-system-mcs251`（QEMU 11.0.50 dirty）。本轮运行了该构建目录的 `qemu:func-mcs251-isa`，7 个功能子测试全部通过。由于 Meson 按当前正在重构且非 clean 的 QEMU 工作树重新生成并编译了相关文件，这只能作为本地接口基线，不能替代后续针对固定提交的可复现 clean build。

### 5.2 构建、启动和镜像加载

QEMU 自带目标文档 [`docs/system/target-mcs251.rst`](https://github.com/zevorn/qemu/blob/213d2d68d7e326980a9c771f160d287488a96011/docs/system/target-mcs251.rst) 给出的命令：

```sh
mkdir build-mcs251
cd build-mcs251
../configure --target-list=mcs251-softmmu
ninja

./qemu-system-mcs251 -M stc32g144k246-evb \
    -bios /absolute/path/to/firmware.hex -nographic
```

加载约定由 [`hw/mcs251/stc32g144k246.c`](https://github.com/zevorn/qemu/blob/213d2d68d7e326980a9c771f160d287488a96011/hw/mcs251/stc32g144k246.c) 实现：

- 后缀大小写不敏感的 `.hex` 使用 Intel HEX loader；记录地址是绝对 MCS-251 地址，支持 extended segment/linear record；start-address record 不改变硬件复位 PC；
- 其他后缀按 raw image 从 Flash 基址 `0xfc2800` 加载，最大 246 KiB；要从 `0xff0000` 复位运行，raw 文件必须把入口放到偏移 `0x2d800`，通常应生成完整、正确填充的 Flash 镜像；
- 对编译器回归优先使用 Intel HEX，避免把链接地址和 raw 文件偏移混为一谈。

### 5.3 内存和可观测输出

| 地址范围 | 大小 | QEMU 功能 |
|---|---:|---|
| `0x000000..0x003fff` | 16 KiB | edata，含寄存器 bank/栈存储 |
| `0x010000..0x02ffff` | 128 KiB | xdata |
| `0x030000..0x030fff` | 4 KiB | `RAMEXE=0` 时的可执行 RAM data alias |
| `0x7e0000..0x7effff` | 64 KiB | `P_SW2.EAXFR=1` 时的 XFR aperture |
| `0x7f0000..0x7fffff` | 64 KiB | `AUXR.EXTRAM=1` 时的外部数据 aperture；机器未接外设 |
| `0x800000..0x800fff` | 4 KiB | `RAMEXE=1` 时的可执行 RAM code alias |
| `0xfc2800..0xffffff` | 246 KiB | 只读用户 Flash |

直接地址 `0x80..0xff` 选择 SFR，间接 `@R0/@R1` 的同值地址选择 edata。UART1 的 `SBUF` 接第一路 QEMU serial chardev；现有 functional tests 正是用 UART 文本 `PASS`/`FAIL` 作为终止判据。没有发现为该目标定义的 semihosting 或 guest-exit ABI，因此 SDCC 端到端测试应复用 UART sentinel + host timeout。

中断向量为 `0xff0003` INT0、`0xff000b` Timer0、`0xff0013` INT1、`0xff001b` Timer1、`0xff0023` UART1。

### 5.4 Source reset 与 CPUMODE 的精确关系

QEMU 提交中的 [`target/mcs251/cpu.c`](https://github.com/zevorn/qemu/blob/213d2d68d7e326980a9c771f160d287488a96011/target/mcs251/cpu.c) 在 reset 时清零 CPU state，故 `AUXR2=0`；[`target/mcs251/helper.c`](https://github.com/zevorn/qemu/blob/213d2d68d7e326980a9c771f160d287488a96011/target/mcs251/helper.c) 用 `source_mode = !CPUMODE`，所以：

```text
reset: AUXR2.CPUMODE=0 -> Source
AUXR2.CPUMODE=1       -> Binary
A5                     -> 仅下一条切到相反 map；QEMU 对连续 A5 逐次翻转
```

STC32G144K246 手册的 AUXR2 表给出 B6 名称 `CPUMODE`、复位值 0，却没有发布 0/1 的含义。QEMU 源码和文档明确把上述极性标为 inference。结论是：

- 默认 Source 代码与 QEMU 复位状态兼容，不需要启动代码写 CPUMODE；
- Binary/动态切换可用于 QEMU 的 ISA 测试，但在声称兼容实机前需要 STC 补充资料或硅验证；
- 普通 STC32G（非 G144K246 子系列）只支持 Source，更不能把 QEMU 的 Binary 支持当作硬件保证。

### 5.5 QEMU 已有覆盖与边界

QEMU 目标文档声明实现完整 Binary/Source opcode map、8/16/32 位操作、24 位代码/数据地址、四 bank、扩展栈、双 DPTR、ESC 和 4 字节中断帧。相关测试：

- [`tests/qtest/stc32-test.c`](https://github.com/zevorn/qemu/blob/213d2d68d7e326980a9c771f160d287488a96011/tests/qtest/stc32-test.c)：复位寄存器、内存区、HEX loader、两套 opcode 反汇编、控制寄存器和外设；
- [`tests/functional/mcs251/test_isa.py`](https://github.com/zevorn/qemu/blob/213d2d68d7e326980a9c771f160d287488a96011/tests/functional/mcs251/test_isa.py)：手工构造 Source/Binary/ESC、classic/native ISA、HEX、TFPU、Timer IRQ/RETI 固件，并等待 UART PASS/FAIL。

构建完成后先运行 QEMU 自测，再运行编译器产物：

```sh
make check-qtest
make check-functional-mcs251
```

模型是功能级、非 cycle-exact；未建模 Flash 擦写/编程、DMA、USB、CAN-FD、ADC/DAC、PWM、I2S、额外 UART/Timer，以及 pipeline/cache/timing。QEMU 可验证功能和静态编码，不能作为优化周期数的测量器。

## 6. “完整 MCS251 后端”可核验验收清单

下面每项都要求自动化证据；只有“能编译 hello world”不算完成。

### A. 基线和工具链接线

- [ ] 固定并记录 SDCC 上游提交、QEMU 提交、手册日期和本文件中的资料 SHA。
- [ ] `sdcc -mmcs251 --version`、`sdcc -mmcs251 --help` 可用，目标预定义宏、默认 Source 模式和 ABI 名称有文档。
- [ ] 新 assembler、linker、crt 和运行库随安装产物部署；全新安装前缀下不依赖源码树文件。
- [ ] release/trunk 的 clean out-of-tree build 通过，现有所有 SDCC 目标的构建和回归不退化。

### B. 汇编器、链接器和完整 ISA

- [ ] 建立机器可读矩阵：65 个助记符家族 × 每个合法操作数形式 × 宽度 × 寻址 × Source/Binary map × 是否需要 A5；每一行记录手册页码、期望字节、长度和 flags。
- [ ] 对矩阵中每行做“汇编到固定黄金字节”测试；黄金值来自 Intel/STC 表，不由待测汇编器或 QEMU 自动反推。
- [ ] Source 是默认；显式 Binary/Source 模式、A5 classic/native escape、共享 Area I、reserved/NOP 行为各有测试。
- [ ] R/WR/DR 的合法编号和重叠编码正确；拒绝奇数 WR、非法 DR、保留位置 32..55、非法位宽/寻址组合。
- [ ] rel8 在 -128/+127 边界，addr11 跨 2 KiB，addr16 跨 64 KiB，addr24 跨 region，16/24 位大端重定位都有正负用例和越界诊断。
- [ ] assembler→object→link→HEX 的 section、symbol、relocation 和绝对地址由独立工具检查；反汇编 round-trip 作为补充而不是唯一 oracle。

### C. ABI 和跨单元互操作

- [ ] 先提交版本化 ABI 文档，明确类型大小/对齐/端序、内存地址空间、near/far 指针、参数、返回值、caller/callee-save、SPX 栈帧、struct/union/bit-field、varargs、函数指针、名字修饰和中断函数 ABI。
- [ ] 若宣称 Keil 调用约定兼容，逐条覆盖 Arm 文档的 R11/R0..R7 参数、WR/DR 参数、返回寄存器、R12..R15 保存、DR12 大 struct 返回、static/reentrant 参数规则。
- [ ] C→C 的调用者/被调者分别放在独立翻译单元；另做 C↔手写汇编互调，避免同一错误在一份编译单元内互相抵消。
- [ ] 递归、深调用、尾调用、函数指针、varargs、1..9 字节 struct、超过 8 字节 struct、bank 切换、near/far 跨 64 KiB 调用均运行验证。
- [ ] 对 R/WR/DR alias 做随机压力测试，证明寄存器分配器不会让相交 live range 同时占用同一底层字节。

### D. C 语义和代码生成

- [ ] 8/16/32 位有符号/无符号 move、cast、加减、逻辑、比较、移位、乘除模、溢出边界全部在 `-O0` 和优化级别下通过。
- [ ] 大端对象布局、非对齐 16/32 位 load/store、volatile、SFR、bit、指针算术和各 memory space 通过。
- [ ] if/switch/循环、短/长分支、near/far call/return、inline asm constraints 和 debug/unwind 所需信息通过。
- [ ] DR60 永不作为普通临时量；DPX/SPX、R10/B、R11/ACC 的专用和 alias 规则都被 allocator/peephole 尊重。
- [ ] `__interrupt` 序言/尾声保存 ABI 必需状态，生成 4 字节帧兼容 RETI；嵌套优先级和被中断函数寄存器活跃用例通过。

### E. crt、链接布局和 QEMU 端到端

- [ ] 链接脚本把 reset stub 放 `0xff0000`，中断向量放固定地址，代码/常量/edata/xdata/栈不越过 QEMU 的映射。
- [ ] crt 初始化 SPX、`.data`、`.bss`，再调用 `main`；main 返回行为有定义。
- [ ] 生成绝对 Intel HEX；另测试 246 KiB raw 镜像和 `0x2d800` reset offset。用脚本解析 HEX/raw 断言地址，不只看 QEMU 是否启动。
- [ ] 由 SDCC 编译的最小 C 程序通过 `qemu-system-mcs251 -M stc32g144k246-evb -bios ... -nographic` 输出 `PASS`；FAIL 和 timeout 均使测试失败。
- [ ] 端到端套件覆盖算术、指针、edata/xdata、全局初始化、跨单元调用、递归/varargs/struct、函数指针、Timer 中断/RETI 和 UART。
- [ ] 默认固件不写 CPUMODE，并在 reset Source 状态运行；Binary map 测试独立标注为 QEMU/通用 80MCS251 覆盖。
- [ ] 在跑 SDCC 产物前，固定提交上的 QEMU `check-qtest` 和 `check-functional-mcs251` 先通过，排除模拟器基线故障。

### F. 基本优化的可测门槛

- [ ] 寄存器分配能使用 R/WR/DR，并正确处理重叠、caller-save、callee-save 和 spill；有高寄存器压力黄金用例。
- [ ] 16/32 位表达式优先选合法 native 宽指令，而不是无条件拆成 8051 byte 序列；同时遵守某些逻辑/移位/乘除并不支持完整 DR 正交组合的手册限制。
- [ ] 实现并分别测试 constant folding、`MOVZ/MOVS` 扩展、`#0data16/#1data16`、`INC/DEC #1/#2/#4`、compare+branch、addressing-mode folding、冗余 move/spill 消除、branch shortening、基本 peephole。
- [ ] 成本模型计入 Source 下经典冲突指令的 A5 额外字节/周期，能在 16/24 位 call/jump 和 near/far load/store 之间合法选择。
- [ ] 冻结一组小型 C benchmark；`--opt-code-size` 在每个指定 kernel 上不得比 `-O0` 更大，且至少有一组展示上述每类优化的确定性汇编改进。
- [ ] `--opt-code-speed` 用手册静态周期表做成本比较；不得用非 cycle-exact QEMU wall-clock 宣称性能收益。
- [ ] 所有优化都先通过未优化语义测试，再跑差分/随机测试比较 `-O0` 与优化产物的 UART 结果。

### G. 发布质量

- [ ] host ASan/UBSan（适用处）、确定性构建、错误诊断、无效汇编输入和链接越界测试通过。
- [ ] 用户文档给出 QEMU 构建/运行、HEX/raw 布局、Source 模式、ABI、memory model、已知限制和可复制示例。
- [ ] “完整”报告附机器可读 ISA 覆盖率：合法矩阵总数、已通过数、跳过数及理由；不得只按 65 个助记符计数。

## 7. 已知缺口、风险和审批项

1. **必须先选 ABI**：Arm/Keil 规则已找到，但是否复制其 static overlay 和对象格式仍是设计选择。对 QEMU 运行而言 SDCC 自有 ABI 足够；对 Keil 汇编/库互操作则远远不够。
2. **STC 文档有歧义/笔误**：包括寄存器字段以 `j/2`、`k/4` 编码却在符号说明中表述不完整，部分位指令伪代码目标写错，个别 ERET 文字与 24 位伪代码不一致。需以 Intel 表、STC 逐形式表、QEMU 和最终硅测试交叉裁决，并把裁决写进测试。
3. **CPUMODE 极性未由 STC 发布**：QEMU 的 0=Source/1=Binary 是有意记录的推断。默认 Source 路径可靠；动态模式切换不能只凭 QEMU 宣称实机兼容。
4. **QEMU 基线不是 clean build**：本轮已有构建的 7 个 MCS-251 功能子测试通过，但它来自正在重构且非 clean 的工作树。方案批准后应在隔离构建目录验证固定提交或重构后的新固定点。
5. **QEMU 不是独立硬件真值**：其实现同样来自这些手册，且功能级而非 cycle-exact。机器码黄金值必须独立来自官方表；发布前最好增加至少一块 STC32G144K246 实机 smoke/差分套件。
6. **完整 ISA 不等于 STC 全 SoC**：DSP32/TFPU/MDU32、DMA 和大量外设不是 MCS251 核心 opcode。第一阶段应锁定 CPU、crt、内存和 UART/Timer 验收，避免把外围驱动扩张成后端阻塞项。
7. **上游策略待定**：若最终贡献到 SDCC/QEMU 上游，要重新确认许可证、生成文件策略和上游 port 命名；本研究没有替任何上游做接受性判断。

## 8. 一手来源索引

- [SDCC 官网](https://sdcc.sourceforge.net/)
- [SDCC 4.6.0 发布目录](https://sourceforge.net/projects/sdcc/files/sdcc/4.6.0/)
- [SDCC 官方 Git 镜像](https://sourceforge.net/p/sdcc/git-mirror/ref/trunk/)
- [SDCC 官方 SVN trunk](https://sourceforge.net/p/sdcc/code/HEAD/tree/trunk/sdcc/)
- [SDCC Adding a port](https://sourceforge.net/p/sdcc/wiki/Adding%20a%20port/)
- [STC32/车规系列资料页](https://www.stcai.com/cp_stc32xl)
- [STC 传统 MCU 手册页](https://www.stcai.com/cp_ctmcusc)
- [Arm/Keil MCS251 v5.60 文档条目](https://documentation-service.arm.com/documentation/101647/latest)
- [本地 QEMU 固定提交](https://github.com/zevorn/qemu/tree/213d2d68d7e326980a9c771f160d287488a96011)
