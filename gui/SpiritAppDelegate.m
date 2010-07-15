//
//  SpiritAppDelegate.m
//  Spirit
//

#import "SpiritAppDelegate.h"
#import "MobileDevice.h"
#import <QuartzCore/QuartzCore.h>
#import <CommonCrypto/CommonDigest.h>
#import <AppKit/AppKit.h>
#import <dlfcn.h>
#import "dddata.h"

#define XZ_FILENAME "freeze-v1.tar.xz"
#define XZ_HASH "12345678901234567890"

#define MODEL_TRANSLATION_TABLE [NSDictionary dictionaryWithObjectsAndKeys: \
	@"iPhone 2G", @"iPhone1,1", \
	@"iPod touch 1G", @"iPod1,1", \
	@"iPhone 3G", @"iPhone1,2", \
	@"iPod touch 2G", @"iPod2,1", \
	@"iPhone 3GS", @"iPhone2,1", \
	@"iPod touch 3G", @"iPod3,1", \
	@"iPad", @"iPad1,1", \
	nil]

void *mobile_device_framework;
int (*am_device_notification_subscribe)(void *, int, int, void *, am_device_notification **);
mach_error_t (*am_device_connect)(struct am_device *device);
mach_error_t (*am_device_is_paired)(struct am_device *device);
mach_error_t (*am_device_validate_pairing)(struct am_device *device);
mach_error_t (*am_device_start_session)(struct am_device *device);

int load_mobile_device() 
{
	mobile_device_framework = dlopen("/System/Library/PrivateFrameworks/MobileDevice.framework/MobileDevice", RTLD_NOW);
	
	am_device_notification_subscribe = dlsym(mobile_device_framework, "AMDeviceNotificationSubscribe");
	am_device_connect = dlsym(mobile_device_framework, "AMDeviceConnect");
	am_device_is_paired = dlsym(mobile_device_framework, "AMDeviceIsPaired");
	am_device_validate_pairing = dlsym(mobile_device_framework, "AMDeviceValidatePairing");
	am_device_start_session = dlsym(mobile_device_framework, "AMDeviceStartSession");
	
	return (mobile_device_framework			== NULL ||
		am_device_notification_subscribe	== NULL ||
		am_device_connect					== NULL ||
		am_device_is_paired					== NULL ||
		am_device_validate_pairing			== NULL ||
		am_device_start_session				== NULL);
}

static NSString *device_copy_value(am_device *device, NSString *key)
{
	NSString * (*am_copy_value)(am_device *, NSString *, NSString *);
	am_copy_value = dlsym(mobile_device_framework, "AMDeviceCopyValue");
	
	if (!am_copy_value)
		return nil;
	
	return am_copy_value(device, nil, key);
}

static NSString *device_firmware_string(am_device *device)
{
	return device_copy_value(device, @"ProductVersion");
}

static NSString *device_model_string(am_device *device)
{
	return device_copy_value(device, @"ProductType");
}

static void device_notification_callback(am_device_notification_callback_info *info, void *foo) 
{
	SpiritAppDelegate *appDelegate = [[NSApplication sharedApplication] delegate];
	if ([appDelegate isJailbreaking]) return;
	
	if (info->msg != ADNCI_MSG_CONNECTED) { [appDelegate setDeviceConnected:NO]; return; }
	if (am_device_connect(info->dev)) { [appDelegate setDeviceConnected:NO]; return; }
	if (!am_device_is_paired(info->dev)) { [appDelegate setDeviceConnected:NO]; return; }
	if (am_device_validate_pairing(info->dev)) { [appDelegate setDeviceConnected:NO]; return; }
	if (am_device_start_session(info->dev)) { [appDelegate setDeviceConnected:NO]; return; }
	
	[appDelegate setDeviceFirmwareVersion:device_firmware_string(info->dev)];
	[appDelegate setDeviceModel:device_model_string(info->dev)];
	
	[appDelegate setDeviceConnected:YES];
}

