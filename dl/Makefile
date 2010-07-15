all: dl dl.exe

%.w32o: %.c afc.h output.h threadshit.h
	/opt/local/bin/i386-mingw32-gcc -m32 -I../winstuff -I/opt/local/i386-mingw32/include -std=gnu99 -c -o $@ $<

dl.exe: dl.w32o afc.w32o sha1dgst.w32o
	/opt/local/bin/i386-mingw32-gcc -L/opt/local/i386-mingw32/lib -std=gnu99 -o $@ $^ -L../winstuff/lib -lMobileDevice -lCoreFoundation -lwsock32
	bash -c 'if [ ! -e crap ]; then mkdir crap; fi; cp dl.bat dl.exe crap/'

%.o: %.c afc.h output.h threadshit.h
	gcc -mmacosx-version-min=10.4 -std=gnu99 -arch i386 -arch ppc -m32 -DTESTING -c -o $@ $<

dl: dl.o afc.o sha1dgst.o igor/ 
	make -C igor
	gcc -mmacosx-version-min=10.4 -arch i386 -arch ppc -m32 -o dl dl.o afc.o sha1dgst.o -F/System/Library/PrivateFrameworks/ -framework MobileDevice -framework CoreFoundation -framework Foundation -framework DeviceLink -lz

clean:
	rm -rf dl *.o *.dSYM *.exe *.w32o crap
