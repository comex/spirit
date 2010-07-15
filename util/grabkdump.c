#include "MobileDevice.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "al.h"
char *thefile;

void qwrite(afc_connection *afc, const char *from, const char *to) {
    printf("Sending %s to %s... ", from, to);
    afc_file_ref ref;
    FILE *fp = fopen(from, "rb");
    A(fp);
    struct stat st;
    fstat(fileno(fp), &st);
    char *buf = (char *) malloc(st.st_size);
    fread(buf, st.st_size, 1, fp);
    fclose(fp);
    AZERO(AFCFileRefOpen(afc, to, 3, &ref));
    AZERO(AFCFileRefWrite(afc, ref, buf, st.st_size));
    AZERO(AFCFileRefClose(afc, ref));
    free(buf);
    printf("done.\n");
}

static void cb(am_device_notification_callback_info *info, void *foo) {
    struct am_device *dev;
    service_conn_t conn, afc_conn;
    afc_connection *afc;
    CFStringRef error;
    Log("connected");
    if(info->msg != ADNCI_MSG_CONNECTED) return;
    dev = info->dev;
    AMDeviceConnect(dev);
    A(AMDeviceIsPaired(dev));
    AZERO(AMDeviceValidatePairing(dev));
    AZERO(AMDeviceStartSession(dev));
    AZERO(AMDeviceStartService(dev, CFSTR("com.apple.afc"), &afc_conn, NULL));
    AZERO(AFCConnectionOpen(afc_conn, 0, &afc));

    FILE *fp = fopen("i_am_install", "w");
    afc_file_ref ref;
    AZERO(AFCFileRefOpen(afc, "spirit/i_am_install", 1, &ref));
    char buf[40960];
    while(1) {
        unsigned int len = 40960;
        AZERO(AFCFileRefRead(afc, ref, buf, &len));
        fwrite(buf, len, 1, fp);
        if(len != 40960) break;
    }
    AZERO(AFCFileRefClose(afc, ref));
    //AZERO(AFCRemovePath(afc, "spirit/i_am_install"));
    
    exit(0);
}
int main(int argc, char **argv) {
    thefile = argv[1];
    AMDAddLogFileDescriptor(fileno(stderr));
    am_device_notification *notif;
    int ret = AMDeviceNotificationSubscribe(cb, 0, 0, NULL, &notif);
    Log("...");
    CFRunLoopRun();
}
