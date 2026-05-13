import re
import struct
from enum import IntEnum, auto

# 操作码枚举，与文档中的 enum COMMAND 一致
class Command(IntEnum):
    op_load = auto()
    op_store = auto()
    op_move = auto()
    op_set = auto()
    op_push = auto()
    op_pop = auto()
    op_add = auto()
    op_sub = auto()
    op_mul = auto()
    op_div = auto()
    op_and = auto()
    op_or = auto()
    op_invert = auto()
    op_not = auto()
    op_compare = auto()
    op_input = auto()
    op_output = auto()
    op_branch = auto()
    op_jump = auto()
    op_jr = auto()
    op_call = auto()
    op_ret = auto()

# 指令信息：操作码，需要的操作数个数
INSTR_INFO = {
    'load':    (Command.op_load, 2),
    'store':   (Command.op_store, 2),
    'move':    (Command.op_move, 2),
    'set':     (Command.op_set, 2),
    'push':    (Command.op_push, 1),
    'pop':     (Command.op_pop, 1),
    'add':     (Command.op_add, 3),
    'sub':     (Command.op_sub, 3),
    'mul':     (Command.op_mul, 3),
    'div':     (Command.op_div, 3),
    'and':     (Command.op_and, 3),
    'or':      (Command.op_or, 3),
    'compare': (Command.op_compare, 3),
    'invert':  (Command.op_invert, 2),
    'not':     (Command.op_not, 2),
    'branch':  (Command.op_branch, 3),
    'jump':    (Command.op_jump, 1),
    'jr':      (Command.op_jr, 1),
    'call':    (Command.op_call, 1),
    'ret':     (Command.op_ret, 0),
    'input':   (Command.op_input, 1),
    'output':  (Command.op_output, 1),
}

# 正则表达式，用于识别不同种类的操作数
REG_REGEX  = re.compile(r'^r(\d+)$')                # 普通寄存器
MEM_REGEX  = re.compile(r'^\[r(\d+)\]$')            # 内存引用 [rX]
LABEL_REGEX= re.compile(r'^\$([a-zA-Z_][a-zA-Z0-9_]*)$')  # 标签 $ident

class AssemblerError(Exception):
    """汇编过程中产生的错误"""
    pass

def to_signed32(val):
    """将整数转换为 32 位有符号表示，若超出范围则抛出 OverflowError"""
    if val < -2**31 or val > 2**32 - 1:
        raise OverflowError(f"数值 {val} 超出 32 位整数范围")
    if val >= 2**31:
        val -= 2**32
    # 此时 val 应在 [-2^31, 2^31-1] 内
    if val < -2**31 or val > 2**31 - 1:
        raise OverflowError(f"数值 {val} 超出 32 位有符号整数范围")
    return val

def parse_operand(operand_str, line_num):
    """
    解析单个操作数字符串，返回 (类型, 值)
    类型: 'reg' , 'mem' , 'label' , 'num'
    值  : 对于 reg/mem 为寄存器号，对于 label 为标签名，对于 num 为已转换好的有符号整数
    """
    s = operand_str.strip()
    if not s:
        raise AssemblerError(f"行{line_num}: 空操作数")

    # 1. 普通寄存器
    m = REG_REGEX.match(s)
    if m:
        reg = int(m.group(1))
        if not (0 <= reg <= 31):
            raise AssemblerError(f"行{line_num}: 寄存器编号 {reg} 超出 0~31")
        return ('reg', reg)

    # 2. 内存引用 [rX]
    m = MEM_REGEX.match(s)
    if m:
        reg = int(m.group(1))
        if not (0 <= reg <= 31):
            raise AssemblerError(f"行{line_num}: 寄存器编号 {reg} 超出 0~31")
        return ('mem', reg)

    # 3. 标签 $ident
    m = LABEL_REGEX.match(s)
    if m:
        return ('label', m.group(1))

    # 4. 常数（十进制或十六进制，允许负号）
    try:
        val = int(s, 0)               # 自动识别 0x 前缀
        val = to_signed32(val)        # 转换为 32 位有符号，并检查范围
        return ('num', val)
    except ValueError:
        raise AssemblerError(f"行{line_num}: 无法识别的操作数 '{s}'")
    except OverflowError as e:
        raise AssemblerError(f"行{line_num}: {e}")

