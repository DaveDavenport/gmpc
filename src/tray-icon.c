#include <config.h>
#ifdef ENABLE_TRAYICON
#include <gtk/gtk.h>
#include <string.h>
#include <glade/glade.h>
#include "playlist3.h"
#include "eggtrayicon.h"
#include "main.h"
#include "misc.h"
#include "config1.h"


#define BORDER_WIDTH 6
void destroy_tray_icon();
void tray_icon_song_change();
void tray_icon_state_change();
void tray_cover_art_fetched(mpd_Song *song);
void tray_leave_cb (GtkWidget *w, GdkEventCrossing *e, gpointer n);
void TrayStatusChanged(MpdObj *mi, ChangedStatusType what, void *userdata);
void tray_icon_pref_construct(GtkWidget *container);
void tray_icon_pref_destroy(GtkWidget *container);
/* do tray */
int create_tray_icon();
void tray_init();

GladeXML *tray_pref_xml = NULL;

int playlist_hidden = FALSE;

extern GladeXML *pl3_xml;

EggTrayIcon *tray_icon = NULL;
GladeXML *tray_xml = NULL;
GdkPixbuf *logo = NULL;
GtkTooltips *tps = NULL;
extern int pl3_hidden;
GtkWidget *tip = NULL;
PangoLayout *tray_layout_tooltip = NULL;

guint tray_timeout = -1;

guint popup_timeout = -1;

GdkPixbuf *cover_pb = NULL;


/* plugin structure */

gmpcPrefPlugin tray_gpp = {
	tray_icon_pref_construct,
	tray_icon_pref_destroy
};

gmpcPlugin tray_icon_plug = {
	"Notification",
	{1,1,1},
	GMPC_INTERNALL,
	0,
	NULL,		/* path */
	&tray_init, 	/*initialize function */
	NULL,
	&TrayStatusChanged,
	NULL,
	&tray_gpp
};


void tray_init()
{
	/* create a tray icon */
	if (cfg_get_single_value_as_int_with_default(config, "tray-icon", "enable",1))
	{
		create_tray_icon();
	}
}





/**/
gchar *tray_get_tooltip_text()
{
	GString *string = g_string_new("");

	gchar result[1024];
	gchar *retval;
	int id;
	if(mpd_check_connected(connection) && mpd_player_get_state(connection) != MPD_PLAYER_STOP)
	{
		mpd_Song *song = mpd_playlist_get_current_song(connection);
		mpd_song_markup(result, 1024, DEFAULT_TRAY_MARKUP, song);
		g_string_append(string, result);
		/* add time */
		if(mpd_status_get_total_song_time(connection) > 0)
		{
			g_string_append_printf(string, "\n<span size=\"small\">Time:\t%02i:%02i/%02i:%02i</span>",
					mpd_status_get_elapsed_song_time(connection)/60, mpd_status_get_elapsed_song_time(connection) %60,
					mpd_status_get_total_song_time(connection)/60, mpd_status_get_total_song_time(connection) %60);
		}
		else
		{
			g_string_append_printf(string, "\n<span size=\"small\">Time:\t%02i:%02i</span>",
					mpd_status_get_elapsed_song_time(connection)/60, mpd_status_get_elapsed_song_time(connection) %60);
		}
	}
	else
	{
		g_string_append(string,"Gnome Music Player Client");
	}

	/* escape all & signs... needed for pango */
	for(id=0;id < string->len; id++)
	{
		if(string->str[id] == '&')
		{
			g_string_insert(string, id+1, "amp;");
			id++;
		}
	}
	/* return a string (that needs to be free'd */
	retval = string->str;
	g_string_free(string, FALSE);
	return retval;
}

