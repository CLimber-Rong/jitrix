#pragma once
#include <stdlib.h>

namespace jitrix::arena {

// 维护一个arena链表来管理jit生成的函数

constexpr unsigned int arena_size = 1024 * 1024; // arena大小

// 这两个接口需要根据不同平台做不同的实现
void *alloc_arena(); // 分配一片可执行的arena
void free_arena(void *); // 释放arena
void *alloc_mmap(unsigned int); // 在mmap上申请一块内存
void free_mmap(void *, unsigned int); // 在mmap上释放一块内存

struct arena_node {
	void *data; // arena地址
	int offset; // arena偏移量
	arena_node *next;
};

arena_node *create_arena_node() {
	arena_node *node = (arena_node *) malloc(sizeof(arena_node));
	node->next = NULL;
	node->offset = 0;
	node->data = alloc_arena();
	return node;
}

void destroy_arena_node(arena_node *node) {
	free_arena(node->data);
	free(node);
}

struct arena {
	arena_node *head;
	arena_node *tail;
	void init();
	void add();
	void destroy();
	void *alloc(unsigned int size);
};

} // namespace jitrix::arena

namespace jitrix::arena {

void arena::init() {
	tail = head = create_arena_node();
}

void arena::add() {
	tail->next = create_arena_node();
	tail = tail->next;
}

void arena::destroy() {
	arena_node *node = head;
	while (node) {
		arena_node *next = node->next;
		destroy_arena_node(node);
		node = next;
	}
}

void *arena::alloc(unsigned int size) {
	void *rst;

	if (tail->offset + size < arena_size) {
		// 空闲充足，直接分配
		rst = ((char *) tail->data) + tail->offset;
	} else {
		// 当前空闲大小不足，直接新增arena
		add();
		rst = tail->data;
	}

	tail->offset += size;
	return rst;
}

} // namespace jitrix::arena

// 根据平台选择不同的实现
#ifdef _WIN32
#include <windows.h>

namespace jitrix::arena {

void *alloc_arena() {
	return VirtualAlloc(NULL, // 让系统决定起始地址
			arena_size, // 要分配的内存大小
			MEM_COMMIT | MEM_RESERVE, // 提交并保留物理存储
			PAGE_EXECUTE_READWRITE // 设置为可读写且可执行
	);
}

void free_arena(void *arena) {
	VirtualFree(arena, 0, MEM_RELEASE);
}

void *alloc_mmap(unsigned int size) {
	return VirtualAlloc(NULL, // 让系统决定起始地址
			arena_size, // 要分配的内存大小
			MEM_COMMIT | MEM_RESERVE, // 提交并保留物理存储
			PAGE_EXECUTE_READWRITE // 设置为可读写且可执行
	);
}

void free_mmap(void *ptr, unsigned int size) {
	VirtualFree(ptr, 0, MEM_RELEASE);
}
} // namespace jitrix::arena

#elif __linux__
#include <sys/mman.h>
#include <unistd.h>

namespace jitrix::arena {

void *alloc_arena() {
	return mmap(NULL, // 让系统决定起始地址
			arena_size, // 要分配的内存大小
			PROT_READ | PROT_WRITE | PROT_EXEC, // 设置为可读写且可执行
			MAP_PRIVATE | MAP_ANONYMOUS, // 私有匿名映射
			-1, // 没有关联的文件
			0 // 无关联文件，映射量为0
	);
}

void free_arena(void *arena) {
	munmap(arena, arena_size);
}

void *alloc_mmap(unsigned int size) {
	return mmap(NULL, // 让系统决定起始地址
			size, // 要分配的内存大小
			PROT_READ | PROT_WRITE | PROT_EXEC, // 设置为可读写且可执行
			MAP_PRIVATE | MAP_ANONYMOUS, // 私有匿名映射
			-1, // 没有关联的文件
			0 // 无关联文件，映射量为0
	);
}

void free_mmap(void *ptr, unsigned int size) {
	munmap(ptr, size);
}
} // namespace jitrix::arena

#endif