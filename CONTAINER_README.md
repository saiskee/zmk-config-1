# ZMK Development Container with Podman

This directory contains files to set up a persistent development container using Podman for ZMK firmware development.

## Prerequisites

- Podman installed on your system
- Git clone of your ZMK config repository

## Setup Instructions

1. Make sure you're in your ZMK config repository directory
2. Run the setup script to create and start the container:

```bash
./setup-container.sh
```

This will:
- Build a Docker image based on the ZMK development image
- Create a persistent container named `zmk-dev-container`
- Mount your current directory to `/workspaces/zmk-config-1` in the container

## Usage

### Entering the Container

To get a shell inside the container:

```bash
podman exec -it zmk-dev-container bash
```

### Building Firmware

Once inside the container, you can build your firmware using the provided build script:

```bash
./build-firmware.sh
```

Options:
- `-b, --board BOARD`: Specify the board (default: eyelash_corne_tp_left)
- `-c, --clean`: Clean the build directory before building
- `-r, --reset`: Build the settings_reset firmware

Examples:
```bash
# Build for left half
./build-firmware.sh -b eyelash_corne_tp_left

# Build for right half
./build-firmware.sh -b eyelash_corne_tp_right

# Build settings reset firmware
./build-firmware.sh -r

# Clean build directory and rebuild
./build-firmware.sh -c
```

The built firmware will be located at `build/zephyr/zmk.uf2`. Copy this file to your keyboard when in bootloader mode.

### Container Management

- Start the container: `podman start zmk-dev-container`
- Stop the container: `podman stop zmk-dev-container`
- Remove the container: `podman rm zmk-dev-container`
- List running containers: `podman ps`

## Workflow

1. Make changes to your ZMK config files in your host system
2. Enter the container and build the firmware
3. Flash the firmware to your keyboard
4. Repeat as needed

Since the container is persistent, you don't need to rebuild it each time - just start it when needed and stop it when done.
