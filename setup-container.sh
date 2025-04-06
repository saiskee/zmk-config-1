#!/bin/bash

# Set variables
CONTAINER_NAME="zmk-dev-container"
IMAGE_NAME="zmk-dev-image"
CONFIG_DIR="$(pwd)"
ZMK_CONFIG_DIR="/workspaces/zmk-config-1"

# Check if container already exists
if podman container exists $CONTAINER_NAME; then
    echo "Container $CONTAINER_NAME already exists."
    echo "Starting existing container..."
    podman start $CONTAINER_NAME
else
    # Build the image
    echo "Building Docker image..."
    podman build -t $IMAGE_NAME -f Dockerfile .

    # Create and start the container
    echo "Creating and starting container..."
    podman run -d \
        --name $CONTAINER_NAME \
        -v "$CONFIG_DIR:$ZMK_CONFIG_DIR" \
        -w $ZMK_CONFIG_DIR \
        $IMAGE_NAME
fi

echo "Container is running in the background."
echo ""
echo "To execute commands in the container, use:"
echo "podman exec -it $CONTAINER_NAME bash"
echo ""
echo "To build your ZMK firmware inside the container, use:"
echo "podman exec -it $CONTAINER_NAME bash -c 'cd $ZMK_CONFIG_DIR && west build -p -b eyelash_corne_tp_left'"
echo ""
echo "To stop the container, use:"
echo "podman stop $CONTAINER_NAME"
