//
//  SpiritAppDelegate.h
//  Spirit
//

#import <Cocoa/Cocoa.h>
#import <AppKit/NSProgressIndicator.h>

@interface SpiritAppDelegate : NSObject {
    IBOutlet NSWindow *window;
	IBOutlet NSButton *button;
	IBOutlet NSProgressIndicator *progress;
	IBOutlet NSTextField *noteText;
	IBOutlet BOOL deviceConnected;
	IBOutlet BOOL jailbreakComplete;
	BOOL isJailbreaking;
	NSTimer *timer;
	NSString *firmwareVersion;
	NSString *deviceModel;
#if 0
	NSString *xzfile;
#endif
}

- (int)getMobileDeviceVersion;

- (IBAction)performClick:(id)sender;

- (void)setDeviceConnected:(BOOL)connected;
- (void)setDeviceFirmwareVersion:(NSString *)version;
- (void)setDeviceModel:(NSString *)model;

- (BOOL)isJailbreaking;
- (BOOL)jailbreakComplete;

@end
