#include <gtk/gtk.h>
#include <string.h>
#include <glade/glade.h>
#include "eggtrayicon.h"
#include "libmpdclient.h"
#include "main.h"
#include "misc.h"

EggTrayIcon *tray_icon = NULL;
GladeXML *tray_xml = NULL;
GdkPixbuf *logo = NULL;
GtkTooltips *tps = NULL;
void exposed_signal(GtkWidget *event)
	{
//	GdkPixbuf *state = NULL;
	gdk_draw_rectangle(event->window, event->style->bg_gc[GTK_STATE_NORMAL], TRUE, 0,0,20,20);
	
	gdk_draw_pixbuf(event->window,event->style->bg_gc[GTK_STATE_NORMAL],logo, 0,0,0,0,20,20, GDK_RGB_DITHER_MAX,0,0);		

	if(info.status == NULL)
	{
		return;
	}

	if(info.hidden == TRUE)
	{
		GdkPoint points[6] = {{19,13},{19,19}, {19,16},{16,16},{16,13},{16,19}};
		gdk_draw_lines(event->window, event->style->fg_gc[GTK_STATE_NORMAL], points, 6);
	}

	if(info.state == MPD_STATUS_STATE_STOP)
		{
		GdkPoint points[5] = {{4,1},{4,8}, {11,8},{11,1},{4,1}};
		gdk_draw_polygon(event->window, event->style->fg_gc[GTK_STATE_NORMAL],TRUE, points, 5);

			
//		state =  gdk_pixbuf_new_from_file(PIXMAP_PATH"media-stop.png", NULL);
		}
	else if(info.state == MPD_STATUS_STATE_PLAY)
	{
		GdkPoint points[4] = {{5,1},{5,11}, {10,6},{5,1}};
		gdk_draw_polygon(event->window, event->style->fg_gc[GTK_STATE_NORMAL],TRUE, points, 4);
//		state =  gdk_pixbuf_new_from_file(PIXMAP_PATH"media-play.png", NULL);
	}
	else if(info.state == MPD_STATUS_STATE_PAUSE)
	{
		GdkPoint points[5] = {{4,1},{4,8}, {6,8},{6,1},{4,1}};
		GdkPoint points2[5] = {{8,1},{8,8}, {10,8},{10,1},{8,1}};	
		gdk_draw_polygon(event->window, event->style->fg_gc[GTK_STATE_NORMAL],TRUE, points, 5);
		gdk_draw_polygon(event->window, event->style->fg_gc[GTK_STATE_NORMAL],TRUE, points2, 5);
		
//	state =  gdk_pixbuf_new_from_file(PIXMAP_PATH"media-pause.png", NULL);
	}
	else return;
//	gdk_pixbuf_add_alpha(state, TRUE, 228,228,228);		
//	gdk_draw_pixbuf(event->window, event->style->bg_gc[GTK_STATE_NORMAL],state, 0,0,0,0,20,20, GDK_RGB_DITHER_MAX,0,0);	
//	g_object_unref(state);

	}

/* this function updates the trayicon on changes */
void update_tray_icon()
{
	if(!info.do_tray) return;
	if(info.status != NULL) 
	{
		if(info.song != info.status->song)
		{
			gchar *str = NULL;
			if(info.status->state == MPD_STATUS_STATE_PLAY || info.status->state == MPD_STATUS_STATE_PAUSE)
			{
				GList *node = g_list_nth(info.playlist, info.status->song);
				if(node != NULL)
				{
					mpd_Song *song = node->data;
					if(song->title != NULL && song->artist != NULL)
					{
						str = g_strdup_printf("%s - %s", song->artist, song->title);
					}
					else
					{
						str = remove_extention_and_basepath(song->file);
					}
				}

			}
			else str = g_strdup("Gnome Music Player Client");
			gtk_tooltips_set_tip(tps, GTK_WIDGET(tray_icon), str, "");
			g_free(str);
		}
		if(info.state != info.status->state)
		{
			gtk_widget_queue_draw(GTK_WIDGET(tray_icon));
		}
		
		
	}
}

void tray_icon_destroyed()
{
	tray_icon = NULL;
	if(info.do_tray)
	{
		g_idle_add((GSourceFunc)create_tray_icon, NULL);
	}
	if(info.hidden)
	{
		gtk_window_present(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")));
		info.hidden = FALSE;
	}
}

void destroy_tray_icon()
{
	gtk_widget_destroy(GTK_WIDGET(tray_icon));
}


int  tray_mouse_menu(GtkWidget *wid, GdkEventButton *event)
{
	if(event->button == 1)
	{
		if(info.hidden)
		{
		gtk_window_present(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")));
		info.hidden = FALSE;
		gtk_widget_queue_draw(GTK_WIDGET(tray_icon));
		}
		else
		{
		gtk_widget_hide(GTK_WIDGET(glade_xml_get_widget(xml_main_window, "main_window")));
		info.hidden = TRUE;
		gtk_widget_queue_draw(GTK_WIDGET(tray_icon));
		}
	}
	else if(event->button == 3)
	{
		GtkMenu *menu = (GtkMenu *)glade_xml_get_widget(tray_xml, "tray_icon_menu");
		gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 3, event->time);
		gtk_widget_show_all(GTK_WIDGET(menu));
	}
	return FALSE;    
}

int create_tray_icon()
{
	GdkPixbuf  *temp;
	GtkWidget *event;
	if(tray_icon != NULL)
	{
		return FALSE;
	}
	/* set up tray icon */
	tray_icon = egg_tray_icon_new("Gnome Music Player Client");
	//	tray_image = gtk_image_new();
	event = gtk_event_box_new();
	gtk_widget_set_usize(event, 20,20);
	gtk_widget_set_app_paintable(event, TRUE);
	gtk_container_add(GTK_CONTAINER(tray_icon), event);
	//	gtk_container_add(GTK_CONTAINER(event), tray_image);
	gtk_widget_show_all(GTK_WIDGET(tray_icon));
	/* set image */
	temp = gdk_pixbuf_new_from_file(PIXMAP_PATH"gmpc-tray.png",NULL);
	logo = gdk_pixbuf_scale_simple(temp, 20,20, GDK_INTERP_BILINEAR);
	g_object_unref(temp);

	g_signal_connect(G_OBJECT(event), "expose-event", G_CALLBACK(exposed_signal), NULL);
	g_signal_connect(G_OBJECT(event), "button-release-event", G_CALLBACK(tray_mouse_menu), NULL);
	g_signal_connect(G_OBJECT(tray_icon), "destroy", G_CALLBACK(tray_icon_destroyed), NULL);
	/* show all */
	gtk_widget_show_all(GTK_WIDGET(tray_icon));
	if(tps == NULL)	tps = gtk_tooltips_new();

	/* we only need to load this one once */
	if(tray_xml == NULL)
	{
		tray_xml =   glade_xml_new(GLADE_PATH"gmpc.glade", "tray_icon_menu", NULL);
		glade_xml_signal_autoconnect(tray_xml);
	}
	return FALSE;
}
