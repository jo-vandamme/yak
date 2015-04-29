#!/bin/bash

sudo qemu-system-x86_64 \
    -hda disk.img \
    -m 10G \
    -smp 2 \
    -vga std \
    -monitor stdio \
    -no-reboot \
    -enable-kvm \
    -cpu host \
