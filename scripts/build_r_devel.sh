#!/bin/bash
set -euo pipefail

# Configuration
R_DEVEL_PREFIX="/opt/R-devel"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R_SOURCE_DIR="${SCRIPT_DIR}/R-devel"
R_TARBALL_URL="https://cran.r-project.org/src/base-prerelease/R-devel.tar.gz"
R_TARBALL="${SCRIPT_DIR}/R-devel.tar.gz"

echo "==============================="
echo "Building R-devel from source"
echo "Installation prefix: ${R_DEVEL_PREFIX}"
echo "==============================="

# Download R-devel tarball (includes recommended packages)
echo "Downloading R-devel tarball..."
curl -L -o "${R_TARBALL}" "${R_TARBALL_URL}"

# Extract tarball
echo "Extracting R-devel..."
rm -rf "${R_SOURCE_DIR}"
tar -xzf "${R_TARBALL}" -C "${SCRIPT_DIR}"

cd "${R_SOURCE_DIR}"

# Configure R
echo "Configuring R..."
./configure \
  --prefix="${R_DEVEL_PREFIX}" \
  --enable-R-shlib \
  --with-blas \
  --with-lapack \
  --with-readline \
  --with-x=no

# Build R
echo "Building R (this may take a while)..."
make -j$(nproc)

# Install R (requires sudo for /opt)
echo "Installing R to ${R_DEVEL_PREFIX}..."
echo "Note: This requires sudo permissions"
sudo make install

# Cleanup tarball
rm -f "${R_TARBALL}"

# Verify installation
if [ -x "${R_DEVEL_PREFIX}/bin/R" ]; then
  echo "==============================="
  echo "R-devel installed successfully!"
  echo "R version:"
  "${R_DEVEL_PREFIX}/bin/R" --version | head -n 1
  echo "==============================="
else
  echo "ERROR: R-devel installation failed"
  exit 1
fi
