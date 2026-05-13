#include "bin.hpp"
#include "runner.hpp"
#include "vm.hpp"
using namespace jitrix;

int main(int argc, char **argv) {
	// 解析参数
	if (argc != 2) {
		fprintf(stderr, "jitrix: error: bad bommand\n");
		return -1;
	}

	if (!strcmp(argv[1], "help")) {
		printf("usage: jitrix <filename>\n");
		return 0;
	}

	if (!strcmp(argv[1], "version")) {
		printf("jitrix version %d.%d.%d\n", VERSION_X, VERSION_Y, VERSION_Z);
		return 0;
	}

	// 读取字节码
	unsigned int size;

	auto code = bin::read_cmd_from_file("test.bin", size);

	// 虚拟机运行
	vm::vm *m = new vm::vm();
	m->init(code, size);
	m->run();

	// 销毁数据
	m->destroy();
	delete m;
	bin::destroy_cmd(code, size);

	// 退出main
	return 0;
}