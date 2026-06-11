#!/bin/bash
################################################################################
# Local Build Script for Testing Package Generation
# This script mimics the GitHub Actions workflow for local testing
################################################################################

set -e  # Exit on error

# Configuration
ROCM_VERSION="${ROCM_VERSION:-}"            # empty => auto-fetch latest
GPU_FAMILY="${GPU_FAMILY:-gfx94X-dcgpu}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${REPO_ROOT}/build"
ROCM_INSTALL_DIR="$REPO_ROOT/rocm-sdk"
ROCM_BANDWIDTH_TEST_ROOT="${REPO_ROOT}"
ROCM_BANDWIDTH_TEST_PKG_PREFIX="rocm-bandwidth-test"

###
# SDK Source Configuration (nightly / release / auto channels)
# Override via environment variables or GitHub Actions repository variables (vars.*).
#
# ROCM_SDK_CHANNEL:
#   "nightly"  - always fetch from the nightly index
#   "release"  - always fetch the latest X.Y.Z release tarball
#   "auto"     - (default) infer from ROCM_VERSION format if supplied,
#                else fall back to nightly
#
# In CI the "Configure ROCm SDK channel" workflow step sets these before
# calling this script.  Locally, set ROCM_SDK_CHANNEL (or the URL vars
# directly) in your environment before running the script.
###
_ROCM_NIGHTLY_INDEX_DEFAULT="https://therock-nightly-tarball.s3.amazonaws.com/index.html"
_ROCM_NIGHTLY_BASE_DEFAULT="https://therock-nightly-tarball.s3.us-east-2.amazonaws.com"
_ROCM_RELEASE_LIST_DEFAULT="https://repo.amd.com/rocm/tarball/"
_ROCM_RELEASE_BASE_DEFAULT="https://repo.amd.com/rocm/tarball"

ROCM_SDK_CHANNEL="${ROCM_SDK_CHANNEL:-auto}"
ROCM_SDK_RELEASE_URL="${ROCM_SDK_RELEASE_URL:-}"

if [ "$ROCM_SDK_CHANNEL" = "nightly" ]; then
    ROCM_SDK_RELEASE_URL=""
    ROCM_SDK_BASE_URL="${ROCM_SDK_NIGHTLY_BASE_URL:-${ROCM_SDK_BASE_URL:-$_ROCM_NIGHTLY_BASE_DEFAULT}}"
    ROCM_SDK_INDEX_URL="${ROCM_SDK_NIGHTLY_INDEX_URL:-${ROCM_SDK_INDEX_URL:-$_ROCM_NIGHTLY_INDEX_DEFAULT}}"
elif [ "$ROCM_SDK_CHANNEL" = "release" ]; then
    ROCM_SDK_RELEASE_URL="${ROCM_SDK_RELEASE_URL:-$_ROCM_RELEASE_LIST_DEFAULT}"
    if [ -z "${ROCM_SDK_BASE_URL:-}" ]; then
        if [ -n "${ROCM_SDK_RELEASE_BASE_URL:-}" ]; then
            ROCM_SDK_BASE_URL="${ROCM_SDK_RELEASE_BASE_URL}"
        else
            ROCM_SDK_BASE_URL="${ROCM_SDK_RELEASE_URL%/}"
        fi
    fi
    ROCM_SDK_INDEX_URL="${ROCM_SDK_INDEX_URL:-}"
else
    ##
    # auto
    if [ -n "${ROCM_SDK_RELEASE_URL}" ]; then
        if [ -z "${ROCM_SDK_BASE_URL:-}" ]; then
            if [ -n "${ROCM_SDK_RELEASE_BASE_URL:-}" ]; then
                ROCM_SDK_BASE_URL="${ROCM_SDK_RELEASE_BASE_URL}"
            else
                ROCM_SDK_BASE_URL="${ROCM_SDK_RELEASE_URL%/}"
            fi
        fi
    else
        ROCM_SDK_BASE_URL="${ROCM_SDK_NIGHTLY_BASE_URL:-${ROCM_SDK_BASE_URL:-$_ROCM_NIGHTLY_BASE_DEFAULT}}"
    fi
    ROCM_SDK_INDEX_URL="${ROCM_SDK_NIGHTLY_INDEX_URL:-${ROCM_SDK_INDEX_URL:-$_ROCM_NIGHTLY_INDEX_DEFAULT}}"
fi

##
# Post-build upload configuration (optional)
if [ -z "${UPLOAD_TARGET:-}" ] && [ -n "${RBT_AUTO_DETECT_LOCAL_UPLOAD:-}" ]; then
    if curl -s -o /dev/null -w '' http://localhost:8080/ 2>/dev/null; then
        UPLOAD_TARGET="http://localhost:8080"
    fi
fi
UPLOAD_TARGET="${UPLOAD_TARGET:-}"
UPLOAD_REPO="${UPLOAD_REPO:-}"

###
# Colors for output
###
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1" >&2
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1" >&2
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1" >&2
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

join_base_and_file() {
    local base="$1"
    local path="$2"
    base="${base%/}"
    printf '%s/%s' "$base" "$path"
}

