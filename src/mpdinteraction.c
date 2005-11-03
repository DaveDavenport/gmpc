#include <stdio.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include "mpdinteraction.h"
#include "playlist3.h"
#include "main.h"
#include "config1.h"
extern config_obj *config;
extern GtkListStore *pl2_store;
/* the internall data structure */
internal_data info;


/* Server Settings plugin */
void server_pref_construct(GtkWidget *container);
void server_pref_destroy(GtkWidget *container);
GladeXML *server_pref_xml = NULL;
gmpcPrefPlugin server_gpp = {
	server_pref_construct,
	server_pref_destroy
};
void ServerStatusChangedCallback(MpdObj *mi, ChangedStatusType what, void *userdata);
gmpcPlugin server_plug = {
	"Server Settings",
	{1,1,1},
	GMPC_INTERNALL,
	0,
	NULL,
	NULL,
	&ServerStatusChangedCallback,
	&server_gpp
};
enum
{
        ENABLED_COL,
	NAME_COL,
        ID_COL,
        N_COLUMNS
};

/* Connection settings plugin */
void connection_pref_construct(GtkWidget *container);
void connection_pref_destroy(GtkWidget *container);
GladeXML *connection_pref_xml = NULL;
gmpcPrefPlugin connection_gpp = {
	connection_pref_construct,
	connection_pref_destroy
};

gmpcPlugin connection_plug = {
	"Connection",
	{1,1,1},
	GMPC_INTERNALL,
	0,
	NULL,
	NULL,
	NULL,	
	&connection_gpp
};






/* this function doesnt use the start/stop_mpd_action because it the user doesnt want to see that */

int update_mpd_status()
{
	if(!mpd_check_connected(connection)) return TRUE;
	mpd_status_queue_update(connection);
/*	mpd_status_update(connection);*/

	/* unlock it */
	return TRUE;
}

int disconnect_to_mpd()
{
	/* disconnect */
	mpd_disconnect(connection);
	return FALSE;
}

void disconnect_callback(MpdObj *mi)
{
	gtk_timeout_remove(update_timeout);
	msg_set_base(_("gmpc - Disconnected"));
	if(cfg_get_single_value_as_int_with_default(config, "player", "window-title",TRUE))
	{
		gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")), _("Gnome Music Player Client"));	
	}

	scroll.exposed = 1;
	info.playlist_id = -1;
	info.playlist_length = -1;
	info.playlist_playtime = 0;
	info.old_pos = -1;
	
	/* disconnect playlist */
	pl3_disconnect();

	update_timeout =  gtk_timeout_add(5000, (GSourceFunc)update_interface, NULL);
	preferences_update();
	info.updating = FALSE;
	gtk_list_store_clear(pl2_store);
}

/* the functiont that connects to mpd */
int connect_to_mpd()
{
	char *string = NULL;
	scroll.exposed = 1;
	info.playlist_playtime = 0;
	string =cfg_get_single_value_as_string_with_default(config, "connection","hostname","localhost");
	mpd_set_hostname(connection,string);
	cfg_free_string(string);
	mpd_set_port(connection, cfg_get_single_value_as_int_with_default(config,"connection","portnumber", 6600));
	mpd_set_connection_timeout(connection, cfg_get_single_value_as_float_with_default(config,"connection","timeout",1.0));

	if(cfg_get_single_value_as_int_with_default(config, "connection", "useauth",0))
	{
		string = cfg_get_single_value_as_string_with_default(config, "connection","password", "");
		mpd_set_password(connection,string);
		cfg_free_string(string);
	}
	else
	{
		mpd_set_password(connection,"");
	}

	if(mpd_connect(connection) < 0)
	{
		debug_printf(DEBUG_INFO,"Connection failed\n");
		return TRUE;
	}


	update_mpd_status();
	mpd_stats_update(connection);
	/* Set the title */
	msg_set_base(_("GMPC - Connected"));
	if(cfg_get_single_value_as_int_with_default(config, "player", "window-title",TRUE))
	{
		gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")), _("Gnome Music Player Client"));		
	}

	return FALSE;
}

/* DEFAULT FUNCTIONS */

gboolean check_connection_state()
{
	return !mpd_check_connected(connection);
}



/******************************************************
 * PLAYER FUNCTIONS 
 */


/* the normal play functions, stop, play, next, prev */
/* returns FALSE when everything went ok */
int next_song()
{
	mpd_player_next(connection);
	return FALSE;
}

int prev_song()
{
	mpd_player_prev(connection);
	return FALSE;
}

int stop_song()
{
	mpd_player_stop(connection);
	return FALSE;
}

int play_song()
{
	int state = mpd_player_get_state(connection);
	printf("state: %i %i\n", state, MPD_PLAYER_PLAY);
	if(state == MPD_PLAYER_STOP)
	{
		mpd_player_play(connection);
	}
	else if (state == MPD_PLAYER_PAUSE || state == MPD_PLAYER_PLAY)
	{
		mpd_player_pause(connection);
	}
	return FALSE;	
}

