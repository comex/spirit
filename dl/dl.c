/* TODO
 * detect too low version of iTunes
 * detect unactivated
 * detect wrong version of FW & device type
 */

#include <CoreFoundation/CoreFoundation.h>
#include "MobileDevice.h"
#include "sha.h"
#define create_data(pl) \
    CFPropertyListCreateXMLData(NULL, pl)
#define create_with_data(data, mut) \
    CFPropertyListCreateFromXMLData(NULL, data, mut, NULL)
#define alloc(x) CFAllocatorAllocate(NULL, (x), 0)
#ifdef __APPLE__
#include <sys/socket.h>
#include <dlfcn.h>
#define _sleep(x) sleep(x)
/*#define create_data(pl)  \
    CFPropertyListCreateData(NULL, pl, kCFPropertyListBinaryFormat_v1_0, 0, NULL)
static CFPropertyListRef create_with_data(CFDataRef data, CFOptionFlags mut) {
    CFPropertyListFormat fmt = kCFPropertyListXMLFormat_v1_0;
    return CFPropertyListCreateWithData(NULL, data, mut, &fmt, NULL);
}*/
#else
#include <windows.h>
#include <winsock.h>
#include <time.h>
#define _sleep(x) Sleep(1000*(x))
#endif
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>

#include "threadshit.h"
#include "_assert.h"
#include "output.h"
#include "afc.h"

// Options
#define DEBUG 1

#ifdef DEBUG

#define TIMER(name) { ull _timer = getms(); Timer("[%s:%d] %s: %llu", __func__, __LINE__, (#name), _timer - timer); timer = _timer; };
#define TIMER_START ull timer = getms();

typedef unsigned long long ull;
static inline ull getms() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((ull) tp.tv_sec) * 1000000 + (ull) tp.tv_usec;
}

#else

#define TIMER(name) 
#define TIMER_START

#endif

#ifndef __APPLE__
const char *xzfn;
extern void write_stuff(); // packer.py
#endif

struct conn;
typedef struct conn conn;

typedef enum {
	kStageFirst,
	kStageSecond,
	kStageAll
} send_file_stage;

// "DLSendFile", filename, stuffdict
// "DLSendFile", data, DLCopyFileAttributes(@filename)


// Globals
CFDataRef empty_data;
unsigned int some_unique;

ev_t ev_files_ready;

static am_device *dev;
service_conn_t ha_conn;
struct afc_connection *ha_afc;

CFStringRef fwkey;
CFDictionaryRef fwmap;
CFStringRef one_path;

// CoreFoundation helpers
CFDataRef read_file(const char *fn) {
    Info("read %s", fn);
    FILE *fp = fopen(fn, "rb");
    _assertWithCode(fp, kExitMissingFile);

	// Calculate file size
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *data = alloc(sz);
    _assert(sz == fread(data, 1, sz, fp));
    _assertZero(fclose(fp));
    return CFDataCreateWithBytesNoCopy(NULL, data, sz, NULL);
}

#define number_with_int(x) CFNumberCreate(NULL, kCFNumberSInt32Type, (int[]){x})
#define number_with_cfindex(x) CFNumberCreate(NULL, kCFNumberCFIndexType, (CFIndex[]){x})

#define make_array(args...) make_array_(sizeof((const void *[]){args}) / sizeof(const void *), (const void *[]){args})
void *make_array_(int num_els, const void **stuff) {
    CFMutableArrayRef ref = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
    while(num_els--) {
        CFArrayAppendValue(ref, *stuff++);
    }
    return ref;
}

#define make_dict(args...) make_dict_(sizeof((const void *[]){args}) / sizeof(const void *) / 2, (const void *[]){args})
void *make_dict_(int num_els, const void **stuff) {
    CFMutableDictionaryRef ref = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    _assert(ref);
    while(num_els--) {
        const void *value = *stuff++;
        const void *key = *stuff++;
        CFDictionarySetValue(ref, key, value);
    }
    return ref;
}

