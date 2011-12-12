#import "GView.h"
#import "GDisplay.h"

static void handleMouse(GView *v, NSEvent *e, UI::EventType t, int button)
{
    NSPoint pt = [v convertPoint:[e locationInWindow] fromView:nil];
    UI::MouseEvent uevent(t, button, pt.x, pt.y);
    [v->window_ handleUIEvent:&uevent];
}

@implementation GView

- (void)drawRect:(NSRect)r {
    [window_ update];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)mouseDown:(NSEvent *)theEvent {
    handleMouse(self, theEvent, UI::MouseDown, UI::ButtonLeft);
}

- (void)rightMouseDown:(NSEvent *)theEvent {
    handleMouse(self, theEvent, UI::MouseDown, UI::ButtonRight);
}

// FIXME: add support for other buttons
- (void)otherMouseDown:(NSEvent *)theEvent {
    handleMouse(self, theEvent, UI::MouseDown, UI::ButtonMiddle);
}

- (void)mouseUp:(NSEvent *)theEvent {
    handleMouse(self, theEvent, UI::MouseUp, UI::ButtonLeft);
}

- (void)rightMouseUp:(NSEvent *)theEvent {
    handleMouse(self, theEvent, UI::MouseUp, UI::ButtonRight);
}

// FIXME
- (void)otherMouseUp:(NSEvent *)theEvent {
    handleMouse(self, theEvent, UI::MouseUp, UI::ButtonMiddle);
}

- (void)mouseMoved:(NSEvent *)theEvent {
    handleMouse(self, theEvent, UI::MouseMove, -1);
}

- (void)mouseDragged:(NSEvent *)theEvent {
    handleMouse(self, theEvent, UI::MouseMove, -1);
}

- (void)scrollWheel:(NSEvent *)theEvent {
    
}

- (void)rightMouseDragged:(NSEvent *)theEvent {
    handleMouse(self, theEvent, UI::MouseMove, -1);
}

- (void)otherMouseDragged:(NSEvent *)theEvent {
    handleMouse(self, theEvent, UI::MouseMove, -1);
}

- (void)mouseEntered:(NSEvent *)theEvent {
    
}

- (void)mouseExited:(NSEvent *)theEvent {
    
}

- (void)keyDown:(NSEvent *)theEvent {
    GDisplayKeyEvent(window_, theEvent, UI::KeyDown);
}

- (void)keyUp:(NSEvent *)theEvent {
    GDisplayKeyEvent(window_, theEvent, UI::KeyUp);
}

- (void)flagsChanged:(NSEvent *)theEvent {
    
}

@end
