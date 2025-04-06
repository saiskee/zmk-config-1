#!/bin/bash

# This script should be run inside the container

# Exit if any command fails
set -e

# Function to display help
show_help() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Build ZMK firmware based on configuration in the current directory."
    echo ""
    echo "Options:"
    echo "  -h, --help             Show this help message and exit"
    echo "  -b, --board BOARD      Board to build for (default: eyelash_corne_tp_left)"
    echo "  -c, --clean            Clean the build directory before building"
    echo "  -r, --reset            Build the settings_reset firmware"
    echo ""
    echo "Example: $0 -b eyelash_corne_tp_right"
}

# Default values
BOARD="eyelash_corne_tp_left"
CLEAN=false
RESET=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -b|--board)
            BOARD="$2"
            shift
            shift
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -r|--reset)
            RESET=true
            BOARD="eyelash_corne_tp_right"
            shift
            ;;
        *)
            echo "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Set shield based on reset flag
if [[ "$RESET" == true ]]; then
    SHIELD="settings_reset"
    echo "Building settings_reset firmware for $BOARD"
else
    SHIELD="nice_view"
    echo "Building firmware for $BOARD with $SHIELD shield"
fi

# Clean build directory if requested
if [[ "$CLEAN" == true ]]; then
    echo "Cleaning build directory..."
    rm -rf build
fi

# Build the firmware
echo "Building firmware..."
if [[ "$RESET" == true ]]; then
    west build -p -b $BOARD -- -DSHIELD=$SHIELD
else
    west build -p -b $BOARD -- -DSHIELD=$SHIELD
fi

echo "Build complete!"
echo "Firmware file location: build/zephyr/zmk.uf2"