int tray_paint_tip(GtkWidget *widget, GdkEventExpose *event,gpointer n)
{
	int width=BORDER_WIDTH, height=0;
	int lwidth=0,  lheight=0;
	GtkStyle *style;
	int from_tray = GPOINTER_TO_INT(n);
	char *tooltiptext = tray_get_tooltip_text();

	if(tooltiptext == NULL)
	{
		tooltiptext = g_strdup(_("Gnome Music Player Deamon"));
	}

	pango_layout_set_markup(tray_layout_tooltip, tooltiptext, strlen(tooltiptext));
	pango_layout_set_wrap(tray_layout_tooltip, PANGO_WRAP_WORD);
	pango_layout_set_width(tray_layout_tooltip, 500000);
	style = widget->style;
	pango_layout_get_pixel_size(tray_layout_tooltip, &lwidth, &lheight);

	gtk_window_get_size(GTK_WINDOW(tip), &width, &height);
	gdk_draw_rectangle(widget->window, widget->style->white_gc, TRUE, 0,0,width, height);
	gdk_draw_rectangle(widget->window, widget->style->black_gc, FALSE, 0,0,width-1, height-1);

	if(cover_pb)
	{
		width = gdk_pixbuf_get_width(cover_pb)+BORDER_WIDTH*2;
		/* draw rectangle, width of image + 2x border */
		gdk_draw_rectangle(widget->window, widget->style->mid_gc[GTK_STATE_NORMAL], TRUE, 1,1,width-2, height-2);
		
		height = gdk_pixbuf_get_height(cover_pb)+BORDER_WIDTH;
		/* draw image outline */
		gdk_draw_rectangle(widget->window, widget->style->black_gc, FALSE, 
				BORDER_WIDTH-1,BORDER_WIDTH-1,
				width-2*BORDER_WIDTH+1,height-BORDER_WIDTH+1);
		/* add a right border to the image */
		width+=BORDER_WIDTH;
		/* draw image */
		gdk_draw_pixbuf(widget->window, NULL, cover_pb, 
				0,0,
				BORDER_WIDTH,BORDER_WIDTH,
				-1,-1,
				GDK_RGB_DITHER_NONE,0,0);	
	}
	else{
		GdkPixbuf *pb = gtk_widget_render_icon(widget, "gmpc", GTK_ICON_SIZE_DND,NULL);
		width = gdk_pixbuf_get_width(pb)+BORDER_WIDTH*2;
		/* draw rectangle, width of image + 2x border */
		gdk_draw_rectangle(widget->window, widget->style->mid_gc[GTK_STATE_NORMAL], TRUE, 1,1,width-2, height-2);
		
		height = gdk_pixbuf_get_height(pb)+BORDER_WIDTH;
		/* add a right border to the image */
		width+=BORDER_WIDTH;
		/* draw image */
		gdk_draw_pixbuf(widget->window, NULL, pb, 
				0,0,
				BORDER_WIDTH,BORDER_WIDTH,
				-1,-1,
				GDK_RGB_DITHER_NONE,0,0);	
		if(pb) g_object_unref(pb);
	}

	gtk_paint_layout (style, widget->window, GTK_STATE_NORMAL, TRUE,
			NULL, widget, "tooltip", width, BORDER_WIDTH, tray_layout_tooltip);


	width  += lwidth;

	if(mpd_status_get_total_song_time(connection)> 0)
	{
		int width2 = 0;
		if((lheight+8+BORDER_WIDTH) >=(height))
		{
			height = lheight+BORDER_WIDTH+8;
		}

		gdk_draw_rectangle(widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
				FALSE,width-lwidth,lheight+BORDER_WIDTH, lwidth ,8);                              		
		width2 = (mpd_status_get_elapsed_song_time(connection)/(float)mpd_status_get_total_song_time(connection))*lwidth;
		gdk_draw_rectangle(widget->window,
				widget->style->mid_gc[GTK_STATE_NORMAL],
				TRUE,width-lwidth,lheight+BORDER_WIDTH, width2 ,8);
		gdk_draw_rectangle(widget->window, 
				widget->style->fg_gc[GTK_STATE_NORMAL],
				FALSE,width-lwidth,lheight+BORDER_WIDTH, width2 ,8);
	}
	else {
		if(lheight > (height))
		{
			height = lheight+BORDER_WIDTH;
		}
	}

	if(widget->allocation.width != width+BORDER_WIDTH || widget->allocation.height != height + BORDER_WIDTH)
	{
		int x_tv,y_tv;
		int x=0,y=0;
		GdkRectangle msize;
		GtkWidget *tv = (GtkWidget *)tray_icon;
		GdkScreen *screen;
		int monitor = 0;
		if(tv != NULL)
		{
			screen = gtk_widget_get_screen(tv);
			monitor = gdk_screen_get_monitor_at_window(screen, tv->window);       			
		}
		else
		{
			screen = gdk_screen_get_default();
		}

		gdk_screen_get_monitor_geometry(screen, monitor, &msize);

		y+=cfg_get_single_value_as_int_with_default(config, "tray-icon","y-offset",0);
		x+=cfg_get_single_value_as_int_with_default(config, "tray-icon","x-offset",0);						
		/* calculate position */
		switch((from_tray)? 0:cfg_get_single_value_as_int_with_default(config, "tray-icon", "popup-location", 0))
		{
			case 0: /* tooltip */
				if(tv)
				{
					gdk_window_get_origin(tv->window, &x_tv, &y_tv);
					x = (int)/*event->x_root*/x_tv + tv->allocation.width/2 - (width)/2;
					y = (int)/*event->y_root*/y_tv+(tv->allocation.height) +5;	

					/* check borders left, right*/	
					if((x+width+BORDER_WIDTH) > msize.width+msize.x)
					{	
						x = msize.x+msize.width-(width+BORDER_WIDTH);
					}
					else if(x < 0)
					{
						x= 0;
					}
					/* check up down.. if can't place it below, place it above */
					if( y+height+BORDER_WIDTH > msize.height+msize.y) 
					{
						y = y_tv -5-(height+BORDER_WIDTH);
					}
					/* place the window */
					gtk_window_move(GTK_WINDOW(tip), x, y);
					break;
				}
			case 1: /* upper left */
				gtk_window_move(GTK_WINDOW(tip), x,y);
				break;
			case 2: /* upper right */
				gtk_window_move(GTK_WINDOW(tip),msize.width-width-BORDER_WIDTH-x, y);	
				break;
			case 3: /* lower left */
				gtk_window_move(GTK_WINDOW(tip), x, msize.height-height-BORDER_WIDTH-y);	
				break;
			case 4: /* lower right */
				gtk_window_move(GTK_WINDOW(tip),msize.width-width-BORDER_WIDTH-x, msize.height-height-BORDER_WIDTH-y);	
				break;                                                  				

		}


		if(width+BORDER_WIDTH > 0 && height + BORDER_WIDTH > 0)
		{		
			gtk_widget_set_usize(tip, width+BORDER_WIDTH, height+BORDER_WIDTH);
		}
	}

	g_free(tooltiptext);

	return TRUE;
}