void random_pl(GtkToggleButton *tb)
{
	if(gtk_toggle_button_get_active(tb) != mpd_player_get_random(connection))
		mpd_player_set_random(connection, !mpd_player_get_random(connection));
}

void repeat_pl(GtkToggleButton *tb)
{
	if(gtk_toggle_button_get_active(tb) != mpd_player_get_repeat(connection))
		mpd_player_set_repeat(connection, !mpd_player_get_repeat(connection));
}

/* TODO: Changed return Values, check for possible errors */
int seek_ps(int n)
{
	mpd_player_seek(connection, mpd_status_get_elapsed_song_time(connection)+n);
	return FALSE;
}

int seek_ns(int n)
{
	return seek_ps(-n);
}

		
/* returns TRUE when an error */
int check_for_errors()
{
	return mpd_check_error(connection);
}


/*****************************************************************
 * Preferences 
 */
void outputs_toggled(GtkCellRendererToggle *cell, gchar *path_str, GtkTreeView *view)
{
	gboolean state;
	gint id;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkTreePath *path = gtk_tree_path_new_from_string(path_str);

	if(gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get(model, &iter, ENABLED_COL, &state, -1);
		gtk_tree_model_get(model, &iter, ID_COL, &id, -1);
		state = !state;
		mpd_server_set_output_device(connection, id, state);

		gtk_list_store_set(GTK_LIST_STORE(model), &iter, ENABLED_COL, state, -1);
	}
	gtk_tree_path_free(path);
}

void create_outputs_tree()
{
	GtkListStore *model;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *col;
	GtkTreeView *tree;

	tree = GTK_TREE_VIEW(glade_xml_get_widget(server_pref_xml, "tv_outputs"));
	model = gtk_list_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_INT);
	gtk_tree_view_set_model(tree, GTK_TREE_MODEL(model));
	g_object_unref(G_OBJECT(model));

	cell = gtk_cell_renderer_toggle_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(col, cell, TRUE);
	gtk_tree_view_column_add_attribute(col, cell, "active", 0);
	gtk_tree_view_column_set_title(col, "Enabled");
	gtk_tree_view_append_column(tree, col);
	gtk_signal_connect(GTK_OBJECT(cell), "toggled",	GTK_SIGNAL_FUNC(outputs_toggled), tree);

	cell = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(col, cell, TRUE);
	gtk_tree_view_column_add_attribute(col, cell, "text", 1);
	gtk_tree_view_column_set_title(col, "Name");
	gtk_tree_view_append_column(tree, col);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_visible(col, FALSE);
	gtk_tree_view_append_column(tree, col);
}
void update_outputs_settings()
{
	GtkTreeIter iter;
	GtkListStore *store;
	GtkFrame *frame;

	frame = GTK_FRAME(glade_xml_get_widget(server_pref_xml, "frm_outputs"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(glade_xml_get_widget(server_pref_xml, "tv_outputs"))));
	gtk_list_store_clear(store);
	if(mpd_check_connected(connection) && mpd_server_check_version(connection, 0,12,0))
	{
		MpdData *data = mpd_server_get_output_devices(connection);
		while(data != NULL && data->value.output_dev->id != -10)
		{
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 
					0, data->value.output_dev->enabled?TRUE:FALSE, 
					1, data->value.output_dev->name, 
					2, data->value.output_dev->id, -1);
			data = mpd_data_get_next(data);
		}
		gtk_widget_set_sensitive(GTK_WIDGET(frame), TRUE);
		gtk_widget_show_all(GTK_WIDGET(frame));            		
	}
	else
	{
		gtk_widget_set_sensitive(GTK_WIDGET(frame), FALSE);
	}
}


