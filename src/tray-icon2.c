/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <gtk/gtk.h>
#include <libmpd/libmpd.h>
#include <string.h>
#include <config.h>
#include "main.h"
#include "playlist3.h"
#include "vala/gmpc-progress.h"
#include "gob/gmpc-metaimage.h"
#include "misc.h"


/* name of config field */
#define TRAY_ICON2_ID "tray-icon2"

#ifdef EGGTRAYICON
#include "egg/eggtrayicon.h"
GtkWidget *tray_icon2_gsi = NULL;
#else
GtkStatusIcon 	*tray_icon2_gsi = NULL;
#endif

static gchar *current_song_checksum = NULL;

static void tray_icon2_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata);

static gboolean tray_icon2_tooltip_destroy(void);
/**
 * Tooltip 
 */
GtkWidget			*tray_icon2_tooltip = NULL;
GtkWidget 			*tray_icon2_tooltip_pb = NULL;
guint				tray_icon2_tooltip_timeout = 0;
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
static GtkBuilder *tray_icon2_preferences_xml = NULL;
void popup_timeout_changed(void);
void popup_position_changed(GtkComboBox *om);
void popup_enable_toggled(GtkToggleButton *but);
void tray_enable_toggled(GtkToggleButton *but);
void tray_icon2_preferences_pm_combo_changed(GtkComboBox *cm, gpointer data);
/**
 * Tray icon
 */

gboolean tray_icon2_get_available(void)
{
	if(tray_icon2_gsi) {
#ifdef EGGTRAYICON
        return GTK_WIDGET_REALIZED(tray_icon2_gsi);
#else
		if(gtk_status_icon_is_embedded(tray_icon2_gsi) &&
				gtk_status_icon_get_visible(tray_icon2_gsi)) {
			return TRUE;
		}
#endif        
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

static void tray_icon2_seek_event(GtkWidget * pb, guint seek_time, gpointer user_data)
{
    int	state = cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "tooltip-timeout", 5);
    printf("seek to: %i\n", (int)seek_time);
    mpd_player_seek(connection, (int)seek_time);
    if(tray_icon2_tooltip_timeout)
    {
        g_source_remove(tray_icon2_tooltip_timeout);
    }
    tray_icon2_tooltip_timeout = g_timeout_add_seconds(state*2, (GSourceFunc)tray_icon2_tooltip_destroy, NULL);
}
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
#ifdef EGGTRAYICON    
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, activate_time);
#else
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, gtk_status_icon_position_menu, gsi, button, activate_time);
#endif
}
#ifndef EGGTRAYICON
static void tray_icon2_embedded_changed(GtkStatusIcon *icon,GParamSpec *arg1, gpointer data)
{
    if(!gtk_status_icon_is_embedded(icon))
    {
        /* the widget isn't embedded anymore */
        create_playlist3();
    }
}
#endif
/**
 * Initialize the tray icon 
 */
static int tray_icon2_button_press_event(gpointer tray, GdkEventButton *event, gpointer data)
{
    if(event->button == 2)
    {
        play_song();
    }
#ifdef EGGTRAYICON
    else if(event->button == 1)
    {
        tray_icon2_activate(NULL, NULL);
    }
    else if(event->button == 3)
    {
        tray_icon2_populate_menu(NULL, event->button, event->time, NULL);
    }
#endif
    else
    {
        return FALSE;
    }

    return TRUE;
}

static int tray_icon2_button_scroll_event(gpointer tray, GdkEventScroll *event, gpointer data)
{
    if(event->direction == GDK_SCROLL_UP) {
        volume_up(); 
    }else if (event->direction == GDK_SCROLL_DOWN) {
        volume_down();
    }else if (event->direction == GDK_SCROLL_LEFT) {
        prev_song();
    }else if (event->direction == GDK_SCROLL_RIGHT) {
        next_song();
    }
    return TRUE;
}

