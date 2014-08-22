/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#if defined(__GNUC__) && \
    ((__GNUC__ == 4 && __GNUC_MINOR >= 6) || __GNUC__ > 4)
#define HAVE_DPUSH 1
#endif

#include "sg/keytable.h"
#include "sg/clock.h"
#include "sg/cvar.h"
#include "sg/entry.h"
#include "sg/event.h"
#include "sg/opengl.h"
#include "sg/version.h"
#include "../private.h"

/* The Gtk headers generate a warning.  */
#if defined(HAVE_DPUSH)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkkeysyms.h>
#if defined(HAVE_DPUSH)
#pragma GCC diagnostic pop
#endif

#include <signal.h>
#include <stdlib.h>

struct sg_gtk {
    /* Timer identifier for the redraw timer */
    gint timer;

    /* Window size */
    int width, height;

    /* The game has been sent the current video state.  */
    int init_sent;

    /* Information about the viewport size is current.  */
    int size_current;

    /* The window status (SG_WINDOW_FULLSCREEN, etc.) */
    unsigned window_status;

    /* The main window.  */
    GtkWindow *window;

    /* The drawing area.  */
    GtkWidget *area;
};

static struct sg_gtk sg_gtk;

void
sg_sys_quit(void)
{
    if (sg_gtk.timer > 0) {
        g_source_remove(sg_gtk.timer);
        sg_gtk.timer = 0;
    }
    gtk_main_quit();
}