CFStringRef data_to_hex(CFDataRef input) {
    CFMutableStringRef result = CFStringCreateMutable(NULL, 40);
    const unsigned char *bytes = CFDataGetBytePtr(input);
    unsigned int len = CFDataGetLength(input);
    unsigned int i;
    for(i = 0; i < len; i++) {
        CFStringAppendFormat(result, NULL, CFSTR("%02x"), (unsigned int) bytes[i]);
    }
    return result;
}

CFDataRef sha1_of_data(CFDataRef input) {
    char *md = alloc(20);
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, CFDataGetBytePtr(input), CFDataGetLength(input));
    SHA1_Final(md, &ctx);
    return CFDataCreateWithBytesNoCopy(NULL, md, 20, NULL);
}


#define send_msg(a, b) send_msg_(__func__, a, b)
#define receive_msg(a) receive_msg_(__func__, a)

static void send_msg_(const char *caller, service_conn_t conn, CFPropertyListRef plist) {
    CFDataRef data = create_data(plist);
    uint32_t size = htonl(CFDataGetLength(data));

    _assert(sizeof(size) == send(conn, &size, sizeof(size), 0));
    _assert(CFDataGetLength(data) == send(conn, CFDataGetBytePtr(data), CFDataGetLength(data), 0));

    CFRelease(data);
}

static void receive_msg_(const char *caller, service_conn_t conn) {
    uint32_t size;

    ssize_t ret = recv((int) conn, &size, sizeof(size), 0);
    _assert(ret == sizeof(size));
    size = ntohl(size);

    void *buf = alloc(size);
    // MSG_WAITALL not supported? on windows
    char *p = buf; uint32_t br = size;
    while(br) {
        ret = recv((int) conn, p, br, 0);
        _assert(ret > 0);
        br -= ret;
        p += ret;
    }
    _assert(p - size == buf);

    CFDataRef data = CFDataCreateWithBytesNoCopy(NULL, buf, size, NULL);
    CFPropertyListRef result = create_with_data(data, kCFPropertyListImmutable);
    
    Debug("%@", result);
    CFRelease(data);
    CFRelease(result);
    Debug("DONE");
}

service_conn_t create_dl_conn(CFStringRef service) {
    service_conn_t it;
    _assertZero(AMDeviceStartService(dev, service, &it, NULL));
    Debug("create_dl_conn receive 1");
    receive_msg(it);
    send_msg(it, make_array(CFSTR("DLMessageVersionExchange"), CFSTR("DLVersionsOk")));
    Debug("create_dl_conn receive 2");
    receive_msg(it);
    return it;
}

void *add_file(CFMutableDictionaryRef files, CFDataRef crap, CFStringRef domain, CFStringRef path, int uid, int gid, int mode) {
    CFDataRef pathdata = CFStringCreateExternalRepresentation(NULL, path, kCFStringEncodingUTF8, 0);
    CFStringRef manifestkey = data_to_hex(sha1_of_data(pathdata));
    CFMutableDictionaryRef dict = make_dict(
        domain, CFSTR("Domain"),
        path, CFSTR("Path"),
        kCFBooleanFalse, CFSTR("Greylist"),
        CFSTR("3.0"), CFSTR("Version"),
        crap, CFSTR("Data")
        );

    char *datahash = alloc(20);
    char *extra = ";(null);(null);(null);3.0";
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, CFDataGetBytePtr(crap), CFDataGetLength(crap));
    SHA1_Update(&ctx, CFDataGetBytePtr(pathdata), CFDataGetLength(pathdata));
    SHA1_Update(&ctx, extra, strlen(extra));
    SHA1_Final(datahash, &ctx);
    
    CFDictionarySetValue(files,
        manifestkey,
        make_dict(
            CFDataCreateWithBytesNoCopy(NULL, datahash, 20, kCFAllocatorNull), CFSTR("DataHash"),
            domain, CFSTR("Domain"),
            number_with_cfindex(CFDataGetLength(crap)), CFSTR("FileLength"),
            number_with_int(gid), CFSTR("Group ID"),
            number_with_int(uid), CFSTR("User ID"),
            number_with_int(mode), CFSTR("Mode"),
            CFDateCreate(NULL, 2020964986UL), CFSTR("ModificationTime")
        ));

    //sleep(10);
    
    CFStringRef templateo = CFStringCreateWithFormat(NULL, NULL, CFSTR("/tmp/stuff.%06d"), ++some_unique);
    CFMutableDictionaryRef info = make_dict(
        templateo, CFSTR("DLFileDest"),
        path, CFSTR("Path"),
        CFSTR("3.0"), CFSTR("Version"),
        crap, CFSTR("Crap")
    );

    return info;
}

