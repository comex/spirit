all: n syslog logger.dylib mobdevlog.dylib install  grabkdump
n: notification.c
	g++ -m32 -o n notification.c -F/System/Library/PrivateFrameworks/ -framework		 MobileDevice -framework CoreFoundation
syslog: syslog.c
	gcc -m32 -o syslog syslog.c -F/System/Library/PrivateFrameworks/ -framework		 MobileDevice -framework CoreFoundation
logger.dylib: logger.c
	gcc -m32 -dynamiclib -o logger.dylib logger.c -framework CoreFoundation
mobdevlog.dylib: mobdevlog.c
	gcc -m32 -dynamiclib -o mobdevlog.dylib mobdevlog.c -framework CoreFoundation
clean:
	rm -rf n syslog logger.dylib mobdevlog.dylib *.dSYM
install: install.c
	gcc -m32 -o install install.c  -F/System/Library/PrivateFrameworks/ -framework	  MobileDevice -framework CoreFoundation -framework Foundation -framework DeviceLink -lcrypto -lz -DTESTING
grabkdump: grabkdump.c
	gcc -m32 -o grabkdump grabkdump.c  -F/System/Library/PrivateFrameworks/ -framework	  MobileDevice -framework CoreFoundation -framework Foundation -framework DeviceLink -lcrypto -lz -DTESTING


