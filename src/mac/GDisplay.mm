#import <Carbon/Carbon.h>
#import "GDisplay.h"
#import "GView.h"
#import "GWindow.h"
#import "GController.h"
#import <sys/resource.hpp>

@interface GDisplay (Private)

// Both must be called without lock
- (void)startGraphics;
- (void)stopGraphics;

- (void)queueModeChange:(GDisplayMode)mode;
- (void)applyChanges;

@end

static bool isWindowedMode(GDisplayMode mode)
{
    return mode == GDisplayWindow || mode == GDisplayFSWindow;
}

/* C++ interface */

class MacWindow : public UI::Window {
    GDisplay *window;

public:
    explicit MacWindow(GDisplay *w)
        : window(w)
    { }

    void close();
};

void MacWindow::close()
{
    [window queueModeChange:GDisplayNone];
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

void GDisplayKeyEvent(GDisplay *w, NSEvent *e, UI::EventType t)
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
    (void)displayLink;
    (void)now;
    (void)outputTime;
    (void)flagsIn;
    (void)flagsOut;

    GDisplay *w = (GDisplay *) displayLinkContext;
    [w update];

    return kCVReturnSuccess;
}

/* Private methods */

@implementation GDisplay (Private)

- (void)startGraphics {
    if (context_)
        return;

    GLint on = 1;
    GLuint attrib[16];
    int i = 0;
    GLuint glmode;
    bool windowed;

    switch (mode_) {
    case GDisplayWindow:
    case GDisplayFSWindow:
        glmode = NSOpenGLPFAWindow;
        windowed = true;
        break;
    case GDisplayFSCapture:
        glmode = NSOpenGLPFAFullScreen;
        windowed = false;
        break;
    default:
        return;
    }

    attrib[i++] = glmode;
    attrib[i++] = NSOpenGLPFAAccelerated;
    attrib[i++] = NSOpenGLPFADoubleBuffer;
    attrib[i++] = NSOpenGLPFAColorSize; attrib[i++] = 24;
    attrib[i++] = NSOpenGLPFAAlphaSize; attrib[i++] = 8;
    attrib[i++] = NSOpenGLPFADepthSize; attrib[i++] = 24;
    // attrib[i++] = NSOpenGLPFAStencilSize; attrib[i++] = 8;
    // attrib[i++] = NSOpenGLPFAAccumSize; attrib[i++] = 0;
    if (!windowed) {
        attrib[i++] = NSOpenGLPFAScreenMask;
        attrib[i++] = CGDisplayIDToOpenGLDisplayMask(display_);
    }
    attrib[i] = 0;

    NSOpenGLPixelFormat *fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:(NSOpenGLPixelFormatAttribute*) attrib];
    assert(fmt);
    format_ = fmt;

    NSOpenGLContext *cxt = nil;
    if (prevContext_) {
        // In tests, context sharing succeeded between windowed contexts and failed between a windowed and a fullscreen context.
        cxt = [[NSOpenGLContext alloc] initWithFormat:fmt shareContext:prevContext_];
        [prevContext_ release];
        prevContext_ = nil;
        if (!cxt)
            Resource::resetGraphics();
    }
    if (!cxt) {
        cxt = [[NSOpenGLContext alloc] initWithFormat:fmt shareContext:nil];
        assert(cxt);
    }
    context_ = cxt;
    [cxt setValues:&on forParameter:NSOpenGLCPSwapInterval];
    if (windowed)
        [cxt setView:view_];
    else
        [cxt setFullScreen];

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

- (void)stopGraphics {
    if (!context_)
        return;

    if (1) {
        // OS X 10.4 and later only
        CVDisplayLinkStop(link_);
        CVDisplayLinkRelease(link_);
        link_ = NULL;
    } else {
        /* NSTimer version */
    }
    // Only one of prevContext_ and context_ is ever set
    prevContext_ = context_;
    context_ = nil;
    [format_ release];
    format_ = nil;
}

- (void)queueModeChange:(GDisplayMode)mode {
    queueMode_ = mode;
    if (queueMode_ != mode_)
        [self performSelectorOnMainThread:@selector(applyChanges) withObject:nil waitUntilDone:NO];
}

