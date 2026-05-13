BIN = jitrix
COMPILER = clang++

all: sten
	$(COMPILER) src/main.cpp \
			-o $(BIN) \
			-O3 -lm -static \
			-std=c++17 \
			-I src \
			-I stencil
	strip $(BIN)

debug: sten
	$(COMPILER) src/main.cpp \
			-o $(BIN) \
			-O0 -g -lm -static \
			-std=c++17 \
			-I src \
			-I stencil

sten:
	clang++ -c stencil/stencil.cpp \
			-o stencil/stencil.o \
			-O3 -fno-plt -fno-pic -static --target=x86_64-linux-gnu \
			-I src \
			-I stencil \
			-std=c++17
	llvm-objdump -d -r -C stencil/stencil.o > stencil/stencil.objdump
	cd stencil && python codegen.py stencil.objdump jit_data.hpp