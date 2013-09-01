/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
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
