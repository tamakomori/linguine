#!/bin/bash

set -eu

echo "Interpreter mode."

for f in syntax/*.ls; do
    echo -n "Running $f ... "
    $1 ./linguine --disable-jit $f > out
    diff $f.out out
    rm out
    echo "ok."
done

echo "JIT mode."

for f in syntax/*.ls; do
    echo -n "Running $f ... "
    $1 ./linguine $f > out
    diff $f.out out
    rm out
    echo "ok."
done

echo 'All tests ok.'
