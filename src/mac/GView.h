#import <Cocoa/Cocoa.h>
#import "ui/window.hpp"

@interface GView : NSOpenGLView {
@public
    UI::Window *window_;
}

- (void)startTimer;
- (void)timerFired:(id)sender;

@end
