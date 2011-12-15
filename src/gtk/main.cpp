#include "sys/rand.hpp"
#include "client/ui/menu.hpp"
#include "client/ui/event.hpp"
#include "client/ui/window.hpp"
#include "client/keyboard/keycode.h"
#include "client/keyboard/keyid.h"
#include "client/keyboard/keytable.h"
#include "sys/resource.hpp"
#include "sys/config.hpp"
#include "sys/path.hpp"
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <stdlib.h>

class GTKWindow : public UI::Window {
public:
    GTKWindow()
        : updateSize(false)
    { }

    virtual void close();

    bool updateSize;
};

void GTKWindow::close()
{
    gtk_main_quit();
}

static const unsigned int MAX_FPS = 100;
static const unsigned int MIN_FRAMETIME = 1000 / MAX_FPS;

static gboolean handle_expose(GtkWidget *area, GTKWindow *w)
{
    GdkGLContext *context = gtk_widget_get_gl_context(area);
    GdkGLDrawable *drawable = gtk_widget_get_gl_drawable(area);
    if (!gdk_gl_drawable_gl_begin(drawable, context)) {
        fputs("Could not begin GL drawing\n", stderr);
        exit(1);
    }

    if (w->updateSize) {
        GtkAllocation a;
        gtk_widget_get_allocation(area, &a);
        glViewport(0, 0, a.width, a.height);
        w->setSize(a.width, a.height);
    }
    w->draw();

    if (gdk_gl_drawable_is_double_buffered(drawable))
        gdk_gl_drawable_swap_buffers(drawable);
    else
        glFlush();
    gdk_gl_drawable_gl_end(drawable);
    return TRUE;
}

static gboolean handle_configure(GTKWindow *w)
{
    w->updateSize = true;
    return TRUE;
}

static gboolean update(gpointer user_data)
{
    GtkWidget *area = GTK_WIDGET(user_data);
    gdk_window_invalidate_rect(area->window, &area->allocation, FALSE);
    gdk_window_process_updates(area->window, FALSE);
    return TRUE;
}

static int mapKey(int key)
{
    switch (key) {
    case KEY_Escape:
        return UI::KEscape;

    case KEY_Space:
    case KEY_Enter:
        return UI::KSelect;

    case KEY_Left:
        return UI::KLeft;

    case KEY_Right:
        return UI::KRight;

    case KEY_Down:
        return UI::KDown;

    case KEY_Up:
        return UI::KUp;

    default:
        return -1;
    }
}

static gboolean handle_key(GTKWindow *w, GdkEventKey *e, UI::EventType t)
{
    int code = e->hardware_keycode, hcode, gcode;
    if (code < 0 || code > 255)
        return true;
    hcode = EVDEV_NATIVE_TO_HID[code];
    gcode = mapKey(hcode);
    if (gcode < 0)
        return true;
    UI::KeyEvent uevt(t, gcode);
    w->handleEvent(uevt);
    return TRUE;
}

static gboolean handle_motion(GTKWindow *w, GdkEventMotion *e)
{
    int x = e->x, y = w->height() - 1 - e->y;
    UI::MouseEvent uevt(UI::MouseMove, -1, x, y);
    w->handleEvent(uevt);
    return TRUE;
}

static gboolean handle_button(GTKWindow *w, GdkEventButton *e,
                              UI::EventType t)
{
    int x = e->x, y = w->height() - 1 - e->y, button;
    switch (e->button) {
    case 1: button = UI::ButtonLeft; break;
    case 2: button = UI::ButtonMiddle; break;
    case 3: button = UI::ButtonRight; break;
    default: button = UI::ButtonOther + e->button - 4; break;
    }
    UI::MouseEvent uevt(t, button, x, y);
    w->handleEvent(uevt);
    return TRUE;
}

static gboolean handle_event(GtkWidget *widget, GdkEvent *event,
                             gpointer user_data)
{
    GTKWindow *w = reinterpret_cast<GTKWindow *>(user_data);
    switch (event->type) {
    case GDK_EXPOSE:
        return handle_expose(widget, w);
    case GDK_MOTION_NOTIFY:
        return handle_motion(w, &event->motion);
    case GDK_BUTTON_PRESS:
        return handle_button(w, &event->button, UI::MouseDown);
    case GDK_BUTTON_RELEASE:
        return handle_button(w, &event->button, UI::MouseUp);
    case GDK_KEY_PRESS:
        return handle_key(w, &event->key, UI::KeyDown);
    case GDK_KEY_RELEASE:
        return handle_key(w, &event->key, UI::KeyUp);
    case GDK_CONFIGURE:
        return handle_configure(w);
    default:
        return FALSE;
    }
}

int main(int argc, char *argv[])
{
    gboolean r;
    GTKWindow w;

    gtk_init(&argc, &argv);
    gtk_gl_init(&argc, &argv);
    Path::init();
    w.setSize(768, 480);
    Rand::global.seed();
    w.setScreen(new UI::Menu);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Game");
    gtk_window_set_default_size(GTK_WINDOW(window), 768, 480);
    g_signal_connect_swapped(G_OBJECT(window), "destroy",
                             G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), area);
    gtk_widget_set_can_focus(area, TRUE);
    unsigned mask = GDK_EXPOSURE_MASK |
        GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
        GDK_POINTER_MOTION_MASK;
    gtk_widget_set_events(area, mask);

    gtk_widget_show(window);
    gtk_widget_grab_focus(area);
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(window));

    static const int ATTRIB_1[] = {
        GDK_GL_RGBA,
        GDK_GL_DOUBLEBUFFER,
        GDK_GL_RED_SIZE, 8,
        GDK_GL_BLUE_SIZE, 8,
        GDK_GL_GREEN_SIZE, 8,
        GDK_GL_ALPHA_SIZE, 8,
        GDK_GL_ATTRIB_LIST_NONE
    };
    static const int ATTRIB_2[] = {
        GDK_GL_RGBA,
        GDK_GL_DOUBLEBUFFER,
        GDK_GL_RED_SIZE, 1,
        GDK_GL_BLUE_SIZE, 1,
        GDK_GL_GREEN_SIZE, 1,
        GDK_GL_ALPHA_SIZE, 1,
        GDK_GL_ATTRIB_LIST_NONE
    };
    const int *attrib[] = { ATTRIB_1, ATTRIB_2, 0 };
    GdkGLConfig *config = 0;
    for (unsigned int i = 0; !config && attrib[i]; ++i)
        config = gdk_gl_config_new_for_screen(screen, attrib[i]);
    if (!config) {
        fputs("Could not initialize GdkGL\n", stderr);
        return 1;
    }
    r = gtk_widget_set_gl_capability(
        area, config, NULL, TRUE, GDK_GL_RGBA_TYPE);
    if (!r) {
        fputs("Could not assign GL capability\n", stderr);
        return 1;
    }

    g_signal_connect(area, "event", G_CALLBACK(handle_event), &w);

    gtk_widget_show_all(window);

    g_timeout_add(10, update, area);

    gtk_main();

    return 0;
}
