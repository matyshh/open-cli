#!/bin/bash

# OpenCLI Installation Script for Android/Termux
# Auto-detects architecture and installs the appropriate binary

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
GITHUB_REPO="matyshh/open-cli"
RELEASE_TAG="0.1beta"
BASE_URL="https://github.com/${GITHUB_REPO}/releases/download/${RELEASE_TAG}"

# Functions
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running on Android/Termux
check_termux() {
    if [[ ! -d "$PREFIX" ]] || [[ "$PREFIX" != *"com.termux"* ]]; then
        print_error "This script is designed for Android/Termux environment"
        print_info "Please run this script in Termux app"
        exit 1
    fi
}

# Auto-detect architecture
detect_architecture() {
    print_info "Detecting system architecture..."
    
    # Try dpkg first (most reliable in Termux)
    if command -v dpkg > /dev/null 2>&1; then
        local dpkg_arch=$(dpkg --print-architecture 2>/dev/null)
        case "$dpkg_arch" in
            aarch64|arm64)
                ARCH="arm64"
                ZIP_NAME="opencli-arm64.zip"
                ;;
            armhf|armv7l|arm)
                ARCH="arm32"
                ZIP_NAME="opencli-arm32.zip"
                ;;
            *)
                print_warning "Unknown dpkg architecture: $dpkg_arch"
                ;;
        esac
    fi
    
    # Fallback to uname if dpkg failed
    if [[ -z "$ARCH" ]]; then
        local uname_arch=$(uname -m)
        case "$uname_arch" in
            aarch64|arm64)
                ARCH="arm64"
                ZIP_NAME="opencli-arm64.zip"
                ;;
            armv7l|armhf|arm)
                ARCH="arm32"
                ZIP_NAME="opencli-arm32.zip"
                ;;
            *)
                print_error "Unsupported architecture: $uname_arch"
                print_info "Supported architectures: arm64, arm32"
                exit 1
                ;;
        esac
    fi
    
    print_success "Detected architecture: $ARCH"
    print_info "Will download package: $ZIP_NAME"
}

