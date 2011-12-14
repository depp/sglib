#include "rand.hpp"
#include "ui/menu.hpp"
#include "ui/event.hpp"
#include "ui/window.hpp"
#include "ui/keyboard/keyid.h"
#include "ui/keyboard/keytable.h"
#include "sys/resource.hpp"
#include "sys/config.hpp"
#include "sys/path.hpp"
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <stdlib.h>

class GTKWindow : public UI::Window {
public:
    virtual void close();
};

void GTKWindow::close()
{ }

static const unsigned int MAX_FPS = 100;
static const unsigned int MIN_FRAMETIME = 1000 / MAX_FPS;

static gboolean expose(GtkWidget *area, GdkEventConfigure *event,
                       gpointer user_data)
{
    GdkGLContext *context = gtk_widget_get_gl_context(area);
    GdkGLDrawable *drawable = gtk_widget_get_gl_drawable(area);
    if (!gdk_gl_drawable_gl_begin(drawable, context)) {
        fputs("Could not begin GL drawing\n", stderr);
        exit(1);
    }

    GTKWindow *w = reinterpret_cast<GTKWindow *>(user_data);
    w->draw();

    if (gdk_gl_drawable_is_double_buffered(drawable))
        gdk_gl_drawable_swap_buffers(drawable);
    else
        glFlush();
    gdk_gl_drawable_gl_end(drawable);
    return TRUE;
}

static gboolean update(gpointer user_data)
{
    GtkWidget *area = GTK_WIDGET(user_data);
    gdk_window_invalidate_rect(area->window, &area->allocation, FALSE);
    gdk_window_process_updates(area->window, FALSE);
    return TRUE;
}

static gboolean key(GtkWidget *widget, GdkEventKey *event,
                    gpointer user_data)
{
    int code = event->hardware_keycode, hcode;
    if (code < 0 || code > 255)
        return true;
    hcode = EVDEV_NATIVE_TO_HID[code];
    printf("Key %d %s\n", code, keyid_name_from_code(hcode));
    return false;
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
        GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK;
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

    /*
    g_signal_connect(area, "configure-event",
                     G_CALLBACK(configure), NULL);
    */
    g_signal_connect(area, "expose-event",
                     G_CALLBACK(expose), &w);
    g_signal_connect(area, "key-press-event", G_CALLBACK(key), &w);
    g_signal_connect(area, "key-release-event", G_CALLBACK(key), &w);

    gtk_widget_show_all(window);

    g_timeout_add(10, update, area);

    gtk_main();

    return 0;
}
