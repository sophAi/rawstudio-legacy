#!/usr/bin/env bash
# ==============================================================================
# Rawstudio Legacy - Debian/Ubuntu/Linux Mint .deb Packaging Script
# 
# Automatically compiles and packages rawstudio-legacy into a .deb file.
# Configured and optimized for Linux Mint 22.3 (MATE) and Ubuntu 24.04 (Noble).
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

PACKAGE_NAME="rawstudio-legacy"
RAW_VERSION="2.1"

# --- 1. Detect OS Distribution and Architecture ---
DIST_TAG="linux"
if [ -f /etc/os-release ]; then
    # shellcheck disable=SC1091
    . /etc/os-release
    if [ "${ID:-}" = "linuxmint" ] && [ -n "${VERSION_ID:-}" ]; then
        DIST_TAG="mint${VERSION_ID}"
    elif [ -n "${UBUNTU_CODENAME:-}" ]; then
        DIST_TAG="${UBUNTU_CODENAME}"
    elif [ -n "${VERSION_CODENAME:-}" ]; then
        DIST_TAG="${VERSION_CODENAME}"
    elif [ -n "${VERSION_ID:-}" ]; then
        DIST_TAG="${ID:-os}${VERSION_ID}"
    fi
fi

VERSION="${RAW_VERSION}-5~${DIST_TAG}"
ARCH="$(dpkg --print-architecture 2>/dev/null || uname -m | sed 's/x86_64/amd64/')"
OUTPUT_DEB="${1:-${SCRIPT_DIR}/${PACKAGE_NAME}_${VERSION}_${ARCH}.deb}"
BUILD_WORKSPACE="${SCRIPT_DIR}/build_deb_workspace"
STAGING="${BUILD_WORKSPACE}/staging"

echo "===================================================================="
echo "  Rawstudio Legacy (${RAW_VERSION}) Debian (.deb) Package Builder"
echo "  Target OS     : ${PRETTY_NAME:-Linux} (Tag: ${DIST_TAG})"
echo "  Architecture  : ${ARCH}"
echo "  Package Name  : ${PACKAGE_NAME}"
echo "  Version       : ${VERSION}"
echo "  Output File   : ${OUTPUT_DEB}"
echo "===================================================================="

# --- 2. Check Build Prerequisites ---
echo "[1/6] Checking build prerequisites..."
REQUIRED_TOOLS=(
    "gcc"
    "g++"
    "make"
    "pkg-config"
    "dpkg-deb"
    "dpkg-shlibdeps"
    "gzip"
    "strip"
)

MISSING_TOOLS=()
for tool in "${REQUIRED_TOOLS[@]}"; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        MISSING_TOOLS+=("$tool")
    fi
done