void update_server_stats()
{
	if(server_pref_xml == NULL) return;
	if(mpd_check_connected(connection))
	{
		gchar *temp;
		temp = g_strdup_printf("%i", mpd_stats_get_total_songs(connection));
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_songs")), temp);
		g_free(temp);
		temp = g_strdup_printf("%i", mpd_stats_get_total_artists(connection));
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_artists")), temp);
		g_free(temp);
		temp = g_strdup_printf("%i", mpd_stats_get_total_albums(connection));
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_albums")), temp);
		g_free(temp);
		temp = g_strdup_printf(_("%i day%s %02i hour%s %02i minute%s"), 
				mpd_stats_get_uptime(connection)/86400,((mpd_stats_get_uptime(connection)/86400) != 1)? "s":"",
				(mpd_stats_get_uptime(connection)%86400)/3600,((mpd_stats_get_uptime(connection)%86400)/3600 != 1)? "s":"",
				(mpd_stats_get_uptime(connection)%3600)/60,((mpd_stats_get_uptime(connection)%3600)/60 != 1)? "s":""
				);
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_uptime")), temp);
		g_free(temp);                                                                                        		
		temp = g_strdup_printf(_("%i day%s %02i hour%s %02i minute%s"), 
				mpd_stats_get_playtime(connection)/86400,((mpd_stats_get_playtime(connection)/86400) != 1)? "s":"",
				(mpd_stats_get_playtime(connection)%86400)/3600,((mpd_stats_get_playtime(connection)%86400)/3600 != 1)? "s":"",
				(mpd_stats_get_playtime(connection)%3600)/60,((mpd_stats_get_playtime(connection)%3600)/60 != 1)? "s":""
				);                                                                     		
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_playtime")), temp);		
		g_free(temp);
	}
	else
	{
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_songs")), "N/A");
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_artists")), "N/A");
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_albums")), "N/A");
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_uptime")), "N/A");
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_playtime")), "N/A");		
	}



}
void xfade_enable_toggled(GtkToggleButton *but)
{

	int bool1  = gtk_toggle_button_get_active(but);
	gtk_widget_set_sensitive(glade_xml_get_widget(server_pref_xml, "sb_fade_time"), bool1);
	if(bool1)
	{
		int fade_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(server_pref_xml, "sb_fade_time")));	
		mpd_status_set_crossfade(connection, fade_time);
	}
	else 
	{
		mpd_status_set_crossfade(connection, 0);
	}	
}

void xfade_time_changed(GtkSpinButton *but)
{
	int fade_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(server_pref_xml, "sb_fade_time")));	
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(server_pref_xml, "cb_fading"))))
	{
		return;
	}
	mpd_status_set_crossfade(connection, fade_time);
}
void xfade_update()
{
	if(mpd_status_get_crossfade(connection) > 0)
	{
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(server_pref_xml, "sb_fade_time")),
				mpd_status_get_crossfade(connection));
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(server_pref_xml, "cb_fading")),
			mpd_status_get_crossfade(connection)?TRUE:FALSE);

	gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(server_pref_xml, "sb_fade_time")), 
			(mpd_status_get_crossfade(connection))?TRUE:FALSE);

}


void ServerStatusChangedCallback(MpdObj *mi, ChangedStatusType what, void *userdata)
{
	if(!server_pref_xml)return;
	if(what&MPD_CST_CROSSFADE)
	{
		xfade_update();
	}
}





void server_pref_destroy(GtkWidget *container)
{
	if(server_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(server_pref_xml, "server-vbox");
		gtk_container_remove(GTK_CONTAINER(container),vbox);
		g_object_unref(server_pref_xml);
		server_pref_xml = NULL;
	}
}
void server_pref_construct(GtkWidget *container)
{
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
	server_pref_xml = glade_xml_new(path, "server-vbox",NULL);

	if(server_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(server_pref_xml, "server-vbox");
		create_outputs_tree();
		update_outputs_settings();
		update_server_stats();
		if(!mpd_check_connected(connection))
		{
			gtk_widget_show(glade_xml_get_widget(server_pref_xml, "hb_warning_mesg"));
			return;
		}
		else 
		{
			gtk_widget_hide(glade_xml_get_widget(server_pref_xml, "hb_warning_mesg"));
		}	


		if(mpd_status_get_crossfade(connection) == 0)
		{
			gtk_toggle_button_set_active((GtkToggleButton *)
					glade_xml_get_widget(server_pref_xml, "cb_fading"), FALSE);
			gtk_widget_set_sensitive(glade_xml_get_widget(server_pref_xml, "sb_fade_time"), FALSE);
		}
		else {
			gtk_toggle_button_set_active((GtkToggleButton *)
					glade_xml_get_widget(server_pref_xml, "cb_fading"), TRUE);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(server_pref_xml, "sb_fade_time")), mpd_status_get_crossfade(connection));
			gtk_widget_set_sensitive(glade_xml_get_widget(server_pref_xml, "sb_fade_time"), TRUE);

		}
		gtk_container_add(GTK_CONTAINER(container),vbox);
		glade_xml_signal_autoconnect(server_pref_xml);
	}
}

/**************************************************
 * Connection Preferences *
 */
/* this function is called from the main loop, it makes sure stuff is up-to-date(r) */




void preferences_update()
{
	if(connection_pref_xml == NULL) return;
	update_server_stats();
	if(!mpd_check_connected(connection))
	{
		gtk_widget_set_sensitive(glade_xml_get_widget(connection_pref_xml, "bt_con"), TRUE);
		gtk_widget_set_sensitive(glade_xml_get_widget(connection_pref_xml, "bt_dis"), FALSE);	    
	}
	else
	{
		gtk_widget_set_sensitive(glade_xml_get_widget(connection_pref_xml, "bt_con"), FALSE);
		gtk_widget_set_sensitive(glade_xml_get_widget(connection_pref_xml, "bt_dis"), TRUE);	   
	}
}
void entry_auth_changed(GtkEntry *entry)
{
	cfg_set_single_value_as_string(config, "connection","password",
			(char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(connection_pref_xml, "entry_auth"))));
}

