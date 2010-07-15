#include <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>
void CFLog(int32_t level, CFStringRef format, ...);

CFIndex (*real1)(CFPropertyListRef, CFWriteStreamRef, CFPropertyListFormat, CFStringRef *);
CFIndex (*real2)(CFAllocatorRef, CFDataRef, CFOptionFlags, CFStringRef *);

static void dump(const char *type, CFPropertyListRef ref) {
    CFDataRef data = CFPropertyListCreateXMLData(NULL, ref);
    printf("%s:\n", type);
    fwrite(CFDataGetBytePtr(data), CFDataGetLength(data), 1, stdout);
    printf("\n");
    CFRelease(data);
}

CFPropertyListRef CFPropertyListCreateFromXMLData (
   CFAllocatorRef allocator,
   CFDataRef xmlData,
   CFOptionFlags mutabilityOption,
   CFStringRef *errorString
) {
    CFPropertyListRef ret = real2(allocator, xmlData, mutabilityOption, errorString);
    dump("Receive", ret);
    return ret;
}

CFIndex CFPropertyListWriteToStream (
   CFPropertyListRef propertyList,
   CFWriteStreamRef stream,
   CFPropertyListFormat format,
   CFStringRef *errorString
) {

    dump("Send", propertyList);
    return real1(propertyList, stream, format, errorString);
}

__attribute__((constructor))
static void init() {
    real1 = dlsym(RTLD_NEXT, "CFPropertyListWriteToStream");
    real2 = dlsym(RTLD_NEXT, "CFPropertyListCreateFromXMLData");
}
