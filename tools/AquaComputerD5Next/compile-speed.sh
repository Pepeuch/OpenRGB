#!/usr/bin/env bash
set -euo pipefail
umask 077
tool_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd -- "$tool_dir/../.." && pwd)"
mkdir -p "$repo_dir/.build/d5next-speed"
"${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -pthread \
 -I"$repo_dir/Controllers/AquaComputerD5NextController" -I"$repo_dir/dependencies/CRCpp" \
 "$tool_dir/D5NextSpeedMain.cpp" "$tool_dir/D5NextSpeedTest.cpp" "$tool_dir/D5NextReadOnly.cpp" \
 "$repo_dir/Controllers/AquaComputerD5NextController/"{D5NextProtocol,D5NextRGBpx,D5NextEffects}.cpp \
 -lmbedcrypto -o "$repo_dir/.build/d5next-speed/d5next-speed-test"