def compile_asm(source_filename, bin_filename):
    """
    汇编主函数
    :param source_filename: 输入的汇编源文件路径
    :param bin_filename:    输出的二进制文件路径
    """
    label_to_addr = {}          # 标签名 -> 指令地址
    instructions = []            # 存储解析后的指令，元素为 (行号, 指令名, 操作数列表)
    pc = 0                       # 当前指令计数器（下一条指令的索引）

    # ---------- 第一遍：收集标签定义，初步检查指令格式 ----------
    with open(source_filename, 'r', encoding='utf-8') as f:
        for line_num, raw_line in enumerate(f, start=1):
            line = raw_line.strip()
            if not line or line.startswith('#'):
                continue                     # 空行或注释

            # 标签声明
            if line.startswith(':'):
                # 提取标签名
                label_part = line[1:].lstrip()
                m = re.match(r'^([a-zA-Z_][a-zA-Z0-9_]*)', label_part)
                if not m:
                    raise AssemblerError(f"行{line_num}: 无效的标签声明 '{line}'")
                label = m.group(1)
                if label in label_to_addr:
                    raise AssemblerError(f"行{line_num}: 标签 '{label}' 重复定义")
                label_to_addr[label] = pc      # 标签指向当前指令地址
                continue                        # 标签行不产生指令

            # 指令行
            parts = line.split(maxsplit=1)
            op_name = parts[0].lower()
            if op_name not in INSTR_INFO:
                raise AssemblerError(f"行{line_num}: 未知指令 '{op_name}'")

            operands_str = parts[1] if len(parts) > 1 else ''
            # 分割操作数（按逗号）
            if operands_str.strip():
                operands_raw = [op.strip() for op in operands_str.split(',')]
            else:
                operands_raw = []

            expected_num = INSTR_INFO[op_name][1]
            if len(operands_raw) != expected_num:
                raise AssemblerError(
                    f"行{line_num}: 指令 '{op_name}' 需要 {expected_num} 个操作数，"
                    f"但提供了 {len(operands_raw)} 个"
                )

            # 解析每个操作数（此时会检查格式，常数也会转换并检查范围）
            parsed_ops = []
            for op_str in operands_raw:
                parsed_ops.append(parse_operand(op_str, line_num))

            instructions.append((line_num, op_name, parsed_ops))
            pc += 1

    # ---------- 第二遍：生成机器码 ----------
    total_insn = len(instructions)
    bin_data = bytearray()
    # 先写入指令总数（4字节，小端序）
    bin_data.extend(struct.pack('<I', total_insn))

    for line_num, op_name, operands in instructions:
        opcode = INSTR_INFO[op_name][0]
        # 准备三个操作数，默认全 0
        op_vals = [0, 0, 0]

        # 按顺序将解析好的操作数填入前几个位置
        for i, (typ, val) in enumerate(operands):
            if typ == 'label':
                # 标签 -> 查找地址
                if val not in label_to_addr:
                    raise AssemblerError(f"行{line_num}: 未定义的标签 '{val}'")
                addr = label_to_addr[val]
                if not (0 <= addr <= 0xFFFFFFFF):
                    raise AssemblerError(f"行{line_num}: 标签 '{val}' 的地址 {addr} 超出 32 位无符号范围")
                op_vals[i] = addr
            elif typ in ('reg', 'mem', 'num'):
                # 寄存器号或常数已经在第一遍检查过范围，直接使用
                op_vals[i] = val
            else:
                raise AssemblerError(f"行{line_num}: 内部错误 - 未知操作数类型")

        # 打包指令：操作码(1B) + 三个操作数(各4B,小端)
        insn_bytes = struct.pack('<BIII', opcode, op_vals[0], op_vals[1], op_vals[2])
        bin_data.extend(insn_bytes)

    # 写入二进制文件
    with open(bin_filename, 'wb') as f:
        f.write(bin_data)

# 如果直接运行脚本，可以进行简单测试（可选）
if __name__ == '__main__':
    import sys
    if len(sys.argv) != 3:
        print("Usage: python asm.py <source.asm> <binary.bin>")
    else:
        try:
            compile_asm(sys.argv[1], sys.argv[2])
        except Exception as e:
            print(f"Error: {e}")
            sys.exit(1)