#ifdef EGGTRAYICON
static int tray_icon2_button_enter_notify_event(GtkWidget *tray, GdkEventCrossing *event, gpointer data)
{

    tray_icon2_create_tooltip();
    return FALSE;
}
#endif
static void tray_icon2_init(void)
{

	if(cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "enable", TRUE))
	{
#ifdef EGGTRAYICON        
        tray_icon2_gsi = (GtkWidget *)egg_tray_icon_new("gmpc");
        gtk_container_add(GTK_CONTAINER(tray_icon2_gsi), gtk_image_new_from_icon_name("gmpc-tray", GTK_ICON_SIZE_MENU));
        gtk_widget_show_all(tray_icon2_gsi);
        gtk_widget_add_events(tray_icon2_gsi, GDK_SCROLL_MASK);
        g_signal_connect(G_OBJECT(tray_icon2_gsi), "button-press-event", G_CALLBACK(tray_icon2_button_press_event), NULL);
        g_signal_connect(G_OBJECT(tray_icon2_gsi), "scroll-event", G_CALLBACK(tray_icon2_button_scroll_event), NULL);

        g_signal_connect(G_OBJECT(tray_icon2_gsi), "enter-notify-event", G_CALLBACK(tray_icon2_button_enter_notify_event), NULL);
#else
		tray_icon2_gsi = gtk_status_icon_new_from_icon_name ("gmpc-tray-disconnected");

		/* connect the (sparse) signals */
		g_signal_connect(G_OBJECT(tray_icon2_gsi), "popup-menu", G_CALLBACK(tray_icon2_populate_menu), NULL);
		g_signal_connect(G_OBJECT(tray_icon2_gsi), "activate", G_CALLBACK(tray_icon2_activate), NULL);
		g_signal_connect(G_OBJECT(tray_icon2_gsi), "notify::embedded", G_CALLBACK(tray_icon2_embedded_changed), NULL);
		if(gtk_check_version(2, 15, 0) == NULL)
		{
			g_signal_connect(G_OBJECT(tray_icon2_gsi), "button-press-event", G_CALLBACK(tray_icon2_button_press_event), NULL);
			g_signal_connect(G_OBJECT(tray_icon2_gsi), "scroll-event", G_CALLBACK(tray_icon2_button_scroll_event), NULL);
		}
#endif
	}
}
/**
 * Destroy and cleanup 
 */
static void tray_icon2_destroy(void)
{
	if(tray_icon2_gsi)
	{
#ifdef EGGTRAYICON
        gtk_widget_destroy(tray_icon2_gsi);
#else
		g_object_unref(tray_icon2_gsi);
#endif        
        tray_icon2_gsi = NULL;
	}
    /* free the currnet song checksum */
    if(current_song_checksum) {
        g_free(current_song_checksum);
        current_song_checksum = NULL;
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
            tray_icon2_status_changed(connection, MPD_CST_SONGID, NULL);
		} else {
#ifndef EGGTRAYICON
			gtk_status_icon_set_visible(GTK_STATUS_ICON(tray_icon2_gsi), TRUE);
#endif            
		}
	} else {
		if(tray_icon2_gsi) {
#ifdef EGGTRAYICON
            gtk_widget_destroy(tray_icon2_gsi);
            tray_icon2_gsi = NULL;
#else
			gtk_status_icon_set_visible(GTK_STATUS_ICON(tray_icon2_gsi), FALSE);
#endif            
		}		
	}
}
/**
 * TOOLTIP 
 */
static gboolean has_buttons = FALSE;
static GtkWidget *play_button = NULL;

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
    has_buttons = FALSE;
	/* remove the timeout */
	return FALSE;	
}

static gboolean tray_icon2_tooltip_button_press_event(GtkWidget *box, GdkEventButton *event, GtkWidget *vbox)
{
    if((event == NULL || event->button == 3) && !has_buttons)
    {
        GtkWidget *hbox,*button;
        int	state = cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "tooltip-timeout", 5);
        if(tray_icon2_tooltip_timeout)
        {
            g_source_remove(tray_icon2_tooltip_timeout);
        }
        tray_icon2_tooltip_timeout = g_timeout_add_seconds(state*2, (GSourceFunc)tray_icon2_tooltip_destroy, NULL);

        has_buttons = TRUE;

        hbox = gtk_hbox_new(TRUE, 6);
        /* prev */
        button = gtk_button_new();
        gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_MEDIA_PREVIOUS,GTK_ICON_SIZE_BUTTON));
        gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(prev_song), NULL);
        /* stop */
        button = gtk_button_new();
        gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_MEDIA_STOP,GTK_ICON_SIZE_BUTTON));
        gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(stop_song), NULL);
        /* pause/play */
		state = mpd_player_get_state(connection);
		if(state != MPD_PLAYER_PLAY)
        {
            play_button = button = gtk_button_new();
            gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY,GTK_ICON_SIZE_BUTTON));
            gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
            g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(play_song), NULL);
        }
        else
        {
            play_button = button = gtk_button_new();
            gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_MEDIA_PAUSE,GTK_ICON_SIZE_BUTTON));

            gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
            g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(play_song), NULL);
        }
        /* next */
        button = gtk_button_new();
        gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_MEDIA_NEXT,GTK_ICON_SIZE_BUTTON));

        gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(next_song), NULL);

        gtk_widget_show_all(hbox);
        gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, TRUE,0);

        return TRUE;
    }
    tray_icon2_tooltip_destroy();
    return TRUE;
}


