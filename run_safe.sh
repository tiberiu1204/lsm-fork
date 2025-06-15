#!/bin/bash

qemu-system-x86_64 \
  --enable-kvm \
  -smp 2 \
  -m 2G \
  -nographic \
  -kernel linux/arch/x86/boot/bzImage \
  -drive file=rootfs.qcow2,format=qcow2,if=virtio \
  -append "root=/dev/vda rw console=ttyS0" \
  -net none