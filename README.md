# Open CLI

A command-line interface for managing open.mp servers

## Features

- Run open.mp servers with configurable options
- Compile Pawn scripts with automatic compiler installation
- Configuration via pawncli.toml
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

The compiler will be installed to `%APPDATA%\pawncli\compiler\<version>` (Windows) or `~/.config/pawncli/compiler/<version>` (Linux/macOS).

## Configuration

### pawncli.toml

You can configure the build settings in `pawncli.toml`:

```toml
[build]
entry_file = "gamemodes/main.pwn"
output_file = "gamemodes/main.amx"
compiler_version = "v3.10.10"

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
