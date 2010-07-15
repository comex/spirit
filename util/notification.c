#include "MobileDevice.h"
CFStringRef c;
#define A(x) if(!(x)) abort();
static void cb(am_device_notification_callback_info *info, void *foo) {
    struct am_device *dev;
    if(info->msg == ADNCI_MSG_CONNECTED) {
        dev = info->dev;
        service_conn_t socket;
        AMDeviceConnect(dev);
        A(AMDeviceIsPaired(dev));
        A(AMDeviceValidatePairing(dev) == 0);
        A(AMDeviceStartSession(dev) == 0);
        AMDeviceStartService(dev, CFSTR("com.apple.mobile.notification_proxy"), &socket, NULL);
        AMDPostNotification(socket, c, NULL);
        AMDShutdownNotificationProxy((void*)socket);
        exit(1);
    }
}
extern "C" void AMDAddLogFileDescriptor(int fd);
int main(int argc, char **argv) {
    if(argc != 2) {
        printf("./n <notification>\n");
        return 1;
    }
    AMDAddLogFileDescriptor(fileno(stderr));
    c = CFStringCreateWithCStringNoCopy(NULL, argv[1], kCFStringEncodingASCII, kCFAllocatorNull);
    am_device_notification *notif;
    int ret = AMDeviceNotificationSubscribe(cb, 0, 0, NULL, &notif);
    CFRunLoopRun();
}