apply_sdk_tarball_base_for_version() {
    local v="$1"
    if echo "$v" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+a[0-9]+'; then
        ROCM_SDK_BASE_URL="${ROCM_SDK_NIGHTLY_BASE_URL:-${ROCM_SDK_BASE_URL:-$_ROCM_NIGHTLY_BASE_DEFAULT}}"
        print_info "Version matches ROCm nightly format (x.y.za…) → tarball base: $ROCM_SDK_BASE_URL"
    elif echo "$v" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+$'; then
        ROCM_SDK_BASE_URL="${ROCM_SDK_RELEASE_BASE_URL:-${ROCM_SDK_BASE_URL:-$_ROCM_RELEASE_BASE_DEFAULT}}"
        print_info "Version matches ROCm release format (X.Y.Z) → tarball base: $ROCM_SDK_BASE_URL"
    fi
}

##
# Fetch latest nightly ROCm version for the specified GPU family.
# Sets the global ROCM_VERSION variable.
fetch_latest_rocm_version() {
    local gpu_family="$1"

    print_info "Fetching latest ROCm version for $gpu_family..."
    print_info "Index URL: $ROCM_SDK_INDEX_URL"

    local latest_version
    latest_version=$(wget -qO- "$ROCM_SDK_INDEX_URL" 2>/dev/null | \
        grep -oP "therock-dist-linux-${gpu_family}-\K[^<\"]+(?=\.tar\.gz)" | \
        grep -v '^ADHOCBUILD' | \
        sort -V | tail -1)

    if [ -z "$latest_version" ]; then
        print_error "Could not fetch latest ROCm nightly version for $gpu_family from $ROCM_SDK_INDEX_URL"
        return 1
    fi

    print_success "Found latest ROCm version: $latest_version"
    ROCM_VERSION="$latest_version"
    return 0
}

##
# Fetch latest release X.Y.Z ROCm version for the specified GPU family.
# Sets the global ROCM_VERSION variable.
fetch_latest_rocm_release_version() {
    local gpu_family="$1"
    local list_url="${ROCM_SDK_RELEASE_URL:-$_ROCM_RELEASE_LIST_DEFAULT}"

    print_info "Fetching latest ROCm release version (X.Y.Z) for $gpu_family..."
    print_info "Release listing URL: $list_url"

    local latest_version
    latest_version=$(wget -qO- "$list_url" 2>/dev/null | \
        grep -oP "therock-dist-linux-${gpu_family}-\K[0-9]+\.[0-9]+\.[0-9]+(?=\.tar\.gz)" | \
        sort -V | uniq | tail -1)

    if [ -z "$latest_version" ]; then
        print_error "No therock-dist-linux-${gpu_family}-X.Y.Z.tar.gz found under release listing"
        return 1
    fi

    print_success "Found latest ROCm release version: $latest_version"
    ROCM_VERSION="$latest_version"
    return 0
}

ensure_recent_cmake_ubuntu() {
    [ -f /etc/os-release ] || return 0
    . /etc/os-release
    case "${ID:-}" in
        ubuntu|debian) ;;
        *) return 0 ;;
    esac
    command -v apt-get >/dev/null 2>&1 || return 0

    local need_major=3 need_minor=25
    local cur_major=0 cur_minor=0 v=""
    if command -v cmake >/dev/null 2>&1; then
        v="$(cmake --version 2>/dev/null | head -1 | awk '{print $3}')"
        cur_major="${v%%.*}"
        local _rest="${v#*.}"
        cur_minor="${_rest%%.*}"
        cur_major="${cur_major:-0}"
        cur_minor="${cur_minor:-0}"
        if { [ "$cur_major" -gt "$need_major" ] 2>/dev/null; } \
           || { [ "$cur_major" -eq "$need_major" ] 2>/dev/null && [ "$cur_minor" -ge "$need_minor" ] 2>/dev/null; }; then
            print_success "cmake $v is recent enough (>= ${need_major}.${need_minor})"
            return 0
        fi
        print_info "cmake $v is older than ${need_major}.${need_minor}; upgrading"
    else
        print_info "cmake not installed; installing recent version (>= ${need_major}.${need_minor})"
    fi

    if [ -n "${VERSION_CODENAME:-}" ] && [ "${RBT_SKIP_KITWARE_REPO:-}" != "1" ]; then
        print_info "Trying Kitware APT repo for cmake (codename: ${VERSION_CODENAME})..."
        apt-get install -y --no-install-recommends ca-certificates gnupg wget >/dev/null 2>&1 || true
        if wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null \
            | gpg --dearmor -o /usr/share/keyrings/kitware-archive-keyring.gpg 2>/dev/null \
            && [ -s /usr/share/keyrings/kitware-archive-keyring.gpg ]; then
            echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ ${VERSION_CODENAME} main" \
                > /etc/apt/sources.list.d/kitware.list
            if apt-get update >/dev/null 2>&1 && apt-get install -y --no-install-recommends cmake; then
                hash -r
                local v_new
                v_new="$(cmake --version 2>/dev/null | head -1 | awk '{print $3}')"
                print_success "Installed cmake $v_new from Kitware APT repo"
                return 0
            fi
            print_warning "Kitware APT install failed; falling back to pip"
        else
            print_warning "Could not reach apt.kitware.com; falling back to pip"
        fi
    fi

    if ! command -v pip3 >/dev/null 2>&1; then
        apt-get install -y --no-install-recommends python3-pip >/dev/null 2>&1 || true
    fi
    if command -v pip3 >/dev/null 2>&1; then
        if pip3 install --break-system-packages --upgrade cmake 2>/dev/null \
           || pip3 install --upgrade cmake; then
            hash -r
            local v_new
            v_new="$(cmake --version 2>/dev/null | head -1 | awk '{print $3}')"
            print_success "Installed cmake $v_new via pip"
            return 0
        fi
    fi

    print_error "Could not install cmake >= ${need_major}.${need_minor}"
    return 1
}