void tray_icon2_create_tooltip(void)
{
	int x=0,y=0,monitor;
	GdkScreen *screen;
	GtkWidget *pl3_win = playlist3_get_window(); 
	GtkWidget *hbox = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *label = NULL;
	GtkWidget *event = NULL;
	GtkWidget *coverimg = NULL;
    GdkColormap *colormap;
	mpd_Song *song = NULL;
    int tooltip_timeout = cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "tooltip-timeout", 5);
	int state = 0; 
	int x_offset = cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "x-offset", 0);
	int y_offset = cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "y-offset", 0);


	song = mpd_playlist_get_current_song(connection);
	/**
	 * if the tooltip still exists destroy it... 
	 */
	if(tray_icon2_tooltip)
	{
        /* Do a check to avoid ugly redraws */
        mpd_Song *song2 = g_object_get_data(G_OBJECT(tray_icon2_tooltip), "song");
        if(song2 && song ){
            if(song->file && song2->file && strcmp(song2->file, song->file) == 0)
            {
                if(tray_icon2_tooltip_timeout)
                {
                    g_source_remove(tray_icon2_tooltip_timeout);
                }
                tray_icon2_tooltip_timeout = g_timeout_add_seconds(tooltip_timeout, (GSourceFunc)tray_icon2_tooltip_destroy, NULL);
                return;
            }
        }
        if(tray_icon2_tooltip_timeout)
        {
            g_source_remove(tray_icon2_tooltip_timeout);
            tray_icon2_tooltip_timeout = 0;
        }
        tray_icon2_tooltip_pb = NULL;
        gtk_widget_destroy(tray_icon2_tooltip);
        tray_icon2_tooltip = NULL;
        //tray_icon2_tooltip_destroy();
	}
	if(cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "show-tooltip", 1) == 0)
		return;
	/* If gmpc is fullscreen, don't show the tooltip */
	if(pl3_window_is_fullscreen())
		return;
	/*
	 * 	Creat the tootlip window 
	 */
	tray_icon2_tooltip = gtk_window_new(GTK_WINDOW_POPUP);
    screen = gtk_window_get_screen(GTK_WINDOW(tray_icon2_tooltip));


    if (gdk_screen_is_composited(screen))
    {
        colormap = gdk_screen_get_rgba_colormap(screen);
        if(colormap)
            gtk_widget_set_colormap(tray_icon2_tooltip, colormap);
        gtk_window_set_opacity(GTK_WINDOW(tray_icon2_tooltip), 0.9);
    }


    /* causes the border */
	gtk_widget_modify_bg(GTK_WIDGET(tray_icon2_tooltip), GTK_STATE_NORMAL, &(pl3_win->style->black));
	gtk_container_set_border_width(GTK_CONTAINER(tray_icon2_tooltip),1);
	gtk_window_set_default_size(GTK_WINDOW(tray_icon2_tooltip), 300,-1);
	gtk_window_set_transient_for(GTK_WINDOW(tray_icon2_tooltip), GTK_WINDOW(pl3_win));

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

    coverimg = gmpc_metaimage_new_size(META_ALBUM_ART,80);
    gmpc_metaimage_set_squared(GMPC_METAIMAGE(coverimg), TRUE);
    gmpc_metaimage_set_connection(GMPC_METAIMAGE(coverimg), connection);
    gmpc_metaimage_set_no_cover_icon(GMPC_METAIMAGE(coverimg),(char *)"gmpc"); 
    /**
     * Force an update if mpd is playing
     */
    state = mpd_player_get_state(connection);
    if(state == MPD_PLAYER_PLAY || state == MPD_PLAYER_PAUSE) {
        gmpc_metaimage_update_cover(GMPC_METAIMAGE(coverimg), connection, MPD_CST_SONGID,NULL);
    } else  {
        gmpc_metaimage_set_cover_na(GMPC_METAIMAGE(coverimg));
    }

    if(mpd_player_get_consume(connection) && mpd_playlist_get_playlist_length(connection) < 100)
    {
        gchar *temp = g_strdup_printf("%i", mpd_playlist_get_playlist_length(connection)); 
        gmpc_metaimage_set_image_text(GMPC_METAIMAGE(coverimg), temp);
        g_free(temp);
    }
    /**
     * Pack the widget in a eventbox so we can set background color 
     */
    event = gtk_event_box_new();
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
	gtk_widget_modify_bg(GTK_WIDGET(event), GTK_STATE_NORMAL, &(pl3_win->style->light[GTK_STATE_NORMAL]));
	gtk_container_set_border_width(GTK_CONTAINER(vbox),3);
	gtk_container_add(GTK_CONTAINER(event), vbox);
	gtk_box_pack_start(GTK_BOX(hbox), event, TRUE,TRUE,0);

    if(!has_buttons){
        g_signal_connect(G_OBJECT(hbox), "button-press-event", G_CALLBACK(tray_icon2_tooltip_button_press_event), vbox);
    }

	/**
	 * If there is a song, show show song info
	 */
	if(song)
	{
        int i;
        char buffer[256];
        int size_offset = cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "size-offset", 1024);
		size_offset = (size_offset < 100)?1024:size_offset;

		/** Artist label */
		if(song->title || song->file || song->name)
		{
			gchar *test = g_strdup_printf("<span size='%i' weight='bold'>[%%title%%|%%shortfile%%][ (%%name%%)]</span>", 14*size_offset);
			mpd_song_markup_escaped(buffer, 256,test,song);
			q_free(test);
			label = gtk_label_new("");
			gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
			gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
			gtk_label_set_markup(GTK_LABEL(label), buffer);
			gtk_box_pack_start(GTK_BOX(vbox), label, FALSE,FALSE,0);
		}
		if(song->artist)
		{
			gchar *test = g_strdup_printf("<span size='%i'>%%artist%%</span>", 10*size_offset);
			label = gtk_label_new("");
			gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
			mpd_song_markup_escaped(buffer, 256,test,song);
			q_free(test);
			gtk_label_set_markup(GTK_LABEL(label), buffer);
			gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);

			gtk_box_pack_start(GTK_BOX(vbox), label, FALSE,FALSE,0);
		}
		if(song->album)
		{
			gchar *test = g_strdup_printf("<span size='%i'>%%album%%[ (%%year%%)]</span>", 8*size_offset);
			label = gtk_label_new("");
			gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);           
			mpd_song_markup_escaped(buffer, 256,test,song);
			q_free(test);
			gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
			gtk_label_set_markup(GTK_LABEL(label), buffer);
			gtk_box_pack_start(GTK_BOX(vbox), label, FALSE,FALSE,0);
		}
		tray_icon2_tooltip_pb = (GtkWidget *) gmpc_progress_new();
        gmpc_progress_set_hide_text(GMPC_PROGRESS(tray_icon2_tooltip_pb), TRUE);
		/* Update the progressbar */
        gmpc_progress_set_time(GMPC_PROGRESS(tray_icon2_tooltip_pb), 
                mpd_status_get_total_song_time(connection),
                mpd_status_get_elapsed_song_time(connection));


        g_signal_connect(G_OBJECT(tray_icon2_tooltip_pb), "seek-event", G_CALLBACK(tray_icon2_seek_event), NULL);

        gtk_widget_modify_bg(GTK_WIDGET(tray_icon2_tooltip_pb), GTK_STATE_NORMAL, &(pl3_win->style->light[GTK_STATE_NORMAL]));
        g_object_set_data_full(G_OBJECT(tray_icon2_tooltip), "song", mpd_songDup(song),(GDestroyNotify)mpd_freeSong); 
		gtk_box_pack_start(GTK_BOX(vbox), tray_icon2_tooltip_pb, TRUE,FALSE,0);

        i = mpd_player_get_next_song_id(connection);
        if(i > 0){
            mpd_Song *next_psong = mpd_playlist_get_song(connection, i);
            if(next_psong)
            {
                gchar *test = g_strdup_printf("<span size='%i'>%s: <i>[[%%title%% - &[%%artist%%]]|%%shortfile%%]</i></span>",
                        7*size_offset,_("Next"));
                label = gtk_label_new("");
                gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);           
                mpd_song_markup_escaped(buffer, 256,test,next_psong);
                q_free(test);
                gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
                gtk_label_set_markup(GTK_LABEL(label), buffer);
                gtk_box_pack_start(GTK_BOX(vbox), label, FALSE,FALSE,0);
                mpd_freeSong(next_psong);
            }
        }
	} else {
		gchar *value = g_markup_printf_escaped("<span size='large'>%s</span>", _("Gnome Music Player Client"));
		label = gtk_label_new("");
		gtk_label_set_markup(GTK_LABEL(label),value);
		gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
		gtk_box_pack_start(GTK_BOX(vbox), label, TRUE,TRUE,0);
		g_free(value);
	}
	/**
	 * 	Position the popup
	 */
	state = cfg_get_single_value_as_int_with_default(config,TRAY_ICON2_ID, "tooltip-position", TI2_AT_TOOLTIP);
	if(state == TI2_AT_TOOLTIP && tray_icon2_get_available()) 
	{

		GdkRectangle rect, rect2;
		GtkOrientation orientation;
#ifdef EGGTRAYICON
        GdkWindow *w = NULL;
        gtk_widget_realize(tray_icon2_gsi);
#if GTK_CHECK_VERSION(2,14,0)        
        w = gtk_widget_get_window(tray_icon2_gsi);
#else
        w = GTK_WIDGET(tray_icon2_gsi)->window;
#endif
        screen = gtk_widget_get_screen(tray_icon2_gsi);
        orientation = egg_tray_icon_get_orientation((EggTrayIcon *)tray_icon2_gsi);
        rect.width = tray_icon2_gsi->allocation.width; 
        rect.height = tray_icon2_gsi->allocation.height; 
        if(w)
        {
            gdk_window_get_origin(w, &(rect.x), &(rect.y));
#else
		if(gtk_status_icon_get_geometry(tray_icon2_gsi, &screen, &rect, &orientation))
        {
#endif        
			monitor  = gdk_screen_get_monitor_at_point(screen, rect.x, rect.y);
			gdk_screen_get_monitor_geometry(screen, monitor, &rect2);
			/* Get Y */
			y= rect.y+rect.height+5-rect2.y+y_offset;
			/* if the lower part falls off the screen, move it up */
			if((y+95) > rect2.height) {
				y = rect.y - 95 - 5;
			}
			if(y < 0) y =0;

			/* Get X */
			x = rect.x - 300/2-rect2.x+x_offset;
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
		gtk_window_move(GTK_WINDOW(tray_icon2_tooltip), rect2.x+x,rect2.y+y);
	} else 
		if (state == TI2_AT_UPPER_LEFT) 
		{
			GdkRectangle rect2;
			screen =gtk_widget_get_screen(pl3_win);
			
			monitor  = gdk_screen_get_monitor_at_window(screen, pl3_win->window);
			gdk_screen_get_monitor_geometry(screen, monitor, &rect2);
			gtk_window_move(GTK_WINDOW(tray_icon2_tooltip), rect2.x+5+x_offset,rect2.y+5+y_offset);
		} else if (state == TI2_AT_UPPER_RIGHT)  {
			GdkRectangle rect2;
			screen =gtk_widget_get_screen(pl3_win);
			
			monitor  = gdk_screen_get_monitor_at_window(screen, pl3_win->window);
			gdk_screen_get_monitor_geometry(screen, monitor, &rect2);
			/** Set Y = 0; */
			y = rect2.y+5;
			/** X is upper right - width */
			x = rect2.x+rect2.width-5-300;
			gtk_window_move(GTK_WINDOW(tray_icon2_tooltip), x+x_offset,y+y_offset);
		} else if (state == TI2_AT_LOWER_LEFT) 
		{
			GdkRectangle rect2;
			screen =gtk_widget_get_screen(pl3_win);

			monitor  = gdk_screen_get_monitor_at_window(screen, pl3_win->window);
			gdk_screen_get_monitor_geometry(screen, monitor, &rect2);
			/** Set Y = window height - size; */
			y = rect2.y+rect2.height-5-95;
			/** X =5 */ 
			x = rect2.x+ 5; 
			gtk_window_move(GTK_WINDOW(tray_icon2_tooltip), x+x_offset,y+y_offset);
		} else 
		{
			GdkRectangle rect2;
			screen =gtk_widget_get_screen(pl3_win);
			
			monitor  = gdk_screen_get_monitor_at_window(screen, pl3_win->window);
			gdk_screen_get_monitor_geometry(screen, monitor, &rect2);
			/** Set Y = window height - size; */
			y = rect2.y+rect2.height-5-95;
			/** X =window width - width */ 
			x = rect2.x+rect2.width-5-300; 
			gtk_window_move(GTK_WINDOW(tray_icon2_tooltip), x+x_offset,y+y_offset);
		}

		gtk_widget_show_all(tray_icon2_tooltip);


        if(has_buttons){
            has_buttons = FALSE;
            tray_icon2_tooltip_button_press_event(hbox,NULL, vbox);
        }
        /**
		 * Destroy it after 5 seconds
		 */
		tray_icon2_tooltip_timeout = g_timeout_add_seconds(tooltip_timeout, (GSourceFunc)tray_icon2_tooltip_destroy, NULL);
}

static void tray_icon2_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata)
{
	char buffer[256];
	mpd_Song *song = mpd_playlist_get_current_song(connection);
	if(what&(MPD_CST_SONGID)|| what&MPD_CST_SONGPOS || what&MPD_CST_PLAYLIST)
	{
		/** 
		 * If enabled by user, show the tooltip.
		 * But only if playing or paused.
		 *
		 */
		if(cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "show-tooltip", TRUE))
		{
			int state = mpd_player_get_state(connection);
            gchar *new_checksum;
            new_checksum = mpd_song_checksum(song);
			if(state == MPD_PLAYER_PLAY || state == MPD_PLAYER_PAUSE)
			{
                if(current_song_checksum == NULL || strcmp(current_song_checksum, new_checksum))
                    tray_icon2_create_tooltip();
            }
            if(current_song_checksum){
                g_free(current_song_checksum);
                current_song_checksum = NULL;
            }
            current_song_checksum = new_checksum;
        }

		if(tray_icon2_gsi)
		{
			mpd_song_markup(buffer, 256,"[%name%: ][%title%|%shortfile%][ - %artist%]",song);
#ifdef EGGTRAYICON
            gtk_widget_set_tooltip_text(tray_icon2_gsi, buffer);
#else
			gtk_status_icon_set_tooltip(tray_icon2_gsi,buffer);
#endif            
		}
	}

	/* update the progress bar if available */
	if(what&MPD_CST_ELAPSED_TIME)
	{
		if(tray_icon2_tooltip && tray_icon2_tooltip_pb)
		{
            gmpc_progress_set_time(GMPC_PROGRESS(tray_icon2_tooltip_pb),
                    mpd_status_get_total_song_time(connection),                                 		
                    mpd_status_get_elapsed_song_time(connection));	
        }
	}
	if(tray_icon2_gsi == NULL)
		return;


	if(what&MPD_CST_STATE)
	{
		int state = mpd_player_get_state(connection);
		if(state == MPD_PLAYER_PLAY){
			mpd_song_markup(buffer, 256,"[%name%: ][%title%|%shortfile%][ - %artist%]",song);
#ifdef EGGTRAYICON
            gtk_image_set_from_icon_name(GTK_IMAGE(gtk_bin_get_child(GTK_BIN(tray_icon2_gsi))), "gmpc-tray-play", GTK_ICON_SIZE_MENU);
            gtk_widget_set_tooltip_text(tray_icon2_gsi, buffer);
#else
            gtk_status_icon_set_from_icon_name(tray_icon2_gsi, "gmpc-tray-play");
            gtk_status_icon_set_tooltip(tray_icon2_gsi,buffer);
#endif            
			if(has_buttons)
			{
				gtk_button_set_image(GTK_BUTTON(play_button), gtk_image_new_from_stock(GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_BUTTON));
			}
		} else if(state == MPD_PLAYER_PAUSE){
			
#ifdef EGGTRAYICON
            gtk_image_set_from_icon_name(GTK_IMAGE(gtk_bin_get_child(GTK_BIN(tray_icon2_gsi))), "gmpc-tray-pause", GTK_ICON_SIZE_MENU);
            gtk_widget_set_tooltip_text(tray_icon2_gsi, _("Gnome Music Player Client"));
#else
            gtk_status_icon_set_from_icon_name(tray_icon2_gsi, "gmpc-tray-pause");
            gtk_status_icon_set_tooltip(tray_icon2_gsi,_("Gnome Music Player Client"));
#endif  
			if(has_buttons)
			{
				gtk_button_set_image(GTK_BUTTON(play_button), gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON));
			}
		} else {
			
#ifdef EGGTRAYICON
            gtk_image_set_from_icon_name(GTK_IMAGE(gtk_bin_get_child(GTK_BIN(tray_icon2_gsi))), "gmpc-tray", GTK_ICON_SIZE_MENU);
            gtk_widget_set_tooltip_text(tray_icon2_gsi, _("Gnome Music Player Client"));
#else
            gtk_status_icon_set_from_icon_name(tray_icon2_gsi, "gmpc-tray");
            gtk_status_icon_set_tooltip(tray_icon2_gsi,_("Gnome Music Player Client"));
#endif  
			if(has_buttons)
			{
				gtk_button_set_image(GTK_BUTTON(play_button), gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON));
			}
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
        /* Set the disconnect image, and reset the GtkTooltip */
