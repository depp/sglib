#import "GWindow.h"
#import "GView.h"

/* C++ interface */

class MacWindow : public UI::Window {
    GWindow *window;

public:
    explicit MacWindow(GWindow *w)
        : window(w)
    { }

    void close();
};

void MacWindow::close()
{
    // [window close];
}

/* Event handling */

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

void GWindowKeyEvent(GWindow *w, NSEvent *e, UI::EventType t)
{
    NSString *c = [e charactersIgnoringModifiers];
    int keyChar = [c characterAtIndex:0];
    int keyCode = mapKey(keyChar);
    UI::KeyEvent uevent(t, keyCode);
    [w handleUIEvent:&uevent];
}

/* Rendering callback */

static CVReturn cvCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *now, const CVTimeStamp *outputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
    static uint64_t vt, ht;
    NSLog(@"vt=%llu; ht=%llu\n", now->videoTime - vt, now->hostTime - ht);
    vt = now->videoTime;
    ht = now->hostTime;

    GWindow *w = (GWindow *) displayLinkContext;
    [w update];

    return kCVReturnSuccess;
}

/* Private methods */

@interface GWindow (Private)

- (void)startTimer;
- (void)stopTimer;

@end

@implementation GWindow (Private)

- (void)startTimer {
    if (1) {
        // OS X 10.4 and later only
        CVDisplayLinkCreateWithActiveCGDisplays(&link_);
        CVDisplayLinkSetOutputCallback(link_, cvCallback, self);
        CGLContextObj cglContext = (CGLContextObj) [context_ CGLContextObj];
        CGLPixelFormatObj cglPixelFormat = (CGLPixelFormatObj) [format_ CGLPixelFormatObj];
        CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(link_, cglContext, cglPixelFormat);
        CVDisplayLinkStart(link_);
    } else {
        /*
        NSTimer *t = [NSTimer timerWithTimeInterval:0.001 target:self selector:@selector(timerFired:) userInfo:nil repeats:YES];
        [[NSRunLoop currentRunLoop] addTimer:t forMode:NSDefaultRunLoopMode];
        [[NSRunLoop currentRunLoop] addTimer:t forMode:NSEventTrackingRunLoopMode];
        */
    }
}

- (void)stopTimer {

}

@end

/* Objective C interface */

@implementation GWindow

- (id)initWithScreen:(UI::Screen *)screen {
    [super init];

    pthread_mutexattr_t attr;
    int r;
    r = pthread_mutexattr_init(&attr);
    if (r) goto error;
    r = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (r) goto error;
    r = pthread_mutex_init(&lock_, &attr);
    if (r) goto error;
    r = pthread_mutexattr_destroy(&attr);
    if (r) goto error;

    uiwindow_ = new MacWindow(self);
    uiwindow_->setScreen(screen);

    return self;

error:
    [self dealloc];
    return nil;
}

/*
- (void)reshape {
    NSRect rect = [self bounds];
    window_->setSize(NSWidth(rect), NSHeight(rect));
}
*/

- (void)showWindow:(id)sender {
    NSScreen *s = [NSScreen mainScreen];
    NSRect r = NSMakeRect(0, 0, 768, 480);
    NSUInteger style = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask;
    NSWindow *w = [[NSWindow alloc] initWithContentRect:r styleMask:style backing:NSBackingStoreBuffered defer:NO screen:s];
    nswindow_ = w;
    [w setTitle:@"Game"];
    [w setDelegate:self];

    GView *v = [[[GView alloc] initWithFrame:r] autorelease];
    view_ = v;
    v->window_ = self;

    [w setContentView:v];
    [w makeKeyAndOrderFront:self];
}

- (void)showFullScreen:(id)sender {
    // Unimplemented
}

- (void)handleUIEvent:(UI::Event *)event
{
    [self lock];
    uiwindow_->handleEvent(*event);
    [self unlock];
}

- (void)update {
    NSLog(@"update");
    [self lock];

    if (!format_) {
        // OpenGL pixel format
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
        NSOpenGLPixelFormat *fmt = [[[NSOpenGLPixelFormat alloc] initWithAttributes:(NSOpenGLPixelFormatAttribute*) attrib] autorelease];
        format_ = fmt;
        NSLog(@"fmt = %p", fmt);

        // OpenGL context
        NSOpenGLContext *cxt = [[NSOpenGLContext alloc] initWithFormat:fmt shareContext:nil];
        context_ = cxt;
        [cxt setValues:&on forParameter:NSOpenGLCPSwapInterval];
        NSLog(@"cxt = %p", cxt);
        [cxt setView:view_];

        // Timer
        [self startTimer];
    }
    if (view_) {
        NSRect b = [view_ bounds];
        [context_ makeCurrentContext];
        glViewport(0, 0, b.size.width, b.size.height);
        uiwindow_->setSize(b.size.width, b.size.height);
        uiwindow_->draw();
        [context_ flushBuffer];
    }

    [self unlock];
}

- (void)lock {
    int r;
    r = pthread_mutex_lock(&lock_);
    assert(!r);
}

- (void)unlock {
    int r;
    r = pthread_mutex_unlock(&lock_);
    assert(!r);
}

@end