static BOOL is_valid_freeze(NSString *path) {
	NSFileHandle *handle = [NSFileHandle fileHandleForReadingAtPath:path];
	if(!handle) return NO;
	NSData *data = [handle readDataToEndOfFile];
	unsigned char md[20];
	CC_SHA1([data bytes], [data length], md);
	return !memcmp(md, XZ_HASH, 20);
}


@implementation SpiritAppDelegate

- (BOOL)isJailbreaking
{
	return isJailbreaking;
}

- (BOOL)jailbreakComplete
{
	return jailbreakComplete;
}

- (int)getMobileDeviceVersion
{
	NSString *path = @"/System/Library/PrivateFrameworks/MobileDevice.framework/Resources/version.plist";
	NSDictionary *info = [NSPropertyListSerialization propertyListFromData:[NSData dataWithContentsOfFile:path]
														  mutabilityOption:kCFPropertyListImmutable
																	format:NULL
														  errorDescription:NULL];
	return [[info objectForKey:@"SourceVersion"] intValue];
	
}

- (void)flipProgressVisibility:(BOOL)progressVisible
{
	[progress setHidden:!progressVisible];
}

- (void)flipVisibility:(BOOL)progressVisible
{
	if([progress respondsToSelector:@selector(animator)]) {
		// CoreAnimation is available
		[progress setHidden:progressVisible];
		[[(id)button animator] setAlphaValue:progressVisible ? 0.0 : 1.0];
		
		if([timer isValid]) {
			[timer invalidate];
			[timer release];
		}
		
		timer = [[NSTimer scheduledTimerWithTimeInterval:0.1
												  target:self
											    selector:@selector(flipProgressVisibility:)
											    userInfo:[NSNumber numberWithBool:progressVisible]
												 repeats:NO] retain];
		
	} else {
		[progress setHidden:!progressVisible];
		[button setHidden:progressVisible];
	}	
}

- (void)didJailbreakWithResultCodeAndOutput:(NSArray *)array
{
	//NSLog(@"%@", array);
	
    NSNumber *resultCode = [array objectAtIndex:0];
    NSString *output = [array objectAtIndex:1];
	//NSLog(@"%@", output);
    [array release];
	[resultCode autorelease];
	[output autorelease];

	isJailbreaking = NO;
	jailbreakComplete = [resultCode intValue] == 0;
	
	[progress stopAnimation:self];	
	[self flipVisibility:NO];
	
	if (jailbreakComplete) {
		[noteText setTitleWithMnemonic:@"Jailbreak Complete!"];
		[button setTitle:@"Quit"];
	} else {
		NSString *text = [NSString stringWithFormat:@"Spirit encountered an unexpected error (%x).", [resultCode intValue]];
		NSAlert *alert = [NSAlert alertWithMessageText:text
										 defaultButton:@"Send Report"
									   alternateButton:@"Don't Send"
										   otherButton:nil
							 informativeTextWithFormat:@"Welp.  If you were doing something weird, don't.  Otherwise, please report the problem to comex."];
		
		[noteText setTitleWithMnemonic:[NSString stringWithFormat:@"Failed to jailbreak (error code: %x).", [resultCode intValue]]];
		[button setTitle:@"Retry"];
		
		NSTextField *field = nil;
		if([alert respondsToSelector:@selector(setAccessoryView:)]) {
			field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 600, 300)];
			[field setStringValue:output];
			[alert setAccessoryView:field];
		}
		
		NSInteger result = [alert runModal];
		if(result == NSAlertDefaultReturn) {
			NSData *data = [[field stringValue] dataUsingEncoding:NSUTF8StringEncoding];
			NSMutableURLRequest *request = [[NSMutableURLRequest alloc] initWithURL:[NSURL URLWithString:@"http://spiritjb.com/post.php?compressed"]];
			[request setValue:@"text/plain" forHTTPHeaderField:@"Content-Type"];
			[request setHTTPMethod:@"POST"];
			[request setHTTPBody:[data gzipDeflate]];
			[NSURLConnection connectionWithRequest:request delegate:nil];
			[request release];
		}
		
		[field release];
	}
}

