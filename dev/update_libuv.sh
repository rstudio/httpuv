#!/usr/bin/env bash
# Usage: dev/update_libuv.sh <version>
# Example: dev/update_libuv.sh 1.52.1
#
# Automates the libuv update steps documented in dev/build-notes.md.
# Must be run from the repository root.

set -euo pipefail

# ---------------------------------------------------------------------------
# 1. Arguments & version
# ---------------------------------------------------------------------------
if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <libuv-version>" >&2
  exit 1
fi

VERSION="$1"
REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "$REPO_ROOT"

LIBUV="src/libuv"
WIN="$LIBUV/src/win"
UNIX="$LIBUV/src/unix"

echo "==> Updating libuv to $VERSION"

# ---------------------------------------------------------------------------
# 2. Update the version in update_libuv.R and run it
# ---------------------------------------------------------------------------
UPDATE_SCRIPT="tools/update_libuv.R"
sed -i "s/^version <- \"[^\"]*\"/version <- \"$VERSION\"/" "$UPDATE_SCRIPT"
git add "$UPDATE_SCRIPT"

echo "==> Running $UPDATE_SCRIPT"
Rscript "$UPDATE_SCRIPT"
git add "$LIBUV"

# ---------------------------------------------------------------------------
# 3. Apply httpuv fixes to source files (before autogen so it runs once)
# ---------------------------------------------------------------------------

# --- Fix: unnamed structs on MinGW (httpuv commits 7106577, 4bea58e) -------
# winapi.h: name the anonymous outer union in REPARSE_DATA_BUFFER as 'u',
# and the anonymous union in IO_STATUS_BLOCK as 'u'.
WINAPI="$WIN/winapi.h"
if grep -q '^\s*};$' "$WINAPI" && ! grep -qF '} u;' "$WINAPI"; then
  echo "==> Fix: naming anonymous unions in winapi.h (7106577)"
  # REPARSE_DATA_BUFFER: '  };' that follows '    } AppExecLinkReparseBuffer;'
  sed -i '/} AppExecLinkReparseBuffer;/{n;s/^  };$/  } u;/}' "$WINAPI"
  # IO_STATUS_BLOCK: '  };' that follows '    PVOID Pointer;'
  sed -i '/PVOID Pointer;/{n;s/^  };$/  } u;/}' "$WINAPI"
else
  echo "    [skip] winapi.h already patched"
fi

# fs.c: update all ReparseBuffer member accesses and FIELD_OFFSET calls
FS="$WIN/fs.c"
if grep -q '->SymbolicLinkReparseBuffer\.' "$FS"; then
  echo "==> Fix: updating ReparseBuffer references in fs.c (7106577, 4bea58e)"
  sed -i \
    -e 's/->SymbolicLinkReparseBuffer\./->u.SymbolicLinkReparseBuffer./g' \
    -e 's/->MountPointReparseBuffer\./->u.MountPointReparseBuffer./g' \
    -e 's/->AppExecLinkReparseBuffer\./->u.AppExecLinkReparseBuffer./g' \
    -e 's/\(REPARSE_DATA_BUFFER, \)MountPointReparseBuffer/\1u.MountPointReparseBuffer/g' \
    -e 's/io_status\.Status/io_status.u.Status/g' \
    "$FS"
else
  echo "    [skip] fs.c ReparseBuffer refs already updated"
fi

# winsock.c: update IO_STATUS_BLOCK member accesses
WINSOCK="$WIN/winsock.c"
if grep -q 'iosb->Status\b\|iosb\.Status\b' "$WINSOCK"; then
  echo "==> Fix: updating iosb references in winsock.c (7106577)"
  sed -i \
    -e 's/iosb->Status\b/iosb->u.Status/g' \
    -e 's/iosb->Pointer\b/iosb->u.Pointer/g' \
    -e 's/iosb\.Status\b/iosb.u.Status/g' \
    "$WINSOCK"
else
  echo "    [skip] winsock.c iosb refs already updated"
fi

# --- Fix: incompatible pointer type on MinGW (httpuv commit ef944cf) -------
UDP="$WIN/udp.c"
if grep -q 'connect(handle->socket, &addr,' "$UDP"; then
  echo "==> Fix: casting sockaddr in udp.c (ef944cf)"
  sed -i 's/connect(handle->socket, &addr,/connect(handle->socket, (struct sockaddr*) \&addr,/g' "$UDP"
else
  echo "    [skip] udp.c sockaddr cast already present"
fi

# --- Fix: empty translation unit warning (httpuv commit 8ab31ef) -----------
SNPRINTF="$WIN/snprintf.c"
if ! grep -q 'make_iso_compilers_happy' "$SNPRINTF"; then
  echo "==> Fix: adding dummy typedef to snprintf.c (8ab31ef)"
  printf '\n/* Workaround for "ISO C forbids an empty translation unit" when compiled\n * with -pedantic. This is flagged as a significant warning by R CMD check.\n */\ntypedef int make_iso_compilers_happy;\n' >> "$SNPRINTF"
