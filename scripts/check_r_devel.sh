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

# Prevent R from loading any startup files (including ~/.Rprofile which loads cpp4r)
export R_PROFILE=""
export R_PROFILE_USER=""
export R_ENVIRON=""
export R_ENVIRON_USER=""

# Install required packages in R-devel if not present (use --vanilla to avoid loading .Rprofile)
echo "Checking/installing required packages in R-devel..."
"${RSCRIPT_DEVEL}" --vanilla -e '
  pkgs <- c("devtools", "roxygen2", "testthat", "usethis", "decor", "desc", "glue", "tibble", "vctrs", "withr", "pkgbuild")
  missing <- pkgs[!sapply(pkgs, requireNamespace, quietly = TRUE)]
  if (length(missing) > 0) {
    install.packages(missing, repos = "https://cloud.r-project.org")
  }
'

# Install cpp4r from local source using R CMD INSTALL
echo "Installing cpp4r into R-devel..."
CPP4R_TARBALL=$("${R_DEVEL}" CMD build --no-manual . 2>/dev/null | grep -oP "^\* creating '\K[^']+" || true)
if [ -z "${CPP4R_TARBALL}" ] || [ ! -f "${CPP4R_TARBALL}" ]; then
  # Fallback: find the tarball
  CPP4R_TARBALL=$(ls -t cpp4r_*.tar.gz 2>/dev/null | head -1)
fi
if [ -z "${CPP4R_TARBALL}" ] || [ ! -f "${CPP4R_TARBALL}" ]; then
  echo "ERROR: Failed to build cpp4r tarball"
  exit 1
fi
echo "Built tarball: ${CPP4R_TARBALL}"
"${R_DEVEL}" CMD INSTALL "${CPP4R_TARBALL}"
rm -f "${CPP4R_TARBALL}"

# Export CXX_STD for configure script
export CXX_STD="${std}"

# Set compiler
if [ "$compiler" = "clang" ]; then
  export USE_CLANG=1
else
  unset USE_CLANG || true
fi

# Ensure results directory exists
mkdir -p "./check-r-devel"
LOG="./check-r-devel/check-${std}-${compiler}-devel.log"

# Clear previous log if it exists
rm -f "${LOG}"

# Capture everything (stdout+stderr) into the log while printing to console
exec > >(tee -a "${LOG}") 2>&1

# Register and document the test package using R-devel
echo "Registering httpuvtest with R-devel..."
"${RSCRIPT_DEVEL}" --vanilla -e 'cpp4r::register("./httpuvtest")'

echo "Documenting httpuvtest with R-devel..."
"${RSCRIPT_DEVEL}" --vanilla -e 'devtools::document("./httpuvtest")'

# Build package tarball using R-devel
echo "Building tarball with R-devel..."
TARBALL=$("${RSCRIPT_DEVEL}" --vanilla -e 'cat(devtools::build("./httpuvtest", quiet = TRUE))')
if [ -z "${TARBALL}" ]; then
  echo "Failed to build tarball for httpuvtest."
  exit 1
fi

echo "Tarball created: ${TARBALL}"

# Run R CMD check on the tarball using R-devel
echo "Running R CMD check with R-devel..."
CXX_STD="${std}" "${R_DEVEL}" CMD check --as-cran --no-manual "${TARBALL}" || true

# If there was an error, copy the install log for inspection
if [ -f "./httpuvtest.Rcheck/00install.out" ]; then
  cp "./httpuvtest.Rcheck/00install.out" "./check-r-devel/install-${std}-${compiler}-devel.log"
  echo "=== BEGIN 00install.out ==="
  cat "./httpuvtest.Rcheck/00install.out"
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

# Cleanup
rm -f "${TARBALL}"
rm -rf ./httpuvtest.Rcheck || true

echo "==============================="
echo "R-devel check complete."
echo "==============================="
