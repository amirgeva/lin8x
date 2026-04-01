#!/bin/sh
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root (sudo)"
    exit 1
fi
qemu-system-x86_64 -kernel bzImage -drive file=rootfs.img,format=raw,if=virtio -append "root=/dev/vda rw init=/init quiet loglevel=0 vga=0x311" -vga std -display gtk,zoom-to-fit=on -nic user,model=virtio-net-pci,hostfwd=tcp::2222-:2222
