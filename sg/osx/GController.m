/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#import "GController.h"
#import "GDisplay.h"
#import "sg/audio_system.h"
#import "sg/entry.h"
#import "sg/cvar.h"
#import "sg/error.h"
#import "sg/version.h"

static GController *gController;

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
    // Don't [NSApp terminate:nil] here because that could deadlock
    // The display thread could be waiting for the lock from our thread
    // and then we'd wait for the display thread to terminate...
    [NSApp performSelectorOnMainThread:@selector(terminate:) withObject:nil waitUntilDone:NO];
}

void
sg_version_platform(struct sg_logger *sp)
{
    (void) sp;
}

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
    struct sg_game_info gameinfo;

    (void)notification;

    NSDictionary *args = [[NSUserDefaults standardUserDefaults] volatileDomainForName:NSArgumentDomain];
    NSEnumerator *e = [args keyEnumerator];
    NSString *key, *value;
    while ((key = [e nextObject])) {
        value = [args objectForKey:key];
        sg_cvar_addarg(NULL, [key UTF8String], [value UTF8String]);
    }
    sg_sys_init();
    sg_audio_sys_pstart();
    sg_sys_getinfo(&gameinfo);
    GDisplay *d = [[[GDisplay alloc] init] autorelease];
    [d setDefaultSize:NSMakeSize(gameinfo.default_width, gameinfo.default_height)];
    [d setMinSize:NSMakeSize(gameinfo.min_width, gameinfo.min_height)];
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
