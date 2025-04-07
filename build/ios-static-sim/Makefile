CC=clang
AR=ar

CPPFLAGS=\
	-I../../include

CFLAGS=\
	-Os \
	-g0 \
	-isysroot `xcrun -sdk iphonesimulator --show-sdk-path` \
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

all: liblinguine.a

#
# The `ar` command doesn't support archiving `Universal Binary` object files.
# We fuse two architecture-specific archives into a single archive by using `lipo` command.
#
liblinguine.a: liblinguine-arm64.a liblinguine-x86_64.a
	lipo -create liblinguine-arm64.a liblinguine-x86_64.a -output liblinguine.a

#
# The following are for arm64 architecture-specific.
#

liblinguine-arm64.a: parser-arm64.tab.o lexer-arm64.yy.o ast-arm64.o hir-arm64.o lir-arm64.o runtime-arm64.o interpreter-arm64.o intrinsics-arm64.o jit-common-arm64.o
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

jit-common-arm64.o: ../../src/jit/jit-common.c
	$(CC) -c -arch arm64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

#
# The following are for x86_64.
#

liblinguine-x86_64.a: parser-x86_64.tab.o lexer-x86_64.yy.o ast-x86_64.o hir-x86_64.o lir-x86_64.o runtime-x86_64.o interpreter-x86_64.o intrinsics-x86_64.o jit-common-x86_64.o
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

jit-common-x86_64.o: ../../src/jit/jit-common.c
	$(CC) -c -arch x86_64 -o $@ $(CPPFLAGS) $(CFLAGS) $<

clean:
	rm -rf *.o *.a
