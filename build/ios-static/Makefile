CC=clang
AR=ar

CPPFLAGS=\
	-I../../include

CFLAGS=\
	-Os \
	-g0 \
	-arch arm64 \
	-isysroot `xcrun -sdk iphonesimulator --show-sdk-path` \
	-fembed-bitcode \
	-mios-version-min=12.0 \
	-ffast-math \
	-ftree-vectorize \
	-std=gnu11 \
	-Wall \
	-Werror \
	-Wextra \
	-Wundef \
	-Wconversion \
	-Wno-multichar

LDFLAGS=-lm

OBJS=\
	parser.tab.o \
	lexer.yy.o \
	ast.o \
	hir.o \
	lir.o \
	runtime.o \
	interpreter.o \
	intrinsics.o \
	jit-common.o

all: liblinguine.a

liblinguine.a: $(OBJS)
	$(AR) rcs $@ $^

parser.tab.o: ../../src/parser.tab.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

lexer.yy.o: ../../src/lexer.yy.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

ast.o: ../../src/ast.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

hir.o: ../../src/hir.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

lir.o: ../../src/lir.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

runtime.o: ../../src/runtime.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

interpreter.o: ../../src/interpreter.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

intrinsics.o: ../../src/intrinsics.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-common.o: ../../src/jit/jit-common.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

clean:
	rm -rf *.o *.a
