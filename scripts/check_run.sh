#!/usr/bin/env bash
set -euo pipefail

# Accept std and compiler as positional parameters
std="$1"
std=$(echo "$std" | tr '[:lower:]' '[:upper:]')
compiler="$2"

# R_MAKEVARS_USER is included AFTER Makeconf, so it can override the compiler.
# Package src/Makevars is included BEFORE Makeconf and cannot override CXX17 etc.
TMPDIR_MAKE=$(mktemp -d)
MAKEVARS_FILE="${TMPDIR_MAKE}/Makevars"
trap 'rm -rf "${TMPDIR_MAKE}"' EXIT

if [ "$compiler" = "clang" ]; then
    cat > "${MAKEVARS_FILE}" << 'EOF'
CC = clang
CXX = clang++
CXX17 = clang++
CXX17STD = -std=gnu++17
CXX20 = clang++
CXX20STD = -std=gnu++20
CXX23 = clang++
CXX23STD = -std=gnu++23
SHLIB_OPENMP_CXXFLAGS = -fopenmp=libgomp
EOF
else
    touch "${MAKEVARS_FILE}"
fi
export R_MAKEVARS_USER="${MAKEVARS_FILE}"

# Ensure results directory and set per-iteration log
mkdir -p "./check-gcc-clang"
LOG="./check-gcc-clang/check-${std}-${compiler}.log"

# clear previous log if it exists
rm -f "${LOG}"

# Capture everything (stdout+stderr) from this point into the per-iteration log
# while still printing to the console via tee.
exec > >(tee -a "${LOG}") 2>&1

# Build httpuv tarball
TARBALL=$(Rscript -e 'cat(devtools::build(".", quiet = TRUE))')
if [ -z "${TARBALL}" ]; then
	echo "Failed to build tarball for httpuv."
	exit 1
fi

# Run R CMD check on the tarball. Skip PDF/manual to avoid TeX font issues.
R CMD check --as-cran --no-manual "${TARBALL}" || true

# If there was an error, copy the install log to the results directory for inspection
if [ -f "./httpuv.Rcheck/00install.out" ]; then
	cp "./httpuv.Rcheck/00install.out" "./check-gcc-clang/install-${std}-${compiler}.log"
	echo "=== BEGIN 00install.out ==="
	cat "./httpuv.Rcheck/00install.out"
	echo "=== END 00install.out ==="
fi

# Inspect log for ERRORs only. Allow WARNINGs and NOTEs.
if grep -q "\bERROR\b" "${LOG}"; then
	echo "R CMD check found ERRORs. See ${LOG} for details."
	grep -n "\bERROR\b" "${LOG}" || true
	exit 1
else
	echo "R CMD check completed with no ERRORs. Warnings/Notes (if any) are allowed. See ${LOG} for full output."
fi

rm -f "${TARBALL}"

echo "Run complete."
