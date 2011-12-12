#import "GController.h"
#import "GDisplay.h"
#import "rand.hpp"
#import "sys/path.hpp"
#import "sys/clock.hpp"
#import "ui/menu.hpp"

@implementation GController

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    (void)notification;
    Path::init();
    initTime();
    Rand::global.seed();

    UI::Screen *s = new UI::Menu;
    GDisplay *w = [[GDisplay alloc] initWithScreen:s];
    [w showWindow:self];
}


@end