#ifdef EGGTRAYICON
        gtk_image_set_from_icon_name(GTK_IMAGE(gtk_bin_get_child(GTK_BIN(tray_icon2_gsi))), "gmpc-tray-disconnected", GTK_ICON_SIZE_MENU);
#else
		gtk_status_icon_set_from_icon_name(tray_icon2_gsi, "gmpc-tray-disconnected");
        gtk_status_icon_set_tooltip(tray_icon2_gsi, _("Gnome Music Player Client"));
#endif        
        /* Destroy notification */
        if(tray_icon2_tooltip)
            tray_icon2_tooltip_destroy();

	}
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
static void tray_update_settings(void)
{
	gtk_toggle_button_set_active((GtkToggleButton *)
			gtk_builder_get_object(tray_icon2_preferences_xml, "ck_tray_enable"), 
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
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(tray_icon2_preferences_xml, "popup_timeout"))));
}

static void update_popup_settings(void)
{
	gtk_toggle_button_set_active((GtkToggleButton *)
			gtk_builder_get_object(tray_icon2_preferences_xml, "ck_popup_enable"),
			cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "show-tooltip", 1));
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(tray_icon2_preferences_xml, "om_popup_position")),
			cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "tooltip-position", 0));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(tray_icon2_preferences_xml, "popup_timeout")),
			cfg_get_single_value_as_int_with_default(config, TRAY_ICON2_ID, "tooltip-timeout", 5));
}

