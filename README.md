# Open CLI

A command-line interface for managing open.mp servers

## Features

- Run open.mp servers with configurable options
- Compile Pawn scripts with automatic compiler installation
- Configuration via opencli.toml
- Directly install Pawn compiler versions

## Inspiration

This project is inspired by [sampctl](https://github.com/southclaws/sampctl), a package manager and server launcher for San Andreas Multiplayer.

## Building

### Requirements

- CMake 3.10+
- C Compiler with C11 support
- libcurl (for non-Windows platforms)

### Build Steps

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .
```

## Installation for Android/Termux

opencli provides a convenient installation script for Android/Termux users that automatically detects your device architecture and installs the appropriate binary.

### Prerequisites

Before running the installation script, make sure you have the required tools installed in Termux:

```bash
# Update package list
pkg update

# Install required dependencies
pkg install curl unzip
```

### Installation Steps

1. **Download the installation script:**
```bash
curl -L -o install.sh https://raw.githubusercontent.com/matyshh/open-cli/master/install.sh
```

2. **Make the script executable:**
```bash
chmod +x install.sh
```

3. **Run the installation script:**
```bash
./install.sh
```

The script will:
- Download the appropriate opencli binary from the latest release
- Install it to `$PREFIX/bin/opencli`
- Set proper execution permissions
- Verify the installation

### Verification

After installation, verify that opencli is working correctly:

```bash
# Check if opencli is available
opencli --help

# Initialize a new project
opencli setup
```

### Troubleshooting

If you encounter any issues during installation:

1. **Permission denied errors:** Make sure you have proper write permissions in Termux
2. **Download failures:** Check your internet connection and try again
3. **Architecture detection issues:** The script supports ARM32 and ARM64 architectures
4. **Missing dependencies:** Ensure `curl` and `unzip` are installed via `pkg install curl unzip`

For additional help, visit the [GitHub repository](https://github.com/matyshh/open-cli) or check the issues section.

## Usage

### Running a server

```bash
# Run with default settings
opencli run

# Run with custom server path
opencli run --server-path /path/to/server

# Run with custom config
opencli run --config /path/to/config.json

# Show help
opencli run --help
```

### Compiling Pawn scripts

```bash
# Compile with default settings
opencli build

# Compile a specific file
opencli build --input path/to/script.pwn

# Specify output file
opencli build --output path/to/output.amx

# Use a specific compiler version
opencli build --compiler v3.10.11

# Add include directories
opencli build --includes path/to/includes

# Show help
opencli build --help
```

### Installing Pawn compiler

```bash
# Install with default version (v3.10.11)
opencli install compiler

# Install a specific version
opencli install compiler --version 3.10.9

# Show help
opencli install --help
```

The compiler will be installed to `%APPDATA%\opencli\compiler\<version>` (Windows) or `~/.config/opencli/compiler/<version>` (Linux/macOS).

## Configuration

### opencli.toml

You can configure the build settings in `opencli.toml`:

```toml
[build]
entry_file = "gamemodes/main.pwn"
output_file = "gamemodes/main.amx"
compiler_version = "v3.10.11"

[build.includes]
paths = [
    "qawno/include"
]

[build.args]
# Default compiler args
args = [
    "-d3",
    "-;+",
    "-(+",
    "-\\+",
    "-Z+",
    "-O1"
]
``` 
