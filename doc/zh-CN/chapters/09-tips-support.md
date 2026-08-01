# 实用技巧与支持

## 在不同编译器之间移植代码

移植的首要原则是先隔离 implementation-defined behavior 与厂商扩展，再处理语法
差异。常见差异包括 named address space、interrupt declaration、bit/SFR 声明、
calling convention、inline assembly 和 startup hook。不要用全局文本替换把一种
编译器的关键字机械地改成另一种；同名类型在不同 memory model 下也可能具有不同
指针宽度和 ABI。

把硬件相关内容集中到小型 header 和 startup module 中，业务逻辑尽量使用标准 C。
通过 `<stdint.h>` 的定宽整数、`sizeof`、`static_assert` 和独立的链接检查表达布局
假设。对于中断共享对象使用 `volatile`，但不要误以为 `volatile` 能提供原子性或
线程同步。

从 Keil C51/C251 示例移植到本项目时，应把示例表达的外设行为转换成 SDCC 风格
测试：使用 `__sfr`、`__sbit`、SDCC startup/interrupt 语法和当前 ABI，而不是建立
Keil 源代码兼容层。只有 QEMU 已实现的外设才进入运行测试；其余示例可先做编译
测试或记录为待覆盖项。

## 随发行版提供的工具

SDCC 套件不仅包含 `sdcc` 驱动程序，还包括预处理器、各架构 assembler/linker、
`sdar` library archiver、`packihx`、`makebin`、`sdcdb` 和多个 uCsim simulator。
具体工具随构建配置和 host 平台变化，安装后的 `bin` 目录及各工具 `--help` 是
准确清单。

源码树还带有回归测试、设备 header、运行库和示例。它们既是用户资源，也是后端
行为的可执行规范。研究 undocumented behavior 时，优先对照测试、源码和实际
生成的 assembly，不要只根据其他编译器推断 SDCC。

## 随发行版提供的文档

英文主手册位于 `doc/sdccman.lyx`，发布版通常提供 PDF、HTML 或纯文本输出。
assembler、linker、simulator 和部分后端另有独立文档；设备 header 的注释及示例
也包含重要信息。本目录提供平行的简体中文手册，不改动英文源文件。

阅读时应核对手册版本与编译器版本。nightly snapshot 的文档可能描述尚未进入稳定
release 的行为，旧发行版也不会自动获得新手册中的选项。

## 在线交流与相关资源

SDCC 项目主页、SourceForge tracker、mailing list、代码仓库和 snapshot download
是主要的官方渠道。提问前先搜索手册、已有 issue 和邮件归档；报告中写明 SDCC
完整版本字符串、host、目标、命令行及最小输入。

与 SDCC 配合使用的开源工具包括构建系统、烧录器、模拟器、逻辑分析工具及不同
目标的 debugger。是否适用于某个设备，应由设备手册、工具文档和实测共同确认。
推荐阅读 ISO C 标准资料、目标 CPU 的官方 architecture/instruction manual、芯片
datasheet，以及 linker/assembler 文档。厂商 application note 可解释硬件，但其中
的编译器专用语法和 ABI 不一定适用于 SDCC。

## 常见问题

### 为什么优化后变量看起来消失了？

编译器可以删除没有 observable effect 的对象和计算，也可以把值只保存在寄存器
中。调试时可降低优化级别，但最终仍应验证发布配置。需要由硬件或中断观察的对象
应正确声明为 `volatile`；仅为了便于调试而添加 `volatile` 会抑制优化，并不能修复
生命周期或同步错误。

### 为什么链接器报告区域溢出？

memory model 只决定默认的分配策略，不会增加物理 RAM/ROM。检查 map 文件中的
segment 起止地址、stack/heap 预留、overlay 结果和 library model 是否一致。修改
`--code-loc`、`--xram-loc` 或 linker `-b` 选项前，先确认设备真实 memory map。

### 为什么单文件能编译，多文件链接后失败？

各 translation unit 可能用了不同 target/model/ABI option，或者函数声明、address
space qualifier 在文件之间不一致。用共同的 build rule，保持 prototype 可见，并
检查最终链接选中的 library directory。

### `--stack-auto` 可以只给一个文件使用吗？

不可以把 calling convention 不同的普通函数随意混合。所有 translation unit 与
运行库必须采用匹配的约定；MCS-251 还必须链接对应的 `*-stack-auto` library。

## 报告缺陷

高质量 bug report 应包含：

- `sdcc --version` 和完整、可复制的编译/链接命令；
- 单个或少量文件组成的最小复现；
- 预期行为、实际行为，以及判断依据；
- target、memory model、设备和 host 信息；
- `.asm`、`.map`、`.cdb`、测试输出或 trace 中首个错误位置；
- 若问题只在 QEMU 出现，说明真实硬件或其他 simulator 的对照结果。

不要只附整个应用工程，也不要只说“优化器有问题”。先用当前 release 或近期
snapshot 复现，并确认源程序本身没有 undefined behavior。安全问题不应在公共
tracker 中提前披露，应按项目公布的安全联系途径报告。

## 请求功能与提交补丁

feature request 应描述用例、目标范围、期望接口和兼容性影响。新后端或 ABI 改动
还需说明 instruction set、object format、memory model、calling convention、
运行库和测试策略。

补丁应遵循仓库编码风格，尽量拆成可独立审查的原子提交，并附 DCO
`Signed-off-by` trailer。提交前运行相关 regression test；影响公共前端或 MCS-51
共享代码时，还要运行未改目标的测试，避免把 MCS-251 支持建立在 MCS-51 行为
回归之上。commit message 说明“为什么”，而不仅是逐项重复 diff。

## 获取帮助、发布策略与质量控制

用户支持通常由社区志愿者提供。给出可复现信息、耐心等待并反馈最终结果，有助于
后续用户复用结论。ChangeLog、release note 和版本控制历史记录了行为变化；遇到
兼容性问题时应同时查阅。

SDCC 的 release 以回归测试、目标维护状态和发布准备情况为依据。snapshot 便于尽早
验证修复，但不等同于稳定发布。质量控制依赖自动回归、不同 host/target 的构建、
真实硬件或 simulator 运行，以及用户对 edge case 的反馈。

源码树中的 example 适合学习工具链基本流程，但不是所有设备的 production-ready
板级支持包。教学中可用它们演示从 C、assembly、link map 到机器执行的完整链条，
同时应明确标准 C、SDCC 扩展与芯片特性的边界。
