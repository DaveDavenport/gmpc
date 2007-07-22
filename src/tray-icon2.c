#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libmpd/libmpd.h>
#include "main.h"
#include "plugin.h"
#include "gmpc-clicklabel.h"
/* name of config field */
#define TRAY_ICON2_ID "tray-icon2"
extern GladeXML *pl3_xml;

GtkStatusIcon *tray_icon2_gsi = NULL;
/**
 * Tooltip 
 */
GtkWidget	*tray_icon2_tooltip = NULL;
GtkWidget 	*tray_icon2_tooltip_pb = NULL;
guint		tray_icon2_tooltip_timeout = 0;
enum{
	TI2_AT_TOOLTIP,
	TI2_AT_UPPER_LEFT,
	TI2_AT_UPPER_RIGHT,
	TI2_AT_LOWER_LEFT,
	TI2_AT_LOWER_RIGHT,
	TI2_AT_NUM_OPTIONS
};
/**
 * Preferences
 */
static GladeXML *tray_icon2_preferences_xml = NULL;
void popup_timeout_changed(void);
void popup_position_changed(GtkComboBox *om);
void popup_enable_toggled(GtkToggleButton *but);
void tray_enable_toggled(GtkToggleButton *but);

/**
 * Tray icon
 */

gboolean tray_icon2_get_available(void)
{
	if(tray_icon2_gsi) {
		if(gtk_status_icon_is_embedded(tray_icon2_gsi) &&
				gtk_status_icon_get_visible(tray_icon2_gsi)) {
			return TRUE;
		}
	}
	return FALSE;
}
/** 
 * click on the tray icon
 */
static void tray_icon2_activate(GtkStatusIcon *gsi, gpointer user_data)
{
	pl3_toggle_hidden();
}
/**
 * Right mouse press on tray icon
 */
static void tray_icon2_populate_menu(GtkStatusIcon *gsi,guint button, guint activate_time, gpointer user_data)
{
	GtkWidget *item;
	GtkWidget *menu = gtk_menu_new();

	if(mpd_check_connected(connection))
	{
		if(mpd_server_check_command_allowed(connection, "play"))
		{
			if(mpd_player_get_state(connection) == MPD_STATUS_STATE_PLAY) 
			{
				item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PAUSE, NULL);
			}else{
				item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY, NULL);
			}
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(play_song), NULL);


			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_STOP, NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(stop_song), NULL);

			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_NEXT, NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(next_song), NULL);


			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PREVIOUS, NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(prev_song), NULL);
			item = gtk_separator_menu_item_new();
			gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);
		}
	}
	else
	{
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CONNECT, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(connect_to_mpd), NULL);
		item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);			
	}
	item = gtk_image_menu_item_new_with_mnemonic(_("Pla_ylist"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
			gtk_image_new_from_stock("gtk-justify-fill", GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(create_playlist3), NULL);


	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT,NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(main_quit), NULL);
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, activate_time);
}

static void tray_icon2_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata)
{
	if(tray_icon2_gsi == NULL)
		return;
	if(what&MPD_CST_STATE)
	{
		int state = mpd_player_get_state(connection);
		if(state == MPD_PLAYER_STOP || state == MPD_PLAYER_UNKNOWN)
		{
			gtk_status_icon_set_from_icon_name(tray_icon2_gsi, "gmpc-tray");
		}
		else if(state == MPD_PLAYER_PLAY){
			gtk_status_icon_set_from_icon_name(tray_icon2_gsi, "gmpc-tray-play");
		}
		else if(state == MPD_PLAYER_PAUSE){
			gtk_status_icon_set_from_icon_name(tray_icon2_gsi, "gmpc-tray-pause");
		}
	}
	if(what&MPD_CST_SONGID)
	{
		/** 
		 * If enabled by user, show the tooltip.
		 * But only if playing or paused.
		 *
		 */
		if(cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "show-tooltip", TRUE))
		{
			int state = mpd_player_get_state(connection);
			if(state == MPD_PLAYER_PLAY || state == MPD_PLAYER_PAUSE)
			{
				tray_icon2_create_tooltip();
			}
		}
	}
	/* update the progress bar if available */
	if(what&MPD_CST_ELAPSED_TIME)
	{
		if(tray_icon2_tooltip && tray_icon2_tooltip_pb)
		{
			int totalTime = mpd_status_get_total_song_time(connection);                                 		
			int elapsedTime = mpd_status_get_elapsed_song_time(connection);	
			gdouble progress = elapsedTime/(gdouble)MAX(totalTime,1);
			gchar*label = g_strdup_printf("%02i:%02i/%02i:%02i", elapsedTime/60, elapsedTime%60,
					totalTime/60,totalTime%60);
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(tray_icon2_tooltip_pb), RANGE(0,1,progress));
			gtk_progress_bar_set_text(GTK_PROGRESS_BAR(tray_icon2_tooltip_pb), label);
			q_free(label);
		}
	}
}
/**
 * Show right icon when (dis)connected
 */
