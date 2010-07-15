wingui.exe: wingui.c ../stage
	/opt/local/bin/i386-mingw32-windres wingui.rc -O coff -o wingui.res
	/opt/local/bin/i386-mingw32-gcc -m32 -I../dl -I../winstuff -std=gnu99 -o wingui.exe wingui.res wingui.c ../stage/data_* -L../winstuff/lib -I/opt/local/i386-mingw32/include -L/opt/local/i386-mingw32/lib -lwininet -lgdi32 -mwindows

