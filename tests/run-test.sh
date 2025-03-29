#!/bin/bash

set -eu

echo "Interpreter mode."

for f in syntax/*.ls; do
    echo -n "Running $f ... "
    ./linguine --safe-mode $f > out
    diff $f.out out
    rm out
    echo "ok."
done

echo "JIT mode."

for f in syntax/*.ls; do
    echo -n "Running $f ... "
    ./linguine $f > out
    diff $f.out out
    rm out
    echo "ok."
done

echo 'All tests ok.'