static void tray_icon2_connection_changed(MpdObj *mi, int connect,void *user_data)
{
	if(tray_icon2_gsi == NULL)
		return;
	if(connect)	{
		tray_icon2_status_changed(mi, MPD_CST_STATE,NULL);
	} else {
		gtk_status_icon_set_from_icon_name(tray_icon2_gsi, "gmpc-tray-disconnected");
	}
}
/**
 * Initialize the tray icon 
 */
static void tray_icon2_init(void)
{
	if(cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "enable", TRUE))
	{
		tray_icon2_gsi = gtk_status_icon_new_from_icon_name ("gmpc-tray-disconnected");
		/* connect the (sparse) signals */
		g_signal_connect(G_OBJECT(tray_icon2_gsi), "popup-menu", G_CALLBACK(tray_icon2_populate_menu), NULL);
		g_signal_connect(G_OBJECT(tray_icon2_gsi), "activate", G_CALLBACK(tray_icon2_activate), NULL);
	}
}
/**
 * Destroy and cleanup 
 */
static void tray_icon2_destroy(void)
{
	if(tray_icon2_gsi)
	{
		g_object_unref(tray_icon2_gsi);
	}
}
/**
 * Get enabled
 */
static gboolean tray_icon2_get_enabled(void)
{
	return cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "enable", TRUE);
}
/**
 * Set Disabled
 */
static void tray_icon2_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, TRAY_ICON2_ID, "enable", enabled);
	if(enabled)	{
		if(!tray_icon2_gsi) {
			tray_icon2_init();
		} else {
			gtk_status_icon_set_visible(GTK_STATUS_ICON(tray_icon2_gsi), TRUE);
		}
	} else {
		if(tray_icon2_gsi) {
			gtk_status_icon_set_visible(GTK_STATUS_ICON(tray_icon2_gsi), FALSE);
		}		
	}
}
/**
 * TOOLTIP 
 */
static void tray_icon2_tooltip_song(void)
{
	mpd_Song *song = mpd_playlist_get_current_song(connection);
	if(song)
	{
		info2_activate();
		info2_fill_song_view(song->file);	
	}
}
static void tray_icon2_tooltip_artist(void)
{
	mpd_Song *song = mpd_playlist_get_current_song(connection);
	if(song && song->artist)
	{
		info2_activate();
		info2_fill_artist_view(song->artist);
	}
}
static void tray_icon2_tooltip_album(void)
{
	mpd_Song *song = mpd_playlist_get_current_song(connection);
	if(song && song->artist && song->album)
	{
		info2_activate();
		info2_fill_album_view(song->artist,song->album);
	}
}


