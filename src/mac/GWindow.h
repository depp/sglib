// The GWindow class is the center of the Mac UI code.  Everything in the UI must touch GWindow because it has the lock.  Rendering can be done in a separate thread, making the lock necessary.  NOTE: All GWindow methods will lock, there is no need to obtain the lock yourself.  The lock maintains exclusion on the UI::Window object and on the OpenGL context, 
#import <Cocoa/Cocoa.h>
#import <CoreVideo/CoreVideo.h>
#import "ui/window.hpp"
#import "ui/event.hpp"
#import <pthread.h>

@class GView;

@interface GWindow : NSObject <NSLocking> {
    // Always valid
    UI::Window *uiwindow_;
    pthread_mutex_t lock_;

    // Valid when the window is visible
    NSOpenGLContext *context_;
    NSOpenGLPixelFormat *format_;
    CVDisplayLinkRef link_;

    // Valid when the window is windowed (not full-screen)
    NSWindow *nswindow_;
    GView *view_;
}

- (id)initWithScreen:(UI::Screen *)screen;

- (void)showWindow:(id)sender;
- (void)showFullScreen:(id)sender;

- (void)handleUIEvent:(UI::Event *)event;

// Safe to call from any thread (all other methods must be called from main thread)
- (void)update;

@end

void GWindowKeyEvent(GWindow *w, NSEvent *e, UI::EventType t);
