#pragma once
#include "arena.hpp"
#include "config.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace jitrix::vm {

/*指令集*/
enum COMMAND {
	// 存储指令
	op_load = 1, // load reg, [reg]
	op_store, // store [reg], reg
	op_move, // move reg, reg
	op_set, // set reg, number
	op_push, // push reg
	op_pop, // pop reg
	// 运算指令
	// op dst_reg, reg, reg或op dst_reg, reg
	op_add, // 加
	op_sub, // 减
	op_mul, // 乘
	op_div, // 除
	op_mod, // 取模
	op_sll, // 逻辑左移
	op_srl, // 逻辑右移
	op_sra, // 算数右移
	op_and, // 与
	op_or, // 或
	op_invert, // 取反
	op_not, // 逻辑非
	op_compare, // 比较
	// 输入输出指令
	op_input, // input reg
	op_output, // output reg
	// 流程控制指令
	op_branch, // branch reg, branch_case_number, addr
	op_jump, // jump addr
	op_jr, // jump reg
	op_call, // call addr
	op_ret // ret
};

enum COMPARE_CASE {
	// 比较的三种情况
	compare_case_less = 1, // 小于
	compare_case_equal = 1 << 1, // 等于
	compare_case_greater = 1 << 2 // 大于
};

// 长度设置
constexpr unsigned int reg_max = 32;
constexpr unsigned int mem_max = 1024 * 1024;
constexpr unsigned int pc_stack_max = 1024;
constexpr unsigned int command_arg_max = 3;
constexpr unsigned int op_stack_max = 1024;

struct vm; // 虚拟机
struct command; // 指令

typedef void (*jit_func_type)();

struct command {
	jit_func_type jit_func; // 对应的JIT函数，没有被JIT则为NULL
	int arg[command_arg_max]; // 操作数
	COMMAND cmd; // 操作码
};

struct vm {
	int reg[reg_max]; // 寄存器
	int mem[mem_max]; // 内存
	command *code; // 字节码
	unsigned int code_len; // 字节码长度
	unsigned int pc_stack_buf[pc_stack_max]; // 程序计数器栈缓冲区
	unsigned int *pc_stack; // 程序计数器栈指针
	int op_stack_buf[op_stack_max]; // 操作栈缓冲区
	int *op_stack; // 操作栈指针

#if JITRIX_JIT_ENABLED == 1
	unsigned int *jump_counter; // 用于统计代码被跳转的次数，这也可以用哈希表代替
	arena::arena jit_arena; // 用于存放jit函数
	void jump_record(unsigned int addr); // 用于统计热代码
	int (*input_ptr)();
	void (*output_ptr)(int);
#endif

	void init(command *_code, unsigned int _code_len); // 初始化
	void destroy(); // 销毁
	int run(); // 运行代码
};

} // namespace jitrix::vm