static gboolean tray_icon2_tooltip_destroy(void)
{
	tray_icon2_tooltip_pb = NULL;
	gtk_widget_destroy(tray_icon2_tooltip);
	tray_icon2_tooltip = NULL;
	/* remove timeout, this is for when it doesn't get called from inside the timeout */
	if(tray_icon2_tooltip_timeout)
	{
		g_source_remove(tray_icon2_tooltip_timeout);
	}
	tray_icon2_tooltip_timeout = 0;
	/* remove the timeout */
	return FALSE;	
}
void tray_icon2_create_tooltip(void)
{
	int x=0,y=0,monitor;
	GdkScreen *screen;
	GtkWidget *pl3_win = glade_xml_get_widget(pl3_xml, "pl3_win");
	GtkWidget *hbox = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *label = NULL;
	GtkWidget *event = NULL;
	GtkWidget *coverimg = NULL;
	mpd_Song *song = NULL;
	int state;
	/**
	 * if the tooltip still exists destroy it... 
	 */
	if(tray_icon2_tooltip)
	{
		tray_icon2_tooltip_destroy();
	}
	/*
	 * 	Creat the tootlip window 
	 */
	tray_icon2_tooltip = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_modify_bg(GTK_WIDGET(tray_icon2_tooltip), GTK_STATE_NORMAL, &(pl3_win->style->black));
	gtk_container_set_border_width(GTK_CONTAINER(tray_icon2_tooltip),1);
	gtk_window_set_default_size(GTK_WINDOW(tray_icon2_tooltip), 300,-1);
	/*
	 * Tooltip exists from 2 parts..
	 * ------------------
	 * |  1 |           |
	 * |	|	2		|
	 * ------------------
	 */
	hbox = gtk_hbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(tray_icon2_tooltip), hbox);	

	/**
	 *  1
	 *
	 * In first box we have image/coverart
	 * Re-use the gmpc-metaimage widget
	 */

	coverimg = gmpc_metaimage_new(META_ALBUM_ART);

	gmpc_metaimage_set_connection(GMPC_METAIMAGE(coverimg), connection);
	gmpc_metaimage_set_size(GMPC_METAIMAGE(coverimg), 80);
	gmpc_metaimage_set_no_cover_icon(GMPC_METAIMAGE(coverimg),"gmpc"); 
	/**
	 * Force an update if mpd is playing
	 */
	state = mpd_player_get_state(connection);
	if(state == MPD_PLAYER_PLAY || state == MPD_PLAYER_PAUSE) {
		gmpc_metaimage_update_cover(GMPC_METAIMAGE(coverimg), connection, MPD_CST_SONGID,NULL);
	} else  {
		gmpc_metaimage_set_cover_na(GMPC_METAIMAGE(coverimg));
	}
	/**
	 * Pack the widget in a eventbox so we can set background color 
	 */
	event = gtk_event_box_new();
	g_signal_connect(G_OBJECT(event), "button-press-event", G_CALLBACK(tray_icon2_tooltip_destroy), NULL);
	gtk_widget_set_size_request(event, 86,86);
	gtk_widget_modify_bg(GTK_WIDGET(event), GTK_STATE_NORMAL, &(pl3_win->style->bg[GTK_STATE_SELECTED]));
	gtk_container_add(GTK_CONTAINER(event), coverimg);
	gtk_box_pack_start(GTK_BOX(hbox), event, FALSE,TRUE,0);
	/**
	 * 2
	 * 
	 * 	Right (2) view
	 */
	/**
	 * Create white background label box
	 */
	event = gtk_event_box_new();
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_modify_bg(GTK_WIDGET(event), GTK_STATE_NORMAL, &(pl3_win->style->white));
	gtk_container_set_border_width(GTK_CONTAINER(vbox),3);
	gtk_container_add(GTK_CONTAINER(event), vbox);
	gtk_box_pack_start(GTK_BOX(hbox), event, TRUE,TRUE,0);
	/**
	 * If there is a song, show show song info
	 */
	song = mpd_playlist_get_current_song(connection);
	if(song)
	{
		/** Artist label */
		if(song->title || song->file || song->name)
		{
			char buffer[256];
			mpd_song_markup(buffer, 256,"[%name%][%title%|%shortfile%]",song);
			label = gmpc_clicklabel_new(buffer);
			g_signal_connect(G_OBJECT(label), "button-press-event", G_CALLBACK(tray_icon2_tooltip_song), NULL);
			gmpc_clicklabel_set_do_bold(GMPC_CLICKLABEL(label),FALSE);
			gmpc_clicklabel_font_size(GMPC_CLICKLABEL(label),3);
			gtk_box_pack_start(GTK_BOX(vbox), label, FALSE,FALSE,0);
		}
		if(song->artist)
		{
			label = gmpc_clicklabel_new(song->artist);
			g_signal_connect(G_OBJECT(label), "button-press-event", G_CALLBACK(tray_icon2_tooltip_artist), NULL);
			gmpc_clicklabel_set_do_bold(GMPC_CLICKLABEL(label),FALSE);
			gmpc_clicklabel_font_size(GMPC_CLICKLABEL(label),0);
			gtk_box_pack_start(GTK_BOX(vbox), label, FALSE,FALSE,0);
		}
		if(song->album)
		{
			label = gmpc_clicklabel_new(song->album);
			g_signal_connect(G_OBJECT(label), "button-press-event", G_CALLBACK(tray_icon2_tooltip_album), NULL);
			gmpc_clicklabel_set_do_bold(GMPC_CLICKLABEL(label),FALSE);
			gmpc_clicklabel_font_size(GMPC_CLICKLABEL(label),-3);
			gtk_box_pack_start(GTK_BOX(vbox), label, FALSE,FALSE,0);
		}
		tray_icon2_tooltip_pb = gtk_progress_bar_new();
		/* Update the progressbar */
		{
			int totalTime = mpd_status_get_total_song_time(connection);                                 		
			int elapsedTime = mpd_status_get_elapsed_song_time(connection);	
			gdouble progress = elapsedTime/(gdouble)MAX(totalTime,1);
			gchar*label = g_strdup_printf("%02i:%02i/%02i:%02i", elapsedTime/60, elapsedTime%60,
					totalTime/60,totalTime%60);
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(tray_icon2_tooltip_pb), RANGE(0,1,progress));
			gtk_progress_bar_set_text(GTK_PROGRESS_BAR(tray_icon2_tooltip_pb), label);
			q_free(label);
		}


		gtk_box_pack_start(GTK_BOX(vbox), tray_icon2_tooltip_pb, TRUE,FALSE,0);
	} else {
		label = gtk_label_new("");
		gtk_label_set_markup(GTK_LABEL(label), _("<span size='large'>Gnome Music Player Client</span>"));
		gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
		gtk_box_pack_start(GTK_BOX(vbox), label, TRUE,TRUE,0);

	}
	/**
	 * 	Position the popup
	 */
	state = cfg_get_single_value_as_int_with_default(config,TRAY_ICON2_ID, "tooltip-position", TI2_AT_TOOLTIP);
	if(state == TI2_AT_TOOLTIP && tray_icon2_get_available()) {

		GdkRectangle rect, rect2;
		GtkOrientation orientation;
		if(gtk_status_icon_get_geometry(tray_icon2_gsi, &screen, &rect, &orientation))
		{
			monitor  = gdk_screen_get_monitor_at_point(screen, rect.x, rect.y);
			gdk_screen_get_monitor_geometry(screen, monitor, &rect2);
			/* Get Y */
			y= rect.y+rect.height+5;
			/* if the lower part falls off the screen, move it up */
			if((y+95) > rect2.height) {
				y = rect.y - 95 - 5;
			}
			if(y < 0) y =0;

			/* Get X */
			x = rect.x - 300/2;
			if((x+300) > rect2.width){
				if(orientation == GTK_ORIENTATION_VERTICAL) {
					x = rect2.width+-300-rect.width-5;
				} else {
					x = rect2.width - 300;
				}
			}
			if(x<0) {
				if(orientation == GTK_ORIENTATION_VERTICAL) {
					x = rect.width+5;
				} else {
					x = 0;
				}
			}
		}
		gtk_window_move(GTK_WINDOW(tray_icon2_tooltip), x,y);
	} else if (state == TI2_AT_UPPER_LEFT) {
		screen =gtk_widget_get_screen(pl3_win);
		GdkRectangle rect2;
		monitor  = gdk_screen_get_monitor_at_window(screen, pl3_win->window);
		gdk_screen_get_monitor_geometry(screen, monitor, &rect2);
		gtk_window_move(GTK_WINDOW(tray_icon2_tooltip), rect2.x+5,rect2.y+5);
	} else if (state == TI2_AT_UPPER_RIGHT) {
		screen =gtk_widget_get_screen(pl3_win);
		GdkRectangle rect2;
		monitor  = gdk_screen_get_monitor_at_window(screen, pl3_win->window);
		gdk_screen_get_monitor_geometry(screen, monitor, &rect2);
		/** Set Y = 0; */
		y = rect2.y+5;
		/** X is upper right - width */
		x = rect2.x+rect2.width-5-300;
		gtk_window_move(GTK_WINDOW(tray_icon2_tooltip), x,y);
	} else if (state == TI2_AT_LOWER_LEFT) {

		screen =gtk_widget_get_screen(pl3_win);
		GdkRectangle rect2;
		monitor  = gdk_screen_get_monitor_at_window(screen, pl3_win->window);
		gdk_screen_get_monitor_geometry(screen, monitor, &rect2);
		/** Set Y = window height - size; */
		y = rect2.y+rect2.height-5-95;
		/** X =5 */ 
		x = rect2.x+ 5; 
		gtk_window_move(GTK_WINDOW(tray_icon2_tooltip), x,y);
	} else {
		screen =gtk_widget_get_screen(pl3_win);
		GdkRectangle rect2;
		monitor  = gdk_screen_get_monitor_at_window(screen, pl3_win->window);
		gdk_screen_get_monitor_geometry(screen, monitor, &rect2);
		/** Set Y = window height - size; */
		y = rect2.y+rect2.height-5-95;
		/** X =window width - width */ 
		x = rect2.x+rect2.width-5-300; 
		gtk_window_move(GTK_WINDOW(tray_icon2_tooltip), x,y);
	}

	/**
	 * Show the tooltip
	 */
	gtk_widget_show_all(tray_icon2_tooltip);
	/**
	 * Destroy it after 5 seconds
	 */
	state = cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "tooltip-timeout", 5);
	tray_icon2_tooltip_timeout = g_timeout_add(state*1000, (GSourceFunc)tray_icon2_tooltip_destroy, NULL);
}

