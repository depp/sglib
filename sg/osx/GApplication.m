/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#import "GApplication.h"

@implementation GApplication

- (void)sendEvent:(NSEvent *)anEvent {
    if (evtDest_) {
        if ([evtDest_ handleEvent:anEvent])
            return;
    }
    [super sendEvent:anEvent];
}

- (void)startEventCapture:(id <GEventCapture>)obj {
    evtDest_ = obj;
}

- (void)stopEventCapture {
    evtDest_ = nil;
}

@end