static void start_restore(service_conn_t conn, CFMutableDictionaryRef files) {
    Info("start_restore with files: %@", files);

    CFStringRef deviceId = AMDeviceCopyDeviceIdentifier(dev);
    CFMutableDictionaryRef m1dict = make_dict(
        CFSTR("6.2"), 							CFSTR("Version"),
        deviceId, 								CFSTR("DeviceId"),
        make_dict(), 							CFSTR("Applications"),
        files, 									CFSTR("Files")
    );
    CFDataRef m1data = create_data(m1dict);

    CFMutableDictionaryRef manifest = make_dict(
        CFSTR("2.0"),							CFSTR("AuthVersion"),
        sha1_of_data(m1data), 					CFSTR("AuthSignature"),
        number_with_int(0), 					CFSTR("IsEncrypted"),
        m1data, 								CFSTR("Data")
    );

    CFMutableDictionaryRef mdict = make_dict(
        CFSTR("kBackupMessageRestoreRequest"), 	CFSTR("BackupMessageTypeKey"),
        kCFBooleanTrue, 						CFSTR("BackupNotifySpringBoard"),
        kCFBooleanTrue, 						CFSTR("BackupPreserveCameraRoll"),
        CFSTR("3.0"), 							CFSTR("BackupProtocolVersion"),
        manifest, 								CFSTR("BackupManifestKey")
    );

    send_msg(conn, make_array(
        CFSTR("DLMessageProcessMessage"), mdict
    ));

    Debug("receive from start_restore");
    //receive_msg(conn);
}

void send_file(service_conn_t conn, CFMutableDictionaryRef info, send_file_stage stage) {
    CFDictionarySetValue(info, CFSTR("DLFileAttributesKey"), make_dict());
    CFDictionarySetValue(info, CFSTR("DLFileSource"), CFDictionaryGetValue(info, CFSTR("DLFileDest")));
    CFDictionarySetValue(info, CFSTR("DLFileIsEncrypted"), number_with_int(0));
    CFDictionarySetValue(info, CFSTR("DLFileOffsetKey"), number_with_int(0));

    if (stage == kStageAll) {
        CFDataRef crap = CFDictionaryGetValue(info, CFSTR("Crap"));
        CFDictionaryRemoveValue(info, CFSTR("Crap"));
        CFDictionarySetValue(info, CFSTR("DLFileStatusKey"), number_with_int(2));
        send_msg(conn, make_array(CFSTR("DLSendFile"), crap, info));
        receive_msg(conn);
    } else if (stage == kStageFirst) {
        CFDataRef crap = CFDictionaryGetValue(info, CFSTR("Crap"));
        CFDictionaryRemoveValue(info, CFSTR("Crap"));
        CFDictionarySetValue(info, CFSTR("DLFileStatusKey"), number_with_int(1));
        send_msg(conn, make_array(CFSTR("DLSendFile"), crap, info));
    } else if (stage == kStageSecond) {
        CFDictionarySetValue(info, CFSTR("DLFileStatusKey"), number_with_int(2));
        send_msg(conn, make_array(CFSTR("DLSendFile"), empty_data, info));
        receive_msg(conn);
    } else {
		// This should never happen.
		Fatal(1, "Invalid stage passed to %s.", __func__);
	}
}


