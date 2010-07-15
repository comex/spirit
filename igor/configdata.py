'.base': {
    'kern': {
        'patch2':       '-_mac_proc_enforce',
        'patch4':       '-_PE_i_can_has_debugger',
        'patch5':       '-_cs_enforcement_disable',
        'patch6':       '-_setup_kmem',
        'current_proc': '+_current_proc',
        'chgproccnt':   '+_chgproccnt',
        'kauth_cred_setresuid': '+_kauth_cred_setresuid',
        'kauth_cred_proc_ref': '+_kauth_cred_proc_ref',
        'kauth_cred_unref': '+_kauth_cred_unref',
        'set_security_token': '+_set_security_token',
        'proc_lock': '+_proc_lock',
        'proc_unlock': '+_proc_unlock',

        # it's still thumb
        'patch4_to':    0x47702001,

        'vnode_patch':  '@ - 08 00 10 00', # must be 1st result
    },
},
'.armv7': {
    '<':            '.base',
    'arch':         'armv7',

    'launchd': {
        # mov r0, #1; bx lr
       -1:              '@ + 01 20 70 47',
        # ldr r0, [r0] -> _launch_data_new_errno
        0:              '+ 00 68 .. .. .. .. 22 46 01 34',
        # lsr r0, r0, #2 -> _setrlimit
        1:              '60 69 29 46 + 80 08',
        # add r0, #3 -> __exit
        2:              'b0 f1 ff 3f .. .. + 03 30 16 f0',
        # ldmia r0, {r0-r3} -> _audit_token_to_au32
        3:              '8d e8 0f 00 0c f1 14 00 + 0f c8',
        # str r2, [sp, #4] -> _launch_data_unpack
        4:              '02 98 00 93 59 46 13 46 + 01 92',
        # str r3, [sp, #8] -> _launch_data_dict_iterate
        5:              '6a 46 01 93 01 33 + 02 93',
        # pop {r4, r7, pc}
        6:              '@ + 90 bd',
        # sub.w sp, r7, #0xc; pop {r4-r7, pc}
        7:              '@ + a7 f1 0c 0d f0 bd',
        # mov r0, r4; mov r1, r5; mov r2, r6;
        # blx _strlcpy; pop {r4-r7, pc}
        8:              '16 46 .. .. .. .. .. .. + 20 46 29 46 32 46',
        # str r0, [r5]; pop {r4-r7, pc}
        9:              '@ + 28 60 f0 bd',
        # ldr r0, [r4]; blx _pthread_detach
       10:              '0b 46 20 46 .. .. .. .. 00 28 .. .. + 20 68',
        # mov r0, r6; pop {r4-r7, pc}
       11:              '@ + 30 46 f0 bd',
    },
},
'.armv7_3.1.x': {
    '<':                '.armv7',

    'kern': {
        'scratch':      '!',
        'patch1':       '% 02 0f 40 f0 .. .. 63 08 03 f0 01 05 e3 0a 13 f0 01 03 1e 93',
        'patch1_to':    0xf0400f00,
        'patch3':       '61 1c 70 46 13 22 05 4b 98 47 % 00 ..',
        'patch3_to':    0x46c046c0,
    }

},
'.armv7_3.2.x': {
    '<':            '.armv7',
    'arch':         'armv7',

    'kern': {
        'patch1':       '% 02 0f .. .. 63 08 03 f0 01 05 e3 0a 13 f0 01 03 1e 93',
        'patch1_to':    0x46c00f02,
        'patch3':       '61 1c 13 22 .. 4b 98 47 00 .. %',
        'patch3_to':    0x1c201c20,
    }

},
'.armv6_3.1.x': {
    '<':            '.base',
    'arch':         'armv6',
    
    'launchd': {
        # Not finished

        # mov r0, #1; bx lr
       -1:              '@ - 01 00 a0 e3 1e ff 2f e1',
        # ldr r0, [r0] -> _launch_data_new_errno
        0:              '- 00 00 90 e5 .. .. .. .. 04 20 a0 e1',
        # lsr r0, r0, #2 -> _setrlimit
        1:              '05 10 a0 e1 - 20 01 a0 e1 .. .. .. .. 01 00 70 e3',
        # add r0, #3 -> __exit
        2:              '01 00 00 .. - 03 00 80 e2',
        # ldmia r0, {r0-r3} -> _audit_token_to_au32
        3:              '14 00 8c e2 - 0f 00 90 e8',
        # str r2, [sp, #4] -> _launch_data_unpack
        4:              '02 30 a0 e1 - 04 20 8d e5',
        # str r3, [sp, #8] -> _launch_data_dict_iterate
        5:              '0d 20 a0 e1 - 08 30 8d e5',
        # pop {r4, r7, pc}
        6:              '@ - 90 80 bd e8',
        # sub.w sp, r7, #0xc; pop {r4-r7, pc}
        7:              '@ - 0c d0 47 e2 f0 80 bd e8',

        # mov r0, r4; mov r1, r6; mov r2, r5;
        # blx _strlcpy; pop {r4-r7, pc}
       -8:              '- 04 00 a0 e1 06 10 a0 e1 05 20 a0 e1 .. .. .. .. f0 80 bd e8',
        # str r0, [r5]; pop {r4-r7, pc}
       -9:              '@ - 00 00 85 e5 b0 80 bd e8',
        # ldr r0, [r4]; blx _pthread_detach
       10:              '00 00 50 e3 .. .. .. .. - 00 00 94 e5 .. .. .. .. 00 10 50 e2',
        # mov r0, r6; pop {r4-r7, pc}
       11:              '@ - 06 00 a0 e1 f0 80 bd e8',
    },

    'kern': {
        'scratch':      '00 00 90 e5 00 10 91 e5 .. .. .. .. 33 ff 2f e1 01 00 70 e2 00 00 a0 33 80 80 bd e8 00 00 a0 e3 80 80 bd e8 .. .. .. .. -',
        'patch1':       '- .. .. .. .. 6b 08 1e 1c eb 0a 01 22 1c 1c 16 40 14 40',
        'patch1_to':    0x46c046c0,
        'patch3':       '13 20 a0 e3 .. .. .. .. 33 ff 2f e1 00 00 50 e3 00 00 00 0a .. 40 a0 e3 - 04 00 a0 e1 90 80 bd e8',
        'patch3_to':    0xe3a00001,

    }
},
'iPhone1,1_3.1.2': {
    '<': '.armv6_3.1.x',
    'kern': { '@binary': 'ipsw/iPhone1,1_3.1.2_7D11/kern' },
    'launchd': { '@binary': 'ipsw/iPhone1,1_3.1.2_7D11/launchd' },
},
'iPhone2,1_3.1.2': {
    '<': '.armv7_3.1.x',
    'kern': { '@binary': 'ipsw/iPhone2,1_3.1.2_7D11/kern' },
    'launchd': { '@binary': 'ipsw/iPhone2,1_3.1.2_7D11/launchd' },
},
'iPhone2,1_3.1.3': {
    '<': '.armv7_3.1.x',
    'kern': { '@binary': 'ipsw/iPhone2,1_3.1.3_7E18/kern' },
    'launchd': { '@binary': 'ipsw/iPhone2,1_3.1.3_7E18/launchd' },
},
'iPod2,1_3.1.2': {
    '<': '.armv6_3.1.x',
    'kern': { '@binary': 'ipsw/iPod2,1_3.1.2_7D11/kern' },
    'launchd': { '@binary': 'ipsw/iPod2,1_3.1.2_7D11/launchd' },
},
'iPod3,1_3.1.3': {
    '<': '.armv7_3.1.x',
    'kern': { '@binary': 'ipsw/iPod3,1_3.1.3_7E18/kern' },
    'launchd': { '@binary': 'ipsw/iPod3,1_3.1.3_7E18/launchd' },
},
'iPod1,1_3.1.2': {
    '<': '.armv6_3.1.x',
    'kern': { '@binary': 'ipsw/iPod1,1_3.1.2_7D11/kern' },
    'launchd': { '@binary': 'ipsw/iPod1,1_3.1.2_7D11/launchd' },
},
'iPhone1,1_3.1.3': {
    '<': '.armv6_3.1.x',
    'kern': { '@binary': 'ipsw/iPhone1,1_3.1.3_7E18/kern' },
    'launchd': { '@binary': 'ipsw/iPhone1,1_3.1.3_7E18/launchd' },
},
'iPhone1,2_3.1.3': {
    '<': '.armv6_3.1.x',
    'kern': { '@binary': 'ipsw/iPhone1,2_3.1.3_7E18/kern' },
    'launchd': { '@binary': 'ipsw/iPhone1,2_3.1.3_7E18/launchd' },
},
'iPod3,1_3.1.2': {
    '<': '.armv7_3.1.x',
    'kern': { '@binary': 'ipsw/iPod3,1_3.1.2_7D11/kern' },
    'launchd': { '@binary': 'ipsw/iPod3,1_3.1.2_7D11/launchd' },
},
'iPhone1,2_3.1.2': {
    '<': '.armv6_3.1.x',
    'kern': { '@binary': 'ipsw/iPhone1,2_3.1.2_7D11/kern' },
    'launchd': { '@binary': 'ipsw/iPhone1,2_3.1.2_7D11/launchd' },
},
'iPad1,1_3.2': {
    '<': '.armv7_3.2.x',
    'launchd': { '@binary': 'ipsw/ipad_dump/launchd' },
    'kern': {
        '@binary': 'ipsw/ipad_dump/kern',
        'scratch':      0xc07b3bec,
        'patch2':       0xc025dc8c,
        'patch4':       0xc01d17c0,
        'patch5':       0xc023fac0,
        'patch6':       0xc02558dc,
        'current_proc': 0xc017cbf1,
        'chgproccnt':   0xc014a0fd,
        'kauth_cred_setresuid': 0xc013b5e1,
        'kauth_cred_proc_ref': 0xc013b845,
        'kauth_cred_unref': 0xc013b159,
        'set_security_token': 0xc014a83d,
        'proc_lock': 0xc0146a99,
        'proc_unlock': 0xc0146a79,
    }
},
'iPod3,1_3.1.3': {
    '<': '.armv7_3.1.x',
    'kern': { '@binary': 'ipsw/iPod3,1_3.1.3_7E18/kern' },
    'launchd': { '@binary': 'ipsw/iPod3,1_3.1.3_7E18/launchd' },
},
'iPod2,1_3.1.3': {
    '<': '.armv6_3.1.x',
    'kern': { '@binary': 'ipsw/iPod2,1_3.1.3_7E18/kern' },
    'launchd': { '@binary': 'ipsw/iPod2,1_3.1.3_7E18/launchd' },
},
'iPod1,1_3.1.3': {
    '<': '.armv6_3.1.x',
    'kern': { '@binary': 'ipsw/iPod1,1_3.1.3_7E18/kern' },
    'launchd': { '@binary': 'ipsw/iPod1,1_3.1.3_7E18/launchd' },
},
