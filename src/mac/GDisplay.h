// The GDisplay object is the center of the Mac UI code: it manages a window from the viewpoint of the common code.  It sends Cocoa events to the common code, and changes the window configuration in response to calls from the common code.
// The display will also manage the OpenGL context and rendering thread / rendering timer.
// Each display has a lock for accessing the OpenGL context and window object.  There is no need to obtain a lock, all calls are safe to call from the main thread and -update is safe to call from any thread.
#import <Cocoa/Cocoa.h>
#import <CoreVideo/CoreVideo.h>
#import "client/ui/window.hpp"
#import "client/ui/event.hpp"
#import "GApplication.h"
#import <pthread.h>

@class GView;

typedef enum {
    GDisplayNone,
    GDisplayWindow,
    GDisplayFSWindow, // Fullscreen window, kiosk mode is optional
    GDisplayFSCapture // Captured display, kiosk mode is mandatory
} GDisplayMode;

@interface GDisplay : NSObject <NSLocking, GEventCapture> {
    // Always valid
    UI::Window *uiwindow_;
    pthread_mutex_t lock_;
    BOOL modeChange_;

    BOOL frameChanged_;
    int width_, height_;

    // Mode is set first, and then OpenGL will be initialized by -[GDisplay update]
    GDisplayMode mode_;
    GDisplayMode queueMode_;

    // Valid when OpenGL is initialized
    NSOpenGLContext *context_;
    NSOpenGLContext *prevContext_;
    NSOpenGLPixelFormat *format_;
    CVDisplayLinkRef link_;

    // Valid in windowed mode
    NSWindow *nswindow_;
    GView *view_;

    // Valid in fullscreen mode
    CGDirectDisplayID display_;
}

- (id)initWithScreen:(UI::Screen *)screen;

- (void)setMode:(GDisplayMode)mode;
- (void)showWindow:(id)sender;
- (void)showFullScreen:(id)sender;

- (void)handleUIEvent:(UI::Event *)event;

// Safe to call from any thread (all other methods must be called from main thread)
- (void)update;

@end

void GDisplayKeyEvent(GDisplay *w, NSEvent *e, UI::EventType t);
