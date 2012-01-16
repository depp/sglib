#import <Carbon/Carbon.h>
#import "GDisplay.h"
#import "GView.h"
#import "GWindow.h"
#import "GController.h"
#import "base/entry.h"
#import "base/keycode/keyid.h"
#import "base/keycode/keytable.h"
#import "base/resource.h"

@interface GDisplay (Private)

// Both must be called without lock
- (void)startGraphics;
- (void)stopGraphics;

- (void)queueModeChange:(GDisplayMode)mode;
- (void)applyChanges;

- (void)frameChanged:(NSNotification *)notification;

- (void)stateChanged;

@end

static bool isWindowedMode(GDisplayMode mode)
{
    return mode == GDisplayWindow || mode == GDisplayFSWindow;
}

/* Event handling */

void GDisplayKeyEvent(GDisplay *w, NSEvent *e, sg_event_type_t t)
{
    int ncode = MAC_NATIVE_TO_HID[[e keyCode] & 0x7F];
    struct sg_event_key evt;
    evt.type = t;
    evt.key = ncode;
    [w handleUIEvent:(union sg_event *) &evt];
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

static void handleMouse(GDisplay *d, NSEvent *e, sg_event_type_t t, int button)
{
    // FIXME need to translate if there are multiple screens
    if ([e window]) {
        NSLog(@"Windowed mouse event...");
        return;
    }
    NSPoint pt = [e locationInWindow];
    struct sg_event_mouse evt;
    evt.type = t;
    evt.button = button;
    evt.x = pt.x;
    evt.y = pt.y;
    [d handleUIEvent:(union sg_event *) &evt];
}

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
            sg_resource_dirtytype(SG_RSRC_TEXTURE);
    }
    if (!cxt) {
        cxt = [[NSOpenGLContext alloc] initWithFormat:fmt shareContext:nil];
        assert(cxt);
    }
    context_ = cxt;
    [cxt setValues:&on forParameter:NSOpenGLCPSwapInterval];
    if (windowed) {
        [cxt setView:view_];
        NSRect frame = [view_ frame];
        frameChanged_ = YES;
        width_ = frame.size.width;
        height_ = frame.size.height;
    } else {
        [cxt setFullScreen];
        frameChanged_ = YES;
        width_ = CGDisplayPixelsWide(display_);
        height_ = CGDisplayPixelsHigh(display_);
    }

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

- (void)frameChanged:(NSNotification *)notification {
    (void)notification;
    NSRect frame = [view_ bounds];
    [self lock];
    frameChanged_ = YES;
    width_ = frame.size.width;
    height_ = frame.size.height;
    [self unlock];
}

- (void)stateChanged {
    unsigned status;
    struct sg_event_status evt;
    if ([NSApp isHidden]) {
        status = 0;
    } else if (mode_ == GDisplayWindow) {
        if ([nswindow_ isMiniaturized])
            status = 0;
        else
            status = SG_STATUS_VISIBLE;
    } else {
        status = SG_STATUS_VISIBLE | SG_STATUS_FULLSCREEN;
    }
    evt.type = SG_EVENT_STATUS;
    evt.status = status;
    sg_sys_event((union sg_event *) &evt);
}

@end

/* Objective C interface */

@implementation GDisplay

- (id)init {
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

    NSNotificationCenter *c = [NSNotificationCenter defaultCenter];
    [c addObserver:self selector:@selector(applicationDidHide:) name:NSApplicationDidHideNotification object:NSApp];
    [c addObserver:self selector:@selector(applicationDidUnhide:) name:NSApplicationDidUnhideNotification object:NSApp];

    return self;

error:
    [self dealloc];
    return nil;
}

- (void)dealloc {
    pthread_mutex_destroy(&lock_);
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}

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
        [NSApp stopEventCapture];
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
        if (mode == GDisplayWindow) {
            [w center];
            [w setFrameAutosaveName:@"Game"];
        }
        if (!view_) {
            view_ = [[GView alloc] initWithFrame:r];
            view_->display_ = self;
            [view_ setPostsFrameChangedNotifications:YES];
            [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(frameChanged:) name:NSViewFrameDidChangeNotification object:view_];
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
        [NSApp startEventCapture:self];
    } else if (mode == GDisplayNone) {
        [[GController sharedInstance] removeDisplay:self];
        return;
    }

    modeChange_ = NO;
    [self stateChanged];
}

