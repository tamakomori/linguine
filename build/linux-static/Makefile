CC=cc
LD=ld
AR=ar
STRIP=strip

CPPFLAGS=\
	-I../../include \
	-DUSE_JIT

CFLAGS=\
	-O2 \
	-g0 \
	-fPIC \
	-ffast-math \
	-ftree-vectorize \
	-std=gnu11 \
	-Wall \
	-Werror \
	-Wextra \
	-Wundef \
	-Wconversion \
	-Wno-multichar \
	-Wno-strict-aliasing \
	-Wno-stringop-truncation


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
	jit-x86_64.o \
	jit-x86.o \
	jit-arm64.o \
	jit-arm32.o \
	jit-ppc64.o \
	jit-ppc32.o \
	jit-mips64.o \
	jit-mips32.o

all: liblinguine.a

liblinguine.a: $(OBJS)
	$(AR) rcs $@ $^

lexer.yy.o: ../../src/lexer.yy.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

parser.tab.o: ../../src/parser.tab.c
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

jit-arm64.o: ../../src/jit/jit-arm64.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-arm32.o: ../../src/jit/jit-arm32.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-x86_64.o: ../../src/jit/jit-x86_64.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-x86.o: ../../src/jit/jit-x86.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-ppc64.o: ../../src/jit/jit-ppc64.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-ppc32.o: ../../src/jit/jit-ppc32.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-mips64.o: ../../src/jit/jit-mips64.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-mips32.o: ../../src/jit/jit-mips32.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

clean:
	rm -rf *.o liblinguine.a
