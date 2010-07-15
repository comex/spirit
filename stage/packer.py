import os, glob, tempfile, hashlib, sys

os.chdir(sys.path[0])

# reallly lame but ...
sys.stdout = open('data_writer.c', 'w')
print '#include <stdio.h>'
print '#include <direct.h>'
print
code = 'void write_stuff() {\n'
for base in ['igor', 'resources', 'dl']:
    code += '\t_mkdir("%s");\n' % base
    for fn in glob.glob(base + '/*'):
        if '.tar.xz' in fn or not os.path.isfile(fn): continue
        ofn = 'data_%s.o' % hashlib.md5(fn).hexdigest()
        os.system('/opt/local/bin/i386-mingw32-objcopy -I binary -O pe-i386 --binary-architecture i386 "%s" %s' % (fn, ofn))
        fp = open(ofn, 'rb')
        fp.seek(-0x100, 2)
        z = fp.read()
        fp.close()
        varname = z[z.find('_binary')+1:z.find('_start')]
        print 'extern unsigned char %s_start[];' % varname
        print 'extern unsigned int %s_end[];' % varname
        print
        fn = fn.replace('\\', '\\\\')
        fn = fn[fn.find(base):]
        code += '\t{\n\t\tFILE *fp = fopen("%s", "wb");\n\t\nfwrite(%s_start, 1, (size_t) &%s_end - (size_t) &%s_start, fp);\n\t\tfclose(fp);\n\t}\n' % (fn, varname, varname, varname)
code += '}\n'
print code
