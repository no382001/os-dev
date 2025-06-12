#!/bin/sh

IMAGE="fat16.img"
SOURCE="root"
SIZE_MB=10

if ! command -v mcopy >/dev/null 2>&1 || ! command -v mkfs.vfat >/dev/null 2>&1; then
    echo "missing required tools: mkfs.vfat or mcopy they should be in mtools dosfstools"
    exit 1
fi

# create the image if it doesn't exist
if [ ! -f "$IMAGE" ]; then
    echo "creating $IMAGE ($SIZE_MB MB)..."
    dd if=/dev/zero of="$IMAGE" bs=1M count=$SIZE_MB
    mkfs.vfat -F 16 -S 512 "$IMAGE"
fi

# enable mtools to access the image without mounting
mdir -i "$IMAGE" >/dev/null 2>&1

# copy files recursively
mcopy -s -i "$IMAGE" "$SOURCE"/* ::

echo "copied contents of $SOURCE to $IMAGE"
