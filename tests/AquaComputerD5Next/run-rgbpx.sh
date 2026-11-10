#!/usr/bin/env bash
# Pure C++ RGBpx tests; SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
umask 077
test_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd -- "$test_dir/../.." && pwd)"
cd -- "$repo_dir"
mkdir -p .build/d5next-rgbpx
test_flags=()
test_suffix=""
if [[ "${1:-}" == "--sanitize" && $# -eq 1 ]]; then
    test_flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
    test_suffix="-sanitize"
    export UBSAN_OPTIONS="${UBSAN_OPTIONS:+$UBSAN_OPTIONS:}halt_on_error=1"
elif [[ $# -ne 0 ]]; then
    echo "Usage: bash tests/AquaComputerD5Next/run-rgbpx.sh [--sanitize]" >&2
    exit 2
fi
"${CXX:-g++}" -std=c++17 -O1 -g -Wall -Wextra -Wpedantic -Werror \
    "${test_flags[@]}" \
    -IControllers/AquaComputerD5NextController -Idependencies/CRCpp \
    tests/AquaComputerD5Next/RGBpxTests.cpp \
    Controllers/AquaComputerD5NextController/D5NextProtocol.cpp \
    Controllers/AquaComputerD5NextController/D5NextRGBpx.cpp \
    Controllers/AquaComputerD5NextController/D5NextEffects.cpp \
    Controllers/AquaComputerD5NextController/D5NextSystemCommands.cpp \
    -o ".build/d5next-rgbpx/rgbpx-tests$test_suffix"
".build/d5next-rgbpx/rgbpx-tests$test_suffix" Documentation/D5NextRGBpxVideoUSBParameters.csv