/* fix it the ugly way */
int tooltip_queue_draw(GtkWidget *widget)
{
	gtk_widget_queue_draw(widget);
	return TRUE;
}

int popup_press_event(GtkWidget *wid, GdkEventKey *event)
{
	tray_leave_cb(wid,NULL,NULL);


	return FALSE;
}

/*
 *
 */


gboolean tray_motion_cb (GtkWidget *event, GdkEventCrossing *event1, gpointer n)
{
//	GtkWidget *tv = (GtkWidget *)tray_icon;
//	GdkRectangle msize;
//	int x=0,y=0;
//	int x_tv,y_tv;
//	int monitor =0;
	int from_tray = GPOINTER_TO_INT(n);
	char *tooltiptext = NULL;
	GtkWidget *eventb;
//	GdkScreen *screen;
/*	if(tv != NULL)
	{
		screen = gtk_widget_get_screen(tv);
		monitor = gdk_screen_get_monitor_at_window(screen, tv->window);
	}
	else
	{
		screen = gdk_screen_get_default();
	}
*/
	if(tip != NULL)
	{
		if(from_tray)
		{
			tray_leave_cb(NULL, NULL, 0);
		}
		else
		{
			return FALSE;
		}
	}
	tooltiptext = tray_get_tooltip_text();
/*	gdk_screen_get_monitor_geometry(screen, monitor, &msize);
*/
	tip = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_title(GTK_WINDOW(tip), "gmpc tray tooltip");


	eventb = gtk_event_box_new();
	g_signal_connect(G_OBJECT(tip), "button-press-event",
			G_CALLBACK(popup_press_event), NULL);

	gtk_container_add(GTK_CONTAINER(tip), eventb);
	gtk_widget_set_app_paintable(eventb, TRUE);

	gtk_window_set_resizable(GTK_WINDOW(tip), FALSE);
	gtk_widget_set_name(eventb, "gtk-tooltips");
	g_signal_connect(G_OBJECT(eventb), "expose-event",
			G_CALLBACK(tray_paint_tip), n);


	gtk_widget_ensure_style (eventb);

	tray_layout_tooltip = gtk_widget_create_pango_layout (eventb, NULL);
	/* set wrapping */
	pango_layout_set_wrap(tray_layout_tooltip, PANGO_WRAP_WORD);
	pango_layout_set_width(tray_layout_tooltip, 100000);

	/* calculate position */
/*
	gdk_window_get_origin(tv->window, &x_tv, &y_tv);
	y+=cfg_get_single_value_as_int_with_default(config, "tray-icon","y-offset",0);
	x+=cfg_get_single_value_as_int_with_default(config, "tray-icon","x-offset",0);			
*/

	/* calculate position */
	/* in first expose the widget will be located */
	/*
	switch((from_tray)? 0:cfg_get_single_value_as_int_with_default(config, "tray-icon", "popup-location", 0))
	{
		case 0:
			if(tv){
				gdk_window_get_origin(tv->window, &x_tv, &y_tv);
				x = (int)x_tv + tv->allocation.width/2 - (width)/2;
				y = (int)y_tv+(tv->allocation.height) +5;	

				if((x+width+8) > msize.width+msize.x)
				{	
					x = msize.x+msize.width-(width+8);
				}
				else if(x < 0)
				{
					x= 0;
				}
				if( y+height+8 > msize.height+msize.y) 
				{
					y = y_tv -5-(height+8);
				}
				break;
			}
		case 1:
			break;
		case 2:
			x= msize.width-width-x;
			y=y;
			break;
		case 3:
			x = x;
			y = msize.height-height-y;
			break;
		case 4:
			x= msize.width-width-x;
			y = msize.height-height-y;
			break;                                                  				
	}
	gtk_window_move(GTK_WINDOW(tip),x,y);
	*/
	gtk_widget_show_all(tip);	


	if(tray_timeout != -1) g_source_remove(tray_timeout);
	tray_timeout = g_timeout_add(400, (GSourceFunc)
			tooltip_queue_draw, eventb);


	g_free(tooltiptext);
	return TRUE;
}

