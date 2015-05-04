#!/bin/bash

qemu-system-x86_64 \
    -hda disk.img \
    -m 8G \
    -smp 16 \
    -vga std \
    -monitor stdio \
    -no-reboot \
    -enable-kvm \
    -cpu host \
