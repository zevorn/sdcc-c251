# MCS251 / STC32G 官方示例取证与 clean-room 测试转化计划

> 调查日期：2026-08-01（Asia/Shanghai）
> 范围：只把 STC、Arm/Keil 第一方发布的 MCS251 / STC32G 示例、应用笔记和下载物作为外部语义来源，并以本地 QEMU `mcs251` / `stc32g144k246-evb` 的已实现能力划定可运行测试边界。本文件不把第三方或供应商代码直接纳入 SDCC 测试。

## 1. 结论先行

1. STC 官方目前有两条互补的示例来源：
   - [STC32G 示例程序总入口](https://www.stcmicro.com/cn/slcx.html)及其[下载目录](https://www.stcmicro.com/rar/demo/)提供可直接下载、可做 SHA-256 固定的 STC32G12K128 通用示例、函数库、ISP 和 FreeRTOS 包。
   - [STC32G144K246 官方知识库分类](https://www.stcaimcu.com/forum-108-5.html)现列出 88 个与目标芯片精确匹配的独立主题。它比旧总包更适合作为 STC32G144K246 行为语义来源，但论坛附件目前没有稳定、无需会话的直接下载 URL。
2. STC32G 示例以 Keil MCS251 方言和工程为主；Arm/Keil 示例也只说明 Keil 工具链行为。二者都不能视作“可直接交给 SDCC 编译”的测试集。建议把官方示例当作外部行为规范，独立重写小型 SDCC 回归测试。
3. 除逐文件明确带有 MIT 等兼容许可证的内容外，调查到的 STC 总包没有顶层 `LICENSE` / `COPYING` / `NOTICE`，若干源文件还要求使用时注明 STC 资料来源。Arm 网站条款也不提供把 Keil 下载代码重新发布进 SDCC 的普遍授权。因此默认策略应是：**不 vendor、不复制实现和注释，只记录来源、摘要、哈希，并 clean-room 重写测试。**
4. 可复现性应以“文件名中的版本/日期 + 精确字节数 + SHA-256”为准。STC 的无日期别名会更新；HTTP `Last-Modified` 多为 2025 年镜像迁移时间，不能当成原始发布日期。
5. 按 QEMU 当前实现，首批可形成端到端运行门槛的是：K246 SRAM、UART1、Timer0/1、GPIO/INT0/INT1，以及 TFPU/DSP32 的结果向量；其余定时器、UART2+、USB、CAN-FD、ADC/DAC、PWM、I2S、DMA、Flash 编程和电源模式应明确延后。官方示例只提供语义，不足以代替“所有 MCS-251 指令 × 寻址方式”的独立覆盖矩阵。

## 2. 取证与保留规则

- 正式仓库只保存：官方页面链接、直接下载 URL、观察到的版本/日期、字节数、SHA-256、包内相对路径和独立写出的测试。
- 原始 ZIP 仅放在本地取证缓存或 CI 的临时目录，不提交到 SDCC 仓库。
- 对论坛帖子只记录帖子永久链接、附件显示名、页面日期和功能描述；未能匿名稳定下载时，不伪造直接 URL 或哈希。
- 每个拟转化测试先写一段“可观察行为”，再由未复制原实现的代码实现测试。避免保留原变量名、控制流、注释、字符串和工程文件。
- 若确需直接复用某个文件，必须逐文件核验许可证和作者；包中一个 MIT 文件不使整个 ZIP 变成 MIT。

## 3. STC 官方可下载包快照

官方入口为[示例程序页](https://www.stcmicro.com/cn/slcx.html)、[英文镜像页](https://www.stcmicro.com/slcx.html)和[目录索引](https://www.stcmicro.com/rar/demo/)。下列摘要均为本次实际下载后计算所得。

| 发布物 | 版本/日期依据 | 官方直接下载 | 字节数 | SHA-256 |
|---|---|---|---:|---|
| STC32G-DEMO-CODE-V9.6 固定日期包 | [官方论坛总包帖](https://www.stcaimcu.com/forum.php?mod=viewthread&tid=1525)显示 2024-04-18 上传、25.61 MB | [STC32G-DEMO-CODE-V9.6-20240418.zip](https://www.stcmicro.com/rar/demo/STC32G-DEMO-CODE-V9.6-20240418.zip) | 26,852,479 | `0c4c2ca809a4919e9cd4fd4d988871e6ef03ae17cdc189c05967ed78e51394d2` |
| STC32G-DEMO-CODE-V9.6 当前别名 | 包内更新记录最新为 2024-10-10；HTTP 修改时间为 2025-10-15，不作为发布日期 | [STC32G-DEMO-CODE-V9.6.zip](https://www.stcmicro.com/rar/demo/STC32G-DEMO-CODE-V9.6.zip) | 27,198,083 | `b7ecbf7d51c7f8a69cb07e3e575a6e4f80f6d2da0fc48a49316c0515c92ad29b` |
| STC32G-SOFTWARE-LIB 固定日期包 | 官方论坛总包帖显示 2024-09-10 上传、14.84 MB | [STC32G-SOFTWARE-LIB-20240910.zip](https://www.stcmicro.com/rar/demo/STC32G-SOFTWARE-LIB-20240910.zip) | 15,558,275 | `5e72080ed16d04098f8d6dd374d47026bdf04142d88803e483f1b2ef34cdcd87` |
| STC32G-SOFTWARE-LIB 当前别名 | 包内更新记录含 2024-12-23、2025-02-05、2025-03-18；HTTP 修改时间为 2025-10-15 | [STC32G-SOFTWARE-LIB.zip](https://www.stcmicro.com/rar/demo/STC32G-SOFTWARE-LIB.zip) | 15,537,137 | `7737354f2ff6734c9098bea5719594a126eef917213e6df901774d4abb010202` |
| 官方 UART ISP bootloader 示例 | 文件名标识 STC32G12K128；目录镜像时间不是原始发布日期 | [ZIP](https://www.stcmicro.com/rar/demo/STC-official-user-UART-ISP-bootloader-demo-STC32G12K128-series.zip) | 733,641 | `b8e11abe4898d3a56b031d9835ff58ee9996d357817faede92cf661d26ceacdb` |
| 官方 CAN ISP bootloader 示例 | 同上 | [ZIP](https://www.stcmicro.com/rar/demo/STC-official-user-CAN-ISP-bootloader-demo-STC32G12K128-series.zip) | 2,274,929 | `e22a9c0156fe958e5a0e47d328b6aa694cab62a76f87f50aab0f1ea1970e6398` |
| 官方 USB ISP bootloader 示例 | 同上 | [ZIP](https://www.stcmicro.com/rar/demo/STC-official-user-USB-ISP-bootloader-demo-STC32G12K128-series.zip) | 1,079,040 | `23678938e22793841802276294fe70fb3097b2a9a1ad8f33acaae834b24db377` |
| FreeRTOS STC32G core demo | 文件名和[官方论坛说明](https://www.stcaimcu.com/forum.php?mod=viewthread&tid=60)标识 v1.0.2、2022-06-09 | [FreeRTOS-STC32G-CORE-V1.0.2-DemoCode-20220609.zip](https://www.stcmicro.com/rar/demo/FreeRTOS-STC32G-CORE-V1.0.2-DemoCode-20220609.zip) | 1,316,543 | `a2bf7e35a9897998a943be343e83b48d465795b1e41c623be3651bd532ffcde7` |

### 3.1 固定日期包与当前别名的差异

- V9.6 当前别名不是 2024-04-18 文件的稳定镜像：当前包的更新记录新增到 2024-10-10，红外、PWM、UART 和部分 PDF 等文件有变化。
- SOFTWARE-LIB 当前别名也不是 2024-09-10 文件的稳定镜像：它增加了 2024-12-23 至 2025-03-18 的更新，并有目录改名。
- 因此长期基线优先用固定日期包；若选择当前别名，必须把上表 SHA-256 当成“2026-08-01 观察快照”，下载后重新核对。

## 4. STC 总包中的测试语义候选

下表路径以压缩包内中文原名表达。ZIP 使用旧式中文文件名编码时，解压工具可能显示乱码；这不改变相对层级和 `main.c` 内容。

| 包内路径 | 官方示例覆盖 | 建议的 SDCC clean-room 转化 |
|---|---|---|
| `STC32G-DEMO-CODE-V9.6/01.1-用P6口做跑马灯-入门版/C语言/main.c` | SFR、位寻址、循环、延时和函数调用 | 写最小 SFR/位操作测试；把硬件灯效改成内存签名或宿主可观察结果，不复制延时循环 |
| `.../02-Timer0-Timer1-Timer2-Timer3-Timer4测试程序/C语言/main.c` | MCS251 中断函数、重装值表达式、多个向量 | 每个已支持定时器独立成例；验证向量、保存/恢复、`RETI`/扩展返回语义，未建模外设不进入首批运行门槛 |
| `.../06-外中断INT0-INT1-INT2-INT3-INT4测试/C语言/main.c` | `code` 数组、位操作、外部中断处理器 | 拆成 `const`/代码空间寻址测试与中断 ABI 测试，避免把板级按键行为带入编译器回归 |
| `.../10-串口1中断模式与电脑收发测试/C语言/main.c` | UART1 字符串发送、128 字节环形缓冲、收发 ISR | 独立重写 UART1 echo；覆盖 `SCON`、`SBUF`、`RI`、`TI`、ISR 与缓冲区索引 |
| `.../35-板上的32K xdata测试程序/main.c` | `far` 指针、绝对地址、32 KiB 多模式读写、`code` 表 | 用目标机实际可用的高地址内存范围重写数据模式测试；UART 仅作结果通道，不沿用板卡地址假设 |
| `.../46-MDU32乘法和除法单元/sample.c` | 有/无符号 32 位乘除、`near` 全局量 | 纯 C 重写边界向量，让 SDCC 自己生成指令或运行库调用；不得链接包内专有 `STC32_MDU32_*.LIB` |
| `.../75-通过定时器周期性调度任务综合例程/Sources/src/*.c`、`Sources/isr/*.c` | 多文件调用、调度控制流、中断声明 | 只提取多翻译单元、函数指针/状态机和 ISR ABI；板级传感器与显示逻辑不作为编译器测试 |
| `STC32G-SOFTWARE-LIB/库文件/STC32G.H` 与 `STC32G_*.c/.h` | STC32G SFR 和库 API 覆盖面 | 当作寄存器/API 清单交叉检查，不直接导入 SDCC 头文件；另写最小、可审计的设备定义 |
| `STC32G-SOFTWARE-LIB/独立程序/01-IO-跑马灯/main.c` 至 `30-*`、`A0-*` 至 `A2-*` | GPIO、定时器、外中断、UART 等独立程序 | 用“每个行为一个测试”拆分，优先纯 CPU、内存、UART 和已建模中断，其他只保留编译测试 |

### 4.1 ISP 与 FreeRTOS 包中的 ABI 候选

- UART ISP 包的关键路径为 `demo/src/Demo.c`、`isp/src/main.c`、`dfu.c`、`iap.c`、`uart.c`、`isr.asm`；CAN/USB 包结构类似，另有 `can.c` 或 USB 协议栈。它们覆盖绝对地址对象、程序放置、重定位、IAP 与 C/汇编边界。建议仅把“绝对地址声明能否正确链接”“远调用/中断入口能否正确返回”转化为小测试；不运行随包 Windows 程序，也不把目标 Flash 编程当作编译器首批验收。
- FreeRTOS 包的关键路径为 `Sources/FreeRTOS/portable/STC32G12K128/port.c`、`portasm.h`、`portmacro.h` 和 `Sources/User/main.c`。端口代码覆盖 DR 寄存器、扩展栈指针、`PUSH`/`POP`、四字节中断帧和任务上下文切换，是 MCS251 ABI 的强语义来源。
- `portasm.h` 等 FreeRTOS 核心/端口文件逐文件带 `SPDX-License-Identifier: MIT` 和 MIT 正文；同包用户/STC 包装文件仍需单独审计。即使某文件允许复用，首选先把栈帧事实写成独立 SDCC 汇编/运行测试，而不是把 Keil 内联汇编直接搬来。

## 5. STC32G144K246 精确匹配示例

[STC32G144K246 产品页](https://www.stcmicro.com/stc/stc32g144k246.html)把“函数库”“示例代码”和 V9.6 开发板包链接到通用或 STC32G12K128 时代的 ZIP，没有找到一个可直接下载、整包命名为 STC32G144K246 的归档。与目标型号精确匹配的第一方材料目前集中在[官方知识库分类](https://www.stcaimcu.com/forum-108-5.html)：页面列出 88 个单项主题，包括端口模式、跑马灯、定时器、外部计数、脉宽测量、外部中断等。

| 官方主题 | 页面日期/附件信息 | 可提取语义 | 取证状态 |
|---|---|---|---|
| [02-定时器测试](https://www.stcaimcu.com/thread-24457-1-1.html) | 知识库单项主题；描述 13 个定时器 | 独立重写 Timer0/Timer1 的 16 位自动重装、IRQ 和端口/内存签名翻转；其余定时器留待硬件模型具备后 | 页面可访问；附件无稳定匿名直链/哈希 |
| [10-串口1中断收发](https://www.stcaimcu.com/thread-24465-1-1.html) | 知识库单项主题 | 重写中断接收后原样返回，保留 UART1 状态位和 IRQ；把 PLL、引脚复用、板级波特率初始化从编译器测试中剥离 | 同上 |
| [38-TFPU](https://www.stcaimcu.com/forum.php?mod=viewthread&tid=24691) | 2026-07-02；页面显示附件 `38-TFPU–三角函数和单精度浮点运算单元.zip`，14.53 KB | 从文档化输入/输出重写 TFPU 指令与状态边界向量；不得从串口打印“看起来正确”推断全部状态位正确 | 同上 |
| [39-DSP32](https://www.stcaimcu.com/forum.php?mod=viewthread&tid=24692) | 2026-07-02；页面显示附件 `39-DSP32–DSP指令-32位64位整数运算器.zip`，12.88 KB | 重写 32/64 位整数、饱和/符号/边界值向量；既测指令选择，也测 ABI 结果保存 | 同上 |
| [85-内部 148 KiB SRAM 读写测试](https://www.stcaimcu.com/thread-24754-1-1.html) | 2026-07-03；页面含代码和汇编附件 | 最直接的高地址数据空间语义：`edata` 0..`0x3fff`、`xdata` `0x10000`..`0x2ffff`、`0x30000`..`0x30fff`；重写成固定模式、边界、跨 64 KiB 和别名检查 | 页面正文可核验；附件无稳定匿名直链/哈希 |

这些帖子的代码头并不一致：若干页面要求使用时注明来自 STC 的资料/程序，有的页面未出现相同文字。它们都没有因此自动成为标准开源许可证。完成逐文件审计前，只从功能描述和硬件地址事实 clean-room 重写，并在测试文档中保留原帖链接。

## 6. Arm / Keil 第一方示例与应用笔记

[Keil MCS251 下载列表](https://www.keil.com/download/list/c251.htm)是官方索引，列出 AN111、AN112、AN114、AN116、AN117、AN118 支持文件，以及 Atmel ADC/I²C、MCB251SB Blinky/诊断/串口/RAM、ROM checksum、Split-ROM 等例程。[MCS251 产品页](https://www.keil.com/products/c251/)说明其目标工具链；[µVision 目录说明](https://www.keil.com/support/man/docs/uv4cl/uv4cl_ov_folderstructure.htm)记录完整安装中的示例位置为 `C:\Keil_v5\MCS251\Examples`。

[Keil 产品下载页](https://www.keil.com/download/product/)在调查时把 MCS251 5.60（2018-05）列为当前版本。[MCS251 评估版表单](https://www.keil.com/demo/eval/c251.htm)公开安装器 MD5 `4E8EFCD4A00FD88081CFE91F9EA44C77`，但必须提交姓名、电子邮件、公司等联系信息才返回安装器；本次没有代用户提交信息，因此没有 installer 的直接 URL、字节数或 SHA-256。[评估版限制页](https://www2.keil.com/limits)说明 MCS251 编译、汇编、链接和调试限于 2 KiB。

### 6.1 已校验的公开支持包

这些 ZIP 均来自官方 MCS251 下载索引，且本次已实际下载并计算哈希。日期来自各官方详情页，而非 ZIP 服务器的镜像时间。

| 发布物 | 官方详情日期 | 官方直接下载 | 字节数 | SHA-256 | 包内关键文件 / 语义 |
|---|---|---|---:|---|---|
| AN114 支持文件 | [1999-08-10](https://www.keil.com/download/docs/36.asp) | [apntex_114.zip](https://www.keil.com/download/files/apntex_114.zip) | 67,708 | `f3953893b9cba59a304bb0f81a96455d1e93891453b21f90740126ff21703beb` | `Start251.a51`：EDATA/XDATA/HDATA 清零、SP/可重入栈、ROMHUGE、配置字和四字节中断帧 |
| AN116 支持文件 | [1999-08-11](https://www.keil.com/download/docs/37.asp) | [apntex_116.zip](https://www.keil.com/download/files/apntex_116.zip) | 32,131 | `877396c0a0fa07fe1677aae9d6f91b29d9f4231568fa7be84da5dacb8fcf9dc4` | `Main.c`、`Bank1.c`、`Func1b1.c`、`Hugecode.prj`：同段/跨段调用、USERCLASS 分段与 Keil `-O6` 示例 |
| AN117 支持文件 | [1999-08-11](https://www.keil.com/download/docs/38.asp) | [apntex_117.zip](https://www.keil.com/download/files/apntex_117.zip) | 24,581 | `af43b4cbd2afe0bbeb7c1465a801717cb02491e34c28c8d4338a5a25c272cbfd` | `Old51.c`、`New251.c`、`Conv51.h`、`Mspace.prj`：8051 到 251 地址空间迁移与代码大小示例 |
| MCB251SB Blinky | [1999-08-11](https://www.keil.com/download/docs/40.asp) | [blink251.zip](https://www.keil.com/download/files/blink251.zip) | 9,542 | `d51f6fe8b092171fccad81ef4c542e56e4aa76a3a0fafc59274bd8ffafb19996` | `FLASH.C`、`SOURCE.BAT`、`BINARY.BAT`、`FLASH_S.HEX`、`FLASH_B.HEX`：Source/Binary 模式、移位和循环 |
| MCB251SB Diagnostics | [1999-08-11](https://www.keil.com/download/docs/42.asp) | [diag251.zip](https://www.keil.com/download/files/diag251.zip) | 30,298 | `90bf0e3116bdddd6fae3d544eca5057d11a707e35134748a32e23542f4451751` | `MAIN.C`、`RAMTEST.C`、`CHECKSUM.C`、`INTSIO.C`、`EXTSIO.C`、`LOOPBACK.C`、`PBTEST.C`、`MCB251SB.H`：far 指针、RAM 模式、ROM checksum、IRQ/UART |
| MCB251SB RAM Test | [1999-08-11](https://www.keil.com/download/docs/46.asp) | [ramtest.zip](https://www.keil.com/download/files/ramtest.zip) | 11,126 | `4ef502608c9d4bead101771dd88eb000c747f3c9058cac3932d79b1e79ac39e1` | `RAMTEST.C` 及 HEX/LST/MAP：far 指针自增、32 位 sentinel |
| ROM Checksum Test | [1999-08-11](https://www.keil.com/download/docs/47.asp) | [romtest.zip](https://www.keil.com/download/files/romtest.zip) | 6,786 | `bdaf63207325e661a6acdda15a8adb841732ab89402e8b747d7551a97c03c719` | `romtest.c` 及 HEX/BIN/LST/MAP：24 位 ROM 指针、checksum |

公开索引中的其他下载也已取证，但与 STC32G 目标外设的匹配度较低：

| 发布物 | 官方直接下载 | 字节数 | SHA-256 | 备注 |
|---|---|---:|---|---|
| AN111 | [apntex_111.zip](https://www.keil.com/download/files/apntex_111.zip) | 1,159,192 | `fd6599e20331bf0b1c3cfa53be0479bd2427ca35c2177e4b256047cc8c3263f0` | Intel 8x930 支持 |
| AN112 | [apntex_112.zip](https://www.keil.com/download/files/apntex_112.zip) | 69,828 | `e2482a45bf7ff5b018341307b58940d45a8522a420a8443f52ff86141ae31442` | Intel 8x930 用户程序 |
| AN118 | [apntex_118.zip](https://www.keil.com/download/files/apntex_118.zip) | 435,469 | `1de2ad561dfacd0557cf08f9b5c85ee780f5a2706c669c4b4755249c4dc60c25` | `timer1.asm`、`Neilc.c`、`Example.c` 等 8x930 板级内容 |
| Atmel MCS251A1 ADC | [t8xmcs251a1_adc.zip](https://www.keil.com/download/files/t8xmcs251a1_adc.zip) | 8,136 | `8697e5ee5c990ee2320200a6c8f6873ddf29c5b6092119da88e6f23ad9a8cd63` | 2004-07-17；`ADC.C`、`START251.A51` |
| Atmel MCS251G1D I²C | [mcs251g1d_i2c.zip](https://www.keil.com/download/files/mcs251g1d_i2c.zip) | 42,436 | `554997d97c5c567a14361a258e82a190207175aa5ae9402d2b85ebb0ce0ef30e` | 2003-03-20；`T8xMCS251G1D_I2C_Example.c`、`PUTCHAR.C`、`START251.A51`；文件明确保留全部权利 |
| External Serial I/O | [extsio.zip](https://www.keil.com/download/files/extsio.zip) | 15,992 | `537c1c060f151dfe48c7a49806f67e8d9755a201166e67bf6bb309f01b5c84d4` | MCB251SB 板级串口测试 |

`limits.h`、MON251 和 PC-Lint 辅助文件也在公开目录，但不是 MCS251 语义测试的优先材料。官方 [Split-ROM 详情页](https://www.keil.com/download/docs/32.asp)明确写明 `251JTAB.ZIP` 当前不可用，因此没有可核验 URL 或哈希。

### 6.2 官方应用笔记

目前最适合在不复制 Keil 源码的前提下转化的两份官方应用笔记是：

| 官方资料 | 一手材料中的关键事实 | SDCC clean-room 候选 |
|---|---|---|
| [AN116: Writing Optimum Code for the MCS 251 Architecture](https://www.keil.com/appnotes/files/apnt_116.pdf) | PDF 为 46,109 字节，SHA-256 `256c14e163ba34ec18332aa34c2213386062c0e72b73c7dd62bd5a09f08c70e0`；HUGE ROM 模式下跨段公共函数使用 `ECALL`/`ERET`，静态、`near` 或同一 64 KiB 段调用可用 `LCALL`/`RET`；另含 UART/`printf` 示例 | 独立写同段、跨 64 KiB 段、`near` 与公开函数矩阵；检查链接器重定位和返回序列，并以运行结果验证，不照搬示例源码 |
| [AN117: Porting 8051 Code to the MCS 251 Architecture](https://www.keil.com/appnotes/files/apnt_117.pdf) | PDF 为 26,444 字节，SHA-256 `c198a776f5e4d9f754da863beb43cf4edd654f766c2a73005f35ee04b7986d81`；说明 C51 的 `data`/`idata`/`xdata`/`pdata`/`code` 到 MCS251 `near`/`const` 等模型的迁移，示例含变量、指针和循环 | 独立写各地址空间对象与指针的读写/转换/边界测试；笔记中的 Keil 字节数只作为历史观察，不设为 SDCC 的固定大小阈值 |

[AN109](https://www.keil.com/appnotes/files/apnt_109.pdf)（120,137 字节，SHA-256 `47993f000e6afdfc81a980e21a41d1847f18008521104cae5420f4420fc57bb7`）还直接讨论启动清零、Source/Binary 模式和二/四字节中断帧，可与 AN114 交叉验证启动及 ABI 需求。[MCS251 User's Guide](https://www.keil.com/support/man/docs/c251/default.htm)和 [A251 User's Guide](https://www.keil.com/support/man/docs/a251/default.htm)则适合查编译器地址空间与汇编语义，但不能作为 SDCC 必须复制 Keil 扩展语法的理由。

其他官方索引项可按以下语义转化；即使已经取得原下载物，未经兼容许可也不应复制其中代码：

- AN114 `START251.A51`：启动入口、栈、段初始化和 C 运行时交接测试。
- MCB251SB RAM Test：近/远数据区的模式写入、地址线和边界测试。
- ROM Checksum / Split-ROM：多段链接布局、跨段调用、重定位和校验区排除规则。
- MCB251SB Blinky / Serial I/O：最小启动、SFR、循环和 UART 输出；板卡时钟/引脚细节与编译器语义分离。
- Atmel ADC/I²C 与 Diagnostics：主要留作编译覆盖或设备头兼容性素材，不应在缺少相应外设模型时成为运行门槛。

## 7. 许可证、版权与再分发边界

### 7.1 STC

- 本次检查的通用示例、函数库和 bootloader ZIP 未找到包级顶层 `LICENSE`、`COPYING` 或 `NOTICE`。
- 多个 STC 源文件只有版权/身份说明和使用时注明来源的要求；这不足以推导出允许修改并再分发到 SDCC 仓库的标准开源授权。
- 包内混有第三方内容。例如 V9.6 的某些显示相关文件逐文件标注 Creative Lau 2020 MIT；FreeRTOS 端口中的若干文件也明确为 MIT。只能在保留对应许可证文本并完成逐文件审计后复用那些具体文件。
- 默认边界：不提交 ZIP、Keil 工程、预编译 `.LIB`/HEX、供应商源文件或原注释；只提交独立实现的测试、测试语义摘要和官方出处。

### 7.2 Arm / Keil

- [Arm 网站条款](https://www.arm.com/company/policies/terms-and-conditions#our-content)说明：下载内容受随附许可证约束；Keil 网站内容的下载许可限于非商业个人用途，并限制修改数字副本、再发布、再传输、复制或其他使用。
- 因此“从 Keil 官方页面可以公开下载”不等于“可以复制进 SDCC”。除非某个下载物附带明确、兼容的单独许可证，否则只以应用笔记陈述的 ABI/地址模型事实和例程的可观察行为为依据独立重写。
- 在代码评审中为每个 clean-room 测试保留来源链接和一句语义来源说明，但不复制受限文本。这一节是保守的工程取证规则，不是法律意见。

## 8. 当前 QEMU `mcs251` / STC32G144K246 能力基线

本节固定在本地 QEMU 分支提交 [`b80cfcb4b6eca7f900c35d4a268fcf62a92bb9c5`](https://github.com/zevorn/qemu/commit/b80cfcb4b6eca7f900c35d4a268fcf62a92bb9c5)。QEMU 文档明确区分 `qemu-system-mcs51` 与 `qemu-system-mcs251`；本计划只能使用后者的 `mcs251-cpu` 和 `stc32g144k246-evb` 机器。以下是“能否在当前模拟器上形成运行测试”的实现事实，不是另一份芯片规范。

### 8.1 固件入口、CPU 和内存

- Intel HEX 由 `-bios` 按 **24 位绝对地址**加载；记录必须落在 `0xfc2800`..`0xffffff`，复位入口固定为 `0xff0000`。原始 BIN 从 `0xfc2800` 开始，复位代码在文件偏移 `0x2d800`。因此 Keil 老开发板示例附带的 16 位 HEX/BIN 不能原样拿来运行。见 [QEMU STC32G 固件装载文档](https://github.com/zevorn/qemu/blob/b80cfcb4b6eca7f900c35d4a268fcf62a92bb9c5/docs/system/target-mcs51.rst#L151-L180)。
- CPU 模型声明覆盖完整的已文档化 MCS-251 Binary/Source opcode map、`ESC`、8/16/32 位操作、24 位代码/数据地址、四个寄存器 bank、扩展栈、双 DPTR/DPS 和四级中断。复位默认 Source mode；`AUXR2.CPUMODE` 极性是 QEMU 根据复位描述作出的推断，不能反过来作为芯片真值。见 [CPU 实现范围](https://github.com/zevorn/qemu/blob/b80cfcb4b6eca7f900c35d4a268fcf62a92bb9c5/docs/system/target-mcs51.rst#L181-L204)。
- 用户指定的 [STC32/车规系列官方资料页](https://www.stcai.com/cp_stc32xl)当前同时发布 [STC32G144K246 合订手册](https://www.stcaimcu.com/data/download/Datasheet/STC32G144K246.pdf)和五个分卷：[1](https://www.stcaimcu.com/data/download/Datasheet/STC32G144K246/STC32G144K246-1.pdf)、[2](https://www.stcaimcu.com/data/download/Datasheet/STC32G144K246/STC32G144K246-2.pdf)、[3](https://www.stcaimcu.com/data/download/Datasheet/STC32G144K246/STC32G144K246-3.pdf)、[4](https://www.stcaimcu.com/data/download/Datasheet/STC32G144K246/STC32G144K246-4.pdf)、[5](https://www.stcaimcu.com/data/download/Datasheet/STC32G144K246/STC32G144K246-5.pdf)。指令/寄存器 expected values 应从这些一手手册固定版本后提取；示例包只补充使用场景。
- 官方示例并不会穷举全部 opcode/操作数组合。“实现所有 MCS251 指令”的验收必须另建 `opcode × Source/Binary/ESC × 寻址方式 × 边界值` 矩阵；本文件中的示例转化只提供真实程序级的补充覆盖，不能替代该矩阵。

| 地址范围 | 当前 QEMU 行为 | 可用测试语义 |
|---|---|---|
| `0x000000`..`0x003fff` | 16 KiB `edata`，含寄存器 bank 和栈 | 低地址对象、栈保护、边界模式 |
| `0x010000`..`0x02ffff` | 128 KiB 内部 `xdata` | 24 位指针、`0x01ffff`→`0x020000` 跨界 |
| `0x030000`..`0x030fff` | 4 KiB executable-RAM 数据视图（`CKCON.RAMEXE=0`） | K246 148 KiB SRAM 示例的最后一段 |
| `0x800000`..`0x800fff` | 与上一段同一 backing store 的代码视图（`CKCON.RAMEXE=1`） | 后续 RAM 执行/别名测试；首批不必启用 |
| `0x7e0000`..`0x7effff` | XFR aperture（需 `P_SW2.EAXFR=1`） | Timer 预分频和 TFPU 时钟寄存器 |
| `0x7f0000`..`0x7fffff` | 外部数据 aperture，但机器未挂设备 | 只能测空 aperture 行为，不映射板级 RAM |
| `0xfc2800`..`0xffffff` | 246 KiB 只读用户 Flash | 链接布局、常量、跨 64 KiB 调用 |

内存范围及 alias 条件来自 [QEMU 机器内存图](https://github.com/zevorn/qemu/blob/b80cfcb4b6eca7f900c35d4a268fcf62a92bb9c5/docs/system/target-mcs51.rst#L206-L246)。

### 8.2 已建模外设、寄存器和可观察行为

| 模块 | 当前寄存器/连线 | 已建模行为 | 官方示例转化边界 |
|---|---|---|---|
| Timer0/1 | `TCON=0x88`、`TMOD=0x89`、`TL0/1=0x8a/0x8b`、`TH0/1=0x8c/0x8d`、`AUXR=0x8e`、`INTCLKO=0x8f`；`TM0PS=0x7efea0`、`TM1PS=0x7efea1` | mode 0/1/2、Timer0 mode 3、内部时钟、P3.4/P3.5 下降沿计数、P3.2/P3.3 gate、x1/x12、预分频、overflow 和 IRQ；默认输入时钟 24 MHz | 只抽取 K246 例程中的 T0/T1。不要把 Timer2..12、48 MHz 板级常量或周期精度纳入通过条件 |
| UART1 | `SCON=0x98`、收发两侧 `SBUF=0x99`，接第一条 QEMU serial chardev；向量 `0xff0023` | `REN`、`RI`、`TI` 和 UART IRQ；发送在 byte chardev 边界立即完成；接收在 `RI` 未清时背压 | 可做中断 echo 和环形缓冲；剥离 Timer2 波特率源、PLL、`P_SW1`、引脚上拉和九位/bit timing 假设 |
| GPIO | P0 `80/93/94`，P1 `90/91/92`，P2 `a0/95/96`，P3 `b0/b1/b2`，P4 `c0/b3/b4`，P5 `c8/c9/ca`，P6 `e8/cb/cc`，P7 `f8/e1/e2`；每组三个地址为 `Pn/PnM1/PnM0` | 准双向、推挽、高阻输入、开漏，区分 pin read 与 read-modify-write latch read；P3.2/P3.3→INT0/1，P3.4/P3.5→T0/T1 counter | 跑马灯可转为端口 mode/latch 序列；要由 qtest 的 `gpio-in`/`gpio-out` 驱动和观察，固件自读不能证明物理输出 |
| INT0/INT1 | P3.2/P3.3；向量 `0xff0003` / `0xff0013` | 边沿/电平选择、pending、优先级和 `RETI` 路径 | K246 INT0..4 只转 INT0/1；INT2..4 延后 |
| DSP32 | `DPUOP=0xd8`、`DPUST=0x86` | 已文档化命令集：转换、正规化、交换、算术、乘除、逻辑/移位、复合和 MAC；同步完成 | 可从例 39 重写结果/状态向量；不测耗时，不链接 STC `.LIB` |
| TFPU | `DMAIR=0xed`，`TFPU_CLKDIV=0x7efe93` | 已文档化命令；只有 CPU 执行 `MOV DMAIR,#immediate` 才启动；binary32 基本运算/转换走 softfloat，另有分类、比较、状态和三角函数 | 可从例 38 重写位模式/容差测试；延迟不作门槛，未公开而由 QEMU 推断的 status/control 位不可当硬件真值 |

Timer、UART、GPIO、DSP32 和 TFPU 的公开实现摘要见 [QEMU 已建模外设](https://github.com/zevorn/qemu/blob/b80cfcb4b6eca7f900c35d4a268fcf62a92bb9c5/docs/system/target-mcs51.rst#L248-L285)。当前可运行的中断向量还包括 Timer0 `0xff000b` 和 Timer1 `0xff001b`，见 [向量表](https://github.com/zevorn/qemu/blob/b80cfcb4b6eca7f900c35d4a268fcf62a92bb9c5/docs/system/target-mcs51.rst#L287-L304)。

### 8.3 明确不能由当前 QEMU 验证的行为

当前模型是 functional、非 cycle-exact：不建模 cache/pipeline timing、中断的一条指令延迟、UART bit timing/第九位、clock output、电气 GPIO 或 power mode。Flash 擦写/编程、DMA、USB、CAN-FD、ADC/DAC、PWM、I2S、附加定时器/UART 等均未实现；DSP32/TFPU 没有命令延迟，TFPU 三角函数使用 host 单精度数学库。见 [QEMU 限制](https://github.com/zevorn/qemu/blob/b80cfcb4b6eca7f900c35d4a268fcf62a92bb9c5/docs/system/target-mcs51.rst#L306-L325)。任何命中这些边界的官方示例只能做编译测试，或只抽取其中已建模的子语义，不能宣称“官方例程已在 QEMU 通过”。

现有 QEMU qtest 已覆盖 memory/HEX、DSP32、TFPU 寄存器、GPIO modes/外中断、Timer modes/gate/counter/prescaler 和 UART1 收发，见 [`tests/qtest/stc32-test.c`](https://github.com/zevorn/qemu/blob/b80cfcb4b6eca7f900c35d4a268fcf62a92bb9c5/tests/qtest/stc32-test.c#L1848-L1900)；functional test 已覆盖 Source/Binary/ESC、经典/原生 ISA、TFPU、Timer IRQ/`RETI`，见 [`tests/functional/mcs251/test_isa.py`](https://github.com/zevorn/qemu/blob/b80cfcb4b6eca7f900c35d4a268fcf62a92bb9c5/tests/functional/mcs251/test_isa.py#L1472-L1556)。这些是“运行基础设施存在”的证据；期望值仍应从 Intel/STC 文档独立推导，不能用 QEMU 自身测试反向证明硬件语义。

## 9. “官方示例语义 → SDCC 风格测试”映射

### 9.1 优先级矩阵

| 优先级 | 一手语义来源 | SDCC clean-room 测试 | 当前 QEMU 验收 |
|---|---|---|---|
| P0 | K246 [85-内部 148 KiB SRAM](https://www.stcaimcu.com/thread-24754-1-1.html) | 用 `__xdata`、`__at` 和 24 位指针独立写边界/固定模式检查；至少覆盖 `0x003fff`、`0x010000`、`0x01ffff`、`0x020000`、`0x02ffff`、`0x030000`、`0x030fff`，并单测 `0x01ffff + 1`。避开当前栈占用，不复制供应商的大循环 | UART 输出唯一 case 签名；每个边界读回一致。当前 `runtime-main.c` 已测 `0x01ffff`→`0x020000`，本例补齐其余窗口 |
| P0 | K246 [10-UART1 中断收发](https://www.stcaimcu.com/thread-24465-1-1.html) | 用 SDCC `__sfr __at` 和 `__interrupt` 重写最小 RX/TX ISR + 小环形缓冲；保留 `SCON/SBUF/REN/RI/TI` 语义，删除 Timer2/PLL/pinmux 初始化 | 双向 chardev 发送固定含 `0x00/0x7f/0x80/0xff` 的序列并要求逐字节 echo；检查 ISR 后主程序继续运行 |
| P0 | K246 [02-13 个定时器](https://www.stcaimcu.com/thread-24457-1-1.html)中的 T0/T1 | 分成 polling overflow/reload、IRQ+返回、可选 mode 2 三个小测试；不保留 13 个 timer 的单体工程结构 | 只要求计数、flag、ISR 次数及寄存器保存正确；不以墙钟时间、48 MHz 常量或 cycle 数判定 |
| P0 | K246 分类中的 00 端口 mode、01.1/01.2 P6 跑马灯 | 重写 P0..P7 的 mode/latch 组合，bit 与 byte 读改写分开；灯效改成有限状态序列 | 新增 host-assisted qtest：截获 `gpio-out`，并通过 `gpio-in` 验证四种 mode 与 pin/latch 区别；串口只报告汇总 |
| P0 | Keil [AN116](https://www.keil.com/appnotes/files/apnt_116.pdf)及 `apntex_116.zip` | 两个 translation unit 分置 `0xffxxxx` / `0xfexxxx`，分别测直接调用、函数指针和返回；map 必须证明跨 64 KiB。另用汇编器测试 `LCALL/RET` 与 `ECALL/ERET` 编码 | 运行结果正确且链接地址符合预期。**不要求 SDCC 模仿 Keil 的同段短调用选择**：当前 SDCC ABI revision 1 的 C 直接调用统一使用 `ECALL/ERET` |
| P0 | Keil [AN117](https://www.keil.com/appnotes/files/apnt_117.pdf)、MCB251 RAM Test、K246 SRAM | 独立写 `__xdata`/`__code` 对象、generic/space-specific 指针转换、循环和跨界测试 | default 与 size-optimized 两条构建均同结果；Keil 笔记的 74→49 字节只是历史数据，不是 SDCC 门槛 |
| P1 | Keil [ROM Checksum Test](https://www.keil.com/download/docs/47.asp) | 用原创固定向量和 `__code` 指针写有限长度 checksum；不复制原数据、控制流、目标校验区或预编译映像 | 默认与 size-optimized 两条 lane 均得到精确 checksum，并证明 24 位代码指针遍历可运行 |
| P1 | K246 04 T0/T1 外部计数、05 gate/低脉宽、06 INT0..4 | 只转 P3.4/P3.5 counter、P3.2/P3.3 gate 与 INT0/1；一个输入刺激对应一个可计数事件 | 用 qtest `gpio-in` 产生明确的 high→low 序列；INT2..4 不纳入 |
| P1 | K246 [38-TFPU](https://www.stcaimcu.com/forum.php?mod=viewthread&tid=24691) | 依据手册独立列 binary32 加减乘除、转换、比较/分类、±0、边界值；三角函数用容差和有限输入 | 结果位模式/容差正确；不比较耗时，不以 QEMU 推断的未公开位定义作为 silicon oracle |
| P1 | K246 [39-DSP32](https://www.stcaimcu.com/forum.php?mod=viewthread&tid=24692) | 对每类 `DPUOP` 至少一个正常值和边界值，逐步扩展到每条已文档化命令；纯 C 算术测试与显式硬件单元测试分开 | 结果、zero/divide-by-zero/shift-count 等有正式定义的状态正确；不比较 latency |
| P1 | AN109/AN114、STC FreeRTOS MCS251 port | 独立写启动清零、四字节中断 frame、扩展栈和 C/汇编保存恢复测试；不复制 `START251.A51` 或 Keil inline assembly | Timer/UART ISR 前后给寄存器和栈填 canary；中断返回后逐项核验，并检查 map/反汇编 |
| P2 / defer | USB、CAN-FD、ADC/DAC、PWM、I2S、SPI/I²C/QSPI、RTC/低功耗、DMA、Flash IAP/EEPROM、Timer2+、UART2+、显示器和传感器示例 | 可作为 parser/编译覆盖或未来设备模型需求清单；若仅抽取其中 Timer0/UART1 子逻辑，测试名必须明确只覆盖该子语义 | 当前不得成为 QEMU runtime gate，也不得宣称整例兼容 |

### 9.2 方言与 ABI 边界

“SDCC 风格改写”意味着重新表达可观察行为，而不是批量替换关键字：

- Keil 的项目文件、startup、segment/class、绝对地址、`interrupt N`、内存限定词和内联汇编必须按当前 SDCC MCS251 前端、汇编器与 linker script/flags 重新设计；不能把能打开 `.uvproj` 当成目标。
- [`docs/mcs251-abi.md`](mcs251-abi.md)明确说明当前是 **SDCC MCS251 ABI revision 1**，不是 Arm/Keil ABI，也不宣称 OMF-251 对象或库兼容；其直接 C 调用/返回约定是 `ECALL`/`ERET`。所以 AN116 的 `LCALL` 优化只能转成独立汇编/链接优化课题，不能作为“兼容 Keil”的验收。
- 不链接供应商 `.LIB`，不运行供应商预编译 HEX/BIN，也不复制原控制流、变量名、注释或输出字符串。测试源码只保留 `Source:` 链接、语义摘要和独立推导的 expected values。
- 目标是“同一公开硬件行为在 SDCC 产物中成立”，不是源码兼容、ABI 兼容或二进制兼容。若将来要增加兼容层，应另立规格和许可审计。

## 10. 构建、运行与判定协议

现有 [`src/mcs251/Makefile.in`](../src/mcs251/Makefile.in) 已提供 `check` / `check-qemu`，默认寻找 `~/oss/qemu/builds/build-mcs251/qemu-system-mcs251`；[`src/mcs251/tests/check-qemu.py`](../src/mcs251/tests/check-qemu.py) 已能用 `sdcc -mmcs251`、`--code-loc 0xff0000` 生成 HEX，在 `stc32g144k246-evb` 上运行，并以 UART `PASS`/`FAIL` 判定。新增例程测试应沿用这一入口但收紧协议：

1. 每个固件输出唯一一行 `CASE:<stable-name>:PASS\n`；任何 `FAIL`、异常退出或固定 timeout 都失败。避免只搜索无上下文的 `PASS` 子串。
2. 每个纯 C 语义案例至少跑当前默认优化和 `--opt-code-size` 两条 lane；结果必须一致。另设代码生成测试检查常量折叠、无效代码消除、冗余扩展/装载消除和已承诺的 native 指令，不把某一种合法指令序列过度固化。
3. 布局相关案例同时检查 `.map` / 反汇编与运行结果：函数确实位于目标 64 KiB 段、绝对对象确实位于指定 RAM，不能因 linker 悄悄放回近地址而得到假阳性。
4. UART echo 使用独立双向 socket/chardev；GPIO、external-counter、timer gate 和 INT0/1 使用 qtest `gpio-in` / `gpio-out`。`check-official-examples.py` 已实现这套 host-assisted runner，并在默认优化与 `--opt-code-size` 两条 lane 中执行。P0-P7 四种 GPIO mode 和 Timer0/1 mode 2 + gate 均已有独立固件案例。
5. expected values 从 STC/Intel 指令集或芯片手册独立生成，QEMU 只作为执行环境。对 QEMU 已声明为推断或非 cycle-exact 的行为，不建立 silicon compatibility 结论。
6. 每个测试记录 `source_url`、官方发布物哈希（若有）、被抽取的语义、被删除的板级依赖和所需 QEMU feature；这样未来在 QEMU/真机上可复用同一测试向量。

建议按四层保存结果：

| 层 | 目的 | 通过条件 |
|---|---|---|
| assemble/compile | 所有 MCS251 指令、SDCC 语法和基本优化能生成 | 无 ICE/诊断漂移；每条目标 opcode/寻址模式有独立覆盖 ID |
| link/layout | 24 位 relocation、segment、绝对对象和跨段调用正确 | map/反汇编满足必要结构约束 |
| QEMU runtime | ABI、内存、ISR 与已建模外设端到端工作 | 精确 case 签名、无 `FAIL`、无 timeout |
| hardware differential（后续） | 排除 QEMU 与 silicon 同源误判 | 同一原创向量在 STC32G144K246 实板结果一致 |

## 11. 建议的 clean-room 转化清单

以下是跨示例的覆盖主题；具体优先级和 QEMU 判据以第 9、10 节为准：

1. 地址空间：`near`、`xdata`/`edata`、代码常量、far 指针、跨 `0xffff` 边界和 STC32G144K246 的内部 SRAM 高地址。
2. 调用模型：同段/跨段直接调用，函数指针间接调用，`LCALL`/`RET` 与 `ECALL`/`ERET` 的链接器选择和返回地址宽度。
3. 中断 ABI：最小 Timer0/Timer1 ISR、UART1 ISR、嵌套前后的通用寄存器/扩展栈指针保存恢复和四字节中断帧。
4. 整数与 DSP：8/16/32/64 位有符号/无符号算术、乘除、移位、比较、边界与别名；硬件单元测试与纯 C 语言语义测试分开。
5. 浮点/TFPU：有限值、零、符号、舍入及状态位边界向量；先验证结果，再逐项纳入状态标志。
6. 代码生成基本优化：常量折叠、死代码、循环归纳变量、同段短调用、冗余扩展/装载消除。性能/代码大小用相对回归阈值，不照搬 Keil 示例的绝对字节数。
7. 多文件链接：启动代码、C/汇编互调、绝对地址对象、段放置、ROM checksum/Split-ROM 的最小化变体。

每个测试建议同时保留三层结果：编译是否成功、反汇编是否满足必要结构约束、目标机运行签名是否正确。只有必须由编译器/链接器保证的结构才做反汇编断言，避免把某一种合法指令序列写成唯一答案。

## 12. 尚未闭合的缺口

- STC32G144K246 的 88 个官方主题尚未发现一个可匿名稳定下载的整包；单帖附件也未获得稳定直接 URL，因此没有可信字节数和 SHA-256。
- 产品页上的 STC32G144K246“示例”链接实际指向旧通用/12K128 包；不能据此声称这些包已覆盖 144K246 全部新增指令、TFPU、DSP32 或内存布局。
- `STC32G-DEMO-CODE-V9.6.zip` 和 `STC32G-SOFTWARE-LIB.zip` 是可变别名；未来下载可能与本文件哈希不同。
- Keil MCS251 5.60 评估安装器需要提交个人/公司联系信息；本次仅能核验页面公开 MD5，不能给出 installer 直链、字节数或 SHA-256。安装器内据称存在的 `C:\KEIL\MCS251\EXAMPLES\Blinky`、`Hello`、`Measure` 尚未核验。
- Keil `251JTAB.ZIP` 的官方详情页明确标记为当前不可用；没有可信下载物可审计。
- 尚未逐文件审计所有 STC 包内第三方许可证；任何直接复用请求都必须另开许可审计。
- 当前 QEMU 可运行性来自提交 `b80cfcb4b6eca7f900c35d4a268fcf62a92bb9c5` 的实现审阅；未来 QEMU 变更后应重新核对第 8 节，而不是无限期沿用结论。
- 官方示例不是穷举指令测试。独立的 `sdas/as251/tests/instruction-forms.tsv` 已按 Intel 手册表覆盖 65 个指令族、269 个合法操作数形式及 Source/Binary 两套 opcode map，并由生成器、汇编 golden bytes 和 manifest 三方一致性检查约束；编译器不会自然生成的全部形式仍以汇编级测试为验收层，后续真机差分不应由 QEMU 结果替代。
- GPIO/外部 counter/INT、timer gate 和 UART RX 的 host-assisted runner 已实现，并已在两条优化 lane 通过。当前外设层剩余的是同一外设内部尚未逐项建模为固件案例的变体（例如 timer prescaler、mode 0/3 和 x1/x12），而不是缺少宿主交互能力。

## 13. 本次已校验下载的复核方法

建议在仓库外临时目录执行，并把输出与第 3 节核对：

```sh
curl -fL -O https://www.stcmicro.com/rar/demo/STC32G-DEMO-CODE-V9.6-20240418.zip
curl -fL -O https://www.stcmicro.com/rar/demo/STC32G-SOFTWARE-LIB-20240910.zip
curl -fL -O https://www.stcmicro.com/rar/demo/FreeRTOS-STC32G-CORE-V1.0.2-DemoCode-20220609.zip
wc -c ./*.zip
shasum -a 256 ./*.zip
```

对当前别名做持续集成取证时，不要假定上次哈希仍有效；先记录下载日期和新哈希，再决定是否升级基线。
