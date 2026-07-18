#!/bin/bash
set -euo pipefail

# Configuration
R_DEVEL_PREFIX="/opt/R-devel"
R_DEVEL="${R_DEVEL_PREFIX}/bin/R"
RSCRIPT_DEVEL="${R_DEVEL_PREFIX}/bin/Rscript"

# Default values matching check-cxx23-gcc
std="${1:-CXX23}"
std=$(echo "$std" | tr '[:lower:]' '[:upper:]')
compiler="${2:-gcc}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "${SCRIPT_DIR}")"

echo "==============================="
echo "Checking C++ code with R-devel"
echo "Standard: $std"
echo "Compiler: $compiler"
echo "==============================="

# Check if R-devel is installed
if [ ! -x "${R_DEVEL}" ]; then
  echo "ERROR: R-devel not found at ${R_DEVEL}"
  echo "Run ./scripts/build_r_devel.sh first to build R-devel"
  exit 1
fi

echo "Using R-devel:"
"${R_DEVEL}" --version | head -n 1

cd "${PROJECT_DIR}"

# Prevent R from loading any startup files
export R_PROFILE=""
export R_PROFILE_USER=""
export R_ENVIRON=""
export R_ENVIRON_USER=""

# Install required packages in R-devel if not present
echo "Checking/installing required packages in R-devel..."
"${RSCRIPT_DEVEL}" --vanilla -e '
  pkgs <- c("devtools", "roxygen2", "testthat", "usethis", "later", "promises",
            "R6", "withr", "pkgbuild", "curl")
  missing <- pkgs[!sapply(pkgs, requireNamespace, quietly = TRUE)]
  if (length(missing) > 0) {
    install.packages(missing, repos = "https://cloud.r-project.org")
  }
'

# Patch src/Makevars with the requested C++ standard
"${SCRIPT_DIR}/check_prepare.sh" "${std}" "${compiler}"

# Set up compiler override via R_MAKEVARS_USER
TMPDIR_MAKE=$(mktemp -d)
MAKEVARS_FILE="${TMPDIR_MAKE}/Makevars"
trap '"${SCRIPT_DIR}/check_restore.sh" "${std}" "${compiler}"; rm -rf "${TMPDIR_MAKE}"' EXIT

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

# Ensure results directory exists
mkdir -p "./check-r-devel"
LOG="./check-r-devel/check-${std}-${compiler}-devel.log"

# Clear previous log if it exists
rm -f "${LOG}"

# Capture everything (stdout+stderr) into the log while printing to console
exec > >(tee -a "${LOG}") 2>&1

# Build httpuv2 tarball using R-devel
echo "Building tarball with R-devel..."
TARBALL=$("${RSCRIPT_DEVEL}" --vanilla -e 'cat(devtools::build(".", quiet = TRUE))')
if [ -z "${TARBALL}" ]; then
  echo "Failed to build httpuv2 tarball."
  exit 1
fi

echo "Tarball created: ${TARBALL}"

# Run R CMD check on the tarball using R-devel
echo "Running R CMD check with R-devel..."
"${R_DEVEL}" CMD check --as-cran --no-manual "${TARBALL}" || true

# If there was an error, copy the install log for inspection
if [ -f "./httpuv2.Rcheck/00install.out" ]; then
  cp "./httpuv2.Rcheck/00install.out" "./check-r-devel/install-${std}-${compiler}-devel.log"
  echo "=== BEGIN 00install.out ==="
  cat "./httpuv2.Rcheck/00install.out"
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

echo "==============================="
echo "R-devel check complete."
echo "==============================="
