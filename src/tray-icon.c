#include <gtk/gtk.h>
#include <string.h>
#include <glade/glade.h>
#include "eggtrayicon.h"
#include "libmpdclient.h"
#include "main.h"
EggTrayIcon *tray_icon;
GtkWidget *tray_image;
GladeXML *tray_xml;

GtkWidget *popup = NULL;
guint timeout = 0;

gchar * get_string()
    {
    char *string;
    mpd_Song *song;
    GList *node = g_list_nth(info.playlist, info.status->song);
    if(node == NULL) return g_strdup("No Song found\n");
    
    song = node->data;
    if(song->artist == NULL || song->title == NULL)
	{
	gchar *basename = g_path_get_basename(song->file);
	string = g_strdup_printf("<b>Title:</b>\t%s",basename);
	g_free(basename);
	}
    else{
    if(song->album == NULL)
    {
    string = g_strdup_printf("<b>Title:</b>\t%s\n<b>Artist:</b>\t%s", song->title, song->artist);
    }
    else
    {
    string = g_strdup_printf("<b>Title:</b>\t%s\n<b>Artist:</b>\t%s\n<b>Album:</b>\t%s", song->title, song->artist, song->album);
    }
    }
    
    return string;
    }

int destroy_popup(GtkWidget *window)
    {
    gtk_widget_destroy(window);
    popup = NULL;
    timeout = 0;
    return FALSE;
    }
    
int paint_window(GtkWidget *popup)
    {
    PangoLayout *layout = NULL;
    GtkStyle *style;
    char *text = get_string();
    style = popup->style;
    g_print("test\n");

    layout = gtk_widget_create_pango_layout(popup, NULL);
    pango_layout_set_markup(layout,text, strlen(text));    
    
    gtk_paint_box(style, popup->window, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		    NULL, popup, "tooltip", 0,0,-1,-1);
    gtk_paint_layout (style, popup->window, GTK_STATE_NORMAL, TRUE,
			NULL, popup, "tooltip", 4, 4, layout);
    gtk_widget_show_all(popup);
    g_free(text);
    return TRUE;
    }


void popup_window()
    {
    gint w, h;
    PangoLayout *layout = NULL;
    char *text = get_string();
    g_print("test1\n");
    if(popup != NULL)
	{
	g_source_remove(timeout);
	destroy_popup(popup);
	}
	
    popup  = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_widget_set_app_paintable(popup, TRUE);
    gtk_window_set_resizable(GTK_WINDOW(popup), FALSE);
    gtk_widget_set_name(popup, "gtk-tooltips");
    
    gtk_widget_ensure_style(popup);    
    
    g_signal_connect(G_OBJECT(popup), "expose-event",
			G_CALLBACK(paint_window), NULL);       
			
    layout = gtk_widget_create_pango_layout(popup, NULL);
    pango_layout_set_markup(layout,text, strlen(text));
    
    pango_layout_get_size (layout, &w, &h);
    w = PANGO_PIXELS(w) + 8;
    h = PANGO_PIXELS(h) + 8;

    gtk_widget_set_usize(popup, w, h);
    g_free(text);			
    gtk_widget_show(popup);
    timeout = gtk_timeout_add(5000, (GSourceFunc)destroy_popup, popup);
    }


/* this function updates the trayicon on changes */
void update_tray_icon()
    {
    if(info.connection != NULL && info.status != NULL)
    {
	if(info.status->song != info.song)
	{
	g_print("popup window\n");
	    popup_window();
    
	}
    }
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