static void send_over(struct afc_connection *afc, const char *from, const char *to) {
    Log("sending over %s", from);
    FILE *fp = fopen(from, "rb");
    if(!fp) {
        Fatal(kExitMissingFile, "%s is missing...", from); 
    }
    afc_file_ref ref;
    
    _assertZero(AFCFileRefOpen(afc, to, 3, &ref));

    ssize_t nb = 0;
    ssize_t filesize = 0;
    char buf[65536];
#ifndef __APPLE__
    char magic[17] = "magicmagicmagicm!";
    if(strstr(from, ".exe")) {
        // Scan for the xz file
        while((nb = fread(buf, 1, sizeof(buf), fp)) > 0) { 
            for(char *p = buf; p < buf + sizeof(buf) - 17; p++) {
                if(!memcmp(p, magic, 16) && p[16] != '!') {
                    fseek(fp, -sizeof(buf) + (p - buf) + 16, SEEK_CUR);
                    goto done;
               }
            }
            if(nb != 17) fseek(fp, -17, SEEK_CUR);
        }
        // It failed...
        Fatal(kExitMissingFile, "I couldn't find a XZ file in this spec-violating PE file...");
    }
done:
#endif

    while((nb = fread(buf, 1, sizeof(buf), fp)) > 0) {
        Log(".%x", nb);
        filesize += nb;
        _assertZero(AFCFileRefWrite(afc, ref, buf, nb));
#ifndef __APPLE__
        if(nb != sizeof(buf)) break; // lol windows
#endif
    }
    _assert(feof(fp));
    fclose(fp);
    _assertZero(AFCFileRefClose(afc, ref));
#ifdef __APPLE__
    struct afc_dictionary *info;
    char *key, *val;

    _assertZero(AFCFileInfoOpen(afc, to, &info));
    while(!AFCKeyValueRead(info, &key, &val)) {
        _assert(key); _assert(val);
        if(!strcmp(key, "st_size")) {
            _assert(filesize == strtoll(val, NULL, 10));
            AFCKeyValueClose(info);
            return;
        }
    }
    Fatal(kExitTransferError, "I didn't get a st_size for %s", to);
    AFCKeyValueClose(info);
#endif

    Log("done sending %s", from);
}



static void send_files_thread() {
    struct afc_connection *afc;
    service_conn_t afc_conn;

    Info("Sending files via AFC.");

    _assertZero(AMDeviceStartService(dev, CFSTR("com.apple.afc"), &afc_conn, NULL));
    _assertZero(AFCConnectionOpen(afc_conn, 0, &afc));

    afc_remove_all(afc, "spirit");
    afc_create_directory(afc, "spirit");


    // the appropriate version of one.dylib
    char one_path_buf[1024];
    CFStringGetCString(one_path, one_path_buf, 1024, kCFStringEncodingUTF8);
    send_over(afc, one_path_buf, "spirit/one.dylib");

    // everything else
    send_over(afc, "igor/install", "spirit/install");
#ifdef __APPLE__
    send_over(afc, "resources/freeze.tar.xz", "spirit/freeze.tar.xz");
#else
    // This will skip to 7zXZ, see send_over
    send_over(afc, xzfn, "spirit/freeze.tar.xz");
#endif
    send_over(afc, CFStringFind(fwkey, CFSTR("iPad"), 0).location != kCFNotFound ? "resources/1024x768.jpg" : "resources/320x480.jpg", "spirit/bg.jpg");

    ev_signal(&ev_files_ready);

    AFCConnectionClose(afc);
    close(afc_conn);
}

