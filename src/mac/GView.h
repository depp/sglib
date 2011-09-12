#import <Cocoa/Cocoa.h>
#import <CoreVideo/CoreVideo.h>
#import "ui/window.hpp"
#import <pthread.h>

@interface GView : NSOpenGLView {
@public
    CVDisplayLinkRef displayLink_;
    pthread_mutex_t displayLock_;
    UI::Window *window_;
}

- (void)startTimer;
- (void)timerFired:(id)sender;

@end
