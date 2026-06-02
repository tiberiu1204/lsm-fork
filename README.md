# LSM-Fork

## Build Rules (Makefile)

- `make init`: Initialize and update git submodules.
- `make rootfs`: Create a Debian-based root filesystem using `debootstrap`. Requires `sudo`.
- `make build_linux`: Compile the Linux kernel using the configuration for the current platform (`amd64` or `arm64`).
- `make modules`: Prepare kernel modules for installation.
- `make install_modules`: Install the compiled kernel source and modules into the rootfs.
- `make all`: Executes `rootfs`, `build_linux`, `modules`, and `install_modules` in sequence.
- `make mount_share`: Bind-mount the `test-app` directory to `/root/share` inside the rootfs.
- `make umount_share`: Unmount the `test-app` share.
- `make clean_rootfs`: Remove the `rootfs` directory.
- `make clean_linux`: Run `make clean` inside the `linux` directory.
- `make clean`: Executes both `clean_rootfs` and `clean_linux`.
- `make linux_compile_commands`: Generate `compile_commands.json` for the Linux kernel.

## Project Setup Guide

1. **Install Dependencies**
   Ensure the following packages are installed on your host system:
   `build-essential`, `debootstrap`, `qemu-system-x86`, `libncurses-dev`, `flex`, `bison`, `libssl-dev`, `libelf-dev`, `bc`.

2. **Initialize Submodules**
   ```bash
   make init
   ```

3. **Build everything**
   This command creates the rootfs, builds the kernel, and installs modules.
   ```bash
   make all
   ```

4. **Start the VM**
   Use `run_safe.sh` to start the VM. This script uses OverlayFS to ensure that any changes made during the session are discarded after the VM stops. It also automatically binds the `test-app` directory to `/root/share` inside the guest.
   ```bash
   ./run_safe.sh
   ```
   - **Default User:** `root`
   - **Default Password:** `1234`

## Component Documentation

### TCP Server

Located in `test-app/tcp-server`. This server receives memory dumps from the LSM hooks via TCP.

- **Build:**
  ```bash
  cd test-app/tcp-server
  make
  ```
- **Run:**
  ```bash
  ./bin/tcp_server
  ```
  The server listens on port `9999` by default.

### Filter-WX

Located in `test-app/filter-wx`. This utility uses `libseccomp` to prevent processes from creating or modifying memory mappings with both Write and Execute permissions (W^X).

- **Build:**
  ```bash
  cd test-app/filter-wx
  make
  ```
- **Run:**
  ```bash
  ./bin/filter-wx <program_to_run> [args...]
  ```
  Example:
  ```bash
  ./bin/filter-wx /bin/bash
  ```

- **Testing:**
  A test program is provided to verify the filter:
  ```bash
  ./bin/test
  ```
