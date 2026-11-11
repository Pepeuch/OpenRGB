#!/usr/bin/env bash
set -euo pipefail
umask 077
test_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd -- "$test_dir/../.." && pwd)"
cd "$repo_dir"
mkdir -p .build/d5next-speed-tests
flags=()
if [[ "${1:-}" == --sanitize && $# == 1 ]]; then
 flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
 export UBSAN_OPTIONS="${UBSAN_OPTIONS:+$UBSAN_OPTIONS:}halt_on_error=1"
elif [[ $# != 0 ]]; then exit 2; fi
g++ -std=c++17 -O1 -g -Wall -Wextra -Wpedantic -Werror "${flags[@]}" \
 -Itools/AquaComputerD5Next -IControllers/AquaComputerD5NextController -Idependencies/CRCpp \
 tests/AquaComputerD5Next/SpeedTests.cpp tools/AquaComputerD5Next/D5NextSpeedTest.cpp \
 Controllers/AquaComputerD5NextController/{D5NextProtocol,D5NextRGBpx,D5NextEffects}.cpp \
 -lmbedcrypto -o .build/d5next-speed-tests/speed-tests
.build/d5next-speed-tests/speed-tests
