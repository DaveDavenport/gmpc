#include <gtk/gtk.h>
#include <glade/glade.h>
#include "eggtrayicon.h"
#include "libmpdclient.h"
#include "main.h"
EggTrayIcon *tray_icon;
GtkWidget *tray_image;
GladeXML *tray_xml;

/* this function updates the trayicon on changes */
void update_tray_icon()
    {
    if(!info.do_tray) return;
    
    
    
    }

int  tray_mouse_menu(GtkWidget *wid, GdkEventButton *event)
    {
    GtkMenu *menu = (GtkMenu *)glade_xml_get_widget(tray_xml, "tray_icon_menu");
    gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 3, event->time);
    gtk_widget_show_all(GTK_WIDGET(menu));
    g_print("test\n");
    return FALSE;    
    }

void create_tray_icon()
    {
    GdkPixbuf *pb, *temp;
    GtkWidget *event;
    /* set up tray icon */
    tray_icon = egg_tray_icon_new("Gnome Music Player Client");
    tray_image = gtk_image_new();
    event = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(tray_icon), event);
    gtk_container_add(GTK_CONTAINER(event), tray_image);
    /* set image */
    temp = gdk_pixbuf_new_from_file(PIXMAP_PATH"gmpc-tray.png",NULL);
    pb = gdk_pixbuf_scale_simple(temp, 20,20, GDK_INTERP_BILINEAR);
    g_object_unref(temp);
    gtk_image_set_from_pixbuf(GTK_IMAGE(tray_image),   pb);  
    g_object_unref(pb);

    g_signal_connect(G_OBJECT(event), "button-release-event", G_CALLBACK(tray_mouse_menu), NULL);
    /* show all */
    gtk_widget_show_all(GTK_WIDGET(tray_icon));

    tray_xml =   glade_xml_new(GLADE_PATH"gmpc.glade", "tray_icon_menu", NULL);
    glade_xml_signal_autoconnect(tray_xml);	
    }
