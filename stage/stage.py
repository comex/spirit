#!/usr/bin/env python
import os, sys, hashlib, plistlib, time
def sy(x):
    if os.system(x):
        raise Exception(x)
myroot = os.path.realpath(os.path.dirname(sys.argv[0]))
os.chdir(myroot)
sy('rm -rf igor')
os.mkdir('igor')
os.chdir('../igor')
configs = [i for i in eval('{%s}' % open('configdata.py').read()).keys() if not i.startswith('.')]
map = {}

for config in configs:
    print 'Configuring for', config
    sy('python config.py "%s"' % config)
    # lol, make
    sy('rm -f insns.txt one.dylib kcode.o')
    sy('make')
    data = open('one.dylib', 'rb').read()
    fn = os.path.join('igor', hashlib.sha1(data).hexdigest() + '.dylib')
    map[config] = fn
    fn2 = os.path.join(myroot, fn)
    if not os.path.exists(fn2):
        open(fn2, 'wb').write(data)

os.chdir(myroot)
plistlib.writePlist(map, 'igor/map.plist')
sy('cp ../igor/install igor/')
os.chdir('../igor')
