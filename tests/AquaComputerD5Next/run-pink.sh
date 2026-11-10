#!/usr/bin/env bash
# Synthetic pink test and production barriers; SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
umask 077
test_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd -- "$test_dir/../.." && pwd)"
cd -- "$repo_dir"
mkdir -p .build/d5next-pink
test_flags=()
test_suffix=""
if [[ "${1:-}" == "--sanitize" && $# -eq 1 ]]; then
    test_flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
    test_suffix="-sanitize"
    export UBSAN_OPTIONS="${UBSAN_OPTIONS:+$UBSAN_OPTIONS:}halt_on_error=1"
elif [[ $# -ne 0 ]]; then
    echo "Usage: bash tests/AquaComputerD5Next/run-pink.sh [--sanitize]" >&2
    exit 2
fi
for policy in production synthetic; do
    policy_flags=()
    if [[ "$policy" == synthetic ]]; then
        policy_flags+=(-DOPENRGB_D5NEXT_PINK_OFFLINE_TESTS)
    fi
    "${CXX:-g++}" -std=c++17 -O1 -g -Wall -Wextra -Wpedantic -Werror -pthread \
        "${test_flags[@]}" "${policy_flags[@]}" \
        -Itools/AquaComputerD5Next -IControllers/AquaComputerD5NextController -Idependencies/CRCpp \
        tests/AquaComputerD5Next/PinkTests.cpp tools/AquaComputerD5Next/D5NextPinkTest.cpp \
        Controllers/AquaComputerD5NextController/D5NextProtocol.cpp \
        -Wl,--wrap=open,--wrap=__open_2,--wrap=close,--wrap=flock,--wrap=faccessat,--wrap=stat,--wrap=fstat,--wrap=ioctl,--wrap=poll,--wrap=__poll_chk,--wrap=read,--wrap=__read_chk,--wrap=fflush \
        -o ".build/d5next-pink/pink-tests-$policy$test_suffix"
    ".build/d5next-pink/pink-tests-$policy$test_suffix"
done
