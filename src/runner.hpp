#pragma once

#include "arena.hpp"
#include "config.hpp"
#include "jit.hpp"
#include "vm.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace jitrix::vm {

inline void vm::init(command *_code, unsigned int _code_len) {
	// 传递参数

	// 初始化代码
	code_len = _code_len;
	code = _code;
	// 初始化栈
	pc_stack = pc_stack_buf + 1;
	op_stack = op_stack_buf + 1;
	// 清零
	memset(reg, 0, sizeof(reg));
	memset(mem, 0, sizeof(mem));
	memset(pc_stack_buf, 0, sizeof(pc_stack_buf));
	memset(op_stack_buf, 0, sizeof(op_stack_buf));
	// 初始化JIT

#if JITRIX_JIT_ENABLED == 1
	jump_counter = (unsigned int *) calloc(code_len, sizeof(command));
	jit_arena.init();
#endif
}

inline void vm::destroy() {
#if JITRIX_JIT_ENABLED == 1
	free(jump_counter);
	jit_arena.destroy();
#endif
}

#if JITRIX_JIT_ENABLED == 1
inline void vm::jump_record(unsigned int addr) {
	if (code[addr].jit_func) {
		// 已jit过，无记录必要
		return;
	}

	jump_counter[addr]++;

	if (jump_counter[addr] >= 1000) {
		// 跳转次数超过一千次则此段代码为热代码，开始编译该基本代码块
		jit::compile(this, addr);
	}
}
#endif

inline int vm::run() {
	// 从第一条指令开始运行程序，返回运行后的reg[0]
	// 出错则返回-1，且将错误信息存储到error中

	for (;;) {
		// 用于简写的别名
		unsigned int &pc = *pc_stack;
		command &cmd = code[pc];
		int *arg = cmd.arg;

#if JITRIX_JIT_ENABLED == 1
		if (cmd.jit_func) {
			// 该条指令已被JIT编译过
			cmd.jit_func();
			continue;
		}
#endif

		switch (cmd.cmd) {
		case op_load:
			reg[arg[0]] = mem[reg[arg[1]]];
			break;

		case op_store:
			mem[reg[arg[0]]] = reg[arg[1]];
			break;

		case op_move:
			reg[arg[0]] = reg[arg[1]];
			break;

		case op_set:
			reg[arg[0]] = arg[1];
			break;

		case op_push:
			*(op_stack++) = reg[arg[0]];
			break;

		case op_pop:
			reg[arg[0]] = *(--op_stack);
			break;

		case op_add:
			reg[arg[0]] = reg[arg[1]] + reg[arg[2]];
			break;

		case op_sub:
			reg[arg[0]] = reg[arg[1]] - reg[arg[2]];
			break;

		case op_mul:
			reg[arg[0]] = reg[arg[1]] * reg[arg[2]];
			break;

		case op_div:
			reg[arg[0]] = reg[arg[1]] / reg[arg[2]];
			break;

		case op_and:
			reg[arg[0]] = reg[arg[1]] & reg[arg[2]];
			break;

		case op_or:
			reg[arg[0]] = reg[arg[1]] | reg[arg[2]];
			break;

		case op_invert:
			reg[arg[0]] = ~reg[arg[1]];
			break;

		case op_not:
			reg[arg[0]] = !reg[arg[1]];
			break;

		case op_compare: {
			// 使用一种数学方法实现无分支比较
			int less = reg[arg[1]] < reg[arg[2]]; // 若a<b则为1
			int greater = reg[arg[1]] > reg[arg[2]]; // 若 a>b则为1
			reg[arg[0]] = 1 << (greater - less + 1); // 结果与朴素的分支比较一致
			break;
		}

		case op_input:
			scanf("%d", &reg[arg[0]]);
			break;

		case op_output:
			printf("%d\n", reg[arg[0]]);
			break;

		case op_branch:
			if (reg[arg[0]] & arg[1]) {
				pc = arg[2];
			} else {
				pc++;
			}

#if JITRIX_JIT_ENABLED == 1
			jump_record(pc);
#endif

			continue;

		case op_jump:
			pc = arg[0];

#if JITRIX_JIT_ENABLED == 1
			jump_record(pc);
#endif

			continue;

		case op_jr:
			pc = reg[arg[0]];

#if JITRIX_JIT_ENABLED == 1
			jump_record(pc);
#endif

			continue;

		case op_call:
			*(++pc_stack) = arg[0];

#if JITRIX_JIT_ENABLED == 1
			jump_record(*pc_stack);
#endif

			continue;

		case op_ret:
			pc_stack--;

			if (pc_stack == pc_stack_buf) {
				return reg[0];
			}

			(*pc_stack)++;

#if JITRIX_JIT_ENABLED == 1
			jump_record(*pc_stack);
#endif

			continue;

		default:
			break;
		}

		++(*pc_stack);
	}
}

} // namespace jitrix::vm