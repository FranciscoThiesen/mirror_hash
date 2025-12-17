#!/bin/bash
set -e

CONTAINER_NAME="reflect_hash_dev"
IMAGE_NAME="reflect_hash:latest"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== reflect_hash Development Environment ==="
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
    echo "Options:"
    echo "  1) Pull pre-built image from mirror_bridge (if available) [~2 min]"
    echo "  2) Build from source (requires ~60 min and ~20GB disk)"
    echo "  3) Use mirror_bridge container (recommended if you have it)"
    echo ""
    read -p "Choose option [1/2/3]: " choice

    case $choice in
        1)
            echo "Attempting to use mirror_bridge image..."
            if docker images --format '{{.Repository}}:{{.Tag}}' | grep -q "mirror_bridge"; then
                # Tag the mirror_bridge image
                docker tag mirror_bridge:latest ${IMAGE_NAME}
                echo "Using mirror_bridge image"
            else
                echo "mirror_bridge image not found. Building from source..."
                docker build -t ${IMAGE_NAME} ${SCRIPT_DIR}
            fi
            ;;
        2)
            echo "Building from source..."
            docker build -t ${IMAGE_NAME} ${SCRIPT_DIR}
            ;;
        3)
            echo "Using mirror_bridge container directly."
            echo "Mount this directory and run:"
            echo "  docker run -it -v ${SCRIPT_DIR}:/workspace/reflect_hash mirror_bridge:latest"
            exit 0
            ;;
        *)
            echo "Invalid option"
            exit 1
            ;;
    esac
fi

echo "Creating container '${CONTAINER_NAME}'..."
docker run -it \
    --name ${CONTAINER_NAME} \
    -v "${SCRIPT_DIR}:/workspace" \
    ${IMAGE_NAME} \
    /bin/bash
