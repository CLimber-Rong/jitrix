#!/usr/bin/env python3
import sys
import re

def parse_objdump(filepath):
    """解析 objdump 文本，返回 jit_* 函数列表."""
    with open(filepath, 'r') as f:
        lines = f.readlines()

    funcs = []
    current = None

    # 匹配函数起始行：地址 <jit_名称()>:
    func_start_re = re.compile(r'^([0-9a-f]+)\s+<jit_(\w+)\(\)>:\s*$')
    # 匹配指令行：地址: 字节码 ... （冒号后可以是空格/制表符）
    insn_re = re.compile(r'^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )*[0-9a-f]{2})\b')
    # 匹配重定位行：前导空格 + 地址: 类型 表达式
    reloc_re = re.compile(r'^\s+([0-9a-f]+):\s+R_\S+\s+(\S+)\s*$')

    for line in lines:
        # 检查新函数开始
        m = func_start_re.match(line)
        if m:
            addr = int(m.group(1), 16)
            name = "jit_" + m.group(2)   # 如 jit_load
            if current:
                funcs.append(current)
            current = {
                'name': name,
                'start_addr': addr,
                'bytes': [],     # (地址, 值) 元组列表
                'relocs': []     # (地址, 参数序号, addend)
            }
            continue

        if current is None:
            continue

        # 检查重定位行
        rm = reloc_re.match(line)
        if rm:
            addr = int(rm.group(1), 16)
            expr = rm.group(2)
            arg_index = -1
            addend = 0

            # 尝试匹配 jit_arg_<n> 后面可选 +0x... 或 -0x...
            m_arg = re.match(r'jit_arg_(\d+)((?:[+-]0x[0-9a-fA-F]+)?)$', expr)
            if m_arg:
                arg_index = int(m_arg.group(1))
                addend_str = m_arg.group(2)
                if addend_str:
                    sign = 1 if addend_str[0] == '+' else -1
                    addend = sign * int(addend_str[1:], 16)
                else:
                    addend = 0
            else:
                # 作为后备，兼容其他符号格式（如 jit_arg+0x4）
                m_arg = re.search(r'jit_arg_(\d+).*', expr)
                arg_index = int(m_arg.group(1))
                m_add = re.search(r'([+-])(0x[0-9a-fA-F]+)$', expr)
                if m_add:
                    sign = 1 if m_add.group(1) == '+' else -1
                    addend = sign * int(m_add.group(2), 16)
                else:
                    addend = 0

            current['relocs'].append((addr, arg_index, addend))
            continue

        # 检查指令行
        im = insn_re.match(line)
        if im:
            addr = int(im.group(1), 16)
            hex_bytes = im.group(2).split()
            for i, b in enumerate(hex_bytes):
                current['bytes'].append((addr + i, int(b, 16)))

    if current:
        funcs.append(current)
    return funcs

def generate_cpp(funcs):
    """根据解析结果生成 C++ 头文件内容."""
    lines = []
    lines.append("#pragma once")
    # 不再包含 <stdint.h>
    lines.append("// auto generated")
    lines.append("namespace jitrix::jit {")

    for idx, func in enumerate(funcs):
        name = func['name']
        start = func['start_addr']

        # 按地址排序字节码
        func['bytes'].sort(key=lambda x: x[0])
        code_bytes = [b for _, b in func['bytes']]

        # 生成 code 数组（使用 unsigned char）
        hex_list = [f"0x{b:02x}" for b in code_bytes]
        code_line = f"const unsigned char {name}_code[] = {{{', '.join(hex_list)}}};"
        lines.append(code_line)
        code_len = len(hex_list)
        lines.append(f"const unsigned int {name}_code_len = {code_len};")

        # 处理空洞（重定位）
        hole_entries = []
        for raddr, arg_index, addend in func['relocs']:
            offset = raddr - start
            hole_entries.append((offset, arg_index, addend))

        hole_len = len(hole_entries)
        if hole_len > 0:
            items = []
            for off, arg_idx, add in hole_entries:
                off_str = f"0x{off:x}"
                add_str = f"0x{add:x}" if add >= 0 else f"-0x{-add:x}"
                items.append(f"{{{arg_idx}, {off_str}, {add_str}}}")
            hole_line = f"const int {name}_hole[{hole_len}][3] = {{{', '.join(items)}}};"
            lines.append(hole_line)
        else:
            # 没有空洞时给出占位数组，保证 C++ 合法
            lines.append(f"const int {name}_hole[1][3] = {{}};")

        len_line = f"const unsigned int {name}_hole_len = {hole_len};"
        lines.append(len_line)

        # 函数之间空一行（最后一个函数后不空）
        if idx != len(funcs) - 1:
            lines.append("")

    lines.append("}")
    return "\n".join(lines) + "\n"

def main():
    if len(sys.argv) != 3:
        print("Usage: python script.py <objdump_file> <output_cpp_file>")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    funcs = parse_objdump(input_path)
    if not funcs:
        print("Warning: no jit_* functions found.", file=sys.stderr)

    code = generate_cpp(funcs)
    with open(output_path, 'w') as f:
        f.write(code)

if __name__ == '__main__':
    main()