/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#import <Cocoa/Cocoa.h>

@protocol GEventCapture

// Return YES if the event was handled, NO otherwise
- (BOOL)handleEvent:(NSEvent *)evt;

@end

@interface GApplication : NSApplication {
    id <GEventCapture> evtDest_;
}

- (void)startEventCapture:(id <GEventCapture>)obj;
- (void)stopEventCapture;

@end
