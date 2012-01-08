#import "GController.h"
#import "GDisplay.h"
#import "impl/clock.h"
#import "impl/cvar.h"
#import "impl/entry.h"
#import "impl/error.h"
#import "impl/lfile.h"
#import "impl/rand.h"

void
sg_platform_failf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    sg_platform_failv(fmt, ap);
    va_end(ap);
}

void
sg_platform_failv(const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
    abort();
}

void
sg_platform_faile(struct sg_error *err)
{
    if (err) {
        if (err->code)
            fprintf(stderr, "error: %s (%s %ld)\n",
                    err->msg, err->domain->name, err->code);
        else
            fprintf(stderr, "error: %s (%s)\n",
                    err->msg, err->domain->name);
    } else {
        fputs("error: an unknown error occurred\n", stderr);
    }
    abort();
}

void
sg_platform_quit(void)
{
    abort();
}

static GController *gController;
extern bool gEditor;

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
    
    NSDictionary *args = [[NSUserDefaults standardUserDefaults] volatileDomainForName:NSArgumentDomain];
    NSEnumerator *e = [args keyEnumerator];
    NSString *key, *value;
    while ((key = [e nextObject])) {
        value = [args objectForKey:key];
        sg_cvar_addarg(NULL, [key UTF8String], [value UTF8String]);
    }
    
    sg_path_init();
    sg_clock_init();
    sg_rand_seed(&sg_rand_global, 1);
    sg_game_init();
    GDisplay *d = [[[GDisplay alloc] init] autorelease];
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
