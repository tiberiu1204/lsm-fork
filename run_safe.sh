#!/bin/bash

ORIGINAL_ROOTFS="./rootfs"
COW_DIR="./overlay"
GUEST_MOUNT_PATH="${COW_DIR}/merged/root/share" 

mkdir -p "${COW_DIR}/upper" "${COW_DIR}/work" "${COW_DIR}/merged"

echo "[*] Mounting OverlayFS..."
sudo mount -t overlay overlay \
  -o lowerdir="${ORIGINAL_ROOTFS}",upperdir="${COW_DIR}/upper",workdir="${COW_DIR}/work" \
  "${COW_DIR}/merged"

sudo mkdir -p "$GUEST_MOUNT_PATH"
echo "[*] Bind-mounting test-app..."
sudo mount --bind test-app "$GUEST_MOUNT_PATH"

echo "[*] Starting QEMU..."
sudo qemu-system-x86_64 \
  --enable-kvm \
  -smp 8 \
  -cpu host \
  -m 16G \
  -nographic \
  -kernel linux/arch/x86/boot/bzImage \
  -virtfs local,path="${COW_DIR}/merged",mount_tag=rootfs,security_model=passthrough,id=rootfs,multidevs=remap \
  -netdev user,id=net0 \
  -device e1000,netdev=net0 \
  -serial mon:stdio \
  -serial tcp::5555,server,nowait \
  -append "root=rootfs rootfstype=9p rootflags=trans=virtio,version=9p2000.L rw console=ttyS0"

echo ""
echo "[*] VM Stopped. Cleaning up..."

echo "[*] Unmounting test-app..."
sudo umount "$GUEST_MOUNT_PATH"

echo "[*] Unmounting OverlayFS..."
sudo umount "${COW_DIR}/merged"

if [ $? -eq 0 ]; then
    echo "[*] Nuking temporary changes..."
    sudo rm -rf "${COW_DIR}"
    echo "[*] Done."
else
    echo "[!] Error: Could not unmount OverlayFS. Check if a process is still using it."
fi
