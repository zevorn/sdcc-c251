#!/usr/bin/env python3

import pathlib
import re
import sys


REQUIRED_HEADINGS = {
    "SDCC 简介",
    "安装 SDCC",
    "使用 SDCC",
    "SDCC 语言扩展",
    "支持的处理器",
    "MCS-51 后端",
    "MCS-251 后端",
    "调试",
    "实用技巧与支持",
    "技术资料与编译器内部原理",
    "MCS-251 ABI revision 2",
    "MCS-251 指令集支持",
}


def main(paths):
    text = "\n".join(pathlib.Path(path).read_text(encoding="utf-8") for path in paths)
    headings = {
        match.group(1).strip()
        for match in re.finditer(r"^# (.+)$", text, flags=re.MULTILINE)
    }
    missing = sorted(REQUIRED_HEADINGS - headings)
    if missing:
        print("缺少一级章节：" + "、".join(missing), file=sys.stderr)
        return 1

    required_tokens = ["-mmcs51", "-mmcs251", "__SDCC_mcs51", "__SDCC_mcs251"]
    for token in required_tokens:
        if token not in text:
            print(f"缺少目标标识：{token}", file=sys.stderr)
            return 1

    forbidden = ["-mc251", "__SDCC_c251"]
    for token in forbidden:
        for line in text.splitlines():
            if token in line and "不提供" not in line:
                print(f"发现未说明的旧兼容入口：{token}", file=sys.stderr)
                return 1

    fences = len(re.findall(r"^```", text, flags=re.MULTILINE))
    if fences % 2:
        print("Markdown 代码围栏没有成对出现", file=sys.stderr)
        return 1

    print(f"已检查 {len(paths)} 个章节，{len(headings)} 个一级章节")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