- (void)setDeviceModel:(NSString *)model
{
	if (deviceModel != model) {
		[deviceModel release];
		deviceModel = [model retain];
	}
}

- (NSString *)deviceModel
{
	return deviceModel;
}

- (void)setDeviceFirmwareVersion:(NSString *)version
{
	if (firmwareVersion != version) {
		[firmwareVersion release];
		firmwareVersion = [version retain];
	}
}

- (NSString *)deviceFirmwareVersion
{
	return firmwareVersion;
}


- (BOOL)deviceConnected
{
	return deviceConnected;
}

- (BOOL)isReadyToJailbreak
{
	static NSDictionary *map = nil;
	if (map == nil) {
		NSString *mappath = [[NSBundle mainBundle] pathForResource:@"map" ofType:@"plist" inDirectory:@"igor"];
		map = [[NSPropertyListSerialization propertyListFromData:[NSData dataWithContentsOfFile:mappath]
												mutabilityOption:kCFPropertyListImmutable
														  format:NULL
												errorDescription:NULL] retain];
	}
	if(![self deviceConnected]) return NO;
	return nil != [map objectForKey:[NSString stringWithFormat:@"%@_%@",
									 [self deviceModel], 
									 [self deviceFirmwareVersion]]];
}

- (NSString *)humanReadableDeviceModel
{
	NSString *attempt = [MODEL_TRANSLATION_TABLE objectForKey:[self deviceModel]];
	
	// This is so for unknown models we don't display "(null)"
	if (attempt) return attempt;
	else return [self deviceModel];
}

- (void)setDeviceConnected:(BOOL)connected
{
	if (jailbreakComplete && !connected) {
		jailbreakComplete = NO;
		return;
	}
	
	deviceConnected = connected;
	
	NSString *buttonTitle = @"";
	
	if (![self deviceConnected]) {
		[self setDeviceFirmwareVersion:nil];
		[self setDeviceModel:nil];
		
		buttonTitle = @"Please connect device.";
	} else if (![self isReadyToJailbreak]) {
		buttonTitle = [NSString stringWithFormat:@"Device %@ (%@) is not supported.", [self humanReadableDeviceModel], [self deviceFirmwareVersion]];
	} else {
		if ([[noteText cell] respondsToSelector:@selector(setBackgroundStyle:)])
			[[noteText cell] setBackgroundStyle:NSBackgroundStyleRaised];	
		
		buttonTitle = [NSString stringWithFormat:@"Ready: %@ (%@) connected.", [self humanReadableDeviceModel], [self deviceFirmwareVersion]];
	}
	
	[noteText setTitleWithMnemonic:buttonTitle];
	[button setEnabled:[self isReadyToJailbreak]];
	[button setTitleWithMnemonic:@"Jailbreak"];
}

