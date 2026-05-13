#!/usr/bin/env bash
set -euo pipefail

std=${1:-CXX17}
std=$(echo "$std" | tr '[:lower:]' '[:upper:]')
compiler=${2:-gcc}

echo "==============================="
echo "Preparing C++ code with $std standard and $compiler compiler"
echo ""

# Add or replace CXX_STD in src/Makevars
if grep -q "^CXX_STD" ./src/Makevars; then
  sed -i "s/^CXX_STD = .*/CXX_STD = ${std}/" ./src/Makevars
else
  echo "CXX_STD = ${std}" >> ./src/Makevars
fi
