#
# Linguine Generic Makefile
#

TARGET_EXE=linguine
TARGET_LIB=liblinguine.a
PREFIX=/usr/local

CC=cc
LD=ld
AR=ar
STRIP=strip
YACC=bison
LEX=flex

CPPFLAGS=\
	-Iinclude \
	-DUSE_JIT

CFLAGS=\
	-O0 \
	-g3 \
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

LIB_OBJS=\
	obj/parser.tab.o \
	obj/lexer.yy.o \
	obj/ast.o \
	obj/hir.o \
	obj/lir.o \
	obj/runtime.o \
	obj/interpreter.o \
	obj/intrinsics.o \
	obj/jit-arm64.o \
	obj/jit-arm32.o \
	obj/jit-x86_64.o \
	obj/jit-x86.o \
	obj/cback.o \
	obj/translation.o

CMD_OBJS=\
	obj/command.o

all: $(TARGET_EXE) $(TARGET_LIB)

$(TARGET_EXE): $(CMD_OBJS) $(TARGET_LIB)
	$(CC) -o $@ $(CFLAGS) $^

$(TARGET_LIB): $(LIB_OBJS)
	$(AR) rcs $@ $^

obj/command.o: src/command.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

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

obj/jit-arm64.o: src/jit-arm64.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/jit-arm32.o: src/jit-arm32.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/jit-x86_64.o: src/jit-x86_64.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/jit-x86.o: src/jit-x86.c obj
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

obj/cback.o: src/cback.c obj
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
	@install -v $(TARGET_EXE) $(PREFIX)/bin/$(TARGET_EXE)
	@install -v $(TARGET_EXE) $(PREFIX)/lib/$(TARGET_LIB)
	@install -v -d $(PREFIX)/include/linguine
	@install -v include/linguine/compat.h $(PREFIX)/include/linguine/compat.h
	@install -v include/linguine/ast.h $(PREFIX)/include/linguine/ast.h
	@install -v include/linguine/hir.h $(PREFIX)/include/linguine/hir.h
	@install -v include/linguine/lir.h $(PREFIX)/include/linguine/lir.h
	@install -v include/linguine/runtime.h $(PREFIX)/include/linguine/runtime.h
	@install -v include/linguine/cback.h $(PREFIX)/include/linguine/cback.h

clean:
	rm -rf *~ src/*~ obj $(TARGET_EXE) $(TARGET_LIB)
