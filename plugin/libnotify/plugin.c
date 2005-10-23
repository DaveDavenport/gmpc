#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "../../config.h"
#include "../../src/plugin.h"
#include "../../src/strfsong.h"
#include "../../src/eggtrayicon.h"
#include "../../src/misc.h"
#include <libnotify/notify.h>

#define DEFAULT_TRAY_MARKUP "[<span size=\"small\">%name%</span>\n][<span size=\"large\">%title%</span>\n][%artist%][\n<span size=\"small\">%album% [(track %track%)]</span>]|%shortfile%|"



NotifyHandle *libnotify_handle = NULL;
int libnotify_initialized = 0;

extern EggTrayIcon *tray_icon;
GtkWidget *libnotify_vbox = NULL;

void libnotify_song_changed(MpdObj *mi, int old_song, int new_song);
void libnotify_state_changed(MpdObj *mi, int old_state, int new_state);

void libnotify_construct(GtkWidget *container);
void libnotify_destroy(GtkWidget *container);

gmpcPrefPlugin libnotify_gpp = {
	libnotify_construct,
	libnotify_destroy
};



/* set the signals I want */
gmpcMpdSignals libnotify_gms = {
	NULL,
	libnotify_song_changed,
	NULL,
	libnotify_state_changed,
	NULL
};
/* main plugin info */
gmpcPlugin plugin= {
	"libnotify plugin",
	{0,0,1},
	GMPC_PLUGIN_NO_GUI,
	0,
	NULL,
	&libnotify_gms,
	&libnotify_gpp
};

gchar *libnotify_get_text()
{
	GString *string = g_string_new("");

	gchar result[1024];
	gchar *retval;
	int id;
	if(mpd_check_connected(connection) && mpd_player_get_state(connection) != MPD_PLAYER_STOP)
	{
		mpd_Song *song = mpd_playlist_get_current_song(connection);
		strfsong(result, 1024, DEFAULT_TRAY_MARKUP, song);
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
			string->str[id] = 'a';
			g_string_insert(string, id+1, "nd");
			id++;
		}
	}

	/* return a string (that needs to be free'd */
	retval = string->str;
	g_string_free(string, FALSE);
	return retval;
}


void libnotify_init()
{
	notify_init("Gnome Music Player Client");
	libnotify_initialized = TRUE;
}


void libnotify_do_popup()
{
	char *text = libnotify_get_text();
	char *title = NULL;
	NotifyHints *hints = NULL;
	NotifyIcon *icon = notify_icon_new_from_uri(PIXMAP_PATH"media-audiofile.png");
	if(tray_icon)
	{
		int x_tv, y_tv;
	
		gdk_window_get_origin(GTK_WIDGET(tray_icon)->window, &x_tv, &y_tv);
		
		hints = notify_hints_new();
		notify_hints_set_int(hints, "x", x_tv+(GTK_WIDGET(tray_icon)->allocation.width)/2);
		notify_hints_set_int(hints, "y", y_tv+GTK_WIDGET(tray_icon)->allocation.height);

	}
	switch(mpd_player_get_state(connection))
	{
		case MPD_STATUS_STATE_PLAY:
			title = g_strdup("Playing:");
			break;
		case MPD_STATUS_STATE_PAUSE:
			title = g_strdup("Paused:");
			break;
		default:
			title = g_strdup("Stopped:");
			break;
	}
			
	libnotify_handle = notify_send_notification(libnotify_handle, NULL, NOTIFY_URGENCY_NORMAL,
			title,text,
			icon,TRUE, cfg_get_single_value_as_int_with_default(config, "libnotify-plugin","popup-timeout",5),
			hints,NULL,0); 

	notify_icon_destroy(icon);
	g_free(text);
	g_free(title);
	if (!libnotify_handle) {
		fprintf(stderr, "failed to send notification\n");
		return;
	}
}


void libnotify_song_changed(MpdObj *mi, int old_song, int new_song)
{
	mpd_Song *song = NULL;
	if(!cfg_get_single_value_as_int_with_default(config, "libnotify-plugin", "enable", 0))
	{
		return;
	}
	if(!libnotify_initialized) libnotify_init();
	libnotify_do_popup();
}

void libnotify_state_changed(MpdObj *mi, int old_state, int new_state)
{
	if(!cfg_get_single_value_as_int_with_default(config, "libnotify-plugin", "enable", 0))
	{                                                                                   	
		return;                                                                     	
	}                                                                                   	
	if(!libnotify_initialized) libnotify_init();
	libnotify_do_popup();
}

void libnotify_enable_toggle(GtkWidget *wid)
{
	int kk = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid));
	cfg_set_single_value_as_int(config, "libnotify-plugin", "enable", kk);
}
void libnotify_spin_value_changed(GtkWidget *wid)
{
	int kk = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(wid));
	cfg_set_single_value_as_int(config, "libnotify-plugin", "popup-timeout", kk);
}
void libnotify_destroy(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), libnotify_vbox);
}

void libnotify_construct(GtkWidget *container)
{
	GtkWidget *enable_cg = gtk_check_button_new_with_mnemonic("_Enable LibNotify");
	GtkWidget *label = NULL;
	GtkWidget *wid2 = NULL;
	libnotify_vbox = gtk_vbox_new(FALSE,6);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable_cg), 	
			cfg_get_single_value_as_int_with_default(config, "libnotify-plugin", "enable", 0));

	label = gtk_label_new("LibNotify Plugin");
	gtk_label_set_markup(GTK_LABEL(label),"<span size=\"large\"><b>LibNotify Plugin</b></span>");
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	
	g_signal_connect(G_OBJECT(enable_cg), "toggled", G_CALLBACK(libnotify_enable_toggle), NULL);
	gtk_box_pack_start(GTK_BOX(libnotify_vbox), label,FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(libnotify_vbox), enable_cg, FALSE, FALSE, 0);
	wid2 = gtk_hbox_new(FALSE,6);
	gtk_box_pack_start(GTK_BOX(libnotify_vbox), wid2,FALSE, FALSE, 0);
	label = gtk_label_new("Timeout:");
	gtk_box_pack_start(GTK_BOX(wid2), label,FALSE, FALSE, 0);
	label = gtk_spin_button_new_with_range(1.0,10.0,1.0); 
	gtk_box_pack_start(GTK_BOX(wid2), label,FALSE, FALSE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(label),(gdouble)cfg_get_single_value_as_int_with_default(config, "libnotify-plugin","popup-timeout",5));
	g_signal_connect(G_OBJECT(label), "value-changed", G_CALLBACK(libnotify_spin_value_changed), NULL);
	
	gtk_container_add(GTK_CONTAINER(container), libnotify_vbox);
	gtk_widget_show_all(container);
}
