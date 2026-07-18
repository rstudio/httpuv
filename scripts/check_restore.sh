#!/usr/bin/env bash
set -euo pipefail

std=${1:-CXX17}
std=$(echo "$std" | tr '[:lower:]' '[:upper:]')
compiler=${2:-gcc}

echo "Restoring files for $std and $compiler"

# Remove the CXX_STD line added by check_prepare.sh
sed -i '/^CXX_STD = /d' ./src/Makevars.in

# Clear check files
rm -rf ./httpuv2.Rcheck || true
