#import <Cocoa/Cocoa.h>

@protocol GEventCapture

// Return YES if the event was handled, NO otherwise
- (BOOL)handleEvent:(NSEvent *)evt;

@end


@interface GApplication : NSApplication {
    id <GEventCapture> evtDest_;
}

- (void)startEventCapture:(id <GEventCapture>)obj;
- (void)stopEventCapture;

@end