void tray_leave_cb (GtkWidget *w, GdkEventCrossing *e, gpointer n)
{
	if(tray_timeout != -1) g_source_remove(tray_timeout);
	if(popup_timeout != -1) g_source_remove(popup_timeout);
	popup_timeout = -1;
	tray_timeout = -1;

	if(tip != NULL)
	{
		gtk_widget_destroy(tip);
		g_object_unref(tray_layout_tooltip);
	}
	/*	
		if(dest != NULL)
		{
		g_object_unref(dest);
		dest= NULL;
		}
		*/
	tip = NULL;
}


/* this draws the actual image to the window */
/* gtk will call this function when the image is exposed and the data is gone */
void exposed_signal(GtkWidget *event)
{
	int state = mpd_player_get_state(connection);
	gdk_draw_rectangle(event->window, event->style->bg_gc[GTK_STATE_NORMAL], TRUE, 0,0,20,20);

	gdk_draw_pixbuf(event->window,event->style->bg_gc[GTK_STATE_NORMAL],
			logo, 0,0,0,0,20,20, GDK_RGB_DITHER_MAX,0,0);

	if(state < 0)
	{
		return;
	}

	if(player_get_hidden())
	{
		GdkPoint points[6] = {{19,13},{19,19}, {19,16},{16,16},{16,13},{16,19}};
		gdk_draw_lines(event->window, event->style->fg_gc[GTK_STATE_NORMAL], points, 6);
	}

	if(state == MPD_PLAYER_STOP)
	{
		GdkPoint points[5] = {{4,1},{4,8}, {11,8},{11,1},{4,1}};
		gdk_draw_polygon(event->window, event->style->fg_gc[GTK_STATE_NORMAL],TRUE, points, 5);
	}
	else if(state == MPD_PLAYER_PLAY)
	{
		GdkPoint points[4] = {{5,1},{5,11}, {10,6},{5,1}};
		gdk_draw_polygon(event->window, event->style->fg_gc[GTK_STATE_NORMAL],TRUE, points, 4);
	}
	else if(state == MPD_PLAYER_PAUSE)
	{
		GdkPoint points[5] = {{4,1},{4,8}, {6,8},{6,1},{4,1}};
		GdkPoint points2[5] = {{8,1},{8,8}, {10,8},{10,1},{8,1}};	
		gdk_draw_polygon(event->window, event->style->fg_gc[GTK_STATE_NORMAL],TRUE, points, 5);
		gdk_draw_polygon(event->window, event->style->fg_gc[GTK_STATE_NORMAL],TRUE, points2, 5);
	}
	else return;

}

