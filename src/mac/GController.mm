#import "GController.h"
#import "GDisplay.h"
#import "rand.hpp"
#import "sys/path.hpp"
#import "sys/clock.hpp"
#import "ui/menu.hpp"

static GController *gController;

@implementation GController

+ (GController *)sharedInstance {
    if (!gController)
        gController = [[self alloc] init];
    return gController;
}

- (id)init {
    if (!(self = [super init]))
        return nil;
    if (gController) {
        [self release];
        self = gController;
    } else {
        gController = self;
    }
    return [self retain];
}

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
