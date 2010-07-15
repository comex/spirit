ALL := common.h.gch one.dylib install insns.txt bpfgen
BIN := /Developer/Platforms/iPhoneOS.platform/Developer/usr/bin
GCC := $(BIN)/gcc-4.2 -arch armv6 -Os --sysroot /var/sdk -DDEBUG=$(shell cat debug) `cat config.cflags` -Wimplicit -isysroot /var/sdk
GCC_UNIVERSAL := $(BIN)/gcc-4.2 -arch armv7 -arch armv6 -Os -Ixz -DDEBUG=$(shell cat debug) `cat config.cflags` -Wimplicit -isysroot /var/sdk
all: config.cflags $(ALL)
one.dylib: one.py insns.txt config.cflags
	python one.py one.dylib
%.o: %.S 
	perl gas-preprocessor.pl $(GCC) -c -o $@ $<
install: install.o lsstuff.o copier.o fb.o debug config.cflags libtar.a liblzma.a
	$(GCC_UNIVERSAL) -DLIB -std=gnu99 -Os -o install install.o lsstuff.o copier.o fb.o libtar.a liblzma.a -F/var/sdk/System/Library/Frameworks -F/var/sdk/System/Library/PrivateFrameworks -framework CoreFoundation -framework Foundation -framework UIKit -framework CoreGraphics -framework IOKit -framework CoreSurface -framework IOSurface -framework IOMobileFramebuffer -framework MobileCoreServices -lz
bpfgen: bpfgen.c 
	gcc -std=gnu99 -m32 -o bpfgen bpfgen.c
insns.txt: bpfgen config.cflags
	./bpfgen `cat config.cflags`
%.o: %.c
	$(GCC_UNIVERSAL) -std=gnu99 -Os -c -o $@ -Iheaders $<
%.o: %.m
	$(GCC_UNIVERSAL) -std=gnu99 -Os -c -o $@ -Iheaders $<
%.h.gch: %.h
	$(GCC) -std=gnu99 -Os -o $@ $<
clean:
	rm -f $(ALL) *.o
distclean:
	rm -f $(ALL) *.o config.cflags config.json config.cache.db
