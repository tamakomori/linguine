#!/bin/sh

set -eu

echo "Enter the version to release."
read VERSION

echo "Press enter to release the version $VERSION."
read str

TARGET_DIR="`pwd`/linguine-$VERSION"
TARGET_TGZ="`pwd`/linguine-$VERSION.tar.gz"
TARGET_ZIP="`pwd`/linguine-$VERSION.zip"

rm -rf "$TARGET_DIR" "$TARGET_ZIP" "$TARGET_TGZ"
mkdir "$TARGET_DIR"
mkdir -p "$TARGET_DIR/windows"
mkdir -p "$TARGET_DIR/macos"

echo 'Building for Windows 32bit...'
cd build/win32
make clean
make -j$(nproc)
cd ../..

echo 'Building for macOS...'
cd build/macos
make clean
make -j$(nproc)
cd ../..

cp build/win32/linguine.exe "$TARGET_DIR/windows/"
cp build/macos/linguine "$TARGET_DIR/macos/"
cp README.md "$TARGET_DIR/"
cp hello.ls "$TARGET_DIR/"
cp LICENSE "$TARGET_DIR/"
cp RELNOTE.txt "$TARGET_DIR/"

zip -9 -r "$TARGET_ZIP" "linguine-$VERSION"
tar czf "$TARGET_TGZ" "linguine-$VERSION"

echo 'Making a release on GitHub...'

yes '' | gh release create "$VERSION" --title "$VERSION" "$TARGET_ZIP" "$TARGET_TGZ"
rm -rf "$TARGET_DIR" "$TARGET_ZIP" "$TARGET_TGZ"

echo '...Done releasing on GitHub.'
