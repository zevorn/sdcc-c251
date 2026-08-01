# SDCC 编译器用户指南（简体中文）

本目录是 `doc/sdccman.lyx` 英文手册的平行中文版本。英文目录和源文件保持不变，
以便下游分支 rebase 到 SDCC 主线时尽量减少冲突。中文手册按原手册的主题和叙述
顺序重组，并增加本分支的 MCS-251 目标、ABI、QEMU 和 GDB stub 内容。

翻译以源码树中的英文 LyX 文档和官方发布的
[SDCC Compiler User Guide](https://sdcc.sourceforge.net/doc/sdccman.pdf) 为依据。
中文章节覆盖英文手册的介绍、安装、使用、语言扩展、处理器说明、调试、实用
技巧、支持和编译器内部原理，并把 MCS-51 与 MCS-251 分成独立章节，便于持续维护。

“SDCC-51”是 8051 用户对“用 SDCC 编译 MCS-51 程序”的习惯称呼，不是目标名。
命令行中使用 `-mmcs51` 选择 MCS-51，使用 `-mmcs251` 选择 MCS-251；本分支不提供
`-mc251` 或 `c251` 兼容入口。

## 构建

构建 PDF：

```sh
make -C doc/zh-CN pdf
```

构建单页 HTML：

```sh
make -C doc/zh-CN html
```

需要 Pandoc、XeLaTeX、简体中文 TeX 宏包和 Noto CJK 字体。在 Debian/Ubuntu 上：

```sh
sudo apt-get install pandoc texlive-xetex texlive-lang-chinese fonts-noto-cjk
```

输出位于 `doc/zh-CN/build/`。GitHub Actions 会对 pull request 和分支 push 构建
PDF/HTML artifact；tag 构建还会把 PDF 附加到对应 GitHub Release。

## 翻译约定

- 忠实保留英文原意、条件、否定和适用范围，同时按自然中文语序行文。
- 命令、选项、路径、C 标识符、寄存器名和对象格式名不翻译。
- ABI、API、overlay、bank、peephole、Source mode、Binary mode 等保留英文。
- port/back-end 在编译器语境中译为“后端”，I/O port 仍译为“端口”。
- runtime library 译为“运行库”，object file 译为“目标文件”。
- 英文源更新时先对照章节结构和 `git diff`，再更新中文内容，不修改英文源来迎合翻译。

英文手册仍是上游规范来源。中文内容与当前程序的 `sdcc --help`、源码或测试结果
不一致时，以当前代码和测试为准，并修正本目录。

MCS-251 的英文 ABI、QEMU/GDB、memory model 和测试资料统一保存在
[`doc/mcs251/`](../mcs251/README.md)。仓库不再使用平行的 `docs/` 目录；英文手册、
中文手册和目标资料都从上游原有的 `doc/` 目录维护。
