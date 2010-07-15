#import "SpiritWindow.h"
extern int load_mobile_device(); // from SpiritAppDelegate

@implementation SpiritWindow


-(id) initWithContentRect:(NSRect)contentRect 
				styleMask:(unsigned int)styleMask 
				  backing:(NSBackingStoreType)backingType 
					defer:(BOOL)flag 
{
	if (load_mobile_device()) {		
		NSAlert *alert = [[NSAlert alloc] init];
		[alert addButtonWithTitle:@"Quit"];
		[alert setMessageText:@"Spirit requires iTunes 9.0 or higher."];
		[alert setInformativeText:@"Please install the latest version of iTunes 9 and run Spirit again."];
		[alert setAlertStyle:NSCriticalAlertStyle];
		[alert runModal];
		
		exit(0);
	}
	
	if ((self = [super initWithContentRect:contentRect
								 styleMask:styleMask
								   backing:backingType
									 defer:flag])) {
		[self setHasShadow:YES];
		[self setMovableByWindowBackground:YES];
		[self setLevel:NSNormalWindowLevel];
		
	}
	
	return self;
}

- (BOOL) canBecomeKeyWindow
{
	return YES;
}

@end
