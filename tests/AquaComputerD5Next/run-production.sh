#!/usr/bin/env bash
# Production macros, no HID backend; SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
test_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd -- "$test_dir/../.." && pwd)"
if [[ $# -ne 0 ]]; then
    echo "Usage: bash tests/AquaComputerD5Next/run-production.sh" >&2
    exit 2
fi
mkdir -p -- "$repo_dir/.build/d5next-tests-production"
cd -- "$repo_dir/.build/d5next-tests-production"
"${QMAKE:-qmake}" "$test_dir/D5NextTests.pro" CONFIG+=d5next_production
make -j2
./d5next-production-tests