/**
 *  PREFERENCES 
 */


void tray_enable_toggled(GtkToggleButton *but)
{
	debug_printf(DEBUG_INFO,"tray-icon.c: changing tray icon %i\n", gtk_toggle_button_get_active(but));
	cfg_set_single_value_as_int(config, TRAY_ICON2_ID, "enable", (int)gtk_toggle_button_get_active(but));
	if(cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "enable", 1)) {
		tray_icon2_set_enabled(TRUE);
	} else {
		tray_icon2_set_enabled(FALSE);
	}
}

/* this sets all the settings in the notification area preferences correct */
static void tray_update_settings()
{
	gtk_toggle_button_set_active((GtkToggleButton *)
			glade_xml_get_widget(tray_icon2_preferences_xml, "ck_tray_enable"), 
			cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "enable", DEFAULT_TRAY_ICON_ENABLE));
}

void popup_enable_toggled(GtkToggleButton *but)
{
	cfg_set_single_value_as_int(config, TRAY_ICON2_ID, "show-tooltip", gtk_toggle_button_get_active(but));
}


void popup_position_changed(GtkComboBox *om)
{
	cfg_set_single_value_as_int(config, TRAY_ICON2_ID, "tooltip-position", gtk_combo_box_get_active(om));
}

void popup_timeout_changed(void)
{
	cfg_set_single_value_as_int(config, TRAY_ICON2_ID, "tooltip-timeout",
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(tray_icon2_preferences_xml, "popup_timeout"))));
}

