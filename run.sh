#!/bin/bash

set -e

ROOTFS_DIR=${ROOTFS_DIR:-rootfs}

ARCH=${1:-x64}

case "$ARCH" in
  x64)
    sudo qemu-system-x86_64 \
      --enable-kvm \
      -smp 2 \
      -m 2G \
      -nographic \
      -kernel linux/arch/x86/boot/bzImage \
      -virtfs local,path=${ROOTFS_DIR},mount_tag=rootfs,security_model=passthrough,id=rootfs,multidevs=remap \
      -netdev user,id=net0 \
      -device e1000,netdev=net0 \
      -serial mon:stdio -serial tcp::5555,server,nowait \
      -append "root=rootfs rootfstype=9p rootflags=trans=virtio,version=9p2000.L rw console=ttyS0"
    ;;
  arm64)
    sudo qemu-system-aarch64 \
      -M virt \
      -cpu cortex-a57 \
      -smp 2 \
      -m 2G \
      -nographic \
      -kernel linux/arch/arm64/boot/Image \
      -append "root=rootfs rootfstype=9p rootflags=trans=virtio,version=9p2000.L rw console=ttyAMA0,115200 earlyprintk=serial,ttyAMA0,115200" \
      -virtfs local,path=${ROOTFS_DIR},mount_tag=rootfs,security_model=passthrough,id=rootfs,multidevs=remap \
      -netdev user,id=net0 \
      -device virtio-net-device,netdev=net0 \
      -serial mon:stdio -serial tcp::5555,server,nowait
    ;;
  mips)
    sudo qemu-system-mips \
      -M malta \
      -cpu 24Kf \
      -smp 2 \
      -m 2G \
      -nographic \
      -kernel linux/vmlinux \
      -append "root=rootfs rootfstype=9p rootflags=trans=virtio,version=9p2000.L rw console=ttyS0" \
      -virtfs local,path=${ROOTFS_DIR},mount_tag=rootfs,security_model=passthrough,id=rootfs,multidevs=remap \
      -netdev user,id=net0 \
      -device e1000,netdev=net0 \
      -serial mon:stdio -serial tcp::5555,server,nowait
    ;;
  *)
    echo "Unsupported architecture: '$ARCH'"
    echo "Usage: $0 [x64|arm64|mips]"
    exit 1
    ;;
esac

