/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#import "GView.h"
#import "GDisplay.h"

static void handleMouse(GView *v, NSEvent *e, pce_event_type_t t, int button)
{
    NSPoint pt = [v convertPoint:[e locationInWindow] fromView:nil];
    struct pce_event_mouse evt;
    evt.type = t;
    evt.button = button;
    evt.x = pt.x;
    evt.y = pt.y;
    [v->display_ handleUIEvent:(union pce_event *) &evt];
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
