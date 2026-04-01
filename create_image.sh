#!/bin/sh
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root (sudo)"
    exit 1
fi
truncate --size 100M rootfs.img
mkfs.ext4 rootfs.img
mkdir -p rootfs
mount -o loop rootfs.img rootfs
cd rootfs
unzip ../lin8x_deps.zip
mkdir -p dev
cp ../include/*.h include/
cp -r ../src .
cp -r ../unit_tests .
cp -r ../python .
cp ../bin/* bin/
cp ../bin/init .
cd ..
umount rootfs

qemu-system-x86_64 -kernel bzImage -drive file=rootfs.img,format=raw,if=virtio -append "root=/dev/vda rw init=/init quiet loglevel=0 vga=0x311" -vga std
