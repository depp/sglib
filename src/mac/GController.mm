#import "GController.h"
#import "GView.h"
#import "rand.hpp"
#import "sys/path.hpp"
#import "sys/clock.hpp"
#import "ui/menu.hpp"

@implementation GController

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSLog(@"app launched");
    Path::init();
    initTime();
    Rand::global.seed();
    UI::Screen::setActive(new UI::Menu);

    NSScreen *s = [NSScreen mainScreen];
    NSRect r = NSMakeRect(0, 0, 768, 480);
    NSUInteger style = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask;
    NSWindow *w = [[NSWindow alloc] initWithContentRect:r styleMask:style backing:NSBackingStoreBuffered defer:NO screen:s];
    [w setTitle:@"Game"];
    GView *v = [[[GView alloc] initWithFrame:r] autorelease];
    [w setContentView:v];
    [w makeKeyAndOrderFront:self];
    [v startTimer];
}

@end