static void update_popup_settings()
{
	gtk_toggle_button_set_active((GtkToggleButton *)
			glade_xml_get_widget(tray_icon2_preferences_xml, "ck_popup_enable"),
			cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "show-tooltip", 1));
	gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(tray_icon2_preferences_xml, "om_popup_position")),
			cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "tooltip-position", 0));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(tray_icon2_preferences_xml, "popup_timeout")),
			cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "tooltip-timeout", 5));
}

static void tray_icon2_preferences_destroy(GtkWidget *container)
{
	if(tray_icon2_preferences_xml) {
		GtkWidget *vbox = glade_xml_get_widget(tray_icon2_preferences_xml, "tray-pref-vbox");
		gtk_container_remove(GTK_CONTAINER(container),vbox);
		g_object_unref(tray_icon2_preferences_xml);
		tray_icon2_preferences_xml = NULL;
	}
}
static void tray_icon2_preferences_construct(GtkWidget *container)
{
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
	tray_icon2_preferences_xml = glade_xml_new(path, "tray-pref-vbox",NULL);

	if(tray_icon2_preferences_xml) {
		GtkWidget *vbox = glade_xml_get_widget(tray_icon2_preferences_xml, "tray-pref-vbox");
		gtk_container_add(GTK_CONTAINER(container),vbox);
		tray_update_settings();
		update_popup_settings();
		glade_xml_signal_autoconnect(tray_icon2_preferences_xml);
	}
}
gmpcPrefPlugin tray_icon2_preferences = {
	tray_icon2_preferences_construct,
	tray_icon2_preferences_destroy
};

gmpcPlugin tray_icon2_plug = {
	.name 						= "Notification",
	.version 					= {0,0,0},
	.plugin_type 				= GMPC_INTERNALL,
	.init 						= tray_icon2_init,
	.destroy					= tray_icon2_destroy,
	.mpd_status_changed 		= tray_icon2_status_changed,
	.mpd_connection_changed 	= tray_icon2_connection_changed,
	.set_enabled 				= tray_icon2_set_enabled,
	.get_enabled				= tray_icon2_get_enabled,
	.pref						= &tray_icon2_preferences
};
