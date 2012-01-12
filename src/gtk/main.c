#if defined(__GNUC__) && \
    ((__GNUC__ == 4 && __GNUC_MINOR >= 6) || __GNUC__ > 4)
#define HAVE_DPUSH 1
#endif

#include "impl/clock.h"
#include "impl/cvar.h"
#include "impl/entry.h"
#include "impl/error.h"
#include "impl/event.h"
#include "impl/kbd/keycode.h"
#include "impl/kbd/keytable.h"
#include "impl/opengl.h"

/* The Gtk headers generate a warning.  */
#if defined(HAVE_DPUSH)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#if defined(HAVE_DPUSH)
#pragma GCC diagnostic pop
#endif

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static gint sg_timer;
static int sg_window_width, sg_window_height;
static int sg_update_size;

static void
quit(void)
{
    if (sg_timer > 0) {
        g_source_remove(sg_timer);
        sg_timer = 0;
    }
    gtk_main_quit();
}

void
sg_platform_quit(void)
{
    quit();
}

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
    quit();
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
    quit();
}

__attribute__((noreturn))
void
sg_platform_return(void)
{
    exit(1);
}

static gboolean
handle_destroy(GtkWidget *widget, GdkEvent *event, void *user_data)
{
    (void) widget;
    (void) event;
    (void) user_data;
    quit();
    return FALSE;
}

static gboolean
handle_expose(GtkWidget *area)
{
    struct sg_event_resize rsz;
    GtkAllocation a;
    GdkGLContext *context = gtk_widget_get_gl_context(area);
    GdkGLDrawable *drawable = gtk_widget_get_gl_drawable(area);

    if (!gdk_gl_drawable_gl_begin(drawable, context)) {
        fputs("Could not begin GL drawing\n", stderr);
        exit(1);
    }

    if (sg_update_size) {
        gtk_widget_get_allocation(area, &a);
        if (a.width != sg_window_width || a.height != sg_window_height) {
            sg_window_width = a.width;
            sg_window_height = a.height;
            rsz.type = SG_EVENT_RESIZE;
            rsz.width = a.width;
            rsz.height = a.height;
            sg_sys_event((union sg_event *) &rsz);
        }
    }
    sg_sys_draw();

    if (gdk_gl_drawable_is_double_buffered(drawable))
        gdk_gl_drawable_swap_buffers(drawable);
    else
        glFlush();
    gdk_gl_drawable_gl_end(drawable);
    return TRUE;
}

static gboolean
handle_configure(void)
{
    sg_update_size = 1;
    return TRUE;
}

static gboolean
update(void *obj)
{
    GtkWidget *area = GTK_WIDGET(obj);
    gdk_window_invalidate_rect(area->window, &area->allocation, FALSE);
    gdk_window_process_updates(area->window, FALSE);
    return TRUE;
}

static gboolean
handle_key(GdkEventKey *e, sg_event_type_t t)
{
    int code = e->hardware_keycode, hcode;
    struct sg_event_key evt;
    if (code < 0 || code > 255)
        return TRUE;
    hcode = EVDEV_NATIVE_TO_HID[code];
    if (hcode == 255)
        return TRUE;
    evt.type = t;
    evt.key = hcode;
    sg_sys_event((union sg_event *) &evt);
    return TRUE;
}

static gboolean
handle_motion(GdkEventMotion *e)
{
    struct sg_event_mouse evt;
    evt.type = SG_EVENT_MMOVE;
    evt.button = -1;
    evt.x = e->x;
    evt.y = sg_window_height - 1 - e->y;
    sg_sys_event((union sg_event *) &evt);
    return TRUE;
}

static gboolean
handle_button(GdkEventButton *e, sg_event_type_t t)
{
    struct sg_event_mouse evt;
    evt.type = t;
    switch (e->button) {
    case 1:  evt.button = SG_BUTTON_LEFT; break;
    case 2:  evt.button = SG_BUTTON_MIDDLE; break;
    case 3:  evt.button = SG_BUTTON_RIGHT; break;
    default: evt.button = SG_BUTTON_OTHER + e->button - 4; break;
    }
    evt.x = e->x;
    evt.y = sg_window_height - 1 - e->y;
    sg_sys_event((union sg_event *) &evt);
    return TRUE;
}

