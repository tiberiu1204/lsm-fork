KDIR ?= ../linux
EXTRA_CFLAGS :=
obj-m = test.o

build:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

