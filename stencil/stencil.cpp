#include "config.hpp"

extern int *jit_arg_0, *jit_arg_1, *jit_arg_2;
extern unsigned int *jit_arg_3;
extern int jit_arg_4, jit_arg_5, jit_arg_6;
extern unsigned int jit_arg_7;
extern int (*jit_arg_8)();
extern void (*jit_arg_9)(int x);
extern void (*jit_arg_10)();

#define mem jit_arg_0
#define reg jit_arg_1
#define op_stack jit_arg_2
#define pc_stack jit_arg_3
#define i1 jit_arg_4
#define i2 jit_arg_5
#define i3 jit_arg_6
#define u1 jit_arg_7
#define input jit_arg_8
#define output jit_arg_9

// 使用musttail进行优化
//  检测编译器是否支持 musttail
#if JITRIX_JIT_MUSTTAIL == 1 && (defined(__clang__) || defined(__GNUC__))
#define NEXT __attribute__((musttail)) return jit_arg_10()
#else
#define NEXT ++(*pc_stack)
#endif

// void jit_load() {
// 	reg[arg[0]] = mem[reg[arg[1]]];
// 	++(*pc_stack);
// }

void jit_load() {
	i1 = mem[u1];
	NEXT;
}

// void jit_store() {
// 	mem[reg[arg[0]]] = reg[arg[1]];
// 	++(*pc_stack);
// }

void jit_store() {
	mem[u1] = i1;
	NEXT;
}

// void jit_move() {
// 	reg[arg[0]] = reg[arg[1]];
// 	++(*pc_stack);
// }

void jit_move() {
	i1 = i2;
	NEXT;
}

// void jit_set() {
// 	reg[arg[0]] = arg[1];
// 	++(*pc_stack);
// }

void jit_set() {
	i1 = i2;
	NEXT;
}

// void jit_push() {
// 	*(op_stack++) = reg[arg[0]];
// 	++(*pc_stack);
// }

void jit_push() {
	*(op_stack++) = i1;
	NEXT;
}

// void jit_pop() {
// 	reg[arg[0]] = *(--op_stack);
// 	++(*pc_stack);
// }

void jit_pop() {
	i1 = *(--op_stack);
	NEXT;
}

// void jit_add() {
// 	reg[arg[0]] = reg[arg[1]] + reg[arg[2]];
// 	++(*pc_stack);
// }

void jit_add() {
	i1 = i2 + i3;
	NEXT;
}

// void jit_sub() {
// 	reg[arg[0]] = reg[arg[1]] - reg[arg[2]];
// 	++(*pc_stack);
// }

void jit_sub() {
	i1 = i2 - i3;
	NEXT;
}

// void jit_mul() {
// 	reg[arg[0]] = reg[arg[1]] * reg[arg[2]];
// 	++(*pc_stack);
// }

void jit_mul() {
	i1 = i2 * i3;
	NEXT;
}

// void jit_div() {
// 	reg[arg[0]] = reg[arg[1]] / reg[arg[2]];
// 	++(*pc_stack);
// }

void jit_div() {
	i1 = i2 / i3;
	NEXT;
}

// void jit_and() {
// 	reg[arg[0]] = reg[arg[1]] & reg[arg[2]];
// 	++(*pc_stack);
// }

void jit_and() {
	i1 = i2 & i3;
	NEXT;
}

// void jit_or() {
// 	reg[arg[0]] = reg[arg[1]] | reg[arg[2]];
// 	++(*pc_stack);
// }

void jit_or() {
	i1 = i2 | i3;
	NEXT;
}

// void jit_invert() {
// 	reg[arg[0]] = ~reg[arg[1]];
// 	++(*pc_stack);
// }

void jit_invert() {
	i1 = ~i2;
	NEXT;
}

// void jit_not() {
// 	reg[arg[0]] = ~reg[arg[1]];
// 	++(*pc_stack);
// }

void jit_not() {
	i1 = !i2;
	NEXT;
}

// void jit_compare() {
// 	// 使用一种数学方法实现无分支比较
// 	int less = reg[arg[1]] < reg[arg[2]]; // 若a<b则为1
// 	int greater = reg[arg[1]] > reg[arg[2]]; // 若 a>b则为1
// 	reg[arg[0]] = 1 << (greater - less + 1); // 结果与朴素的分支比较一致
// 	++(*pc_stack);
// }

void jit_compare() {
	int less = i1 < i2;
	int greater = i1 > i2;
	i3 = 1 << (greater - less + 1);
	NEXT;
}

// void jit_input() {
// 	scanf("%d", &reg[arg[0]]);
// 	++(*pc_stack);
// }

void jit_input() {
	i1 = input();
	NEXT;
}

// void jit_input() {
// 	printf("%d\n", reg[arg[0]]);
// 	++(*pc_stack);
// }

void jit_output() {
	output(i1);
	NEXT;
}

// void jit_addpc() {
// 	(*pc_stack) += arg[1];
// }

void jit_addpc() {
	(*pc_stack) += u1;
}