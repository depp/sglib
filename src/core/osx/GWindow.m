/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
// A GWindow is an NSWindow that can become key even if it's borderless.
#import "GWindow.h"

@implementation GWindow

- (BOOL)canBecomeKeyWindow {
    return YES;
}

@end
