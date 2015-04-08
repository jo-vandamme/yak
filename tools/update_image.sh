#!/bin/bash

echo "[copying to disk]"
sudo losetup /dev/loop0 disk.img
sudo mount /dev/loop0 /mnt
sudo cp -rf sysroot/* /mnt/
sudo umount /mnt
sudo losetup -d /dev/loop0