static void restore_thread() {
    struct afc_dictionary *info;
    Info("Connecting to mobilebackup...");
    service_conn_t conn = create_dl_conn(CFSTR("com.apple.mobilebackup"));
    int err;
    TIMER_START
    // Library/Preferences/SystemConfiguration is migrated to /var/preferences/SystemConfiguration

    Info("Beginning restore process.");

    CFMutableDictionaryRef files = make_dict();
    CFMutableDictionaryRef overrides = add_file(files, read_file("resources/overrides.plist"), CFSTR("HomeDomain"), CFSTR("Library/Preferences/SystemConfiguration/../../../../../var/db/launchd.db/com.apple.launchd/overrides.plist"), 0, 0, 0600);
    CFMutableDictionaryRef use_gmalloc = add_file(files, empty_data, CFSTR("HomeDomain"), CFSTR("Library/Preferences/SystemConfiguration/../../../../../var/db/.launchd_use_gmalloc"), 0, 0, 0600);
	TIMER(about_to_start_restore)
    start_restore(conn, files);
    TIMER(start_restore)
    Info("waiting for files ready");
    ev_wait(&ev_files_ready);
    Info("files are ready.");

    send_file(conn, overrides, kStageAll);
    send_file(conn, use_gmalloc, kStageAll);
    close(conn);
    
	Info("Completed restore.");
}

static void completion_thread() {
    ev_wait(&ev_threadcount); // Wait for all other threads to complete.
    Info("Completed successfully.");

    exit(0);
}

static void device_notification_callback(am_device_notification_callback_info *info, void *foo) {
    if(info->msg != ADNCI_MSG_CONNECTED) return;

    Info("Connected to the AppleMobileDevice.");
    AMDeviceConnect(info->dev);
    _assert(AMDeviceIsPaired(info->dev));
    _assertZero(AMDeviceValidatePairing(info->dev));
    _assertZero(AMDeviceStartSession(info->dev));
    dev = info->dev;
    
    fwkey = CFStringCreateWithFormat(
        NULL,
        NULL,
        CFSTR("%@_%@"),
        AMDeviceCopyValue(dev, 0, CFSTR("ProductType")),
        AMDeviceCopyValue(dev, 0, CFSTR("ProductVersion"))
    );
    Info("Version %@", fwkey);

    one_path = CFDictionaryGetValue(fwmap, fwkey);
    if(!one_path) {
        Fatal(kExitUnknownFirmware, "I don't know about firmware %@", fwkey);
    }
    
    ev_init(&ev_files_ready);

#ifdef __APPLE__
    some_unique = arc4random();
#else
    some_unique = (int) time(NULL);
#endif


    create_a_thread(send_files_thread);
    restore_thread();
    exit(0);
}

int main(int argc, char **argv) {
	TIMER_START

	//for(int i = 0; i < 1000; i++) {Info("Now listening for devices..."); } exit(1);
#ifndef __APPLE__
    xzfn = argv[1];
#endif

	Info("Now listening for devices...");

    empty_data = CFDataCreateWithBytesNoCopy(NULL, "", 0, kCFAllocatorNull);
    void (*add_file_descriptor)(int);

#ifdef __APPLE__
    add_file_descriptor = dlsym(RTLD_NEXT, "AMDAddLogFileDescriptor");
    if(!add_file_descriptor) {
        add_file_descriptor = dlsym(RTLD_NEXT, "AMDSetLogFileDescriptor");
    }

    if(add_file_descriptor) {
        add_file_descriptor(fileno(stderr));
        AMDSetLogLevel(1000);
    } else {
        Warn("Couldn't find AMD***LogFileDescriptor");
    }
#endif
    fwmap = create_with_data(read_file("igor/map.plist"), kCFPropertyListImmutable); 

    am_device_notification *notif;
    int ret = AMDeviceNotificationSubscribe(device_notification_callback, 0, 0, NULL, &notif);
    CFRunLoopRun();
}