/* this function updates the trayicon on changes */
void tray_icon_song_change()
{
	if(cfg_get_single_value_as_int_with_default(config, "tray-icon", "do-popup", 1) &&
			mpd_player_get_state(connection) != MPD_PLAYER_STOP)
	{
		if(popup_timeout == -1 && tip == NULL&& cfg_get_single_value_as_int_with_default(config, "tray-icon","popup-timeout",5))
		{
			popup_timeout = g_timeout_add(cfg_get_single_value_as_int_with_default(config, "tray-icon","popup-timeout",5)*1000,
					(GSourceFunc)(tray_leave_cb),
					NULL);
		}
		if(tip == NULL)
		{
			tray_motion_cb((GtkWidget*)tray_icon,NULL,GINT_TO_POINTER(0));
		}
		else if(popup_timeout != -1 && cfg_get_single_value_as_int_with_default(config, "tray-icon","popup-timeout",5))
		{
			g_source_remove(popup_timeout);
			popup_timeout = g_timeout_add(cfg_get_single_value_as_int_with_default(config, "tray-icon","popup-timeout",5)*1000, 
					(GSourceFunc)(tray_leave_cb),			
					NULL);
		}
	}
	/* always do this, so the cover art also shows when you do a mouse over */
	if(cover_pb){
		g_object_unref(cover_pb);
		cover_pb = NULL;
	}
	if(cover_pb == NULL){
		gchar *path= NULL;
		int ret = 0; 
		ret = cover_art_fetch_image_path(mpd_playlist_get_current_song(connection), &path);
		if(ret == COVER_ART_OK_LOCAL)
		{
			cover_pb = gdk_pixbuf_new_from_file_at_size(path, 80,80, NULL);
		}
		else if (ret == COVER_ART_NOT_FETCHED)
		{

			cover_art_fetch_image(mpd_playlist_get_current_song(connection),
					(CoverArtCallback)tray_cover_art_fetched,NULL);
		}
		if(path)g_free(path);
	}
}

void tray_icon_state_change()
{
	int state = mpd_player_get_state(connection);
	if(state == MPD_PLAYER_STOP || state == MPD_PLAYER_UNKNOWN)
	{
		if(cover_pb)
		{
			g_object_unref(cover_pb);
			cover_pb = NULL;
		}
	}
	else if(state == MPD_PLAYER_PLAY){
		tray_icon_song_change();
	}

	if(tray_icon)
	{
		gtk_widget_queue_draw(GTK_WIDGET(tray_icon));
	}

}

/* if the item was destroyed and the user still wants an icon recreate it */
/* if the main window was hidden and the icon was destroyed, popup the main window because otherwise you wouldnt be able to 
 * get it back
 */

void tray_icon_destroyed()
{
	tray_icon = NULL;
	if(cfg_get_single_value_as_int_with_default(config, "tray-icon", "enable", 1))
	{
		g_idle_add((GSourceFunc)create_tray_icon, NULL);
	}
	if(player_get_hidden())
	{
		player_show();
		if(playlist_hidden)
		{
			create_playlist3();
		}
	}
}

/* destroy the tray icon */
void destroy_tray_icon()
{
	gtk_widget_destroy(GTK_WIDGET(tray_icon));
}

/* wrong name: this handles clickes on the tray icon
 * button1: present/hide window
 * button2: play/pause
 * button3: menu
 */

void tray_icon_info()
{
	mpd_Song *song = mpd_playlist_get_current_song(connection);
	if(song)
	{
		call_id3_window_song(mpd_songDup(song));
	}
}

