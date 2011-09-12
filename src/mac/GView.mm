#import "GView.h"
#import "ui/event.hpp"
#import "ui/screen.hpp"
#import "ui/menu.hpp"
// #import "clock.hpp"
#import "resource.hpp"
#import "opengl.hpp"

class MacWindow : public UI::Window {
    GView *view_;

public:
    MacWindow(GView *v)
        : view_(v)
    { }

    void close();
};

void MacWindow::close()
{
    [[view_ window] close];
}

static CVReturn cvCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *now, const CVTimeStamp *outputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
    GView *v = (GView *) displayLinkContext;
    [v lock];
    v->window_->draw();
    [[v openGLContext] flushBuffer];
    [v unlock];
    return kCVReturnSuccess;
}

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
    pthread_mutex_lock(&v->displayLock_);
    v->window_->handleEvent(UI::KeyEvent(t, keyCode));
    pthread_mutex_unlock(&v->displayLock_);
}

static void handleMouse(GView *v, NSEvent *e, UI::EventType t, int button)
{
    NSPoint pt = [e locationInWindow];
    pt = [v convertPoint:pt fromView:nil];
    // NSLog(@"Mouse: (%f, %f) %d", pt.x, pt.y, button);
    pthread_mutex_lock(&v->displayLock_);
    v->window_->handleEvent(UI::MouseEvent(t, button, pt.x, pt.y));
    pthread_mutex_unlock(&v->displayLock_);
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

    pthread_mutexattr_t attr;
    int r;
    r = pthread_mutexattr_init(&attr);
    r = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    r = pthread_mutex_init(&displayLock_, &attr);
    r = pthread_mutexattr_destroy(&attr);

    [[self openGLContext] setValues:&on forParameter:NSOpenGLCPSwapInterval];
    window_ = new MacWindow(self);
    window_->setScreen(new UI::Menu);

    return self;
}

- (void)dealloc {
    CVDisplayLinkRelease(displayLink_);
    pthread_mutex_destroy(&displayLock_);
    [super dealloc];
}

- (void)lock {
    pthread_mutex_lock(&displayLock_);
    [[self openGLContext] makeCurrentContext];
}

- (void)unlock {
    pthread_mutex_unlock(&displayLock_);
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
    if (1) {
        CVDisplayLinkCreateWithActiveCGDisplays(&displayLink_);
        CVDisplayLinkSetOutputCallback(displayLink_, cvCallback, self);
        CGLContextObj cglContext = (CGLContextObj) [[self openGLContext] CGLContextObj];
        CGLPixelFormatObj cglPixelFormat = (CGLPixelFormatObj) [[self pixelFormat] CGLPixelFormatObj];
        CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink_, cglContext, cglPixelFormat);
        CVDisplayLinkStart(displayLink_);
    } else {
        NSTimer *t = [NSTimer timerWithTimeInterval:0.001 target:self selector:@selector(timerFired:) userInfo:nil repeats:YES];
        [[NSRunLoop currentRunLoop] addTimer:t forMode:NSDefaultRunLoopMode];
        [[NSRunLoop currentRunLoop] addTimer:t forMode:NSEventTrackingRunLoopMode];
    }
}

- (void)timerFired:(id)sender {
    [self setNeedsDisplay:YES];
}

@end
