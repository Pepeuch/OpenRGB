#!/usr/bin/env bash
set -euo pipefail
test_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd -- "$test_dir/../.." && pwd)"
build_suffix=""
test_config=()
if [[ "${1:-}" == "--sanitize" && $# -eq 1 ]]; then
    build_suffix="-sanitize"
    test_config+=(CONFIG+=d5next_sanitize)
    export UBSAN_OPTIONS="${UBSAN_OPTIONS:+$UBSAN_OPTIONS:}halt_on_error=1:suppressions=$test_dir/ubsan.supp"
elif [[ $# -ne 0 ]]; then
    echo "Usage: bash tests/AquaComputerD5Next/run-detection.sh [--sanitize]" >&2
    exit 2
fi
mkdir -p -- "$repo_dir/.build/d5next-tests-detection$build_suffix"
cd -- "$repo_dir/.build/d5next-tests-detection$build_suffix"
"${QMAKE:-qmake}" "$test_dir/D5NextTests.pro" CONFIG+=d5next_detection "${test_config[@]}"
make -j2
./d5next-detection-tests
