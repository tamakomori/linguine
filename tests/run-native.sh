#!/bin/sh

set -eu

echo "Interpreter...";
for tc in syntax/*.ls; do
    echo "$tc";
    ../linguine --disable-jit $tc > out;
    diff $tc.out out;
done

echo "JIT...";
for tc in syntax/*.ls; do
    echo "$tc";
    ../linguine $tc > out;
    diff $tc.out out;
done
