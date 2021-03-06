/**
 * This header is generated by class-dump-z 0.2a.
 * class-dump-z is Copyright (C) 2009 by KennyTM~, licensed under GPLv3.
 *
 * Source: /System/Library/Frameworks/UIKit.framework/UIKit
 */

#import "UIDateTableCell.h"

@class NSDate, UILabel;

__attribute__((visibility("hidden")))
@interface UIWeekMonthDayTableCell : UIDateTableCell {
@private
	UILabel* _weekdayLabel;
	NSDate* _date;
	float _weekdayWidth;
	BOOL _weekdayLast;
}
-(void)dealloc;
-(id)date;
-(void)setDate:(id)date;
-(void)setWeekdayLast:(BOOL)last;
-(void)setWeekdayWidth:(float)width;
-(id)_weekdayLabelColor;
-(void)setBackgroundColor:(id)color;
-(void)setWeekdayString:(id)string;
-(void)updateHighlightColors;
-(void)layoutSubviews;
@end

