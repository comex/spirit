/**
 * This header is generated by class-dump-z 0.2a.
 * class-dump-z is Copyright (C) 2009 by KennyTM~, licensed under GPLv3.
 *
 * Source: /System/Library/Frameworks/UIKit.framework/UIKit
 */

#import <Foundation/NSObject.h>

@class NSString, UIViewController;

__attribute__((visibility("hidden")))
@interface UIViewControllerAction : NSObject {
@private
	UIViewController* _viewController;
	NSString* _name;
	BOOL _animated;
}
@property(assign, nonatomic) UIViewController* viewController;
@property(retain, nonatomic) NSString* name;
@property(assign, nonatomic) BOOL animated;
-(id)initWithViewController:(id)viewController name:(id)name animated:(BOOL)animated;
-(void)dealloc;
@end

