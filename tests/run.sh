#!/bin/sh

set -eu

echo "[MIPS 32-bit]"
cd ../build/linux
make clean
make CFLAGS=-static CC=mips-linux-gnu-gcc LD=mips-linux-gnu-ld AR=mips-linux-gnu-ar STRIP=mips-linux-gnu-strip linguine
cp linguine ../../tests/
cd ../../tests
./helper.sh qemu-mips-static
echo ""

echo "[MIPS 64-bit]"
cd ../build/linux
make clean
make CFLAGS=-static CC=mips64-linux-gnuabi64-gcc LD=mips64-linux-gnuabi64-ld AR=mips64-linux-gnuabi64-ar STRIP=mips64-linux-gnuabi64-strip linguine
cp linguine ../../tests/
cd ../../tests
./helper.sh qemu-mips64-static
echo ""

echo "[Arm 32-bit]"
cd ../build/linux
make clean
make CFLAGS=-static CC=arm-linux-gnueabihf-gcc LD=arm-linux-gnueabihf-ld AR=arm-linux-gnueabihf-ar STRIP=arm-linux-gnueabihf-strip linguine
cp linguine ../../tests/
cd ../../tests
./helper.sh qemu-armhf-static
echo ""

echo "[Arm 64-bit]"
cd ../build/linux
make clean
make CFLAGS=-static CC=aarch64-linux-gnu-gcc LD=aarch64-linux-gnu-ld AR=aarch64-linux-gnu-ar STRIP=aarch64-linux-gnu-strip linguine
cp linguine ../../tests/
cd ../../tests
./helper.sh qemu-arm64-static
echo ""

echo "[x86 32-bit]"
cd ../build/linux
make clean
make CFLAGS=-static CC=i686-linux-gnu-gcc LD=i686-linux-gnu-ld AR=i686-linux-gnu-ar STRIP=i686-linux-gnu-strip linguine
cp linguine ../../tests/
cd ../../tests
./helper.sh qemu-i386-static
echo ""

echo "[x86 64-bit]"
cd ../build/linux
make clean
make CFLAGS=-static CC=x86_64-linux-gnu-gcc LD=x86_64-linux-gnu-ld AR=x86_64-linux-gnu-ar STRIP=x86_64-linux-gnu-strip linguine
cp linguine ../../tests/
cd ../../tests
./helper.sh qemu-x86_64-static
echo ""

echo "[PowerPC 32-bit]"
cd ../build/linux
make clean
make CFLAGS=-static CC=powerpc-linux-gnu-gcc LD=powerpc-linux-gnu-ld AR=powerpc-linux-gnu-ar STRIP=powerpc-linux-gnu-strip linguine
cp linguine ../../tests/
cd ../../tests
./helper.sh qemu-ppc-static
echo ""

echo "[PowerPC 64-bit]"
cd ../build/linux
make clean
make CFLAGS=-static CC=powerpc64le-linux-gnu-gcc LD=powerpc64le-linux-gnu-ld AR=powerpc64le-linux-gnu-ar STRIP=powerpc64le-linux-gnu-strip linguine
cp linguine ../../tests/
cd ../../tests
./helper.sh qemu-ppc64el-static
echo ""
