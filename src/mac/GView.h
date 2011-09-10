#import <Cocoa/Cocoa.h>

@interface GView : NSOpenGLView {

}

- (void)startTimer;
- (void)timerFired:(id)sender;

@end
