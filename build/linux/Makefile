CC=cc
LD=ld
AR=ar
STRIP=strip
YACC=bison
LEX=flex

CPPFLAGS=\
	-I../../include \
	-DUSE_JIT \
	-DUSE_TRANSLATION

CFLAGS=\
	-static \
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
	parser.tab.o \
	lexer.yy.o \
	ast.o \
	hir.o \
	lir.o \
	runtime.o \
	interpreter.o \
	intrinsics.o \
	jit-arm64.o \
	jit-arm32.o \
	jit-x86_64.o \
	jit-x86.o \
	jit-ppc64.o \
	jit-ppc32.o \
	cback.o \
	translation.o

DLL_OBJS=\
	parser-pic.tab.o \
	lexer-pic.yy.o \
	ast-pic.o \
	hir-pic.o \
	lir-pic.o \
	runtime-pic.o \
	interpreter-pic.o \
	intrinsics-pic.o \
	jit-arm64-pic.o \
	jit-arm32-pic.o \
	jit-x86_64-pic.o \
	jit-x86-pic.o \
	jit-ppc64-pic.o \
	jit-ppc32-pic.o \
	cback-pic.o \
	translation-pic.o

CMD_OBJS=\
	command.o

all: linguine liblinguine.a liblinguine.so

linguine: $(CMD_OBJS) liblinguine.a
	$(CC) -o $@ $(CFLAGS) $^

liblinguine.so: $(DLL_OBJS)
	$(CC) -o $@ $(CFLAGS) -shared -fPIC $^
	$(STRIP) $@

liblinguine.a: $(LIB_OBJS)
	$(AR) rcs $@ $^

command.o: ../../src/command.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

command-pic.o: ../../src/command.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

lexer.yy.o: ../../src/lexer.yy.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

lexer-pic.yy.o: ../../src/lexer.yy.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

parser.tab.o: ../../src/parser.tab.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

parser-pic.tab.o: ../../src/parser.tab.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

ast.o: ../../src/ast.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

ast-pic.o: ../../src/ast.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

hir.o: ../../src/hir.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

hir-pic.o: ../../src/hir.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

lir.o: ../../src/lir.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

lir-pic.o: ../../src/lir.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

runtime.o: ../../src/runtime.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

runtime-pic.o: ../../src/runtime.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

interpreter.o: ../../src/interpreter.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

interpreter-pic.o: ../../src/interpreter.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

intrinsics.o: ../../src/intrinsics.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

intrinsics-pic.o: ../../src/intrinsics.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

jit-arm64.o: ../../src/jit-arm64.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-arm64-pic.o: ../../src/jit-arm64.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

jit-arm32.o: ../../src/jit-arm32.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-arm32-pic.o: ../../src/jit-arm32.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

jit-x86_64.o: ../../src/jit-x86_64.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-x86_64-pic.o: ../../src/jit-x86_64.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

jit-x86.o: ../../src/jit-x86.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-x86-pic.o: ../../src/jit-x86.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

jit-ppc64.o: ../../src/jit-ppc64.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-ppc64-pic.o: ../../src/jit-ppc64.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

jit-ppc32.o: ../../src/jit-ppc32.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-ppc32-pic.o: ../../src/jit-ppc32.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

cback.o: ../../src/cback.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

cback-pic.o: ../../src/cback.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

translation.o: ../../src/translation.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

translation-pic.o: ../../src/translation.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) -fPIC $<

../../src/lexer.yy.c: ../../src/lexer.l
	$(LEX) -o ../../src/lexer.yy.c --prefix=ast_yy ../../src/lexer.l

../../src/parser.tab.c: ../../src/parser.y
	$(YACC) -Wcounterexamples -d -p ast_yy -o ../../src/parser.tab.c ../../src/parser.y

clean:
	rm -rf *.o liblinguine.a liblinguine.so linguine
