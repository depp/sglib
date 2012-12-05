/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#import <Cocoa/Cocoa.h>
@class GDisplay;

@interface GController : NSObject {
    NSMutableArray *displays_;
}

+ (GController *)sharedInstance;
- (void)addDisplay:(GDisplay *)display;
- (void)removeDisplay:(GDisplay *)display;

@end
