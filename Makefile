#
# Linguine Generic Makefile
#

TARGET=linguine
PREFIX=/usr/local

CC=cc
LD=ld
AR=ar
STRIP=strip
YACC=bison
LEX=flex

CPPFLAGS=\
	-Iinclude \
	-DUSE_JIT \
	-DUSE_TRANSLATION

CFLAGS=\
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
	-Wno-strict-aliasing \
	-Wno-stringop-truncation

LDFLAGS=-lm

OBJS=\
	obj/parser.tab.o \
	obj/lexer.yy.o \
	obj/ast.o \
	obj/hir.o \
	obj/lir.o \
	obj/runtime.o \
	obj/interpreter.o \
	obj/intrinsics.o \
	obj/jit-common.o \
	obj/jit-x86_64.o \
	obj/jit-x86.o \
	obj/jit-arm64.o \
	obj/jit-arm32.o \
	obj/jit-ppc64.o \
	obj/jit-ppc32.o \
	obj/jit-mips64.o \
	obj/jit-mips32.o \
	obj/command.o \
	obj/cback.o \
	obj/translation.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(CFLAGS_EXTRA) $^

obj/lexer.yy.o: src/lexer.yy.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/parser.tab.o: src/parser.tab.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/ast.o: src/ast.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/hir.o: src/hir.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/lir.o: src/lir.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/runtime.o: src/runtime.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/interpreter.o: src/interpreter.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/intrinsics.o: src/intrinsics.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/jit-common.o: src/jit/jit-common.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/jit-x86_64.o: src/jit/jit-x86_64.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/jit-x86.o: src/jit/jit-x86.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/jit-arm64.o: src/jit/jit-arm64.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/jit-arm32.o: src/jit/jit-arm32.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/jit-ppc64.o: src/jit/jit-ppc64.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/jit-ppc32.o: src/jit/jit-ppc32.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/jit-mips64.o: src/jit/jit-mips64.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/jit-mips32.o: src/jit/jit-mips32.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/command.o: src/cli/command.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/cback.o: src/cli/cback.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/translation.o: src/translation.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

src/lexer.yy.c: src/lexer.l
	$(LEX) -o src/lexer.yy.c --prefix=ast_yy src/lexer.l

src/parser.tab.c: src/parser.y
	$(YACC) -Wcounterexamples -d -p ast_yy -o src/parser.tab.c src/parser.y

obj:
	mkdir -p obj

install:
	@install -v $(TARGET) $(PREFIX)/bin/$(TARGET)

clean:
	rm -rf *~ src/*~ obj $(TARGET)
