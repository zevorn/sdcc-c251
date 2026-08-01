# 安装 SDCC

## 配置选项

源码树使用 Autoconf。推荐在源码目录之外构建，使生成文件、目标文件和安装树与源码
分离：

```sh
git clone git@github.com:zevorn/sdcc-c251.git
cd sdcc-c251
mkdir build
cd build
../configure --enable-mcs251-port --prefix="$PWD/install"
make -j4
make install
```

`--prefix` 决定默认安装前缀。常见的 `--bindir`、`--includedir`、`--libdir` 和
`--docdir` 可进一步覆盖各类文件的位置。编译器在运行时还会根据可执行文件位置、
编译时前缀和环境变量推导头文件及库的搜索目录。

多数目标默认启用；可用 `--disable-<port>-port` 排除不需要的后端。MCS-251 在本
下游分支中显式使用：

```sh
../configure --enable-mcs251-port
```

可选组件包括 ucSim、器件运行库、`packihx`、SDCDB、sdbinutils、non-free 器件包和
文档。`../configure --help` 是当前源码树配置能力的权威列表，不应从旧版本文档
猜测选项。

如果宿主编译器默认进入 C23 模式并拒绝历史源码中的 K&R 函数定义，可为宿主构建
显式选择 GNU C17：

```sh
CFLAGS=-std=gnu17 ../configure --enable-mcs251-port
```

这只影响编译 SDCC 本身，不改变 SDCC 为目标程序选择的 C 语言方言。

## 安装目录和搜索路径

典型安装包含：

| 内容 | 默认相对位置 |
|---|---|
| 编译器及工具 | `bin/` |
| 通用和器件头文件 | `share/sdcc/include/` |
| 运行库和启动对象 | `share/sdcc/lib/<target>/` |
| 文档 | `share/sdcc/doc/` |

`sdcc -V` 会打印实际调用的预处理器、汇编器和链接器命令，可用于定位搜索路径问题。
`-I` 增加头文件目录，`-L` 增加库目录，`-l` 选择库。环境变量适合临时测试，长期
构建应在项目构建系统中显式记录路径，避免依赖交互式 shell 状态。

运行库必须匹配目标、内存模型和关键 ABI 选项。MCS-51 的 small、medium、large、
huge 模型不能随意混用；MCS-251 的 small/large 与各自的 `--stack-auto` 运行库也
分别独立。

## 在 Linux 和 macOS 上构建

安装依赖后，通常按 `configure`、`make`、`make install` 的顺序执行。并行构建失败
时，可先用单任务重跑以得到稳定的第一处错误：

```sh
make -j1 V=1
```

macOS 可使用 Apple Clang。应避免把 Homebrew、MacPorts 和手工安装的同名工具混在
同一个 PATH 中；Bison、Boost 或 GNU 工具版本错误时，`configure` 日志通常比最终
编译错误更有诊断价值。

## Windows 构建和安装

Windows 用户可使用官方安装包或 ZIP 快照，也可在 MSYS2/Cygwin/MinGW 环境构建。
不同 shell 对路径、引号和重定向的解释不同；复制 Unix 命令时要特别检查盘符、
反斜杠、空格和 `-Wl` 参数。

本分支的 `SDCC Windows package` workflow 会为每次 pull request 和 `main` push 构建
64 位 Windows ZIP。进入仓库的 Actions 页面，打开成功的 Windows package run，下载
`sdcc-mcs251-windows-x64-<commit>.zip` artifact 并解压即可。包内包含 `sdcc.exe`、
MCS-51/MCS-251 assembler、linker、header 和四种 MCS-251 配置对应的运行库。

这些 `.exe` 在 CI 中使用 UCRT64/MinGW 构建并静态链接宿主依赖。发布 ZIP 之前，CI
会离开 MSYS2 build shell，从普通 PowerShell 分别编译并链接 MCS-51 和 MCS-251
Intel HEX。因此，使用下载包不需要安装 MSYS2；只需把解压目录下的 `bin` 加入
`PATH`，或直接执行 `bin\sdcc.exe`。

安装包升级前应记录旧版本路径，并防止系统 PATH 同时命中多个 `sdcc.exe`。使用
`where sdcc` 和 `sdcc --version` 验证实际运行的程序。

## VPATH 和独立构建目录

SDCC 支持 VPATH 风格的独立构建。源码目录只包含版本控制文件，构建目录保存
Makefile、生成的配置头、对象和测试结果。多个构建目录可以针对不同选项并存，例如：

```text
sdcc-project/
├── source/
├── build-release/
├── build-debug/
└── build-mcs251/
```

删除构建目录即可重新配置，不需要清理源码树。不要在一个已配置目录中反复切换
不兼容的宿主编译器或目标集合。

## 构建和阅读文档

英文手册的上游目标为 `sdcc-doc`，依赖 LyX、LaTeX、latex2html 和索引工具。中文
手册使用独立目标：

```sh
make -C doc/zh-CN pdf
make -C doc/zh-CN html
```

生成的 PDF 和 HTML 位于 `doc/zh-CN/build/`。CI 会在干净 Ubuntu 环境重新构建，
从而验证所需字体、宏包和命令都已显式声明。

## 测试编译器

完整回归测试可能需要多个模拟器、交叉工具和较长时间。安装前至少执行与修改相关的
单元和回归目标。本分支的 MCS-251 测试入口见后文；MCS-51 还应执行基线比较和 QEMU
非回归检查。

## 安装故障排查

排查顺序如下：

1. 查看 `config.log` 中第一处失败的能力检查；
2. 确认宿主编译器、Bison、Flex、Boost、Python 和 make 的实际路径及版本；
3. 清空或新建构建目录后重新配置；
4. 使用 `make -j1 V=1` 捕获完整失败命令；
5. 区分“构建 SDCC 失败”和“SDCC 生成的目标程序失败”；
6. 检查安装目录写权限，但不要用 root 权限掩盖路径配置错误。

`configure` 负责探测宿主能力并生成 Makefile；`make` 构建已选择组件；
`make install` 只把构建结果复制到安装树。三者发生错误时应在相应阶段修复。

## 套件组件

| 程序 | 用途 |
|---|---|
| `sdcc` | C 编译驱动 |
| `sdcpp` | C 预处理器 |
| `sdas8051` | MCS-51 汇编器 |
| `sdas251` | MCS-251 汇编器 |
| `sdld` / `sdld8051` | ASxxxx 对象链接器 |
| `sdar` / `sdranlib` | 库管理 |
| `packihx` | 规范化 Intel HEX |
| `makebin` | 从 Intel HEX 生成二进制镜像 |
| `s51` 等 | ucSim 模拟器前端 |
| `sdcdb` | 基于 CDB 的源码级调试器 |

并非每个安装都包含全部组件；以 `sdcc --version`、安装目录和构建配置为准。
