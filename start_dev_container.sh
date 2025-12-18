#!/bin/bash
set -e

CONTAINER_NAME="mirror_hash_dev"
IMAGE_NAME="mirror_hash:latest"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== mirror_hash Development Environment ==="
echo ""

# Check if container already exists
if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Container '${CONTAINER_NAME}' already exists."

    # Check if it's running
    if docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
        echo "Container is running. Attaching..."
        docker exec -it ${CONTAINER_NAME} /bin/bash
    else
        echo "Starting existing container..."
        docker start -ai ${CONTAINER_NAME}
    fi
    exit 0
fi

# Check if image exists
if ! docker images --format '{{.Repository}}:{{.Tag}}' | grep -q "^${IMAGE_NAME}$"; then
    echo "Image '${IMAGE_NAME}' not found."
    echo ""
    echo "Building from source (requires ~60 min and ~20GB disk)..."
    echo "This compiles LLVM/Clang with C++26 reflection support."
    echo ""
    read -p "Continue? [y/N]: " choice

    if [[ "$choice" =~ ^[Yy]$ ]]; then
        docker build -t ${IMAGE_NAME} ${SCRIPT_DIR}
    else
        echo "Aborted."
        exit 1
    fi
fi

echo "Creating container '${CONTAINER_NAME}'..."
docker run -it \
    --name ${CONTAINER_NAME} \
    -v "${SCRIPT_DIR}:/workspace" \
    ${IMAGE_NAME} \
    /bin/bash