- (void)showWindow:(id)sender {
    (void)sender;
    [self setMode:GDisplayWindow];
}

- (void)showFullScreen:(id)sender {
    (void)sender;
    [self setMode:GDisplayFSWindow];
}

- (void)toggleFullScreen:(id)sender {
    if (mode_ == GDisplayWindow)
        [self showFullScreen:sender];
    else if (mode_ == GDisplayFSWindow || mode_ == GDisplayFSCapture)
        [self showWindow:sender];
}

- (void)handleUIEvent:(union sg_event *)event {
    [self lock];
    sg_sys_event(event);
    [self unlock];
}

- (void)update {
    if (!context_)
        [self startGraphics];
    if (!context_)
        return;

    [self lock];
    [context_ makeCurrentContext];
    if (frameChanged_) {
        [context_ update];
        glViewport(0, 0, width_, height_);
        struct sg_event_resize evt;
        evt.type = SG_EVENT_RESIZE;
        evt.width = width_;
        evt.height = height_;
        sg_sys_event((union sg_event *) &evt);
    }
    sg_sys_draw();
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
    nswindow_ = nil;
    [self setMode:GDisplayNone];
}

- (void)windowDidDeminiaturize:(NSNotification *)notification {
    (void) notification;
    [self stateChanged];
}

- (void)windowDidExpose:(NSNotification *)notification {
    (void) notification;
    [self stateChanged];
}

- (void)windowDidMiniaturize:(NSNotification *)notification {
    (void) notification;
    [self stateChanged];
}

- (void)applicationDidHide:(NSNotification *)notification {
    (void) notification;
    [self stateChanged];
}

- (void)applicationDidUnhide:(NSNotification *)notification {
    (void) notification;
    [self stateChanged];
}

- (BOOL)handleEvent:(NSEvent *)event {
    switch ([event type]) {
    case NSLeftMouseDown:
        handleMouse(self, event, SG_EVENT_MDOWN, SG_BUTTON_LEFT);
        return YES;

    case NSLeftMouseUp:
        handleMouse(self, event, SG_EVENT_MUP, SG_BUTTON_LEFT);
        return YES;

    case NSRightMouseDown:
        handleMouse(self, event, SG_EVENT_MDOWN, SG_BUTTON_RIGHT);
        return YES;

    case NSRightMouseUp:
        handleMouse(self, event, SG_EVENT_MUP, SG_BUTTON_RIGHT);
        return YES;

    case NSOtherMouseDown:
        handleMouse(self, event, SG_EVENT_MDOWN, SG_BUTTON_MIDDLE);
        return YES;

    case NSOtherMouseUp:
        handleMouse(self, event, SG_EVENT_MUP, SG_BUTTON_MIDDLE);
        return YES;

    case NSMouseMoved:
    case NSLeftMouseDragged:
    case NSRightMouseDragged:
    case NSOtherMouseDragged:
        handleMouse(self, event, SG_EVENT_MMOVE, -1);
        return YES;

    // case NSMouseEntered:
    // case NSMouseExited:

    case NSKeyDown:
        if ([event modifierFlags] & NSCommandKeyMask)
            return NO;
        GDisplayKeyEvent(self, event, SG_EVENT_KDOWN);
        return YES;

    case NSKeyUp:
        GDisplayKeyEvent(self, event, SG_EVENT_KUP);
        return YES;

    // case NSFlagsChanged:
    // case NSAppKitDefined:
    // case NSSystemDefined:
    // case NSApplicationDefined:
    // case NSPeriodic:
    // case NSCursorUpdate:
    // case NSScrollWheel:
    // case NSTabletPoint:
    // case NSTabletProximity:
    }
    return NO;
}

// Note: this won't get called in "capture" fullscreen mode but it doesn't matter since you can't see the menubar anyway.
- (BOOL)validateMenuItem:(NSMenuItem *)item {
    if ([item action] == @selector(toggleFullScreen:)) {
        if (mode_ == GDisplayWindow) {
            [item setTitle:NSLocalizedString(@"Enter Full Screen", nil)];
        } else {
            [item setTitle:NSLocalizedString(@"Exit Full Screen", nil)];
        }
    }
    return YES;
}

@end