void
sg_sys_abort(const char *msg)
{
    fputs("Critical error, exiting.\n", stderr);
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

/* Toggle whether the window is full-screen.  */
static void
sg_gtk_toggle_fullscreen(void)
{
    if (sg_gtk.window_status & SG_WINDOW_VISIBLE) {
        if (sg_gtk.window_status & SG_WINDOW_FULLSCREEN)
            gtk_window_unfullscreen(sg_gtk.window);
        else
            gtk_window_fullscreen(sg_gtk.window);
    }
}

/* Handle window destroyed event.  This is called after the main
   window is already closed.  */
static gboolean
sg_gtk_handle_destroy(GtkWidget *widget, GdkEvent *event, void *user_data)
{
    (void) widget;
    (void) event;
    (void) user_data;
    sg_sys_quit();
    return FALSE;
}

/* Handle a window exposed event, and redraw the window.  */
static gboolean
sg_gtk_handle_expose(GtkWidget *area)
{
    GLenum err;
    union sg_event event;
    GdkGLContext *context = gtk_widget_get_gl_context(area);
    GdkGLDrawable *drawable = gtk_widget_get_gl_drawable(area);

    if (!gdk_gl_drawable_gl_begin(drawable, context)) {
        fputs("Could not begin GL drawing\n", stderr);
        exit(1);
    }

    if (!sg_gtk.init_sent) {
        err = glewInit();
        if (err)
            sg_sys_abort("Could not initialize GLEW.");
        event.type = SG_EVENT_VIDEO_INIT;
        sg_game_event(&event);
        sg_gtk.init_sent = 1;
    }

    sg_game_draw(sg_gtk.width, sg_gtk.height, sg_clock_get());

    if (gdk_gl_drawable_is_double_buffered(drawable))
        gdk_gl_drawable_swap_buffers(drawable);
    else
        glFlush();
    gdk_gl_drawable_gl_end(drawable);
    return TRUE;
}

/* Handle a window configured event (window resize).  */
static gboolean
sg_gtk_handle_configure(void)
{
    GtkAllocation allocation;
    gtk_widget_get_allocation(sg_gtk.area, &allocation);
    sg_gtk.width = allocation.width;
    sg_gtk.height = allocation.height;
    return TRUE;
}

/* Handle the timer firing, and update the window.  */
static gboolean
sg_gtk_update(void *obj)
{
    GtkWidget *area = GTK_WIDGET(obj);
    gdk_window_invalidate_rect(area->window, &area->allocation, FALSE);
    gdk_window_process_updates(area->window, FALSE);
    return TRUE;
}

/* Handle a keyboard event.  */
static gboolean
sg_gtk_handle_key(GdkEventKey *e, sg_event_type_t t)
{
    int code = e->hardware_keycode, hcode;
    union sg_event evt;
    if (e->keyval == GDK_F11) {
        if (e->type == GDK_KEY_PRESS)
            sg_gtk_toggle_fullscreen();
        return TRUE;
    }
    if (code < 0 || code > 255)
        return TRUE;
    hcode = EVDEV_NATIVE_TO_HID[code];
    if (hcode == 255)
        return TRUE;
    evt.key.type = t;
    evt.key.key = hcode;
    sg_game_event(&evt);
    return TRUE;
}

/* Handle a mouse motion event.  */
static gboolean
sg_gtk_handle_motion(GdkEventMotion *e)
{
    union sg_event evt;
    evt.mouse.type = SG_EVENT_MMOVE;
    evt.mouse.button = -1;
    evt.mouse.x = e->x;
    evt.mouse.y = sg_gtk.height - 1 - e->y;
    sg_game_event(&evt);
    return TRUE;
}

/* Handle a mouse button event.  */
static gboolean
sg_gtk_handle_button(GdkEventButton *e, sg_event_type_t t)
{
    union sg_event evt;
    int button;
    switch (e->button) {
    case 1:  button = SG_BUTTON_LEFT; break;
    case 2:  button = SG_BUTTON_MIDDLE; break;
    case 3:  button = SG_BUTTON_RIGHT; break;
    default: button = SG_BUTTON_OTHER + e->button - 4; break;
    }
    evt.mouse.type = t;
    evt.mouse.button = button;
    evt.mouse.x = e->x;
    evt.mouse.y = sg_gtk.height - 1 - e->y;
    sg_game_event(&evt);
    return TRUE;
}

#if 0

struct wstate {
    GdkWindowState flag;
    char name[12];
};

static void
sg_gtk_print_wstate_flags(GdkWindowState st, GdkWindowState ch)
{
    static const struct wstate FLAGS[] = {
        { GDK_WINDOW_STATE_WITHDRAWN,  "withdrawn" },
        { GDK_WINDOW_STATE_ICONIFIED,  "iconified" },
        { GDK_WINDOW_STATE_MAXIMIZED,  "maximized" },
        { GDK_WINDOW_STATE_STICKY,     "sticky" },
        { GDK_WINDOW_STATE_FULLSCREEN, "fullscreen" },
        { GDK_WINDOW_STATE_ABOVE,      "above" },
        { GDK_WINDOW_STATE_BELOW,      "below" },
        { 0, "" }
    };
    int i;
    fputs("Window state:", stderr);
    for (i = 0; FLAGS[i].flag; ++i) {
        if ((FLAGS[i].flag & ch) == 0)
            continue;
        fputc(' ', stderr);
        fputc(FLAGS[i].flag & st ? '+' : '-', stderr);
        fputs(FLAGS[i].name, stderr);
    }
    fputc('\n', stderr);
}

#else
#define sg_gtk_print_wstate_flags(x,y) (void)0
#endif

static gboolean
sg_gtk_handle_window_state(GtkWidget *widget, GdkEvent *event,
                           gpointer user_data)
{
    /* The states are: WITHDRAWN, ICONIFIED, MAXIMIZED, STICKY,
       FULLSCREEN, ABOVE, BELOW */
    GdkWindowState st = event->window_state.new_window_state;
    int fullscreen, shown;
    unsigned status;
    union sg_event evt;
    (void) user_data;
    (void) widget;
    sg_gtk_print_wstate_flags(st, event->window_state.changed_mask);
    fullscreen = (st & GDK_WINDOW_STATE_FULLSCREEN) != 0;
    shown = !(st & (GDK_WINDOW_STATE_ICONIFIED | GDK_WINDOW_STATE_WITHDRAWN));
    if (!shown) {
        status = 0;
    } else {
        status = SG_WINDOW_VISIBLE;
        if (fullscreen)
            status |= SG_WINDOW_FULLSCREEN;
    }
    if (status != sg_gtk.window_status) {
        evt.status.type = SG_EVENT_WINDOW;
        evt.status.status = status;
        sg_game_event(&evt);
        sg_gtk.window_status = status;
    }
    return TRUE;
}

static gboolean
sg_gtk_handle_event(GtkWidget *widget, GdkEvent *event, void *obj)
{
    (void) obj;
    switch (event->type) {
    case GDK_EXPOSE:
        return sg_gtk_handle_expose(widget);
    case GDK_MOTION_NOTIFY:
        return sg_gtk_handle_motion(&event->motion);
    case GDK_BUTTON_PRESS:
        return sg_gtk_handle_button(&event->button, SG_EVENT_MDOWN);
    case GDK_BUTTON_RELEASE:
        return sg_gtk_handle_button(&event->button, SG_EVENT_MUP);
    case GDK_KEY_PRESS:
        return sg_gtk_handle_key(&event->key, SG_EVENT_KDOWN);
    case GDK_KEY_RELEASE:
        return sg_gtk_handle_key(&event->key, SG_EVENT_KUP);
    case GDK_CONFIGURE:
        return sg_gtk_handle_configure();
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
sg_gtk_init(int argc, char *argv[])
{
    GtkWidget *window, *area;
    GdkScreen *screen;
    GdkGLConfig *config;
    GdkGeometry geom;
    int opt;
    unsigned mask, i;
    gboolean r;
    struct sg_game_info gameinfo;

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
    sg_sys_getinfo(&gameinfo);
    if (gameinfo.min_width < 128)
        gameinfo.min_width = 128;
    if (gameinfo.min_height < 128)
        gameinfo.min_height = 128;

    geom.min_width  = gameinfo.min_width;
    geom.min_height = gameinfo.min_height;
    geom.min_aspect = gameinfo.min_aspect;
    geom.max_aspect = gameinfo.max_aspect;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), area);

    gtk_window_set_title(GTK_WINDOW(window), "Game");
    gtk_window_set_default_size(
        GTK_WINDOW(window), gameinfo.default_width, gameinfo.default_height);
    gtk_window_set_geometry_hints(
        GTK_WINDOW(window), area, &geom,
        GDK_HINT_MIN_SIZE | GDK_HINT_ASPECT);

    g_signal_connect_swapped(G_OBJECT(window), "destroy",
                             G_CALLBACK(sg_gtk_handle_destroy), NULL);

    gtk_widget_set_can_focus(area, TRUE);
    mask = GDK_EXPOSURE_MASK |
        GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
        GDK_POINTER_MOTION_MASK |
        GDK_STRUCTURE_MASK;
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

    g_signal_connect(area, "event", G_CALLBACK(sg_gtk_handle_event), NULL);
    g_signal_connect(window, "window-state-event",
                     G_CALLBACK(sg_gtk_handle_window_state), NULL);

    gtk_widget_show_all(window);

    sg_gtk.timer = g_timeout_add(10, sg_gtk_update, area);

    sg_gtk.window = GTK_WINDOW(window);
    sg_gtk.area = area;
}

int
main(int argc, char *argv[])
{
    signal(SIGPIPE, SIG_IGN);
    sg_gtk_init(argc, argv);
    gtk_main();
    return 0;
}

void
sg_version_platform(struct sg_logger *lp)
{
    char cv[16], rv[16];
    snprintf(cv, sizeof(cv), "%d.%d.%d",
             GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
    snprintf(rv, sizeof(rv), "%d.%d.%d",
             gtk_major_version, gtk_minor_version, gtk_micro_version);
    sg_version_lib(lp, "GTK+", cv, rv);
}
