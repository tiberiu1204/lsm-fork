
PLATFORM := amd64
nproc := $(shell nproc)

ifeq ($(PLATFORM),arm64)
CROSS_COMPILE := aarch64-linux-gnu-
ARCH := arm64
KCFLAGS := "-Wno-format-overflow -Wno-unused-but-set-variable -Wno-int-conversion -Wno-suggest-attribute=format -Wno-override-init -Wno-unterminated-string-initialization -Wno-format-truncation"
else
CROSS_COMPILE :=
ARCH := x86_64
KCFLAGS := "-Wno-suggest-attribute=format -Wno-override-init -Wno-unterminated-string-initialization -Wno-format-truncation"
endif

ROOTFS_PACKAGES = build-essential
ROOTFS_GCC = $(shell pwd)/rootfs/usr/bin/gcc

all: rootfs linux modules install_modules

init:
	git submodule update --init --recursive

# Mount the test-app folder into a directory names 'share' inside the rootfs
mount_share:
	sudo mkdir -p rootfs/root/share
	sudo mount --bind test-app rootfs/root/share

umount_share:
		sudo umount -f rootfs/root/share;

rootfs:
	sudo debootstrap --arch=$(PLATFORM) stable rootfs && \
	sudo chroot rootfs /bin/bash -c "yes '1234' | passwd && apt update && apt install $(ROOTFS_PACKAGES) -y"

build_linux: $(PLATFORM).config
	cd linux && \
	cp ../$(PLATFORM).config ./.config && \
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) W=1 KCFLAGS=$(KCFLAGS) -j$(nproc)

modules: build_linux
	cd linux && \
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules_prepare -j $(nproc)

install_modules: rootfs modules
	sudo mkdir -p rootfs/usr/src && \
	sudo rm -f rootfs/usr/linux && \
	sudo cp -r linux rootfs/usr/src
	cd linux && \
	sudo INSTALL_MOD_PATH=../rootfs make CC=$(ROOTFS_GCC) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules_install

clean_rootfs:
	sudo rm -rf rootfs

clean_linux:
	cd linux && \
	make clean -j$(nproc)

clean: clean_rootfs clean_linux

.PHONY: clean clean_linux clean_rootfs umount_share mount_share