static gboolean
handle_event(GtkWidget *widget, GdkEvent *event, void *obj)
{
    (void) obj;
    switch (event->type) {
    case GDK_EXPOSE:
        return handle_expose(widget);
    case GDK_MOTION_NOTIFY:
        return handle_motion(&event->motion);
    case GDK_BUTTON_PRESS:
        return handle_button(&event->button, SG_EVENT_MDOWN);
    case GDK_BUTTON_RELEASE:
        return handle_button(&event->button, SG_EVENT_MUP);
    case GDK_KEY_PRESS:
        return handle_key(&event->key, SG_EVENT_KDOWN);
    case GDK_KEY_RELEASE:
        return handle_key(&event->key, SG_EVENT_KUP);
    case GDK_CONFIGURE:
        return handle_configure();
    default:
        return FALSE;
    }
}

static const int SG_GL_ATTRIB_1[] = {
    GDK_GL_RGBA,
    GDK_GL_DOUBLEBUFFER,
    GDK_GL_RED_SIZE, 8,
    GDK_GL_BLUE_SIZE, 8,
    GDK_GL_GREEN_SIZE, 8,
    GDK_GL_ALPHA_SIZE, 8,
    GDK_GL_ATTRIB_LIST_NONE
};

static const int SG_GL_ATTRIB_2[] = {
    GDK_GL_RGBA,
    GDK_GL_DOUBLEBUFFER,
    GDK_GL_RED_SIZE, 1,
    GDK_GL_BLUE_SIZE, 1,
    GDK_GL_GREEN_SIZE, 1,
    GDK_GL_ALPHA_SIZE, 1,
    GDK_GL_ATTRIB_LIST_NONE
};

static const int *const SG_GL_ATTRIB[] = {
    SG_GL_ATTRIB_1, SG_GL_ATTRIB_2, 0
};

static void
init(int argc, char *argv[])
{
    GtkWidget *window, *area;
    GdkScreen *screen;
    GdkGLConfig *config;
    GdkGeometry geom;
    int width = 640, height = 480, opt;
    unsigned mask, i;
    gboolean r;

    gtk_init(&argc, &argv);
    gtk_gl_init(&argc, &argv);

    while ((opt = getopt(argc, argv, "d:")) != -1) {
        switch (opt) {
        case 'd':
            sg_cvar_addarg(NULL, NULL, optarg);
            break;

        default:
            fputs("Invalid usage\n", stderr);
            exit(1);
        }
    }

    sg_sys_init();
    sg_sys_getsize(&width, &height);

    geom.min_width = 320;
    geom.min_height = 180;
    geom.min_aspect = 0.5;
    geom.max_aspect = 2.0;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), area);

    gtk_window_set_title(GTK_WINDOW(window), "Game");
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    gtk_window_set_geometry_hints(
        GTK_WINDOW(window), area, &geom,
        GDK_HINT_MIN_SIZE | GDK_HINT_ASPECT);

    g_signal_connect_swapped(G_OBJECT(window), "destroy",
                             G_CALLBACK(handle_destroy), NULL);

    gtk_widget_set_can_focus(area, TRUE);
    mask = GDK_EXPOSURE_MASK |
        GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
        GDK_POINTER_MOTION_MASK;
    gtk_widget_set_events(area, mask);

    gtk_widget_show(window);
    gtk_widget_grab_focus(area);
    screen = gtk_window_get_screen(GTK_WINDOW(window));

    config = NULL;
    for (i = 0; !config && SG_GL_ATTRIB[i]; ++i)
        config = gdk_gl_config_new_for_screen(screen, SG_GL_ATTRIB[i]);
    if (!config) {
        fputs("Could not initialize GdkGL\n", stderr);
        exit(1);
    }

    r = gtk_widget_set_gl_capability(
        area, config, NULL, TRUE, GDK_GL_RGBA_TYPE);
    if (!r) {
        fputs("Could not assign GL capability\n", stderr);
        exit(1);
    }

    g_signal_connect(area, "event", G_CALLBACK(handle_event), NULL);

    gtk_widget_show_all(window);

    sg_timer = g_timeout_add(10, update, area);
}

int
main(int argc, char *argv[])
{
    init(argc, argv);
    gtk_main();
    return 0;
}