int  tray_mouse_menu(GtkWidget *wid, GdkEventButton *event)
{
	if(event->button == 1 && event->state != (GDK_CONTROL_MASK|GDK_BUTTON1_MASK))
	{
		if(player_get_hidden())
		{
			player_show();
			gtk_widget_queue_draw(GTK_WIDGET(tray_icon));
			if(cfg_get_single_value_as_int_with_default(config,"playlist","hide-with-player",1))
			{
				if(playlist_hidden)
				{
					create_playlist3();
					playlist_hidden = FALSE;
				}
			}
		}
		else
		{
			player_hide();
			gtk_widget_queue_draw(GTK_WIDGET(tray_icon));
			if(cfg_get_single_value_as_int_with_default(config,"playlist","hide-with-player",1))
			{
				if(pl3_xml != NULL && !pl3_hidden)
				{
					pl3_close();
					playlist_hidden = TRUE;
				}
			}

		}
	}
	else if (event->button == 2 || (event->button == 1 && event->state == (GDK_CONTROL_MASK|GDK_BUTTON1_MASK)))
	{
		gchar *string = cfg_get_single_value_as_string_with_default(config, "tray-icon","middle-mouse-action","pause");
		if(!strcmp(string,"pause") || !strcmp(string,"play")){
			play_song();
		}
		else if (!strcmp(string,"next")) {
			next_song();
		}
		else if (!strcmp(string,"prev")) {
			prev_song();
		}
		else if (!strcmp(string,"stop")) {
			stop_song();
		}
	}
	else if(event->button == 3)
	{
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();


		item = gtk_image_menu_item_new_with_mnemonic(_("Pl_ay/Pause"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock("gtk-media-play", GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(play_song), NULL);


		item = gtk_image_menu_item_new_with_mnemonic(_("_Stop"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock("gtk-media-stop", GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(stop_song), NULL);

		item = gtk_image_menu_item_new_with_mnemonic(_("_Next"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock("gtk-media-next", GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(next_song), NULL);


		item = gtk_image_menu_item_new_with_mnemonic(_("_Previous"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock("gtk-media-previous", GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(prev_song), NULL);
		item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);

		item = gtk_image_menu_item_new_with_mnemonic(_("Pla_ylist"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock("gtk-justify-fill", GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(create_playlist3), NULL);

		if(mpd_player_get_state(connection) == MPD_PLAYER_PLAY ||
				mpd_player_get_state(connection) == MPD_PLAYER_PAUSE)
		{
			item = gtk_image_menu_item_new_with_mnemonic(_("Song _Information"));
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
					gtk_image_new_from_stock("gtk-info", GTK_ICON_SIZE_MENU));
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(tray_icon_info), NULL);

		}
		item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);

		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(main_quit), NULL);
		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, event->time);
	}
	return FALSE;
}

int scroll_event(GtkWidget *eventb, GdkEventScroll *event)
{
	if(event->type == GDK_SCROLL)
	{
		if(event->direction == GDK_SCROLL_UP)
		{
			mpd_status_set_volume(connection,mpd_status_get_volume(connection)+5);
		}
		else if (event->direction == GDK_SCROLL_DOWN)
		{
			mpd_status_set_volume(connection,mpd_status_get_volume(connection)-5);
		}
		else if(event->direction == GDK_SCROLL_LEFT)
		{
			mpd_player_prev(connection);
		}
		else if (event->direction == GDK_SCROLL_RIGHT)
		{
			mpd_player_next(connection);
		}
	}

	return FALSE;
}


int create_tray_icon()
{
	GdkPixbuf  *temp;
	GtkWidget *event;
	gchar *path;
	GtkWidget *ali;
	if(tray_icon != NULL)
	{
		return FALSE;
	}
	/* set up tray icon */
	tray_icon = egg_tray_icon_new(_("Gnome Music Player Client"));
	event = gtk_event_box_new();
	gtk_widget_set_usize(event, 20,20);
	gtk_widget_set_app_paintable(event, TRUE);
	ali = gtk_alignment_new(0.5,0.5,0,0);
	gtk_container_add(GTK_CONTAINER(ali), event);
	gtk_container_add(GTK_CONTAINER(tray_icon), ali);
	gtk_widget_show_all(GTK_WIDGET(tray_icon));
	/* set image */
	path = gmpc_get_full_image_path("gmpc-tray.png");
	temp = gdk_pixbuf_new_from_file(path,NULL);
	g_free(path);
	logo = gdk_pixbuf_scale_simple(temp, 20,20, GDK_INTERP_BILINEAR);
	if(temp) g_object_unref(temp);

	g_signal_connect(G_OBJECT(event), "expose-event", G_CALLBACK(exposed_signal), NULL);
	g_signal_connect(G_OBJECT(event), "button-release-event", G_CALLBACK(tray_mouse_menu), NULL);
	g_signal_connect(G_OBJECT(tray_icon), "destroy", G_CALLBACK(tray_icon_destroyed), NULL);

	g_signal_connect(G_OBJECT(event), "enter-notify-event", 
			G_CALLBACK(tray_motion_cb), GINT_TO_POINTER(1));
	g_signal_connect(G_OBJECT(event), "leave-notify-event",
			G_CALLBACK(tray_leave_cb), NULL);
	g_signal_connect(G_OBJECT(event), "scroll-event", G_CALLBACK(scroll_event), NULL);
	/* show all */
	gtk_widget_show_all(GTK_WIDGET(tray_icon));
	if(tps == NULL)	tps = gtk_tooltips_new();

	return FALSE;
}
void tray_cover_art_fetched(mpd_Song *song)
{
	mpd_Song *current = mpd_playlist_get_current_song(connection);
	if(current->artist && current->album)
	{
		if(!strcmp(current->artist,song->artist) &&
				!strcmp(current->album, song->album))
		{
			if(tip && !cover_pb)
			{
				gchar *path= NULL;
				int ret = 0; 
				ret = cover_art_fetch_image_path(mpd_playlist_get_current_song(connection), &path);
				if(ret == COVER_ART_OK_LOCAL)
				{
					cover_pb = gdk_pixbuf_new_from_file_at_size(path, 80,80, NULL);
				}
				if(path)g_free(path);

			}
		}
	}
}

void   TrayStatusChanged(MpdObj *mi, ChangedStatusType what, void *userdata)
{
	if(what&MPD_CST_STATE)
	{
		tray_icon_state_change();
	}
	else if(what&MPD_CST_SONGID)
	{
		tray_icon_song_change();
	}
}
/* PREFERENCES */


void tray_enable_toggled(GtkToggleButton *but)
{
	debug_printf(DEBUG_INFO,"tray-icon.c: changing tray icon %i\n", gtk_toggle_button_get_active(but));
	cfg_set_single_value_as_int(config, "tray-icon", "enable", (int)gtk_toggle_button_get_active(but));
	if(cfg_get_single_value_as_int_with_default(config, "tray-icon", "enable", 1))
	{
		create_tray_icon();
	}
	else
	{
		destroy_tray_icon();
	}
}

void hide_player_toggled(GtkToggleButton *but)
{
	cfg_set_single_value_as_int(config, "player", "hide-startup", (int)gtk_toggle_button_get_active(but));
}

/* this sets all the settings in the notification area preferences correct */
void tray_update_settings()
{
	gtk_toggle_button_set_active((GtkToggleButton *)
			glade_xml_get_widget(tray_pref_xml, "ck_tray_enable"), 
			cfg_get_single_value_as_int_with_default(config, "tray-icon", "enable", DEFAULT_TRAY_ICON_ENABLE));
	gtk_toggle_button_set_active((GtkToggleButton *)
			glade_xml_get_widget(tray_pref_xml, "ck_hide_player"), 
			cfg_get_single_value_as_int_with_default(config, "player", "hide-startup", DEFAULT_HIDE_ON_STARTUP));

}

void popup_enable_toggled(GtkToggleButton *but)
{
	cfg_set_single_value_as_int(config, "tray-icon", "do-popup", gtk_toggle_button_get_active(but));
}


void popup_position_changed(GtkComboBox *om)
{
	cfg_set_single_value_as_int(config, "tray-icon", "popup-location", gtk_combo_box_get_active(om));
}

void popup_timeout_changed()
{
	cfg_set_single_value_as_int(config, "tray-icon", "popup-timeout",
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(tray_pref_xml, "popup_timeout"))));
}

void update_popup_settings()
{
	gtk_toggle_button_set_active((GtkToggleButton *)
			glade_xml_get_widget(tray_pref_xml, "ck_popup_enable"),
			cfg_get_single_value_as_int_with_default(config, "tray-icon", "do-popup", 1));
	gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(tray_pref_xml, "om_popup_position")),
			cfg_get_single_value_as_int_with_default(config, "tray-icon", "popup-location", 0));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(tray_pref_xml, "popup_timeout")),
			cfg_get_single_value_as_int_with_default(config, "tray-icon", "popup-timeout", 5));
}

void tray_icon_pref_destroy(GtkWidget *container)
{
	if(tray_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(tray_pref_xml, "tray-pref-vbox");
		gtk_container_remove(GTK_CONTAINER(container),vbox);
		g_object_unref(tray_pref_xml);
		tray_pref_xml = NULL;
	}
}
void tray_icon_pref_construct(GtkWidget *container)
{
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
	tray_pref_xml = glade_xml_new(path, "tray-pref-vbox",NULL);

	if(tray_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(tray_pref_xml, "tray-pref-vbox");
		gtk_container_add(GTK_CONTAINER(container),vbox);
		tray_update_settings();
		update_popup_settings();
		glade_xml_signal_autoconnect(tray_pref_xml);
	}
}
#endif
