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
/* we don't want to load the image for the popup everytime we display it.. so I keep the pixbuf open */
GdkPixbuf *gmpc_image = NULL;
gint pixbuf_width   = 0, pixbuf_height = 0;
guint timeout       = 0;

gchar * get_string()
{
	GString *string;
	mpd_Song *song;
	int i;
	/* because we don't want to pass the GString we need to get a pointer to the gstrings string. */
	/* because we do the free on the string before the return */
	char *retval = NULL;
	GList *node = g_list_nth(info.playlist, info.status->song);
	/* check if there actually a song to display */
	if(node == NULL)
	{
		return g_strdup("No Song found\n");
	}
	/* create an empty string */
	string = g_string_new("");
	/* get the mpd_Song struct. that is where the info is stored */
	song = node->data;
	/* if there is no artist name or title name we use the filename */
	if(song->artist  == NULL || song->title == NULL)
	{
		gchar *basename     = g_path_get_basename(song->file);
		g_string_printf(string,"%s",basename);
		g_free(basename);
	}
	else
	{
		g_string_printf(string, "%s\n%s", song->title, song->artist);

		if(song->album != NULL)
		{
			g_string_append_printf(string,"\n%s", song->album);
		}
	}
	/* catch & signs and convert them so */

	for(i= 0;i < string->len;i++)
	{
		if(string->str[i] == '&')
		{
			g_string_insert(string, i+1, "amp;");
		}
	}


	retval  = string->str;
	g_string_free(string, FALSE);
	return retval;
}
/* this destroys the popup.. and set the timeout handler to 0 */
int destroy_popup(GtkWidget *window)
{
	gtk_widget_destroy(window);
	popup               = NULL;
	timeout             = 0;
	return FALSE;
}
/* if the image is clicked we (for now) want to remove it */
int popup_clicked(GtkWidget *window)
{
	g_source_remove(timeout);
	destroy_popup(popup);
	return FALSE;
}
/* this does the actual painting on the window */
int paint_window(GtkWidget *popup)
{
	PangoLayout *layout = NULL;
	GtkStyle *style;
	int w, h, text_height;
	char *text          = get_string();
	style               = popup->style;
	layout  	    = gtk_widget_create_pango_layout(popup, NULL);

	gtk_paint_box(style, popup->window, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
			NULL, popup, "tooltip", 0,0,-1,-1);
	/* draw the background + the border */
	pango_layout_set_markup(layout,text, strlen(text));
	/* draw the image */
	gdk_draw_pixbuf(popup->window, NULL/*style->fg_gc[GTK_STATE_NORMAL]*/,
			gmpc_image,
			0,0,
			0,0,		
			pixbuf_width, pixbuf_height,
			GDK_RGB_DITHER_NONE,0,0);

	/* draw the text */
	pango_layout_get_size (layout, &w, &h);

	h = PANGO_PIXELS(h);;

	text_height = MAX(((pixbuf_height-h)/2) ,4);
	gtk_paint_layout (style, popup->window, GTK_STATE_NORMAL, TRUE,
			NULL, popup, "tooltip", 4+pixbuf_width, text_height,layout);
	/* make sure we all see the result */
	gtk_widget_show_all(popup);
	g_free(text);
	return TRUE;
}

/* this calculates the size for the popup and creates the window for it */
/* this function doesnt do drawing.. that we do when the popup actually appears */
void popup_window()
{
	gint w, h;
	PangoLayout *layout = NULL;
	char *text          = get_string();
	if(popup != NULL)
	{
		g_source_remove(timeout);
		destroy_popup(popup);
	}
	/* we need to do this once.. we keep it stored for later use */
	if(gmpc_image == NULL)
	{
		GdkPixbuf *temp;
		temp = gdk_pixbuf_new_from_file(PIXMAP_PATH"gmpc-tray.png", NULL);
		if(temp == NULL)
		{
			g_error("Failed to open gmpc_tray.png. Did you install it correctly?");
		}
		gmpc_image    = gdk_pixbuf_scale_simple(temp, 64,64,GDK_INTERP_HYPER);
		/* we don't need the original anymore */	
		g_object_unref(temp);
		/* we wont want this to be freeed anywhere */
		g_object_ref(gmpc_image);
		/* no need to get this all the time from the pixbuf */

		pixbuf_width        = gdk_pixbuf_get_width(gmpc_image);
		pixbuf_height       = gdk_pixbuf_get_height(gmpc_image);
	}


	popup  = gtk_window_new(GTK_WINDOW_POPUP);
	
	gtk_widget_set_app_paintable(popup, TRUE);
	gtk_window_set_resizable(GTK_WINDOW(popup), FALSE);
	gtk_widget_set_name(popup, "gtk-tooltips");

	gtk_widget_ensure_style(popup);    

	g_signal_connect(G_OBJECT(popup), "expose-event",
			G_CALLBACK(paint_window), NULL);       

	g_signal_connect(G_OBJECT(popup), "button-press-event",
			G_CALLBACK(popup_clicked), NULL);       


	layout = gtk_widget_create_pango_layout(popup, NULL);
	pango_layout_set_markup(layout,text, strlen(text));

	pango_layout_get_size (layout, &w, &h);
	w = PANGO_PIXELS(w) + 12 + pixbuf_width;
	h = MAX(PANGO_PIXELS(h) + 8, pixbuf_height);

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
