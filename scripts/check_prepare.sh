#!/usr/bin/env bash
set -euo pipefail

std=${1:-CXX17}
std=$(echo "$std" | tr '[:lower:]' '[:upper:]')
compiler=${2:-gcc}

echo "==============================="
echo "Preparing C++ code with $std standard and $compiler compiler"
echo ""

# Add or replace CXX_STD in src/Makevars.in
# configure regenerates src/Makevars from src/Makevars.in during R CMD check,
# so we must patch Makevars.in (not Makevars directly).
if grep -q "^CXX_STD" ./src/Makevars.in; then
  sed -i "s/^CXX_STD = .*/CXX_STD = ${std}/" ./src/Makevars.in
else
  sed -i "1s/^/CXX_STD = ${std}\n/" ./src/Makevars.in
fi
