#!/usr/bin/env bash
set -euo pipefail

rm -f check-results.md

for std in CXX23 CXX20 CXX17; do
  for compiler in clang gcc; do
    echo "==============================="
    echo "Checking C++ code with $std standard and $compiler compiler"

    mkdir -p ./check-gcc-clang

    ./scripts/check_prepare.sh "$std" "$compiler"

    touch ./check-gcc-clang/check-results.md

    # Run check, but don't exit on failure
    if ! ./scripts/check_run.sh "$std" "$compiler"; then
      echo "WARNING: check_run.sh failed for $std standard with $compiler, continuing..."
      echo "$std + $compiler = fail" >> ./check-gcc-clang/check-results.md || true
    else
      echo "$std + $compiler = ok" >> ./check-gcc-clang/check-results.md || true
    fi

    ./scripts/check_restore.sh "$std" "$compiler"

    echo "==============================="
    echo ""
  done
done
