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

all: linguine

#
# These targets are Universal Binary objects.
#

linguine: command.o liblinguine.a
	$(CC) -o linguine -arch arm64 -arch x86_64 $(CFLAGS) $^

command.o: ../../src/command.c
	$(CC) -c -arch arm64 -arch x86_64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

#
# The `ar` command doesn't support archiving `Universal Binary` object files.
# We fuse two architecture-specific archives into a single archive by using `lipo` command.
#
liblinguine.a: liblinguine-arm64.a liblinguine-x86_64.a
	lipo -create liblinguine-arm64.a liblinguine-x86_64.a -output liblinguine.a

#
# The following are for arm64 architecture-specific.
#

liblinguine-arm64.a: parser-arm64.tab.o lexer-arm64.yy.o ast-arm64.o hir-arm64.o lir-arm64.o runtime-arm64.o interpreter-arm64.o intrinsics-arm64.o jit-arm64.o cback-arm64.o translation-arm64.o
	$(AR) rcs $@ $^

parser-arm64.tab.o: ../../src/parser.tab.c
	$(CC) -c -arch arm64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

lexer-arm64.yy.o: ../../src/lexer.yy.c
	$(CC) -c -arch arm64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

ast-arm64.o: ../../src/ast.c
	$(CC) -c -arch arm64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

hir-arm64.o: ../../src/hir.c
	$(CC) -c -arch arm64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

lir-arm64.o: ../../src/lir.c
	$(CC) -c -arch arm64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

runtime-arm64.o: ../../src/runtime.c
	$(CC) -c -arch arm64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

interpreter-arm64.o: ../../src/interpreter.c
	$(CC) -c -arch arm64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

intrinsics-arm64.o: ../../src/intrinsics.c
	$(CC) -c -arch arm64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-arm64.o: ../../src/jit-arm64.c
	$(CC) -c -arch arm64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

cback-arm64.o: ../../src/cback.c
	$(CC) -c -arch arm64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

translation-arm64.o: ../../src/translation.c
	$(CC) -c -arch arm64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

#
# The following are for x86_64.
#

liblinguine-x86_64.a: parser-x86_64.tab.o lexer-x86_64.yy.o ast-x86_64.o hir-x86_64.o lir-x86_64.o runtime-x86_64.o interpreter-x86_64.o intrinsics-x86_64.o jit-x86_64.o cback-x86_64.o translation-x86_64.o
	$(AR) rcs $@ $^

parser-x86_64.tab.o: ../../src/parser.tab.c
	$(CC) -c -arch x86_64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

lexer-x86_64.yy.o: ../../src/lexer.yy.c
	$(CC) -c -arch x86_64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

ast-x86_64.o: ../../src/ast.c
	$(CC) -c -arch x86_64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

hir-x86_64.o: ../../src/hir.c
	$(CC) -c -arch x86_64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

lir-x86_64.o: ../../src/lir.c
	$(CC) -c -arch x86_64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

runtime-x86_64.o: ../../src/runtime.c
	$(CC) -c -arch x86_64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

interpreter-x86_64.o: ../../src/interpreter.c
	$(CC) -c -arch x86_64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

intrinsics-x86_64.o: ../../src/intrinsics.c
	$(CC) -c -arch x86_64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

jit-x86_64.o: ../../src/jit-x86_64.c
	$(CC) -c -arch x86_64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

cback-x86_64.o: ../../src/cback.c
	$(CC) -c -arch x86_64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

translation-x86_64.o: ../../src/translation.c
	$(CC) -c -arch x86_64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

#
# The following are not architecture-specific.
#

../../src/parser.tab.c: ../../src/parser.y
	$(YACC) -Wcounterexamples -d -p ast_yy -o ../../src/parser.tab.c ../../src/parser.y

../../src/lexer.yy.c: ../../src/lexer.l
	$(LEX) -o ../../src/lexer.yy.c --prefix=ast_yy ../../src/lexer.l

#
# Phony.
#

clean:
	rm -rf *.o *.a linguine
