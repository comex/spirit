#include "MobileDevice.h"
#include <assert.h>

static void cb(am_device_notification_callback_info *info, void *foo) {
    struct am_device *dev;
    CFStringRef error;
    printf("... %x\n", info->msg);
    if(info->msg == ADNCI_MSG_CONNECTED) {
        dev = info->dev;
        afc_connection *socket = NULL;
        AMDeviceConnect(dev);
        assert(AMDeviceIsPaired(dev));
        assert(AMDeviceValidatePairing(dev) == 0);
        assert(AMDeviceStartSession(dev) == 0);
        AMDeviceStartService(dev, CFSTR("com.apple.syslog_relay"), (void*)&socket, NULL);
        printf("socket=%p\n", socket);
        char c;
        while(recv((int) socket, &c, 1, 0) == 1) {
            if(c != 0)
                putchar(c);
        }
    }
}
int main(int argc, char **argv) {
    am_device_notification *notif;
    int ret = AMDeviceNotificationSubscribe(cb, 0, 0, NULL, &notif);
    CFRunLoopRun();
}