void auth_enable_toggled(GtkToggleButton *but)
{
	cfg_set_single_value_as_int(config, "connection", "useauth",gtk_toggle_button_get_active(but));
	gtk_widget_set_sensitive(glade_xml_get_widget(connection_pref_xml, "entry_auth"), 
			cfg_get_single_value_as_int_with_default(config, "connection","useauth",0));	
}
void update_preferences_information()
{
	cfg_set_single_value_as_string(config,"connection","hostname", 
			(char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(connection_pref_xml, "hostname_entry"))));
	cfg_set_single_value_as_int(config, "connection", "portnumber",
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(connection_pref_xml, "port_spin"))));
	cfg_set_single_value_as_float(config,"connection","timeout",
			gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(glade_xml_get_widget(connection_pref_xml, "timeout_spin"))));
	/* update timeout live */
	if(mpd_check_connected(connection))
	{
		mpd_set_connection_timeout(connection, 
				gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(glade_xml_get_widget(connection_pref_xml, "timeout_spin"))));
	}
}

void preferences_window_autoconnect(GtkToggleButton *tog)
{
	cfg_set_single_value_as_int(config, "connection", "autoconnect", gtk_toggle_button_get_active(tog));
}

void preferences_window_connect(GtkWidget *but)
{
	debug_printf(DEBUG_INFO,"*DEBUG** connect\n");
	if(!mpd_check_connected(connection))
		if(!connect_to_mpd())
		{
			/*	gtk_timeout_remove(update_timeout);
				update_timeout =  gtk_timeout_add(400, (GSourceFunc)update_interface, NULL);
				*/		}
}

void preferences_window_disconnect(GtkWidget *but)
{
	debug_printf(DEBUG_INFO,"**DEBUG** disconnect\n");    
	disconnect_to_mpd();
	/*	gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(connection_pref_xml, "db_lu")),"n/a");*/
}







/* this sets all the settings in the authentification area preferences correct */
void update_auth_settings()
{
	char *string = 	cfg_get_single_value_as_string_with_default(config, "connection","password", "");
	gtk_toggle_button_set_active((GtkToggleButton *)
			glade_xml_get_widget(connection_pref_xml, "ck_auth"), 
			cfg_get_single_value_as_int_with_default(config, "connection", "useauth", 0));
	gtk_widget_set_sensitive(glade_xml_get_widget(connection_pref_xml, "entry_auth"), 
			cfg_get_single_value_as_int_with_default(config, "connection", "useauth", 0));
	gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(connection_pref_xml, "entry_auth")),string);
	cfg_free_string(string);
}














void connection_pref_destroy(GtkWidget *container)
{
	if(connection_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(connection_pref_xml, "connection-vbox");
		gtk_container_remove(GTK_CONTAINER(container),vbox);
		g_object_unref(connection_pref_xml);
		connection_pref_xml = NULL;
	}
}
void connection_pref_construct(GtkWidget *container)
{
	gchar *string;
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
	connection_pref_xml = glade_xml_new(path, "connection-vbox",NULL);
	update_auth_settings();
	string = cfg_get_single_value_as_string_with_default(config, "connection","hostname","localhost");
	gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(connection_pref_xml, "hostname_entry")), string);
	cfg_free_string(string);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(connection_pref_xml, "port_spin")), 
			cfg_get_single_value_as_int_with_default(config, "connection","portnumber",6600));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(connection_pref_xml, "timeout_spin")), 
			cfg_get_single_value_as_float_with_default(config,"connection", "timeout",1.0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(connection_pref_xml, "ck_autocon")), 
			cfg_get_single_value_as_int_with_default(config,"connection", "autoconnect", 0));

	if(connection_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(connection_pref_xml, "connection-vbox");
		/* set the right sensitive stuff */
		if(!mpd_check_connected(connection))
		{
			gtk_widget_set_sensitive(glade_xml_get_widget(connection_pref_xml, "bt_con"), TRUE);
			gtk_widget_set_sensitive(glade_xml_get_widget(connection_pref_xml, "bt_dis"), FALSE);	    
		}
		else
		{
			gtk_widget_set_sensitive(glade_xml_get_widget(connection_pref_xml, "bt_con"), FALSE);
			gtk_widget_set_sensitive(glade_xml_get_widget(connection_pref_xml, "bt_dis"), TRUE);	    
		}
		gtk_container_add(GTK_CONTAINER(container),vbox);
		glade_xml_signal_autoconnect(connection_pref_xml);
	}
}