if [ ${#MISSING_TOOLS[@]} -gt 0 ]; then
    echo "ERROR: Missing required tools: ${MISSING_TOOLS[*]}"
    exit 1
fi

REQUIRED_PKGS=(
    "build-essential"
    "pkg-config"
    "libgtk-3-dev"
    "liblcms2-dev"
    "liblensfun-dev"
    "libexiv2-dev"
    "libgphoto2-dev"
    "libfftw3-dev"
    "libsqlite3-dev"
    "libxml2-dev"
    "libtiff-dev"
    "libdbus-1-dev"
)

MISSING_PKGS=()
for pkg in "${REQUIRED_PKGS[@]}"; do
    if ! dpkg -s "$pkg" 2>/dev/null | grep -q "Status: install ok installed"; then
        MISSING_PKGS+=("$pkg")
    fi
done

if [ ${#MISSING_PKGS[@]} -gt 0 ]; then
    echo "ERROR: Missing required development libraries:"
    echo "  ${MISSING_PKGS[*]}"
    echo "Please install them via: sudo apt install ${MISSING_PKGS[*]}"
    exit 1
fi

# --- 3. Compile Project ---
echo "[2/6] Building rawstudio-legacy binaries and plugins..."
if [ ! -f "Makefile" ]; then
    echo "-> Running ./configure..."
    ./configure --prefix=/usr --disable-static --enable-experimental
fi

make -j"$(nproc)"

# --- 4. Staging Installation ---
echo "[3/6] Installing files into staging environment..."
rm -rf "${BUILD_WORKSPACE}"
mkdir -p "${STAGING}"

make install DESTDIR="${STAGING}"

# Setup compatibility symlinks and directory structures
mkdir -p "${STAGING}/usr/bin"
mkdir -p "${STAGING}/usr/lib"
mkdir -p "${STAGING}/usr/share/pixmaps"
mkdir -p "${STAGING}/usr/share/icons/hicolor/48x48/apps"
mkdir -p "${STAGING}/usr/share/applications"

# Ensure binary symlink: rawstudio -> rawstudio-legacy
if [ ! -e "${STAGING}/usr/bin/rawstudio" ]; then
    ln -s rawstudio-legacy "${STAGING}/usr/bin/rawstudio"
fi

# Ensure library/plugin symlink: /usr/lib/rawstudio -> rawstudio-legacy
if [ -d "${STAGING}/usr/lib/rawstudio-legacy" ] && [ ! -e "${STAGING}/usr/lib/rawstudio" ]; then
    ln -s rawstudio-legacy "${STAGING}/usr/lib/rawstudio"
fi

# Ensure datadir symlink: /usr/share/rawstudio -> rawstudio-legacy
if [ -d "${STAGING}/usr/share/rawstudio-legacy" ] && [ ! -e "${STAGING}/usr/share/rawstudio" ]; then
    ln -s rawstudio-legacy "${STAGING}/usr/share/rawstudio"
fi

# Ensure pixmaps symlink: /usr/share/pixmaps/rawstudio-legacy -> rawstudio
if [ -d "${STAGING}/usr/share/pixmaps/rawstudio" ] && [ ! -e "${STAGING}/usr/share/pixmaps/rawstudio-legacy" ]; then
    ln -s rawstudio "${STAGING}/usr/share/pixmaps/rawstudio-legacy"
fi

# Ensure desktop entry symlink: rawstudio.desktop -> rawstudio-legacy.desktop
if [ -f "${STAGING}/usr/share/applications/rawstudio-legacy.desktop" ] && [ ! -e "${STAGING}/usr/share/applications/rawstudio.desktop" ]; then
    ln -s rawstudio-legacy.desktop "${STAGING}/usr/share/applications/rawstudio.desktop"
fi

# Install application icon into hicolor theme
if [ -f "${STAGING}/usr/share/icons/rawstudio.png" ]; then
    cp -a "${STAGING}/usr/share/icons/rawstudio.png" "${STAGING}/usr/share/icons/hicolor/48x48/apps/rawstudio-legacy.png"
    cp -a "${STAGING}/usr/share/icons/rawstudio.png" "${STAGING}/usr/share/icons/hicolor/48x48/apps/rawstudio.png"
    cp -a "${STAGING}/usr/share/icons/rawstudio.png" "${STAGING}/usr/share/pixmaps/rawstudio-legacy.png"
fi

# Strip unneeded debug symbols from binaries and libraries
echo "[4/6] Stripping debug symbols to optimize package size..."
find "${STAGING}/usr/bin" -type f -exec strip --strip-all {} + 2>/dev/null || true
find "${STAGING}/usr/lib" -type f -name "*.so*" -exec strip --strip-unneeded {} + 2>/dev/null || true

# --- 5. Generate Debian Control and Metadata ---
echo "[5/6] Generating Debian package metadata..."
DOC_DIR="${STAGING}/usr/share/doc/${PACKAGE_NAME}"
mkdir -p "${DOC_DIR}"
cp -a README.md "${DOC_DIR}/"
cp -a COPYING "${DOC_DIR}/copyright" 2>/dev/null || true

RFC_DATE="$(date -R)"
cat << EOF > "${DOC_DIR}/changelog.Debian"
${PACKAGE_NAME} (${VERSION}) ${DIST_TAG}; urgency=medium

  * Fix Segmentation fault when parsing Panasonic RW2 thumbnail JPEG markers.
  * Fix RawSpeed CFA filter array overwrite when cameras.xml lacks CFA definition.
  * Support RawSpeed CFA_GREEN2 green pixel in RW2 black levels decoding.
  * Use g_find_program_in_path to check for enfuse without stderr errors.
  * Validate preview redraw rectangle dimensions to eliminate Pixman warnings.
  * Add RAWSTUDIO_PLUGIN_DIR environment variable support for portable plugin loading.

 -- Rawstudio Legacy Packager <support@rawstudio.org>  ${RFC_DATE}

${PACKAGE_NAME} (${RAW_VERSION}-1~${DIST_TAG}) ${DIST_TAG}; urgency=medium

  * Modernized Rawstudio legacy release for Linux Mint 22.3 / Ubuntu 24.04.
  * Replaced deprecated GConf dependency with GLib GKeyFile configuration.
  * Aligned liblensfun header paths and fixed lensfun-version detection macro.
  * Updated camera database using 2023 Darktable RawSpeed cameras.xml.
  * Added fallback dcraw 9.28 and ufraw 0.23 raw loader plugin.
  * Modernized Exiv2 >= 0.28.0 compatibility.
  * Resolved non-free Unicode license issue with Google V8 UTF-16 implementation.

 -- Rawstudio Legacy Packager <support@rawstudio.org>  ${RFC_DATE}
EOF
gzip -9n "${DOC_DIR}/changelog.Debian"

# Set standard Unix permissions
find "${STAGING}" -type d -exec chmod 0755 {} +
find "${STAGING}" -type f -exec chmod 0644 {} +
chmod 0755 "${STAGING}/usr/bin/rawstudio-legacy"

# Setup DEBIAN control
mkdir -p "${STAGING}/DEBIAN"
INSTALLED_SIZE=$(du -sk "${STAGING}/usr" | cut -f1)

# Dynamically calculate library dependencies using dpkg-shlibdeps
echo "-> Resolving runtime library dependencies with dpkg-shlibdeps..."
mkdir -p "${BUILD_WORKSPACE}/debian"
echo "librawstudio-legacy 2.1 ${PACKAGE_NAME}" > "${BUILD_WORKSPACE}/debian/shlibs.local"
touch "${BUILD_WORKSPACE}/debian/control"

PLUGINS_SO=( "${STAGING}"/usr/lib/rawstudio-legacy/plugins/*.so )

SHLIB_DEPS=$(
    cd "${BUILD_WORKSPACE}" && \
    dpkg-shlibdeps \
        --ignore-missing-info \
        -l"${STAGING}/usr/lib" \
        -O \
        "${STAGING}/usr/bin/rawstudio-legacy" \
        "${STAGING}/usr/lib/librawstudio-legacy-2.1.so" \
        "${PLUGINS_SO[@]}" 2>/dev/null \
    | sed -n 's/^shlibs:Depends=//p' \
    | sed "s/${PACKAGE_NAME}, //g; s/, ${PACKAGE_NAME}//g; s/${PACKAGE_NAME}//g"
)

if [ -z "${SHLIB_DEPS}" ]; then
    echo "Warning: dpkg-shlibdeps could not determine dependencies automatically; using standard defaults."
    SHLIB_DEPS="libc6 (>= 2.38), libgtk-3-0t64 (>= 3.0.0), libglib2.0-0t64 (>= 2.79.0), liblcms2-2 (>= 2.2), liblensfun1 (>= 0.3.4), libexiv2-27 (>= 0.27.2), libsqlite3-0 (>= 3.6.0), libxml2 (>= 2.7.4), libgphoto2-6t64 (>= 2.5.10), libfftw3-single3 (>= 3.3.10), libdbus-1-3 (>= 1.9.14), libjpeg8 (>= 8c), libtiff6 (>= 4.0.3)"
fi

cat << EOF > "${STAGING}/DEBIAN/control"
Package: ${PACKAGE_NAME}
Version: ${VERSION}
Architecture: ${ARCH}
Maintainer: Rawstudio Legacy Team <support@rawstudio.org>
Installed-Size: ${INSTALLED_SIZE}
Depends: ${SHLIB_DEPS}
Recommends: enfuse
Provides: rawstudio (= ${RAW_VERSION})
Conflicts: rawstudio
Replaces: rawstudio
Section: graphics
Priority: optional
Homepage: https://github.com/rawstudio/rawstudio
Description: RAW digital camera image processor (Legacy Edition)
 Rawstudio is a highly specialized open-source application for processing
 RAW images from digital cameras. It focuses on fast, efficient review
 and non-destructive batch conversion of digital camera RAW formats into
 JPEG, PNG, or TIFF.
 .
 This legacy edition has been modernized for Linux Mint / Ubuntu:
  - Fixed Panasonic RW2 Bayer CFA decoding (Tag 0x0009) fixing magenta/purple tint.
  - Fixed Pixman/Cairo invalid rectangle errors during preview rendering.
  - Fixed TIFF metadata parsing buffer overrun on Panasonic RW2 thumbnails.
  - Replaced obsolete GConf dependency with GLib GKeyFile configuration.
  - Fixed liblensfun 0.3.x header detection and calibration formulas.
  - Upgraded camera database with 2023 camera profiles from Darktable RawSpeed.
  - Bundled updated dcraw 9.28 and ufraw 0.23 fallback decoder.
  - Modernized Exiv2 >= 0.28.0 compatibility.
  - Replaced legacy Unicode code with Google V8 BSD-3 implementation.
EOF

# Maintainer postinst script
cat << 'EOF' > "${STAGING}/DEBIAN/postinst"
#!/bin/sh
set -e

if [ "$1" = "configure" ]; then
    ldconfig || true
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database -q /usr/share/applications || true
    fi
    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor || true
    fi
    if command -v update-mime-database >/dev/null 2>&1; then
        update-mime-database /usr/share/mime || true
    fi
    if [ -f /usr/local/bin/rawstudio ] || [ -f /usr/local/lib/librawstudio-2.1.so ]; then
        echo "===================================================================="
        echo "Notice: Detected older Rawstudio files installed in /usr/local/ !"
        echo "Because /usr/local has higher priority than /usr, please consider"
        echo "removing stale /usr/local files to prevent conflicts:"
        echo "  sudo rm -rf /usr/local/bin/rawstudio /usr/local/lib/librawstudio* /usr/local/lib/rawstudio /usr/local/share/rawstudio && sudo ldconfig"
        echo "===================================================================="
    fi
fi

exit 0
EOF
chmod 0755 "${STAGING}/DEBIAN/postinst"

# Maintainer postrm script
cat << 'EOF' > "${STAGING}/DEBIAN/postrm"
#!/bin/sh
set -e

if [ "$1" = "remove" ] || [ "$1" = "purge" ]; then
    ldconfig || true
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database -q /usr/share/applications || true
    fi
    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor || true
    fi
    if command -v update-mime-database >/dev/null 2>&1; then
        update-mime-database /usr/share/mime || true
    fi
fi

exit 0
EOF
chmod 0755 "${STAGING}/DEBIAN/postrm"

# Generate md5sums
cd "${STAGING}"
find usr -type f -exec md5sum {} + > DEBIAN/md5sums
chmod 0644 DEBIAN/md5sums DEBIAN/control

# --- 6. Build .deb package ---
echo "[6/6] Creating Debian package with dpkg-deb..."
dpkg-deb --root-owner-group --build "${STAGING}" "${OUTPUT_DEB}"

# Cleanup workspace
rm -rf "${BUILD_WORKSPACE}"

echo ""
echo "===================================================================="
echo " Successfully created: ${OUTPUT_DEB}"
echo "===================================================================="
dpkg-deb -I "${OUTPUT_DEB}"

exit 0