else
  echo "    [skip] snprintf.c dummy typedef already present"
fi

# --- Fix: Solaris support (httpuv commit 1898a29) ---------------------------
MAKEFILE_AM="$LIBUV/Makefile.am"
if ! grep -q 'DSUNOS_NO_IFADDRS' "$MAKEFILE_AM"; then
  echo "==> Fix: adding -DSUNOS_NO_IFADDRS to Makefile.am (1898a29)"
  # Insert after the '-D_XOPEN_SOURCE=500 \' line that is followed by '-D_REENTRANT'
  perl -i -0pe \
    's/(-D_XOPEN_SOURCE=500 \\\n)([ \t]*-D_REENTRANT)/$1                   -DSUNOS_NO_IFADDRS \\\n$2/' \
    "$MAKEFILE_AM"
else
  echo "    [skip] Makefile.am Solaris flag already present"
fi

# --- Fix: pragma NOTE (httpuv commit 421f092) --------------------------------
CORE="$UNIX/core.c"
if grep -q '^#pragma GCC diagnostic' "$CORE"; then
  echo "==> Fix: replacing #pragma with # pragma in core.c (421f092)"
  sed -i \
    -e 's/^#pragma GCC diagnostic push$/# pragma GCC diagnostic push/' \
    -e 's/^#pragma GCC diagnostic ignored/# pragma GCC diagnostic ignored/' \
    -e 's/^#pragma GCC diagnostic pop$/# pragma GCC diagnostic pop/' \
    "$CORE"
else
  echo "    [skip] core.c pragma already workaround-ed"
fi

# --- Fix: ISO C90 mixed declarations warning (httpuv commit 1431d4f) --------
CONFIGURE_AC="$LIBUV/configure.ac"
if ! grep -q 'Wno-declaration-after-statement' "$CONFIGURE_AC"; then
  echo "==> Fix: adding -Wno-declaration-after-statement to configure.ac (1431d4f)"
  sed -i \
    's/CC_CHECK_CFLAGS_APPEND(\[-Wno-unused-parameter\])/CC_CHECK_CFLAGS_APPEND([-Wno-unused-parameter])\nCC_CHECK_CFLAGS_APPEND([-Wno-declaration-after-statement])/' \
    "$CONFIGURE_AC"
else
  echo "    [skip] configure.ac flag already present"
fi

# ---------------------------------------------------------------------------
# 4. Run autogen.sh (uses the already-patched Makefile.am and configure.ac)
# ---------------------------------------------------------------------------
echo "==> Running autogen.sh"
cd "$LIBUV"
./autogen.sh

# Rename the file with ~ in the name (causes issues with R CMD check)
if [[ -f m4/lt~obsolete.m4 ]]; then
  mv m4/lt~obsolete.m4 m4/lt_obsolete.m4
fi

cd "$REPO_ROOT"

# Add generated files (-f because they are listed in .gitignore)
git add -f \
  "$LIBUV/Makefile.in" \
  "$LIBUV/aclocal.m4" \
  "$LIBUV/ar-lib" \
  "$LIBUV/compile" \
  "$LIBUV/config.guess" \
  "$LIBUV/config.sub" \
  "$LIBUV/configure" \
  "$LIBUV/depcomp" \
  "$LIBUV/install-sh" \
  "$LIBUV/ltmain.sh" \
  "$LIBUV/m4/libtool.m4" \
  "$LIBUV/m4/libuv-extra-automake-flags.m4" \
  "$LIBUV/m4/lt_obsolete.m4" \
  "$LIBUV/m4/ltoptions.m4" \
  "$LIBUV/m4/ltsugar.m4" \
  "$LIBUV/m4/ltversion.m4" \
  "$LIBUV/missing"

git commit -m "Update to libuv $VERSION with httpuv fixes"

# ---------------------------------------------------------------------------
# 5. Check for any remaining #pragma diagnostic ignored in C files
# ---------------------------------------------------------------------------
echo "==> Checking for remaining '#pragma diagnostic ignored' in C files"
PRAGMA_FILES=$(find "$LIBUV/src" -name "*.c" -exec grep -rl "^#pragma.*diagnostic ignored" {} \; 2>/dev/null || true)
if [[ -n "$PRAGMA_FILES" ]]; then
  echo "WARNING: Files still have '#pragma diagnostic ignored' (should be '# pragma'):" >&2
  echo "$PRAGMA_FILES" >&2
else
  echo "    [ok] No remaining #pragma diagnostic ignored found"
fi

echo ""
echo "Done. Update dev/build-notes.md if any fixes needed manual adjustment."
