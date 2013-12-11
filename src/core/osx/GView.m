/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#import "GView.h"
#import "GDisplay.h"

static void handleMouse(GView *v, NSEvent *e, sg_event_type_t t, int button)
{
    NSPoint pt = [v convertPoint:[e locationInWindow] fromView:nil];
    struct sg_event_mouse evt;
    evt.type = t;
    evt.button = button;
    evt.x = pt.x;
    evt.y = pt.y;
    [v->display_ handleUIEvent:(union sg_event *) &evt];
}

@implementation GView

- (void)drawRect:(NSRect)r {
    (void)r;
    [display_ update];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)mouseDown:(NSEvent *)theEvent {
    handleMouse(self, theEvent, SG_EVENT_MDOWN, SG_BUTTON_LEFT);
}

- (void)rightMouseDown:(NSEvent *)theEvent {
    handleMouse(self, theEvent, SG_EVENT_MDOWN, SG_BUTTON_RIGHT);
}

// FIXME: add support for other buttons
- (void)otherMouseDown:(NSEvent *)theEvent {
    handleMouse(self, theEvent, SG_EVENT_MDOWN, SG_BUTTON_MIDDLE);
}

- (void)mouseUp:(NSEvent *)theEvent {
    handleMouse(self, theEvent, SG_EVENT_MUP, SG_BUTTON_LEFT);
}

- (void)rightMouseUp:(NSEvent *)theEvent {
    handleMouse(self, theEvent, SG_EVENT_MUP, SG_BUTTON_RIGHT);
}

// FIXME
- (void)otherMouseUp:(NSEvent *)theEvent {
    handleMouse(self, theEvent, SG_EVENT_MUP, SG_BUTTON_MIDDLE);
}

- (void)mouseMoved:(NSEvent *)theEvent {
    handleMouse(self, theEvent, SG_EVENT_MMOVE, -1);
}

- (void)mouseDragged:(NSEvent *)theEvent {
    handleMouse(self, theEvent, SG_EVENT_MMOVE, -1);
}

- (void)scrollWheel:(NSEvent *)theEvent {
    (void)theEvent;
}

- (void)rightMouseDragged:(NSEvent *)theEvent {
    handleMouse(self, theEvent, SG_EVENT_MMOVE, -1);
}

- (void)otherMouseDragged:(NSEvent *)theEvent {
    handleMouse(self, theEvent, SG_EVENT_MMOVE, -1);
}

- (void)mouseEntered:(NSEvent *)theEvent {
    (void)theEvent;
}

- (void)mouseExited:(NSEvent *)theEvent {
    (void)theEvent;
}

- (void)keyDown:(NSEvent *)theEvent {
    GDisplayKeyEvent(display_, theEvent, SG_EVENT_KDOWN);
}

- (void)keyUp:(NSEvent *)theEvent {
    GDisplayKeyEvent(display_, theEvent, SG_EVENT_KUP);
}

- (void)flagsChanged:(NSEvent *)theEvent {
    (void)theEvent;
}

@end
