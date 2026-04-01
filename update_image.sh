#!/bin/sh
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root (sudo)"
    exit 1
fi
if [ ! -f rootfs.img ]; then
    echo "rootfs.img not found. Run create_image.sh first."
    exit 1
fi
mkdir -p rootfs
mount -o loop rootfs.img rootfs
cp -u include/*.h rootfs/include/
cp -ru src rootfs/
cp -ru unit_tests rootfs/
cp -ru python rootfs/
cp -u bin/* rootfs/bin/
cp -u bin/init rootfs/init
umount rootfs
