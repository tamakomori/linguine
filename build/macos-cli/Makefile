CC=cc
LD=ld
AR=ar
YACC=bison
LEX=flex

CPPFLAGS=\
	-I../../include \
	-DUSE_JIT \
	-DUSE_TRANSLATION

CFLAGS=\
	-arch arm64 \
	-arch x86_64 \
	-O2 \
	-g0 \
	-ffast-math \
	-ftree-vectorize \
	-std=gnu11 \
	-Wall \
	-Werror \
	-Wextra \
	-Wundef \
	-Wconversion \
	-Wno-multichar \
	-Wno-strict-aliasing

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
	jit-x86_64.o \
	jit-arm64.o \
	command.o \
	cback.o \
	translation.o

all: linguine

linguine: $(OBJS)
	$(CC) -o linguine -arch arm64 -arch x86_64 $(CFLAGS) $^

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

jit-arm64.o: ../../src/jit/jit-arm64.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-x86_64.o: ../../src/jit/jit-x86_64.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

command.o: ../../src/cli/command.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

cback.o: ../../src/cli/cback.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

translation.o: ../../src/translation.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

../../src/parser.tab.c: ../../src/parser.y
	$(YACC) -Wcounterexamples -d -p ast_yy -o ../../src/parser.tab.c ../../src/parser.y

../../src/lexer.yy.c: ../../src/lexer.l
	$(LEX) -o ../../src/lexer.yy.c --prefix=ast_yy ../../src/lexer.l

clean:
	rm -rf *.o linguine
