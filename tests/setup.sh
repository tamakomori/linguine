#!/bin/sh

echo "Installing..."

sudo apt-get install -y \
     qemu-user \
     qemu-user-binfmt \
     crossbuild-essential-i386 \
     crossbuild-essential-amd64 \
     crossbuild-essential-powerpc \
     crossbuild-essential-ppc64el \
     crossbuild-essential-armel \
     crossbuild-essential-arm64