- (void)alertDidEnd:(NSAlert *)alert returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
	if(returnCode == NSAlertSecondButtonReturn) {
		[[alert window] orderOut:self];
		exit(0);
	}
}
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification 
{
	[self setDeviceConnected:NO];

#if 0	
	NSArray *array = NSSearchPathForDirectoriesInDomains(NSDownloadsDirectory, NSUserDomainMask, YES);
	xzfile = nil;
	for(int i = 0; i < [array count]; i++) { // no fast enumeration :(
			if(is_valid_freeze([array objectAtIndex:i])) {
			xzfile = [path retain];
			break;
		}
	}
	
	if(!xzfile) {
		NSAlert *alert = [NSAlert alertWithMessageText:@XZ_FILENAME " was not found in the Downloads directory."
										 defaultButton:@"Download"
									   alternateButton:@"Quit"						  
										   otherButton:@"Choose File..."
							 informativeTextWithFormat:@"If you haven't downloaded it before, choose Download.  If you are asked to choose a destination directory, specify Downloads.\n\n"
														"Alternately, select a file in a different location."];
		NSInteger result = [alert runModal];
		if(result == NSAlertDefaultReturn) {
			
		} else if(result == NSAlertAlternateReturn) {
		} else {
			exit(0);
		}
	}
#endif
	
	if([self getMobileDeviceVersion] < 2510600) {
		NSAlert *alert = [[NSAlert alloc] init];
		[alert addButtonWithTitle:@"Continue"];
		[alert addButtonWithTitle:@"Quit"];		
		[alert setMessageText:@"Old version of iTunes"];
		[alert setInformativeText:@"Spirit has not been tested with this version of iTunes.  You can continue, but it might not work."];
		[alert setAlertStyle:NSWarningAlertStyle];
		[alert beginSheetModalForWindow:window
						  modalDelegate:self
						 didEndSelector:@selector(alertDidEnd:returnCode:contextInfo:)
							contextInfo:NULL]; 
	}	
	
		
	am_device_notification *notif;
	am_device_notification_subscribe(device_notification_callback, 0, 0, NULL, &notif);
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
	return YES;
}

- (void)performJailbreak
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSTask *task = [[NSTask alloc] init];
	NSDate *startDate = [NSDate date];
	NSNumber *exitStatus;
    NSData *data;
    NSString *output;
    NSPipe *outputPipe = [NSPipe pipe];
    NSPipe *errorPipe = [NSPipe pipe];

	@try {
		[task setLaunchPath:[[NSBundle mainBundle] pathForResource:@"dl" ofType:nil]];
		[task setArguments:[NSArray arrayWithObjects:nil]];
	    [task setCurrentDirectoryPath:[[NSBundle mainBundle] resourcePath]];
        [task setStandardError:errorPipe];
        [task setStandardOutput:outputPipe];
		[task launch];
		[task waitUntilExit];
		
		exitStatus = [[NSNumber numberWithInt:[task terminationStatus]] retain];
        data = [[errorPipe fileHandleForReading] readDataToEndOfFile];
        output = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
		NSLog(@"hi");
        if(exitStatus) {
            // 256 exit codes are not enough!
            NSData *codeData = [[outputPipe fileHandleForReading] readDataToEndOfFile];
            NSString *codeString = [[NSString alloc] initWithData:codeData encoding:NSUTF8StringEncoding];
            exitStatus = [[NSNumber numberWithInt:[codeString intValue]] retain];
            [codeString release];
        }
	} @catch (NSException *exception) {
		exitStatus = [[NSNumber numberWithInt:0x42] retain];
        output = [[NSString alloc] initWithFormat:@"%@: %@", [exception name], [exception reason]];
	}

    [task release];
	
	NSDate *endDate = [NSDate date];
	NSTimeInterval diff = [endDate timeIntervalSinceDate:startDate];
	
	// This thread should always take at least a second
	if (diff < 1.0)
		[NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.0 - diff]];
	
	[self performSelectorOnMainThread:@selector(didJailbreakWithResultCodeAndOutput:)
                           withObject:[[NSArray arrayWithObjects:exitStatus, output, nil] retain]
                        waitUntilDone:NO];
    [pool release];
}

- (IBAction)performClick:(id)sender
{
	if(isJailbreaking)
		return;

	if(jailbreakComplete)
		[[NSApplication sharedApplication] terminate:nil];
	
	isJailbreaking = YES;
	
	[progress startAnimation:self];
	[self flipVisibility:YES];
	[noteText setTitleWithMnemonic:@"Jailbreaking..."];
	
	[NSThread detachNewThreadSelector:@selector(performJailbreak) toTarget:self withObject:nil];
}

@end

// It's easier if we have this code here, rather than in a new file
int main(int argc, char *argv[])
{			
    return NSApplicationMain(argc,  (const char **) argv);
}


