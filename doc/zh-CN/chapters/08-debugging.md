# 调试

本章先介绍 SDCC 自带的源代码级调试器 SDCDB，再说明其他调试器使用的
ELF/DWARF 路径，最后给出本分支 MCS-251 与 QEMU GDB stub、trace log 的联调
方法。调试信息的格式和调试器的传输协议是两件事：QEMU 能提供 GDB Remote
Serial Protocol（RSP）并不意味着 GDB 能直接理解 SDCC 的 CDB 文件。

## 使用 SDCDB 调试

### 生成调试信息

编译和链接时都要传入 `--debug`，并保留生成的 `.cdb`、`.adb`、`.map` 与目标
映像：

```sh
sdcc -mmcs51 --debug -o blink.ihx blink.c
sdcdb blink
```

`--debug` 让编译器在汇编输出中留下源程序与符号记录，汇编器据此生成 ADB，
链接器最终生成 CDB。删除中间文件会使断点和变量名无法解析。调试版本不一定与
发布版本具有完全相同的代码布局，因此应对实际运行的映像使用与之配套的调试
文件。

### SDCDB 的工作方式

SDCDB 读取 CDB 符号信息，通过模拟器执行、停止并检查程序。它采用类似 GDB 的
命令风格，但只实现适合 SDCC 目标的子集。典型流程如下：

```text
(sdcdb) break main
(sdcdb) run
(sdcdb) next
(sdcdb) print counter
(sdcdb) continue
```

常用命令包括：

- `break`、`clear`、`delete`：设置、清除或删除断点；
- `run`、`continue`、`step`、`next`：启动、继续、单步进入或单步越过；
- `print`、`display`、`info`：查看表达式、持续显示项或调试状态；
- `list`、`frame`、`where`：查看源代码、当前栈帧或调用链；
- `set`：修改变量或调试器设置；
- `help`、`quit`：查看帮助或结束会话。

具体可用命令取决于目标和模拟器。启动后用 `help` 查看当前构建实际支持的命令，
不要把完整 GDB 命令集视为 SDCDB 的承诺。

### 命令行选项与前端

SDCDB 可以选择模拟器、传入模拟器参数、指定搜索路径，并从命令文件批量读取
操作。选项会随版本演进，应以 `sdcdb --help` 为准。DDD 和 Emacs/XEmacs 可以把
SDCDB 当作 GDB 风格后端使用，但图形前端显示的能力仍受 SDCDB 本身限制。

## 使用其他调试器：ELF 与 DWARF

部分 SDCC 后端能够由汇编器和链接器生成 ELF，并在其中携带 DWARF。此时可使用
支持相应架构的 GDB 或其他调试器。需要同时满足三个条件：

1. SDCC 后端能产生正确的 DWARF；
2. 工具链能输出并保留 ELF，而不只是 Intel HEX；
3. 调试器认识该 CPU 的寄存器、指令、地址空间和栈展开规则。

缺少任意一项时，仍可使用符号表或 map 文件进行机器级调试，但不能期待完整的
C 源代码单步、变量求值和栈回溯。

## MCS-251 与 QEMU GDB stub

当前 QEMU MCS-251 目标实现了 GDB RSP，接受常规的 `-S`、`-s` 和 `-gdb`
选项，并通过 `qXfer:features:read` 发布 `mcs251-core.xml`。target description
包含 43 个寄存器：

- R0 至 R31；
- R56 至 R63，即包含 DPX 和 SPX 的字节位置；
- PSW 与 PSW1；
- 一个由 GDB 表示为 32 位、架构上截断为 24 位的 PC。

SDCC 的 `--debug` 目前为 MCS-251 生成 CDB/ADB，而非带 DWARF 的 ELF。因此
QEMU 的机器级 GDB endpoint 已经可用，未经修改的 GDB 却不能从 CDB 获得完整
C 源代码调试。map 与 CDB 标签仍能提供设置断点所需的精确 24 位地址。完整支持
还需要 CDB-to-GDB bridge，或者为 `sdcdb` 增加 RSP transport 与 MCS-251 stack
unwinder。现有 MCS-51 unwinder 假定 8 位 SP 和两字节返回地址，不能原样复用。

### 自动检查 GDB stub