static void tray_icon2_preferences_destroy(GtkWidget *container)
{
	if(tray_icon2_preferences_xml) {
		GtkWidget *vbox =(GtkWidget *) gtk_builder_get_object(tray_icon2_preferences_xml, "tray-pref-vbox");
		gtk_container_remove(GTK_CONTAINER(container),vbox);
		g_object_unref(tray_icon2_preferences_xml);
		tray_icon2_preferences_xml = NULL;
	}
}
void tray_icon2_preferences_pm_combo_changed(GtkComboBox *cm, gpointer data)
{
    int level = gtk_combo_box_get_active(cm);
    cfg_set_single_value_as_int(config, "Default","min-error-level", level);
}
static void tray_icon2_preferences_construct(GtkWidget *container)
{
	gchar *path = gmpc_get_full_glade_path("preferences-trayicon.ui");
	tray_icon2_preferences_xml = gtk_builder_new();
    gtk_builder_add_from_file(tray_icon2_preferences_xml, path, NULL);
    q_free(path);

	if(tray_icon2_preferences_xml) {
		GtkWidget *vbox =(GtkWidget *) gtk_builder_get_object(tray_icon2_preferences_xml, "tray-pref-vbox");
		gtk_container_add(GTK_CONTAINER(container),vbox);
		tray_update_settings();
		update_popup_settings();
        gtk_builder_connect_signals(tray_icon2_preferences_xml, NULL);
	}
    gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(tray_icon2_preferences_xml, "pm-combo")),
        cfg_get_single_value_as_int_with_default(config, "Default","min-error-level", ERROR_INFO));
}
gmpcPrefPlugin tray_icon2_preferences = {
	tray_icon2_preferences_construct,
	tray_icon2_preferences_destroy
};

gmpcPlugin tray_icon2_plug = {
	.name 						= N_("Notification"),
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
