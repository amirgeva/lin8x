#!/bin/sh
truncate --size 100M rootfs.img
mkfs.ext4 rootfs.img
mkdir rootfs
mount rootfs.img rootfs
cd rootfs
unzip ../lin8x_deps.zip
cp ../include/*.h include/
cp -r ../src .
cp -r ../unit_tests .
cp -r ../python .
cp ../init.script init
cd ..
umount rootfs

qemu-system-x86_64 -kernel bzImage -drive file=rootfs.img,format=raw,if=virtio -append "root=/dev/vda rw init=/init" -device cirrus-vga
