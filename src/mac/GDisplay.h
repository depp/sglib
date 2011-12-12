// The GDisplay class is the center of the Mac UI code.  Everything in the UI must touch GDisplay because it has the lock.  Rendering can be done in a separate thread, making the lock necessary.  NOTE: All GDisplay methods will lock, there is no need to obtain the lock yourself.  The lock maintains exclusion on the UI::Window object and on the OpenGL context, 
#import <Cocoa/Cocoa.h>
#import <CoreVideo/CoreVideo.h>
#import "ui/window.hpp"
#import "ui/event.hpp"
#import <pthread.h>

@class GView;

typedef enum {
    GDisplayNone,
    GDisplayWindow,
    GDisplayFSWindow, // Fullscreen window, kiosk mode is optional
    GDisplayFSCapture // Captured display, kiosk mode is mandatory
} GDisplayMode;

@interface GDisplay : NSObject <NSLocking> {
    // Always valid
    UI::Window *uiwindow_;
    pthread_mutex_t lock_;

    // Mode is set first, and then OpenGL will be initialized by -[GDisplay update]
    GDisplayMode mode_;

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