check_and_install_dependencies() {
    print_info "Checking for required build dependencies..."

    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS=$ID
    else
        print_error "Cannot detect OS. Please install dependencies manually."
        exit 1
    fi

    MISSING_TOOLS=()
    command -v cmake >/dev/null 2>&1 || MISSING_TOOLS+=("cmake")
    command -v make >/dev/null 2>&1 || MISSING_TOOLS+=("make")
    command -v gcc >/dev/null 2>&1 || MISSING_TOOLS+=("gcc")
    command -v g++ >/dev/null 2>&1 || MISSING_TOOLS+=("g++")
    command -v git >/dev/null 2>&1 || MISSING_TOOLS+=("git")
    command -v wget >/dev/null 2>&1 || MISSING_TOOLS+=("wget")
    command -v tar >/dev/null 2>&1 || MISSING_TOOLS+=("tar")
    command -v doxygen >/dev/null 2>&1 || MISSING_TOOLS+=("doxygen")
    command -v python3 >/dev/null 2>&1 || MISSING_TOOLS+=("python3")

    MISSING_LIBS=()
    if [[ "$OS" =~ ^(ubuntu|debian)$ ]]; then
        [ -f /usr/include/pci/pci.h ] || MISSING_LIBS+=("libpci-dev")
        [ -f /usr/include/numa.h ] || MISSING_LIBS+=("libnuma-dev")
        [ -f /usr/include/drm/drm.h ] || MISSING_LIBS+=("libdrm-dev")
        [ -f /usr/include/yaml-cpp/yaml.h ] || MISSING_LIBS+=("libyaml-cpp-dev")
        command -v rpmbuild >/dev/null 2>&1 || MISSING_LIBS+=("rpm")
        command -v unzip >/dev/null 2>&1 || MISSING_LIBS+=("unzip")
    elif [[ "$OS" =~ ^(centos|rhel|rocky|almalinux|amzn)$ ]]; then
        [ -f /usr/include/pci/pci.h ] || MISSING_LIBS+=("pciutils-devel")
        [ -f /usr/include/yaml-cpp/yaml.h ] || MISSING_LIBS+=("yaml-cpp-devel")
        command -v rpmbuild >/dev/null 2>&1 || MISSING_LIBS+=("rpm-build")
    fi

    MISSING_DEPS=()
    [ ${#MISSING_TOOLS[@]} -ne 0 ] && MISSING_DEPS+=("${MISSING_TOOLS[@]}")
    [ ${#MISSING_LIBS[@]} -ne 0 ] && MISSING_DEPS+=("${MISSING_LIBS[@]}")

    if [ ${#MISSING_DEPS[@]} -ne 0 ]; then
        print_warning "Missing dependencies: ${MISSING_DEPS[*]}"
        echo ""

        if [[ "$OS" =~ ^(ubuntu|debian)$ ]]; then
            print_info "Installing dependencies for Ubuntu/Debian..."
            apt-get update
            apt-get install -y \
                build-essential \
                cmake \
                patchelf \
                git \
                curl \
                libcurl4-openssl-dev \
                tar \
                libnuma-dev \
                libdrm-dev \
                wget \
                libpci3 \
                libpci-dev \
                doxygen \
                unzip \
                libyaml-cpp-dev \
                rpm \
                python3 \
                python3-pip \
                dpkg-dev \
                file \
                apt-utils
        elif [[ "$OS" =~ ^(centos|rhel|rocky|almalinux|amzn)$ ]]; then
            print_info "Installing dependencies for CentOS/RHEL/Rocky/AlmaLinux..."

            if [ -d /opt/python ]; then
                print_info "Detected manylinux environment - some tools may be pre-installed"
            fi

            print_info "Enabling PowerTools/CRB repository..."
            if command -v dnf >/dev/null 2>&1; then
                dnf install -y dnf-plugins-core 2>/dev/null || true
                dnf config-manager --set-enabled powertools 2>/dev/null || \
                dnf config-manager --set-enabled crb 2>/dev/null || \
                dnf config-manager --set-enabled devel 2>/dev/null || \
                print_warning "Could not enable PowerTools/CRB/devel repo (may already be enabled)"
            else
                yum install -y yum-utils 2>/dev/null || true
                yum-config-manager --enable powertools 2>/dev/null || \
                yum-config-manager --enable PowerTools 2>/dev/null || \
                yum-config-manager --enable devel 2>/dev/null || \
                print_warning "Could not enable PowerTools/devel repo (may already be enabled)"
            fi

            print_info "Installing EPEL repository..."
            yum install -y epel-release 2>/dev/null || print_warning "EPEL may already be installed"

            print_info "Installing build dependencies..."
            yum install -y \
                gcc \
                gcc-c++ \
                make \
                git \
                curl \
                libcurl-devel \
                wget \
                tar \
                numactl-devel \
                rpm-build \
                dpkg \
                createrepo_c \
                file \
                pciutils-devel \
                doxygen \
                python3 \
                python3-pip \
                || print_warning "Some packages may already be installed"

            print_info "Installing cmake..."
            yum install -y cmake3 || yum install -y cmake || print_warning "cmake installation may have failed"

            print_info "Installing yaml-cpp..."
            yum install -y yaml-cpp-devel yaml-cpp-static 2>/dev/null || \
                print_warning "yaml-cpp may not be available - will try to continue"

            ##
            # Install gcc-toolset for C++20
            if [[ "$OS" =~ ^(centos|rhel|almalinux|rocky)$ ]]; then
                GCC_TOOLSET_INSTALLED=""
                for ver in 14 13 12 11; do
                    if yum list available gcc-toolset-${ver} &>/dev/null; then
                        print_info "Found gcc-toolset-${ver} - installing for C++20 support..."
                        if yum install -y gcc-toolset-${ver}; then
                            GCC_TOOLSET_INSTALLED="gcc-toolset-${ver}"
                            print_success "Installed gcc-toolset-${ver}"
                            break
                        fi
                    fi
                done
                if [ -z "$GCC_TOOLSET_INSTALLED" ]; then
                    if yum list available devtoolset-11 &>/dev/null; then
                        print_info "Found devtoolset-11 - installing for C++20 support..."
                        if yum install -y devtoolset-11; then
                            GCC_TOOLSET_INSTALLED="devtoolset-11"
                            print_success "Installed devtoolset-11"
                        fi
                    fi
                fi
                if [ -z "$GCC_TOOLSET_INSTALLED" ]; then
                    print_warning "No gcc-toolset (11-14) or devtoolset-11 available"
                fi
            fi
        else
            print_error "Unsupported OS: $OS"
            exit 1
        fi

        print_success "Dependencies installed successfully"
        echo ""
    else
        print_success "All required dependencies found"
    fi
    echo ""
}

##
# Check and install dependencies (installs wget etc. - required before fetch_latest_rocm_version)
###
check_and_install_dependencies
ensure_recent_cmake_ubuntu

##
# Determine ROCm version
###
if [ -n "$ROCM_VERSION" ]; then
    print_info "Using specified ROCm version: $ROCM_VERSION"
elif [ "$ROCM_SDK_CHANNEL" = "nightly" ]; then
    print_info "No ROCm version specified; fetching latest from ROCm nightly index..."
    if [ -z "$ROCM_SDK_INDEX_URL" ]; then
        print_error "ROCM_SDK_CHANNEL=nightly requires ROCM_SDK_INDEX_URL"
        exit 1
    fi
    fetch_latest_rocm_version "$GPU_FAMILY"
    if [ -z "$ROCM_VERSION" ]; then
        print_error "Failed to determine ROCm nightly version"
        exit 1
    fi
elif [ "$ROCM_SDK_CHANNEL" = "release" ]; then
    print_info "No ROCm version specified; fetching latest release X.Y.Z..."
    if [ -z "$ROCM_SDK_RELEASE_URL" ]; then
        print_error "ROCM_SDK_CHANNEL=release requires ROCM_SDK_RELEASE_URL"
        exit 1
    fi
    fetch_latest_rocm_release_version "$GPU_FAMILY"
    if [ -z "$ROCM_VERSION" ]; then
        print_error "Failed to determine ROCm release version"
        exit 1
    fi
elif [ -n "$ROCM_SDK_RELEASE_URL" ]; then
    print_info "No ROCm version specified; resolving latest release..."
    fetch_latest_rocm_release_version "$GPU_FAMILY"
    if [ -z "$ROCM_VERSION" ]; then
        print_error "Failed to determine ROCm release version"
        exit 1
    fi
elif [ -n "$ROCM_SDK_INDEX_URL" ]; then
    print_info "No ROCm version specified, fetching latest from nightly index..."
    fetch_latest_rocm_version "$GPU_FAMILY"
    if [ -z "$ROCM_VERSION" ]; then
        print_error "Failed to determine ROCm version"
        exit 1
    fi
else
    print_error "ROCM_VERSION is required"
    print_info "Set ROCM_VERSION, or configure ROCM_SDK_RELEASE_URL or ROCM_SDK_INDEX_URL"
    echo ""
    echo "  export ROCM_VERSION=\"7.11.0a20260121\""
    echo "  ./build_packages_local.sh"
    echo ""
    exit 1
fi

apply_sdk_tarball_base_for_version "$ROCM_VERSION"

##
# Print configuration
echo "================================================================================"
echo "  ROCm Bandwidth Test (RBT) - Local Package Build Script"
echo "================================================================================"
print_info "ROCm Version: $ROCM_VERSION"
print_info "ROCm SDK channel: ${ROCM_SDK_CHANNEL:-auto}"
print_info "GPU Family: $GPU_FAMILY"
print_info "Build Type: $BUILD_TYPE"
print_info "Build Directory: $BUILD_DIR"
print_info "ROCm Install: $ROCM_INSTALL_DIR"
print_info "SDK Source: $ROCM_SDK_BASE_URL"
if [ -n "$UPLOAD_TARGET" ]; then
    print_info "Upload Target: $UPLOAD_TARGET"
fi
echo "================================================================================"
echo ""

##
# Step 1: Download ROCm SDK
print_info "Step 1: Downloading ROCm SDK tarball..."
TARBALL_URL=$(join_base_and_file "$ROCM_SDK_BASE_URL" "therock-dist-linux-${GPU_FAMILY}-${ROCM_VERSION}.tar.gz")
TARBALL_FILE="$ROCM_INSTALL_DIR/rocm-sdk.tar.gz"

mkdir -p "$ROCM_INSTALL_DIR"

if [ -f "$ROCM_INSTALL_DIR/install/bin/hipconfig" ]; then
    print_warning "ROCm SDK already exists at $ROCM_INSTALL_DIR/install"
    if [ -t 0 ]; then
        read -p "Do you want to re-download? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            rm -rf "$ROCM_INSTALL_DIR/install"
        else
            print_info "Using existing ROCm SDK"
            export ROCM_PATH="$ROCM_INSTALL_DIR/install"
            print_success "ROCm SDK path set to: $ROCM_PATH"
            echo ""
            goto_step2=true
        fi
    else
        print_info "Non-interactive mode: reusing existing ROCm SDK"
        export ROCM_PATH="$ROCM_INSTALL_DIR/install"
        print_success "ROCm SDK path set to: $ROCM_PATH"
        echo ""
        goto_step2=true
    fi
fi

if [ -z "$goto_step2" ]; then
    print_info "Downloading from: $TARBALL_URL"
    if wget --spider "$TARBALL_URL" 2>/dev/null; then
        wget --show-progress -O "$TARBALL_FILE" "$TARBALL_URL"
        print_success "Download complete"
    else
        print_error "Failed to download ROCm SDK tarball"
        print_error "URL: $TARBALL_URL"
        exit 1
    fi

    print_info "Extracting ROCm SDK..."
    mkdir -p "$ROCM_INSTALL_DIR/install"
    tar -xzf "$TARBALL_FILE" -C "$ROCM_INSTALL_DIR/install" --strip-components=1
    print_success "Extraction complete"

    export ROCM_PATH="$ROCM_INSTALL_DIR/install"
    print_success "ROCm SDK installed to: $ROCM_PATH"
fi
echo ""

##
# Step 2: Setup environment
print_info "Step 2: Setting up ROCm environment..."
print_info "Locating HIP device libraries (amdgcn/bitcode)..."
HIP_DEVICE_LIB_PATH=$(find "$ROCM_PATH" -type d -path "*/amdgcn/bitcode" 2>/dev/null | head -1)

if [ -z "$HIP_DEVICE_LIB_PATH" ]; then
    print_error "Could not find amdgcn/bitcode directory in $ROCM_PATH"
    exit 1
fi

export PATH="${ROCM_PATH}/bin:${PATH}"
export LD_LIBRARY_PATH="${ROCM_PATH}/lib:${LD_LIBRARY_PATH:-}"
export CMAKE_PREFIX_PATH="${ROCM_PATH}:${CMAKE_PREFIX_PATH:-}"
export HIP_DEVICE_LIB_PATH="${HIP_DEVICE_LIB_PATH}"

ROCM_VERSION_MAJOR_MINOR=$(echo "$ROCM_VERSION" | grep -oP '^\d+\.\d+')
if [ -z "$ROCM_VERSION_MAJOR_MINOR" ]; then
    print_error "Could not extract major.minor version from ROCM_VERSION: $ROCM_VERSION"
    exit 1
fi

ROCM_MAJOR=$(echo "$ROCM_VERSION_MAJOR_MINOR" | cut -d'.' -f1)
ROCM_MINOR=$(echo "$ROCM_VERSION_MAJOR_MINOR" | cut -d'.' -f2)
ROCM_LIBPATCH_VERSION=$(printf "%02d%02d" "$ROCM_MAJOR" "$ROCM_MINOR")

export ROCM_LIBPATCH_VERSION
export ROCM_MAJOR
print_success "Set ROCM_LIBPATCH_VERSION=$ROCM_LIBPATCH_VERSION, ROCM_MAJOR=$ROCM_MAJOR"

##
# Git safe directory (for Actions containers)
if [ -n "${GITHUB_WORKSPACE:-}" ] && command -v git >/dev/null 2>&1; then
    git config --global --add safe.directory "$GITHUB_WORKSPACE" 2>/dev/null || true
fi

RELEASE_DATE="$(date +%Y%m%d)"
BASE_PACKAGE_RELEASE="r${ROCM_LIBPATCH_VERSION}.${RELEASE_DATE}"

if [ "$GITHUB_EVENT_NAME" = "pull_request" ] && [ -n "${GITHUB_HEAD_REF:-}" ]; then
    GIT_BRANCH="${GITHUB_HEAD_REF}"
else
    GIT_BRANCH="${GITHUB_REF_NAME:-$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")}"
fi
if [ -n "$GITHUB_SHA" ]; then
    GIT_COMMIT_SHORT="${GITHUB_SHA:0:7}"
else
    GIT_COMMIT_SHORT=$(git rev-parse --short HEAD 2>/dev/null || echo "0000000")
fi
SANITIZED_BRANCH=$(echo "$GIT_BRANCH" | sed 's/[^A-Za-z0-9.+~]/./g')

if [ "${GITHUB_EVENT_NAME}" != "pull_request" ] && [[ "$GIT_BRANCH" =~ ^rel ]]; then
    GITHUB_RUN_NUMBER="${GITHUB_RUN_NUMBER:-1}"
    PACKAGE_RELEASE="$GITHUB_RUN_NUMBER"
    print_success "Set CPACK package release: $PACKAGE_RELEASE (release branch)"
elif [ "$GITHUB_EVENT_NAME" = "pull_request" ]; then
    PACKAGE_RELEASE="${BASE_PACKAGE_RELEASE}.${SANITIZED_BRANCH}.${GIT_COMMIT_SHORT}"
    print_success "Set CPACK package release: $PACKAGE_RELEASE (PR)"
else
    PACKAGE_RELEASE="$BASE_PACKAGE_RELEASE"
    print_success "Set CPACK package release: $PACKAGE_RELEASE"
fi

export CPACK_DEBIAN_PACKAGE_RELEASE="$PACKAGE_RELEASE"
export CPACK_RPM_PACKAGE_RELEASE="$PACKAGE_RELEASE"

if [[ "$OS" =~ ^(centos|rhel|almalinux|rocky)$ ]]; then
    GCC_TOOLSET_ENABLED=""
    for pkg in gcc-toolset-14 gcc-toolset-13 gcc-toolset-12 gcc-toolset-11 devtoolset-11; do
        ENABLE_SCRIPT=$(rpm -ql ${pkg}-runtime 2>/dev/null | grep '/enable$' | head -1)
        if [ -n "$ENABLE_SCRIPT" ] && [ -f "$ENABLE_SCRIPT" ]; then
            TOOLSET_ROOT=$(dirname "$ENABLE_SCRIPT")
            source "$ENABLE_SCRIPT"
            export GCC_TOOLCHAIN="${TOOLSET_ROOT}/root/usr"
            GCC_TOOLSET_ENABLED="$pkg"
            print_success "Enabled ${pkg} from ${TOOLSET_ROOT}"
            break
        fi
    done
    if [ -z "$GCC_TOOLSET_ENABLED" ]; then
        print_warning "No gcc-toolset found"
    fi

    if [ -x "$ROCM_PATH/bin/hipcc" ]; then
        export CMAKE_CXX_COMPILER="$ROCM_PATH/bin/hipcc"
        print_success "Set CMAKE_CXX_COMPILER to hipcc"
    fi

    export CMAKE_COMMAND="cmake3"
    print_info "Using cmake3"
else
    export CMAKE_COMMAND="cmake"
    print_info "Using cmake"

    if [[ "$OS" =~ ^(ubuntu|debian)$ ]]; then
        if [ -x "$ROCM_PATH/bin/hipcc" ]; then
            export CMAKE_CXX_COMPILER="$ROCM_PATH/bin/hipcc"
            print_success "Set CMAKE_CXX_COMPILER to hipcc"
        fi
        if compgen -G "/usr/include/c++/*/barrier" >/dev/null 2>&1; then
            export GCC_TOOLCHAIN="/usr"
            print_success "Set GCC_TOOLCHAIN=/usr so hipcc finds system libstdc++ (C++20 <barrier>)"
        else
            print_warning "No /usr/include/c++/.../barrier found; install g++ 11+ (e.g. apt install g++-11 build-essential)"
        fi
    fi
fi

print_info "Environment variables set:"
echo "  ROCM_PATH=$ROCM_PATH"
echo "  HIP_DEVICE_LIB_PATH=$HIP_DEVICE_LIB_PATH"
echo "  ROCM_LIBPATCH_VERSION=$ROCM_LIBPATCH_VERSION"
if [ -n "${CMAKE_CXX_COMPILER:-}" ]; then
    echo "  CMAKE_CXX_COMPILER=$CMAKE_CXX_COMPILER"
fi
if [ -n "${GCC_TOOLCHAIN:-}" ]; then
    echo "  GCC_TOOLCHAIN=$GCC_TOOLCHAIN"
fi
echo "  CMAKE_COMMAND=$CMAKE_COMMAND"

##
# Verify ROCm installation
print_info "Verifying ROCm installation..."
if [ -x "$ROCM_PATH/bin/hipconfig" ]; then
    print_success "hipconfig found"
else
    print_warning "hipconfig not found"
fi

if [ -d "$ROCM_PATH/lib" ]; then
    LIB_COUNT=$(ls -1 "$ROCM_PATH/lib"/*.so 2>/dev/null | wc -l)
    print_success "Found $LIB_COUNT shared libraries"
else
    print_error "ROCm lib directory not found"
    exit 1
fi
echo ""

##
# Step 3: Configure CMake
print_info "Step 3: Configuring CMake with relocatable paths..."
if [ -d "$BUILD_DIR" ]; then
    print_warning "Build directory exists. Cleaning..."
    rm -rf "$BUILD_DIR"
fi

INSTALL_PREFIX="/opt/rocm/extras-${ROCM_MAJOR}"
RPATH_LIST="\$ORIGIN:\$ORIGIN/../lib:${INSTALL_PREFIX}/lib:/opt/rocm/lib:/opt/rocm/lib64"

mkdir -p "${BUILD_DIR}"

CMAKE_ARGS=(
    -B "$BUILD_DIR"
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DROCM_PATH="$ROCM_PATH"
    -DROCM_MAJOR_VERSION="$ROCM_MAJOR"
    -DHIP_PLATFORM=amd

    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"
    -DCPACK_PACKAGING_INSTALL_PREFIX="${INSTALL_PREFIX}"
    -DCMAKE_SKIP_RPATH=FALSE
    -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=FALSE
    -DCMAKE_INSTALL_RPATH="${RPATH_LIST}"
    -DRPATH_MODE=OFF
    -DCMAKE_VERBOSE_MAKEFILE=ON
    -DFETCH_ROCMPATH_FROM_ROCMCORE=ON
    -DAMD_APP_BUILD_RELOCATABLE_PACKAGE=ON
    -DAMD_APP_STANDALONE_BUILD_PACKAGE=OFF
    -DAMD_APP_ROCM_BUILD_PACKAGE=OFF
    -DCMAKE_MODULE_PATH="${ROCM_BANDWIDTH_TEST_ROOT}/cmake/modules"
)

if [ -n "${CMAKE_CXX_COMPILER:-}" ]; then
    CMAKE_ARGS+=(-DCMAKE_CXX_COMPILER="${CMAKE_CXX_COMPILER}")
fi

if [ -n "${GCC_TOOLCHAIN:-}" ]; then
    CMAKE_ARGS+=(-DCMAKE_CXX_FLAGS="--gcc-toolchain=$GCC_TOOLCHAIN")
    print_success "Set --gcc-toolchain=$GCC_TOOLCHAIN"
fi

$CMAKE_COMMAND "${CMAKE_ARGS[@]}"

if [ $? -eq 0 ]; then
    print_success "CMake configuration successful"
else
    print_error "CMake configuration failed"
    exit 1
fi
echo ""

##
# Step 4: Build RBT
print_info "Step 4: Building RBT..."
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
print_info "Using $NPROC parallel jobs"

make -C "$BUILD_DIR" -j"$NPROC"

if [ $? -eq 0 ]; then
    print_success "Build successful"
else
    print_error "Build failed"
    exit 1
fi
echo ""

##
# Step 5: Create packages
print_info "Step 5: Creating packages..."

cd "$BUILD_DIR"

##
# DEB: when dpkg-deb is available (Debian/Ubuntu and manylinux with dpkg installed)
if command -v dpkg-deb >/dev/null 2>&1; then
    print_info "Creating DEB package..."
    cpack -G DEB --verbose

    if [ $? -eq 0 ]; then
        DEB_FILE=$(ls amdrocm*-rbt*.deb 2>/dev/null | head -1)
        if [ -n "$DEB_FILE" ]; then
            print_success "Created DEB package: $DEB_FILE"
            DEB_SIZE=$(du -h "$DEB_FILE" | cut -f1)
            print_info "Package size: $DEB_SIZE"

            print_info "Verifying DEB package..."
            dpkg-deb -I "$DEB_FILE" | head -20
        fi
    else
        print_warning "DEB package creation failed"
    fi
fi

##
# RPM: when rpmbuild is available (RPM-based systems)
if command -v rpmbuild >/dev/null 2>&1; then
    print_info "Creating RPM package..."
    cpack -G RPM --verbose

    if [ $? -eq 0 ]; then
        RPM_FILE=$(ls amdrocm*-rbt*.rpm 2>/dev/null | head -1)
        if [ -n "$RPM_FILE" ]; then
            print_success "Created RPM package: $RPM_FILE"
            RPM_SIZE=$(du -h "$RPM_FILE" | cut -f1)
            print_info "Package size: $RPM_SIZE"

            print_info "Verifying RPM package..."
            rpm -qip "$RPM_FILE" | head -20
        fi
    else
        print_warning "RPM package creation failed"
    fi
fi

##
# TGZ: always built; cmake already configured CPACK_GENERATOR="DEB;RPM;TGZ"
# for AMD_APP_BUILD_RELOCATABLE_PACKAGE=ON — no second cmake pass needed.
print_info "Creating TGZ package..."
cpack -G TGZ --verbose

if [ $? -eq 0 ]; then
    TGZ_FILE=$(ls amdrocm*-rbt*.tar.gz 2>/dev/null | head -1)
    if [ -n "$TGZ_FILE" ]; then
        print_success "Created TGZ package: $TGZ_FILE"
        TGZ_SIZE=$(du -h "$TGZ_FILE" | cut -f1)
        print_info "Package size: $TGZ_SIZE"
    fi
else
    print_error "TGZ package creation failed"
fi

cd - > /dev/null
echo ""

##
# Step 6: Upload packages (optional)
if [ -n "$UPLOAD_TARGET" ]; then
    print_info "Step 6: Uploading packages to $UPLOAD_TARGET..."

    if [ -z "$UPLOAD_REPO" ]; then
        UPLOAD_REPO="${GITHUB_REPOSITORY##*/}"
        [ -z "$UPLOAD_REPO" ] && UPLOAD_REPO=$(git remote get-url origin 2>/dev/null | sed 's|.*/||; s|\.git$||')
        [ -z "$UPLOAD_REPO" ] && UPLOAD_REPO=$(basename "$(git rev-parse --show-toplevel 2>/dev/null)" 2>/dev/null)
        [ -z "$UPLOAD_REPO" ] && UPLOAD_REPO="rocm_bandwidth_test"
    fi
    UPLOAD_BRANCH="${GITHUB_REF_NAME:-$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")}"
    UPLOAD_BRANCH=$(echo "$UPLOAD_BRANCH" | sed 's|[^a-zA-Z0-9._-]|-|g')
    UPLOAD_DATE=$(date +%Y-%m-%d)
    UPLOAD_SUBPATH="${UPLOAD_REPO}/${UPLOAD_BRANCH}/${UPLOAD_DATE}"

    print_info "Upload path: .../${UPLOAD_SUBPATH}/"

    PKGS=$(find "$BUILD_DIR" -maxdepth 1 -name 'amdrocm*-rbt*' \( -name '*.deb' -o -name '*.rpm' -o -name '*.tar.gz' \) 2>/dev/null)
    if [ -z "$PKGS" ]; then
        print_error "No packages found to upload"
    else
        UPLOAD_OK=true

        case "$UPLOAD_TARGET" in
            scp://*)
                SCP_DEST="${UPLOAD_TARGET#scp://}"
                SCP_DEST="${SCP_DEST%/}/${UPLOAD_SUBPATH}/"
                print_info "Uploading via SCP to $SCP_DEST"
                SCP_HOST="${SCP_DEST%%:*}"
                SCP_PATH="${SCP_DEST#*:}"
                ssh "$SCP_HOST" "mkdir -p '$SCP_PATH'" 2>/dev/null || true
                for pkg in $PKGS; do
                    print_info "  $(basename "$pkg")"
                    if ! scp "$pkg" "$SCP_DEST"; then
                        print_error "SCP upload failed for $(basename "$pkg")"
                        UPLOAD_OK=false
                    fi
                done
                ;;
            rsync://*)
                RSYNC_DEST="${UPLOAD_TARGET#rsync://}"
                RSYNC_DEST="${RSYNC_DEST%/}/${UPLOAD_SUBPATH}/"
                print_info "Uploading via rsync to $RSYNC_DEST"
                if ! echo "$PKGS" | xargs -I{} rsync -avz --progress {} "$RSYNC_DEST"; then
                    print_error "Rsync upload failed"
                    UPLOAD_OK=false
                fi
                ;;
            http://*|https://*)
                UPLOAD_URL="${UPLOAD_TARGET%/}/${UPLOAD_SUBPATH}"
                print_info "Uploading via HTTP PUT to $UPLOAD_URL/"
                for pkg in $PKGS; do
                    local_name=$(basename "$pkg")
                    print_info "  $local_name"
                    HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" \
                        -X PUT -T "$pkg" \
                        "${UPLOAD_URL}/${local_name}")
                    if [ "$HTTP_CODE" -ge 200 ] && [ "$HTTP_CODE" -lt 300 ]; then
                        print_success "  Uploaded $local_name (HTTP $HTTP_CODE)"
                    else
                        print_error "  Failed to upload $local_name (HTTP $HTTP_CODE)"
                        UPLOAD_OK=false
                    fi
                done
                if [[ "$UPLOAD_TARGET" =~ localhost|127\.0\.0\.1 ]]; then
                    SHARE_IP=$(ip -4 route get 1.1.1.1 2>/dev/null | grep -oP 'src \K[0-9.]+' | head -1)
                    [ -z "$SHARE_IP" ] && SHARE_IP=$(hostname -I 2>/dev/null | awk '{print $1}')
                    if [ -n "$SHARE_IP" ]; then
                        SHARE_URL=$(echo "$UPLOAD_URL" | sed "s|localhost|$SHARE_IP|;s|127\.0\.0\.1|$SHARE_IP|")
                        print_info "Share this URL: ${SHARE_URL}/"
                    fi
                fi
                print_info "Browse uploads: ${UPLOAD_URL}/"
                ;;
            *)
                LOCAL_DEST="${UPLOAD_TARGET%/}/${UPLOAD_SUBPATH}"
                print_info "Copying packages to $LOCAL_DEST"
                mkdir -p "$LOCAL_DEST"
                for pkg in $PKGS; do
                    print_info "  $(basename "$pkg")"
                    if ! cp "$pkg" "$LOCAL_DEST/"; then
                        print_error "Copy failed for $(basename "$pkg")"
                        UPLOAD_OK=false
                    fi
                done
                ;;
        esac

        if [ "$UPLOAD_OK" = true ]; then
            print_success "All packages uploaded successfully"
        else
            print_warning "Some uploads failed"
        fi
    fi
    echo ""
fi

##
# Summary
echo "================================================================================"
print_success "Package build completed successfully!"
echo "================================================================================"
print_info "Generated packages are in: $BUILD_DIR"
echo ""
PKGS=$(find "$BUILD_DIR" -maxdepth 1 -name "amdrocm*-rbt*" \( -name '*.deb' -o -name '*.rpm' -o -name '*.tar.gz' \) 2>/dev/null)
if [ -n "$PKGS" ]; then
    echo "$PKGS" | xargs ls -lh
else
    print_warning "No packages found in build directory"
fi
echo ""

##
# Installation instructions
echo "================================================================================"
echo "  Installation Instructions"
echo "================================================================================"
echo ""
echo "Ubuntu/Debian (DEB):"
echo "  sudo dpkg -i $BUILD_DIR/amdrocm${ROCM_MAJOR}-rbt_*.deb"
echo ""
echo "CentOS/RHEL (RPM):"
echo "  sudo rpm -i --replacefiles --nodeps $BUILD_DIR/amdrocm${ROCM_MAJOR}-rbt-*.rpm"
echo ""
echo "Any Linux (TGZ - Relocatable):"
echo "  sudo mkdir -p /opt/rocm/extras-${ROCM_MAJOR}"
echo "  sudo tar -xzf $BUILD_DIR/amdrocm${ROCM_MAJOR}-rbt-*.tar.gz -C /opt/rocm/extras-${ROCM_MAJOR}"
echo "  export PATH=/opt/rocm/extras-${ROCM_MAJOR}/bin:\$PATH"
echo "  export LD_LIBRARY_PATH=/opt/rocm/extras-${ROCM_MAJOR}/lib:\$LD_LIBRARY_PATH"
echo ""
echo "================================================================================"

print_success "Done!"