- (void)applyChanges {
    [self setMode:queueMode_];
}

@end

/* Objective C interface */

@implementation GDisplay

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

- (void)setMode:(GDisplayMode)mode {
    if (mode == mode_ || modeChange_)
        return;
    modeChange_ = YES;
    [self stopGraphics];

    // There are two main ways to make a fullscreen window.
    // On OS X >= 10.6, we can use enterFullScreenMode on a windowless NSView and supply the options we want.
    // On OS X <= 10.4, enterFullScreenMode does not exist and we must manually size the window and set the UI mode.  This requires linking to Carbon.
    // On OS X 10.5, enterFullScreenMode exists, but using it precludes setting the UI mode.
    // Since I don't have 10.6, this code uses the 10.4 method.

    if (isWindowedMode(mode_)) {
        if (mode_ == GDisplayFSWindow) {
            SetSystemUIMode(kUIModeNormal, 0);
        }
        if (!isWindowedMode(mode)) {
            if (nswindow_) {
                [nswindow_ close];
                nswindow_ = nil;
            }
            [view_ release];
            view_ = nil;
        }
    } else if (mode_ == GDisplayFSCapture) {
        CGReleaseAllDisplays();
    } else if (mode_ == GDisplayNone) {
        [[GController sharedInstance] addDisplay:self];
    }

    mode_ = mode;

    if (isWindowedMode(mode)) {
        NSScreen *s = [NSScreen mainScreen];
        NSRect r;
        NSUInteger style;
        NSWindow *w;
        if (mode == GDisplayWindow) {
            r = NSMakeRect(0, 0, 768, 480);
            style = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask;
        } else {
            r = [s frame];
            style = NSBorderlessWindowMask;
        }
        if (nswindow_) {
            [nswindow_ close];
            nswindow_ = nil;
        }
        w = [[GWindow alloc] initWithContentRect:r styleMask:style backing:NSBackingStoreBuffered defer:NO screen:s];
        nswindow_ = w;
        [w setTitle:@"Game"];
        [w setDelegate:self];
        if (!view_) {
            view_ = [[GView alloc] initWithFrame:r];
            view_->window_ = self;
        }
        [w setContentView:view_];

        if (mode == GDisplayFSWindow) {
            SetSystemUIMode(kUIModeAllSuppressed, 0);
        }
        [nswindow_ makeKeyAndOrderFront:self];
        // -[GDisplay update] is called by -[GView drawRect:], no need to call it manually
    } else if (mode == GDisplayFSCapture) {
        CGDisplayErr err;
        err = CGCaptureAllDisplays();
        assert(!err);
        display_ = CGMainDisplayID();
        [self update];
    } else if (mode == GDisplayNone) {
        [[GController sharedInstance] removeDisplay:self];
    }

    modeChange_ = NO;
}

- (void)showWindow:(id)sender {
    (void)sender;
    [self setMode:GDisplayWindow];
}

- (void)showFullScreen:(id)sender {
    (void)sender;
    [self setMode:GDisplayFSCapture];
}

- (void)handleUIEvent:(UI::Event *)event
{
    [self lock];
    uiwindow_->handleEvent(*event);
    [self unlock];
}

- (void)update {
    if (!context_)
        [self startGraphics];
    if (!context_)
        return;

    [self lock];
    [context_ makeCurrentContext];
    if (isWindowedMode(mode_)) {
        NSRect b = [view_ bounds];
        glViewport(0, 0, b.size.width, b.size.height);
        uiwindow_->setSize(b.size.width, b.size.height);
    } else if (mode_ == GDisplayFSCapture) {
        size_t w = CGDisplayPixelsWide(display_);
        size_t h = CGDisplayPixelsHigh(display_);
        glViewport(0, 0, w, h);
        uiwindow_->setSize(w, h);
    }
    uiwindow_->draw();
    [context_ flushBuffer];
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

- (void)windowWillClose:(NSNotification *)notification {
    (void)notification;
    [self setMode:GDisplayNone];
}

@end
