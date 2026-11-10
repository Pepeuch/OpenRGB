#!/usr/bin/env bash
# D5 NEXT offline tests; SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

test_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd -- "$test_dir/../.." && pwd)"
test_config=()
build_suffix=""
if [[ $# -gt 1 ]]; then
    echo "Usage: bash tests/AquaComputerD5Next/run.sh [--sanitize|--sanitize-known-baseline]" >&2
    exit 2
fi
if [[ "${1:-}" == "--sanitize" || "${1:-}" == "--sanitize-known-baseline" ]]; then
    test_config+=(CONFIG+=d5next_sanitize)
    build_suffix="-sanitize"
    export UBSAN_OPTIONS="${UBSAN_OPTIONS:+$UBSAN_OPTIONS:}halt_on_error=1"
    if [[ "$1" == "--sanitize-known-baseline" ]]; then
        echo "UBSan: suppressing nonnull diagnostics in the existing RGBControllerInterface.h."
        echo "See tests/AquaComputerD5Next/ubsan.supp and Documentation/AquaComputerD5Next.md."
        export UBSAN_OPTIONS="$UBSAN_OPTIONS:suppressions=$test_dir/ubsan.supp"
    fi
elif [[ $# -ne 0 ]]; then
    echo "Usage: bash tests/AquaComputerD5Next/run.sh [--sanitize|--sanitize-known-baseline]" >&2
    exit 2
fi

mkdir -p -- "$repo_dir/.build/d5next-tests$build_suffix"
cd -- "$repo_dir/.build/d5next-tests$build_suffix"
"${QMAKE:-qmake}" "$test_dir/D5NextTests.pro" "${test_config[@]}"
make -j2
./d5next-tests
