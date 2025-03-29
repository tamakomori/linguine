#!/bin/sh

set -eu

sudo apt-get install -y \
     qemu-user \
     qemu-user-binfmt \
     crossbuild-essential-i386 \
     crossbuild-essential-amd64 \
     crossbuild-essential-powerpc \
     crossbuild-essential-ppc64el \
     crossbuild-essential-armhf \
     crossbuild-essential-arm64

echo "[PowerPC 32-bit]"
cd ../build/linux
make clean
make CFLAGS=-static CC=powerpc-linux-gnu-gcc LD=powerpc-linux-gnu-ld AR=powerpc-linux-gnu-ar STRIP=powerpc-linux-gnu-strip linguine
cp linguine ../../tests/
cd ../../tests
./run-test.sh
echo ""

echo "[PowerPC 64-bit]"
cd ../build/linux
make clean
make CFLAGS=-static CC=powerpc64le-linux-gnu-gcc LD=powerpc64le-linux-gnu-ld AR=powerpc64le-linux-gnu-ar STRIP=powerpc64le-linux-gnu-strip linguine
cp linguine ../../tests/
cd ../../tests
./run-test.sh
echo ""

echo "[x86 32-bit]"
cd ../build/linux
make clean
make CFLAGS=-static CC=i686-linux-gnu-gcc LD=i686-linux-gnu-ld AR=i686-linux-gnu-ar STRIP=i686-linux-gnu-strip linguine
cp linguine ../../tests/
cd ../../tests
./run-test.sh
echo ""

echo "[x86 64-bit]"
cd ../build/linux
make clean
make CFLAGS=-static CC=x86_64-linux-gnu-gcc LD=x86_64-linux-gnu-ld AR=x86_64-linux-gnu-ar STRIP=x86_64-linux-gnu-strip linguine
cp linguine ../../tests/
cd ../../tests
./run-test.sh
echo ""
