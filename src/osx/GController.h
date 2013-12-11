/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#import <Cocoa/Cocoa.h>
@class GDisplay;

@interface GController : NSObject {
    NSMutableArray *displays_;
}

+ (GController *)sharedInstance;
- (void)addDisplay:(GDisplay *)display;
- (void)removeDisplay:(GDisplay *)display;

@end
