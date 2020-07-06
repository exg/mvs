#!/bin/bash

set -e

root=$(
    cd "$(dirname "$0")" &&
    pwd -P
)

mkdir -p "$root"/build
cd "$root"/build
if [ $# -eq 0 ]; then
    cmake -DCMAKE_CXX_FLAGS='-g -O2' ..
else
    cmake "$@" ..
fi
cmake --build .
