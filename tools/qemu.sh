#!/bin/bash

qemu-system-x86_64 \
    -hda disk.img \
    -m 128M \
    -smp 8 \
    -vga std \
    -monitor stdio \
    -no-reboot \
    -enable-kvm \
    -cpu host \
