CC=i686-w64-mingw32-gcc
LD=i686-w64-mingw32-ld
AR=i686-w64-mingw32-ar
STRIP=i686-w64-mingw32-strip

CPPFLAGS=\
	-I../../include \
	-DUSE_JIT \
	-DUSE_TRANSLATION

CFLAGS=\
	-O2 \
	-g0 \
	-ffast-math \
	-ftree-vectorize \
	-finput-charset=utf-8 \
	-fexec-charset=utf-8 \
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
	jit-common.o \
	jit-x86.o \
	command.o \
	cback.o \
	translation.o

all: linguine.exe

linguine.exe: $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(CFLAGS_EXTRA) $^

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

jit-x86.o: ../../src/jit/jit-x86.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

command.o: ../../src/cli/command.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

cback.o: ../../src/cli/cback.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

translation.o: ../../src/translation.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

clean:
	rm -rf *.o linguine.exe
