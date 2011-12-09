VERSION=trunk
MODULENAME=vloopback

ifneq ($(KERNELRELEASE),)

obj-m	:= $(MODULENAME).o

else

KVER  := $(shell uname -r)
KLINK := $(shell test -e /lib/modules/${KVER}/source/ && echo source || echo "build")
KSRC  := /lib/modules/$(KVER)/$(KLINK)
PWD   := $(shell pwd)
DEST  := /lib/modules/$(KVER)/kernel/drivers/misc

# Fix some problem with suse < 9.2 and suse >= 9.2
is_suse := $(shell test -e /etc/SuSE-release && echo 1 || echo 0)
ifeq ($(is_suse),1)
  suse_version := $(shell grep VERSION /etc/SuSE-release | cut -f 3 -d " "| tr -d .)
  is_suse_92_or_greater := $(shell test $(suse_version) -ge 92 && echo 1)
  ifeq ($(is_suse_92_or_greater),1)
	KSRC := /lib/modules/$(KVER)/build
  endif
endif
             

default: all examples

all: 
	$(MAKE) -C $(KSRC) SUBDIRS=$(PWD) modules


current: clean svn all

svn:
	svn update

install:
	$(MAKE) -C example install
	install -d $(DEST)
	install -m 644 -c $(MODULENAME).ko $(DEST)
	-/sbin/depmod -a

examples:
	$(MAKE) -C example

uninstall:
	rm -f $(DEST)/$(MODULENAME).ko
	-/sbin/depmod -a
clean:
	rm -f .*.cmd *.o *.mod.c *.ko .v* *~ core Modules.symvers Module.symvers modules.order
	rm -rf .tmp_versions/ Module.markers
	rm -f example/dummy example/feed example/resize example/invert

dist: clean
	cd .. ; tar cfvz $(MODULENAME)-$(VERSION).tar.gz  --exclude='.svn' $(VERSION)/ 
endif
