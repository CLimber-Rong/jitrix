#pragma once

#include "config.hpp"

#if JITRIX_JIT_ENABLED == 1

#include "arena.hpp"
#include "jit_data.hpp"
#include "stdio.h"
#include "vm.hpp"

#define JITRIX_JIT_MAKE_DATA(name) \
	jit_stencil(name##_code, name##_code_len, name##_hole, name##_hole_len)

namespace jitrix::jit {

struct jit_stencil {
	const unsigned char *code;
	const unsigned int code_len;
	const int (*hole)[3];
	const unsigned int hole_len;
	jit_stencil(const unsigned char *code, const unsigned int code_len, const int (*hole)[3],
			const unsigned int hole_len)
		: code(code)
		, code_len(code_len)
		, hole(hole)
		, hole_len(hole_len) {
	}
};

const jit_stencil stencil[] = { jit_stencil(NULL, 0, NULL, 0), JITRIX_JIT_MAKE_DATA(jit_load),
	JITRIX_JIT_MAKE_DATA(jit_store), JITRIX_JIT_MAKE_DATA(jit_move), JITRIX_JIT_MAKE_DATA(jit_set),
	JITRIX_JIT_MAKE_DATA(jit_push), JITRIX_JIT_MAKE_DATA(jit_pop), JITRIX_JIT_MAKE_DATA(jit_add),
	JITRIX_JIT_MAKE_DATA(jit_sub), JITRIX_JIT_MAKE_DATA(jit_mul), JITRIX_JIT_MAKE_DATA(jit_div),
	JITRIX_JIT_MAKE_DATA(jit_and), JITRIX_JIT_MAKE_DATA(jit_or), JITRIX_JIT_MAKE_DATA(jit_invert),
	JITRIX_JIT_MAKE_DATA(jit_not), JITRIX_JIT_MAKE_DATA(jit_compare),
	JITRIX_JIT_MAKE_DATA(jit_input), JITRIX_JIT_MAKE_DATA(jit_output) };

#if JITRIX_JIT_MUSTTAIL == 1
const jit_stencil stencil_addpc = JITRIX_JIT_MAKE_DATA(jit_addpc);
#endif

constexpr unsigned int jit_hole_max = 11;

// 以下两个输入输出函数用于填补输入输出指令的空洞
int input() {
	int x;
	scanf("%d", &x);
	return x;
}

void output(int x) {
	printf("%d\n", x);
}

inline vm::jit_func_type compile_single_cmd(
		vm::vm *m, unsigned int code_addr, void *(&hole)[jit_hole_max]) {
	// 获取地址为code_addr的单条指令中，除了尾递归函数（如果有的话）地址以外的所有待填值
	// 同时返回针对此条指令生成的JIT函数
	vm::COMMAND cmd = m->code[code_addr].cmd;
	auto &arg = m->code[code_addr].arg;

	const jit_stencil &sten = stencil[cmd];

	auto &mem = m->mem;
	auto &reg = m->reg;
	auto &op_stack = m->op_stack;
	auto &pc_stack = m->pc_stack;
	auto &input_ptr = m->input_ptr;
	auto &output_ptr = m->output_ptr;

	// 不同指令中同样意义的待填值
	hole[0] = &mem;
	hole[1] = &reg;
	hole[2] = &op_stack;
	hole[3] = &pc_stack;
	hole[8] = (void *) &input_ptr;
	hole[9] = (void *) &output_ptr;

	// 不同指令中有不同意义的待填值
	void *&i1 = hole[4];
	void *&i2 = hole[5];
	void *&i3 = hole[6];
	void *&u1 = hole[7];

	// 根据指令赋予不同意义
	switch (cmd) {
	case vm::op_load:
		i1 = &reg[arg[0]];
		u1 = &reg[arg[1]];
		break;

	case vm::op_store:
		u1 = &reg[arg[0]];
		i1 = &reg[arg[1]];
		break;

	case vm::op_move:
		i1 = &reg[arg[0]];
		i2 = &reg[arg[1]];
		break;

	case vm::op_set:
		i1 = &reg[arg[0]];
		i2 = &arg[1];
		break;

	case vm::op_push:
		i1 = &reg[arg[0]];
		break;

	case vm::op_pop:
		i1 = &reg[arg[0]];
		break;

	case vm::op_add:
	case vm::op_sub:
	case vm::op_mul:
	case vm::op_div:
	case vm::op_and:
	case vm::op_or:
		i1 = &reg[arg[0]];
		i2 = &reg[arg[1]];
		i3 = &reg[arg[2]];
		break;

	case vm::op_invert:
	case vm::op_not:
		i1 = &reg[arg[0]];
		i2 = &reg[arg[1]];
		break;

	case vm::op_compare:
		i1 = &reg[arg[1]];
		i2 = &reg[arg[2]];
		i3 = &reg[arg[0]];
		break;

	case vm::op_input:
		i1 = &reg[arg[0]];
		break;

	case vm::op_output:
		i1 = &reg[arg[0]];
		break;

	default:
		break;
	}

	unsigned char *ptr = (unsigned char *) m->jit_arena.alloc(sten.code_len);
	// 申请一块新的内存用于存放编译产物

	memcpy(ptr, sten.code, sten.code_len); // 复制代码模板

	return (vm::jit_func_type) ptr;
}

inline vm::jit_func_type patch(vm::jit_func_type func, vm::COMMAND cmd, void **hole) {
	// 给定函数地址，指令类型，代填值，进行填补

	unsigned char *ptr = (unsigned char *) func;
	const jit_stencil &sten = stencil[cmd];

	for (int i = 0; i < sten.hole_len; i++) {
		// 遍历并逐个填上空洞

		// 该空洞具体需要填入的64位偏移量
		long long offset = (long long) hole[sten.hole[i][0]] + sten.hole[i][2]
						 - ((long long) ptr + sten.hole[i][1]);

		int value = (int) offset; // 将64位偏移量截断为32位

		memcpy(ptr + sten.hole[i][1], &value, sizeof(int)); // 填补空洞
	}

	return (vm::jit_func_type) ptr;
}

#if JITRIX_JIT_MUSTTAIL == 1
inline vm::jit_func_type compile_addpc(vm::vm *m, unsigned int pc) {
	void *hole[jit_hole_max];

	unsigned char *ptr = (unsigned char *) m->jit_arena.alloc(
			sizeof(unsigned int) + stencil_addpc.code_len);
	// 申请一块新的内存用于存放pc偏移量和编译产物

	unsigned int *pc_addr = (unsigned int *) ptr;
	*pc_addr = pc;
	// 不同指令中同样意义的待填值
	hole[3] = &(m->pc_stack);
	hole[7] = pc_addr;

	ptr += sizeof(unsigned int); // 偏移到实际代码存储的地址

	memcpy(ptr, stencil_addpc.code, stencil_addpc.code_len);

	for (int i = 0; i < stencil_addpc.hole_len; i++) {
		// 遍历并逐个填上空洞

		// 该空洞具体需要填入的64位偏移量
		long long offset = (long long) hole[stencil_addpc.hole[i][0]] + stencil_addpc.hole[i][2]
						 - ((long long) ptr + stencil_addpc.hole[i][1]);

		int value = (int) offset; // 将64位偏移量截断为32位

		memcpy(ptr + stencil_addpc.hole[i][1], &value, sizeof(int)); // 填补空洞
	}

	return (vm::jit_func_type) ptr;
}
#endif

inline void compile(vm::vm *m, unsigned int code_addr) {
	// 以code_addr为首地址，生成一个JIT函数
	// 如果支持尾递归优化，则生成一个执行整个基本代码块的函数
	// 否则以单条指令为单位进行编译

#if JITRIX_JIT_MUSTTAIL != 1

	// 不使用尾递归，只一个个编译单条指令

	for (unsigned int code_len = m->code_len; code_addr < code_len; code_addr++) {
		void *hole[jit_hole_max];
		vm::command cmd = m->code[code_addr];

		if (cmd.cmd == vm::op_jump || cmd.cmd == vm::op_branch || cmd.cmd == vm::op_call
				|| cmd.cmd == vm::op_ret || next_cmd == vm::op_jr) {
			return;
		}

		m->code[code_addr].jit_func = compile_single_cmd(m, code_addr, hole);
		m->code[code_addr].jit_func = patch(m->code[code_addr].jit_func, cmd.cmd, hole);
	}

#else
	// 实现尾递归的整体做法是，让函数始终持有上一个编译的JIT函数和当前编译的JIT函数
	// 接着把当前生成的JIT函数填补到上一个JIT函数中
	// 最后用一个不需要尾递归的addpc填补到最后一个JIT函数中
	// 整体过程类似链表

	vm::jit_func_type prev_func, next_func;
	void *prev_hole[jit_hole_max], *next_hole[jit_hole_max];
	vm::COMMAND prev_cmd, next_cmd;

	m->code[code_addr].jit_func = prev_func = compile_single_cmd(
			m, code_addr, prev_hole); // 编译基本块的第一条指令
	prev_cmd = m->code[code_addr].cmd;

	unsigned int addr = code_addr + 1, code_len = m->code_len;

	while (addr < code_len) {
		next_cmd = m->code[addr].cmd;

		if (next_cmd == vm::op_jump || next_cmd == vm::op_branch || next_cmd == vm::op_call
				|| next_cmd == vm::op_ret || next_cmd == vm::op_jr) {
			//碰到控制流指令，结束编译
			break;
		}

		next_func = compile_single_cmd(m, addr, next_hole);

		vm::jit_func_type *next_func_ptr = (vm::jit_func_type *) m->jit_arena.alloc(
				sizeof(vm::jit_func_type *));

		*next_func_ptr = next_func;

		prev_hole[10] = (void *) next_func_ptr;

		patch(prev_func, prev_cmd, prev_hole);

		prev_func = next_func;
		memcpy(prev_hole, next_hole, sizeof(prev_hole));
		prev_cmd = next_cmd;

		addr++;
	}

	// 基本块编译完毕，现在填补addpc

	next_func = compile_addpc(m, addr - code_addr); // 程序计数器需要偏移的指令条数

	vm::jit_func_type *next_func_ptr = (vm::jit_func_type *) m->jit_arena.alloc(
			sizeof(vm::jit_func_type *));

	*next_func_ptr = next_func;

	prev_hole[10] = (void *) next_func_ptr;

	patch(prev_func, prev_cmd, prev_hole);

#endif
}

} // namespace jitrix::jit

#endif