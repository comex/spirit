#pragma once
#include <CoreFoundation/CoreFoundation.h>
#ifdef __APPLE__
#define OUTFD stderr
#else
#define OUTFD stdout
#endif
#ifdef TESTING // NSLog's output is prettier
void NSLog(CFStringRef, ...);
#define BaseLog(format, stuff...) NSLog(CFSTR(format), ##stuff)
#else
#define BaseLog(fmt, args...) \
    do { \
        CFStringRef _string = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR(fmt), ##args); \
        CFIndex _size = CFStringGetLength(_string)+1; \
        char *_buf = malloc(_size); \
        CFStringGetCString(_string, _buf, _size, kCFStringEncodingASCII); \
        fprintf(OUTFD, "%s\n", _buf); \
        free(_buf); \
        CFRelease(_string); \
    } while(0)
#endif

#define Log(args...)   BaseLog("LOG: "   args)
#define Error(args...) BaseLog("ERROR: " args)
#define Warn(args...)  BaseLog("WARN: "  args)
#define Debug(args...) BaseLog("DEBUG: " args)
#define Info(args...)  BaseLog("INFO: "  args)
#define Timer(args...) BaseLog("TIMER: "  args)
#define Fatal(code, args, rest...) do { BaseLog("FATAL(" #code "): " args, ##rest); printf("%d\n", code); exit(1); } while(0)

#define kExitUnknownFirmware    0x10000001
#define kExitMissingFile        0x10000002
#define kExitNotActivated       0x10000003
#define kExitTransferError      0x10000004