# Check for required tools
check_dependencies() {
    print_info "Checking required dependencies..."
    
    local missing_tools=()
    
    # Check for download tools
    if ! command -v wget > /dev/null 2>&1 && ! command -v curl > /dev/null 2>&1; then
        missing_tools+=("wget or curl")
    fi
    
    # Check for unzip (required for .zip extraction)
    if ! command -v unzip > /dev/null 2>&1; then
        missing_tools+=("unzip")
    fi
    
    if [[ ${#missing_tools[@]} -gt 0 ]]; then
        print_error "Missing required tools: ${missing_tools[*]}"
        print_info "Please install missing tools:"
        for tool in "${missing_tools[@]}"; do
            case "$tool" in
                "wget or curl")
                    print_info "  pkg install wget"
                    print_info "  # OR"
                    print_info "  pkg install curl"
                    ;;
                "unzip")
                    print_info "  pkg install unzip"
                    ;;
            esac
        done
        exit 1
    fi
    
    print_success "All dependencies available"
}

# Download and extract binary
download_binary() {
    local download_url="${BASE_URL}/${ZIP_NAME}"
    local temp_zip="/tmp/opencli.zip"
    local temp_dir="/tmp/opencli_extract"
    
    print_info "Downloading OpenCLI from: $download_url"
    
    # Clean up any previous extraction
    rm -rf "$temp_dir"
    mkdir -p "$temp_dir"
    
    # Try wget first, then curl
    if command -v wget > /dev/null 2>&1; then
        if wget -q --show-progress -O "$temp_zip" "$download_url"; then
            print_success "Download completed with wget"
        else
            print_error "Download failed with wget"
            return 1
        fi
    elif command -v curl > /dev/null 2>&1; then
        if curl -L -o "$temp_zip" "$download_url"; then
            print_success "Download completed with curl"
        else
            print_error "Download failed with curl"
            return 1
        fi
    else
        print_error "No download tool available"
        return 1
    fi
    
    # Verify download
    if [[ ! -f "$temp_zip" ]] || [[ ! -s "$temp_zip" ]]; then
        print_error "Downloaded file is missing or empty"
        return 1
    fi
    
    # Extract the zip file
    print_info "Extracting ZIP package..."
    if unzip -q "$temp_zip" -d "$temp_dir"; then
        print_success "Extraction completed"
    else
        print_error "Failed to extract ZIP package"
        return 1
    fi
    
    # Find the opencli binary in extracted files
    local binary_path=$(find "$temp_dir" -name "opencli" -type f -executable | head -1)
    if [[ -z "$binary_path" ]]; then
        # Try without executable check (permissions might not be set yet)
        binary_path=$(find "$temp_dir" -name "opencli" -type f | head -1)
    fi
    
    if [[ -z "$binary_path" ]]; then
        print_error "OpenCLI binary not found in extracted files"
        print_info "Available files:"
        find "$temp_dir" -type f
        return 1
    fi
    
    print_success "Found OpenCLI binary: $binary_path"
    EXTRACTED_BINARY="$binary_path"
    TEMP_DIR="$temp_dir"
    TEMP_ZIP="$temp_zip"
    return 0
}

# Install binary
install_binary() {
    local target_path="$PREFIX/bin/opencli"
    
    # Check if opencli already exists
    if [[ -f "$target_path" ]]; then
        print_warning "OpenCLI is already installed at: $target_path"
        echo -n "Do you want to overwrite it? [y/N]: "
        read -r response
        case "$response" in
            [yY]|[yY][eE][sS])
                print_info "Overwriting existing installation..."
                ;;
            *)
                print_info "Installation cancelled by user"
                exit 0
                ;;
        esac
    fi
    
    # Copy and set permissions
    print_info "Installing OpenCLI to: $target_path"
    
    if cp "$DOWNLOADED_FILE" "$target_path"; then
        print_success "Binary copied successfully"
    else
        print_error "Failed to copy binary"
        exit 1
    fi
    
    if chmod +x "$target_path"; then
        print_success "Execute permissions set"
    else
        print_error "Failed to set execute permissions"
        exit 1
    fi
    
    # Verify installation
    if [[ -x "$target_path" ]]; then
        print_success "OpenCLI installed successfully!"
        
        # Test the binary
        print_info "Testing OpenCLI installation..."
        if "$target_path" --help > /dev/null 2>&1; then
            print_success "OpenCLI is working correctly"
        else
            print_warning "OpenCLI may have issues (--help failed)"
            print_info "You can still try to use it"
        fi
    else
        print_error "Installation verification failed"
        exit 1
    fi
}

# Cleanup
cleanup() {
    if [[ -n "$DOWNLOADED_FILE" ]] && [[ -f "$DOWNLOADED_FILE" ]]; then
        rm -f "$DOWNLOADED_FILE"
        print_info "Cleaned up temporary files"
    fi
}

# Show usage information
show_usage() {
    print_success "OpenCLI is now installed and ready to use!"
    echo
    print_info "Usage examples:"
    echo "  opencli setup     - Initialize a new open.mp project"
    echo "  opencli build     - Build your Pawn scripts"
    echo "  opencli run       - Run your compiled scripts"
    echo "  opencli install   - Install compiler versions"
    echo "  opencli --help    - Show detailed help"
    echo
    print_info "Configuration:"
    echo "  Edit opencli.toml in your project directory to configure"
    echo "  include paths, compiler versions, and build settings"
    echo
    print_info "For more information, visit:"
    echo "  https://github.com/${GITHUB_REPO}"
}

# Main installation flow
main() {
    echo "=================================="
    echo "   OpenCLI Installer for Termux   "
    echo "=================================="
    echo
    
    # Set trap for cleanup
    trap cleanup EXIT
    
    # Run installation steps
    check_termux
    detect_architecture
    check_dependencies
    
    if download_binary; then
        install_binary
        show_usage
    else
        print_error "Installation failed during download"
        exit 1
    fi
}

# Run main function
main "$@"
