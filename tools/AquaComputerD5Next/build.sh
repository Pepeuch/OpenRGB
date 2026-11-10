#!/usr/bin/env bash
# Native Linux read-only diagnostic; SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
umask 077
tool_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd -- "$tool_dir/../.." && pwd)"
mkdir -p -- "$repo_dir/.build/d5next-readonly"
"${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/Controllers/AquaComputerD5NextController" -I"$repo_dir/dependencies/CRCpp" \
    "$tool_dir/D5NextReadOnly.cpp" "$tool_dir/D5NextDiagnostic.cpp" \
    "$repo_dir/Controllers/AquaComputerD5NextController/D5NextProtocol.cpp" \
    -o "$repo_dir/.build/d5next-readonly/d5next-readonly"
