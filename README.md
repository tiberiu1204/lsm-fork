# Process Ancestry (WIP)

## Testing environment

First debootstrap a root filesystem (kernel not included). \
You will need to chroot into it and set the root password.
```bash
# populate root filesystem
$ mkdir -p rootfs/
$ sudo debootstrap stable rootfs/

# change root password
$ sudo chroot rootfs /bin/bash
$ passwd
$ exit
```

Then, initialize all submodules (at the moment only the Linux kernel).
```bash
$ git submodule update --init --recursive
```

Next, compile the kernel using a minimal config, optimized for virtualized targets. \
The kernel modules will be installed into the `rootfs/` directory but the kernel
itself doesn't have to be.

```bash
$ cd linux/

# this will make the kernel aware (to some extent) that it's running in a VM
# so no need to emulate some devices / cpu features; output saved in ".config"
$ make x86_64_defconfig kvm_guest.config

# run this only if you need to make manual changes to the config
# OPTIONAL: enable debug info
#       search for CONFIG_DEBUG_INFO_DWARF5 by pressing / (same as vim)
#       go to the first match found by pressing 1
$ make menuconfig

# compile it using all available cores
$ make -j $(nproc)

# install modules in the rootfs directory
$ make modules_prepare -j $(nproc)
$ sudo INSTALL_MOD_PATH=../rootfs make modules_install
```

Now you can start the VM using the `run.sh` script.

```bash
$ ./run.sh

# NOTE: to forcibly close QEMU, do <Ctrl-A> + X
```

## Development

Because we are using a 9p rootfs, we can create a bind mount for the `kmod/`
directory inside the rootfs. This allows us to work on the module (and compile
it) on our host. Meanwhile, all updates will be immediately visible to the
guest system.

```bash
$ sudo mkdir -p rootfs/root/kmod
$ sudo mount --bind kmod rootfs/root/kmod
```

In the `kmod/` directory you have a test module. Change it as you go. \
The Makefile will reference the linux kernel repo during compilation in order to
make sure that your module is compatible with the kernel that will be running in
the VM.

```bash
$ cd kmod
$ make
```

Inside the VM, you can insert the module, remove it and dump the kernel debug log.

```bash
# INSIDE THE VM!
$ insmod kmod/test.ko
$ rmmod test

$ dmesg
```

## Language server tips

If you need a `compile_commands.json` for your kernel module (or the kernel
itself if you're working directly on its source code), try this *after* the
compilation is done:

```bash
$ cd linux
$ ./scripts/clang-tools/gen_compile_commands.py
```

Use this with your language server and you'll be able to index the kernel
sources referenced by the module. This also accounts for macro definitions
such as `#ifdef`, etc.