`check-gdbstub.py` 会完成以下闭环：用 `--debug` 构建程序；确认 `_main` 在 CDB
和 map 中的 24 位地址一致；以暂停状态启动 QEMU；读取 target XML；安装 RSP
断点并继续；检查停下时的 PC；通过寄存器写包往返验证 R0；最后单步一条指令。

```sh
make -C build/src/mcs251 check-gdbstub \
    QEMU_MCS251="$HOME/oss/qemu/builds/build-mcs251/qemu-system-mcs251" \
    MCS251_LIBRARY_DIR=/path/to/device/lib/build/mcs251-small
```

该检查只实现自身需要的最小 RSP 子集，不要求主机安装带 MCS-251 反汇编器的
GDB。若要同时保留有界的启动 trace，可直接运行脚本并追加 `--trace-log`：

```sh
python3 src/mcs251/tests/check-gdbstub.py \
    --sdcc build/bin/sdcc \
    --qemu "$HOME/oss/qemu/builds/build-mcs251/qemu-system-mcs251" \
    --source src/mcs251/tests/debug-smoke.c \
    --device-include device/include \
    --library-dir build/device/lib/build/mcs251-small \
    --trace-log /tmp/mcs251-startup.trace
```

### 手工连接

先生成调试记录，并把 map、CDB 与映像保存在一起：

```sh
sdcc -mmcs251 --debug --no-xinit-opt \
    --code-loc 0xff0000 '-Wl-b GSINIT0=0xfc2800' \
    -o firmware.hex firmware.c
```

然后让 CPU 停在 reset instruction 之前并开放 RSP 端口：

```sh
qemu-system-mcs251 \
    -M stc32g144k246-evb -bios firmware.hex \
    -display none -monitor none -serial stdio \
    -S -gdb tcp::1234
```

客户端连接 1234 端口后，可从 `firmware.map` 取得 `_main` 地址，也可使用如下
CDB 记录中的同一地址：

```text
L:G$main$0$0:FC2818
```

RSP 寄存器 42 是 PC；寄存器 32 至 35 暴露 R56 至 R59（DPX）；寄存器 36 至
39 暴露 R60 至 R63（SPX）。硬件栈地址是 SPX 中 SPH:SP 的低 16 位。

## 使用 trace log 定位栈与地址异常

调用/返回、interrupt frame 或非法地址出错时，开启指令与 CPU 状态日志：

```sh
qemu-system-mcs251 \
    -M stc32g144k246-evb -bios firmware.hex \
    -display none -monitor none -serial stdio \
    -d in_asm,cpu,nochain -D /tmp/mcs251.trace
```

每个 trace block 会在解码后的指令旁记录 24 位 PC、SPX、DPX、PSW/PSW1 和
R0–R31。发生异常或返回地址损坏时，从第一个意外 PC 倒推，对照每次 `ECALL`、
`ERET`、`PUSH`、`POP` 和中断入口前后的 SPX。如果 trace 不能显示保存的 frame，
再用 GDB stub 的 memory-read packet 检查 SPX 周围的字节。

`cpu,nochain` 产生日志很快，尤其是固件进入预期的无限循环之后。应优先使用
`-S` 加断点、较短的测试超时，或上面的自动检查；收集到所需状态后立即结束
QEMU。

### 已确认的有符号位移案例

aggregate-return 回归测试给出了区分编译器与模拟器错误的实例。在 PC
`0xfc2837`、SPX 为 `0x0039` 时，合法编码是
`mov r11,@dr60+0xfffb`，即 `mov a,@spx-5`。目标地址应为 `0x0034`，那里保存
着 caller 压入的三字节 hidden result pointer。如果 QEMU 把 `0xfffb` 零扩展，
便会误读 `0x010034`，trace 随后显示三个零字节进入 DPL/DPH/DPXL，测试打印
`FAIL`。这能明确指出 emulator 的 displacement bug，而不是 SDCC stack frame
或 `ECALL`/`ERET` 约定出错。

诊断应以首个错误状态为边界。程序最终进入正常的 `__exitEmu` 循环，只说明固件
已经结束，并不能证明此前生成的 UART 数据正确。如果 MCS-51 或 MCS-251 测试在
QEMU 上失败，应先用相同映像、map 与 trace 复核模拟器实现；确认是 QEMU 问题
后，再把最小复现、命令、首个错误 PC 和相关寄存器状态记录到对应 QEMU PR。
