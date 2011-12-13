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
    GDisplay *d = [[[GDisplay alloc] initWithScreen:s] autorelease];
    [d showWindow:self];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication {
    (void)theApplication;
    return !displays_ || ![displays_ count];
}

- (void)addDisplay:(GDisplay *)display {
    if (!displays_)
        displays_ = [[NSMutableArray alloc] initWithObjects:&display count:1];
    else
        [displays_ addObject:display];
}

- (void)removeDisplay:(GDisplay *)display {
    if (displays_) {
        [displays_ removeObjectIdenticalTo:display];
        if (![displays_ count] && ![[NSApp windows] count])
            [NSApp terminate:self];
    }
}

@end
