all: hfsdude
%.o: %.c
	gcc-4.2 -arch armv6 -isysroot /var/sdk -I. -c -o $@ $<
cfiles = $(wildcard *.c)
ofiles = $(cfiles:.c=.o)
hfsdude: $(ofiles)
	gcc-4.2 -arch armv6 -isysroot /var/sdk -o $@ $(ofiles)
	ldid -S $@
clean:
	rm -f *.o hfsdude
