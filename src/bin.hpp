#pragma once
#include "arena.hpp"
#include "vm.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace jitrix::bin {

vm::command *read_cmd_from_mem(void *mem, unsigned int &size) {
	// 将内存转为无符号字节流
	unsigned char *stream = (unsigned char *) mem;

	// 前4字节为指令条数
	memcpy(&size, stream, sizeof(unsigned int));
	stream += sizeof(unsigned int);

	// 为指令数组分配内存，需要分配到mmap防止JIT链接失败
	vm::command *cmd = (vm::command *) arena::alloc_mmap(size * sizeof(vm::command));

	// 逐条解码指令
	for (unsigned int i = 0; i < size; ++i) {
		cmd[i].jit_func = NULL;
		// 一字节的操作码
		cmd[i].cmd = (vm::COMMAND) stream[0];
		stream++;
		// 操作数，每个4字节
		memcpy(cmd[i].arg, stream, vm::command_arg_max * sizeof(int));
		stream += vm::command_arg_max * sizeof(int);
	}

	// 返回指令条数及数组指针
	return cmd;
}

vm::command *read_cmd_from_file(const char *filename, unsigned int &size) {
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		return NULL;
	}

	// 获取文件总大小
	fseek(fp, 0, SEEK_END);
	unsigned int file_size = ftell(fp);
	if (file_size < 4) { // 至少要有 4 字节存放指令条数
		fclose(fp);
		return NULL;
	}
	fseek(fp, 0, SEEK_SET);

	// 读取前 4 字节作为指令条数
	unsigned int count;
	if (fread(&count, sizeof(count), 1, fp) != 1) {
		fclose(fp);
		return NULL;
	}

	// 计算符合规范的预期文件大小：4(条数) + count * (1(操作码) + 参数个数*4(操作数))
	// 操作数固定为 4 字节，直接使用字面量 4
	unsigned int expected_size = 4 + count * (1 + vm::command_arg_max * 4);
	if (file_size != expected_size) {
		fclose(fp);
		return NULL;
	}

	// 分配缓冲区，读取整个文件内容
	unsigned char *buffer = (unsigned char *) malloc(file_size);
	fseek(fp, 0, SEEK_SET);
	if (fread(buffer, 1, file_size, fp) != file_size) {
		free(buffer);
		fclose(fp);
		return NULL;
	}

	fclose(fp);

	// 调用内存解析函数，得到指令数组
	vm::command *cmd = read_cmd_from_mem(buffer, size);
	free(buffer); // 释放临时缓冲区
	return cmd;
}

void destroy_cmd(vm::command *cmd, unsigned int len) {
	// 销毁字节码
	arena::free_mmap(cmd, len * sizeof(vm::command));
}

} // namespace jitrix::bin