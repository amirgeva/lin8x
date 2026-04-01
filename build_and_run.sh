#!/bin/sh
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root (sudo)"
    exit 1
fi
./genmk.sh && make && ./update_image.sh && ./run_image.sh
