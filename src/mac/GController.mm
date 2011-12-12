#import "GController.h"
#import "GWindow.h"
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

    UI::Screen *s = new UI::Menu;
    GWindow *w = [[GWindow alloc] initWithScreen:s];
    // [w showWindow:self];
    [w showFullScreen:self];
}


@end
