#!/bin/bash

set -e

realpath () {
    python -c "import os.path, sys; print os.path.realpath(sys.argv[1])" "$1"
}

root="$(dirname "$(realpath "$0")")"
mkdir -p "$root"/build
cd "$root"/build
if [ $# -eq 0 ]; then
    cmake -DCMAKE_CXX_FLAGS='-g -O2' ..
else
    cmake "$@" ..
fi
cmake --build .
