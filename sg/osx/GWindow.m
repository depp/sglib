/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
// A GWindow is an NSWindow that can become key even if it's borderless.
#import "GWindow.h"

@implementation GWindow

- (BOOL)canBecomeKeyWindow {
    return YES;
}

@end
