#import "GView.h"
#import "ui/event.hpp"
#import "ui/screen.hpp"
#import "ui/menu.hpp"
// #import "clock.hpp"
#import "resource.hpp"
#import "opengl.hpp"

static int mapKey(int key)
{
    switch (key) {
    case 27: // Escape
        return UI::KEscape;

    case 32: // Space
    case 13: // Return
        return UI::KSelect;

    case NSLeftArrowFunctionKey:
        return UI::KLeft;

    case NSUpArrowFunctionKey:
        return UI::KUp;

    case NSRightArrowFunctionKey:
        return UI::KRight;

    case NSDownArrowFunctionKey:
        return UI::KDown;

    default:
        return -1;
    }
}

static void handleKey(GView *v, NSEvent *e, UI::EventType t)
{
    (void)v;
    NSString *c = [e charactersIgnoringModifiers];
    int keyChar = [c characterAtIndex:0];
    int keyCode = mapKey(keyChar);
    // NSLog(@"Key: %@ -> %d -> %d", c, keyChar, keyCode);
    v->window_->handleEvent(UI::KeyEvent(t, keyCode));
}

static void handleMouse(GView *v, NSEvent *e, UI::EventType t, int button)
{
    NSPoint pt = [e locationInWindow];
    pt = [v convertPoint:pt fromView:nil];
    // NSLog(@"Mouse: (%f, %f) %d", pt.x, pt.y, button);
    v->window_->handleEvent(UI::MouseEvent(t, button, pt.x, pt.y));
}

@implementation GView

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)mouseDown:(NSEvent *)theEvent {
    handleMouse(self, theEvent, UI::MouseDown, UI::ButtonLeft);
}

- (void)rightMouseDown:(NSEvent *)theEvent {
    handleMouse(self, theEvent, UI::MouseDown, UI::ButtonRight);
}

- (void)otherMouseDown:(NSEvent *)theEvent {
    handleMouse(self, theEvent, UI::MouseDown, UI::ButtonMiddle);
}

- (void)mouseUp:(NSEvent *)theEvent {
    handleMouse(self, theEvent, UI::MouseUp, UI::ButtonLeft);
}

- (void)rightMouseUp:(NSEvent *)theEvent {
    handleMouse(self, theEvent, UI::MouseUp, UI::ButtonRight);
}

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
    handleKey(self, theEvent, UI::KeyDown);
}

- (void)keyUp:(NSEvent *)theEvent {
    handleKey(self, theEvent, UI::KeyUp);
}

- (void)flagsChanged:(NSEvent *)theEvent {
    
}

- (id)initWithFrame:(NSRect) frame
{
    GLint on = 1;
    GLuint attrib[] = {
        NSOpenGLPFANoRecovery,
        NSOpenGLPFAWindow,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAStencilSize, 8,
        NSOpenGLPFAAccumSize, 0,
        0
    };

    NSOpenGLPixelFormat* fmt = [[[NSOpenGLPixelFormat alloc] initWithAttributes:(NSOpenGLPixelFormatAttribute*) attrib] autorelease]; 

    if (!fmt)
        NSLog(@"No OpenGL pixel format");

    self = [super initWithFrame:frame pixelFormat:fmt];
    if (!self)
        return nil;

    [[self openGLContext] setValues:&on forParameter:NSOpenGLCPSwapInterval];
    window_ = new UI::Window;
    window_->setScreen(new UI::Menu);

    return self;
}

- (void)drawRect:(NSRect)rect {
    window_->draw();
    [[self openGLContext] flushBuffer];
}

- (void)reshape {
    NSRect rect = [self bounds];
    window_->setSize(NSWidth(rect), NSHeight(rect));
}

- (void)startTimer {
    NSTimer *t = [NSTimer timerWithTimeInterval:0.001 target:self selector:@selector(timerFired:) userInfo:nil repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:t forMode:NSDefaultRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:t forMode:NSEventTrackingRunLoopMode];
}

- (void)timerFired:(id)sender {
    [self setNeedsDisplay:YES];
}

@end
