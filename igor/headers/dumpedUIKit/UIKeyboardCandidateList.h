/**
 * This header is generated by class-dump-z 0.2a.
 * class-dump-z is Copyright (C) 2009 by KennyTM~, licensed under GPLv3.
 *
 * Source: /System/Library/Frameworks/UIKit.framework/UIKit
 */

#import "UIKit-Structs.h"
#import "NSObject.h"


@protocol UIKeyboardCandidateList <NSObject>
-(void)setCandidates:(id)candidates inlineText:(id)text inlineRect:(CGRect)rect maxX:(float)x layout:(BOOL)layout;
-(void)layout;
-(void)setUIKeyboardCandidateListDelegate:(id)delegate;
-(void)showCandidateAtIndex:(unsigned)index;
-(void)showNextCandidate;
-(void)showPageAtIndex:(unsigned)index;
-(void)showNextPage;
-(void)showPreviousPage;
-(id)currentCandidate;
-(unsigned)currentIndex;
-(id)candidateAtIndex:(unsigned)index;
-(void)candidateAcceptedAtIndex:(unsigned)index;
-(unsigned)count;
-(void)configureKeyboard:(id)keyboard;
@optional
-(void)setCandidates:(id)candidates type:(int)type inlineText:(id)text inlineRect:(CGRect)rect maxX:(float)x layout:(BOOL)layout;
-(void)showCaret:(BOOL)caret gradually:(BOOL)gradually;
-(void)setCompletionContext:(id)context;
-(void)obsoleteCandidates;
@end
