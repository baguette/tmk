#!/bin/sh

echo "Running bootstrap build"
TMAKE_BUILD=$(./bootstrap.sh |grep gcc |grep 'tmake\.c')

echo
echo "Using build command: $TMAKE_BUILD"
echo

echo "Building tmake only with clang-analyzer"
scan-build $@ -o clang-int $TMAKE_BUILD

