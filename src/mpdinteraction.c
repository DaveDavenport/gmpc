#include <stdio.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <string.h>
#include "main.h"
#include "config1.h"
#include "mpdinteraction.h"

extern GtkWidget *pl3_cp_tree;

/* old stuff */
static void preferences_update(void);
static void disconnect_callback(MpdObj *);

/* Server Settings plugin */
static void server_pref_construct(GtkWidget *);
static void server_pref_destroy(GtkWidget *);

/* Connection settings plugin */
static void connection_pref_construct(GtkWidget *container);
static void connection_pref_destroy(GtkWidget *container);

static void ServerConnectionChangedCallback(MpdObj *mi, int connected, gpointer data);
static void ServerStatusChangedCallback(MpdObj *mi, ChangedStatusType what, void *userdata);

GladeXML *server_pref_xml = NULL;
gmpcPrefPlugin server_gpp = {
	server_pref_construct,
	server_pref_destroy
};

// Glade prototypes, would be static without glade
void xfade_enable_toggled(GtkToggleButton *but);
void xfade_time_changed(GtkSpinButton *but);
void entry_auth_changed(GtkEntry *entry);
void auth_enable_toggled(GtkToggleButton *but);

void preferences_window_autoconnect(GtkToggleButton *tog);
void preferences_window_connect(GtkWidget *but);
void preferences_window_disconnect(GtkWidget *but);
void update_preferences_name(void);
void update_preferences_hostname(void);
void update_preferences_portnumber(void);
void update_preferences_information(void);
void connection_profiles_changed(GtkComboBox *combo, gpointer data);
void connection_add_profile(void);
void connection_remove_profile(void);

static gchar *current = NULL;

gmpcPlugin server_plug = {
	"Server Settings", 	/** name */
	{1,1,1},			/** Version */
	GMPC_INTERNALL,		/** Plugin Type */
	0,					/** Internal Id */
	NULL,				/** path to plugin */
	NULL,				/** init */
	NULL,				/** browser ext */
	&ServerStatusChangedCallback,	/** status changed */
	&ServerConnectionChangedCallback,	/** connection changed */
	&server_gpp,			/** preferences */
	NULL,				/** Metadata */
	NULL,				/** get enabled */
	NULL				/** set enabled */
};
enum
{
	ENABLED_COL,
	NAME_COL,
	ID_COL,
	N_COLUMNS
};

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
	NULL, /* init function */
	NULL,
	NULL,
	NULL,
	NULL,
	&connection_gpp,
	NULL,
	NULL,
	NULL
};

/* this function doesnt use the start/stop_mpd_action because it the user doesnt want to see that */

int update_mpd_status()
{
	if(!mpd_check_connected(connection)) return TRUE;
	mpd_status_update(connection);

	/* unlock it */
	return TRUE;
}

static void disconnect_callback(MpdObj *mi)
{
	/* disconnect playlist */
	debug_printf(DEBUG_INFO, "Going To Clear the playlist-list");
	playlist_list_clear(PLAYLIST_LIST(playlist),GTK_TREE_VIEW(pl3_cp_tree));
	debug_printf(DEBUG_INFO, "Done Clearing the playlist-list");

}

/* the functiont that connects to mpd */
int connect_to_mpd()
{
	char *string = NULL;
	/**
	 * Set Hostname
	 */
	string = connection_get_hostname();
	mpd_set_hostname(connection,string);
	cfg_free_string(string);
	/** 
	 * Set port
	 */
	mpd_set_port(connection,connection_get_port());
	/**
	 * Timeout
	 */
	mpd_set_connection_timeout(connection, cfg_get_single_value_as_float_with_default(config,"connection","timeout",DEFAULT_TIMEOUT));

	if(connection_use_auth())
	{
		string = connection_get_password();
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
	/* Set the title */
	update_mpd_status();
	mpd_stats_update(connection);
	/* set that user wants to connect */
	gmpc_connected = TRUE;

	return FALSE;
}

/* DEFAULT FUNCTIONS */
static void ServerConnectionChangedCallback(MpdObj *mi, int connected, gpointer data)
{
	if(connected)
	{
		connect_callback(mi);
	}
	else
	{
		disconnect_callback(mi);
	}
	preferences_update();
}


/******************************************************
 * PLAYER FUNCTIONS
 */


/* the normal play functions, stop, play, next, prev */
/* returns FALSE when everything went ok */
int next_song()
{
	if(mpd_server_check_command_allowed(connection, "next") == MPD_SERVER_COMMAND_ALLOWED)
	{
		mpd_player_next(connection);
	}
	return FALSE;
}

int prev_song()
{
	if(mpd_server_check_command_allowed(connection, "previous") == MPD_SERVER_COMMAND_ALLOWED)
	mpd_player_prev(connection);
	return FALSE;
}

int stop_song()
{
	if(mpd_server_check_command_allowed(connection, "stop") == MPD_SERVER_COMMAND_ALLOWED)
		mpd_player_stop(connection);
	return FALSE;
}

int play_song()
{
	int state = mpd_player_get_state(connection);
	if(state == MPD_PLAYER_STOP)
	{
		if(mpd_server_check_command_allowed(connection, "play") == MPD_SERVER_COMMAND_ALLOWED)
			mpd_player_play(connection);
	}
	else if (state == MPD_PLAYER_PAUSE || state == MPD_PLAYER_PLAY)
	{
		if(mpd_server_check_command_allowed(connection, "pause") == MPD_SERVER_COMMAND_ALLOWED)
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
	if(mpd_server_check_command_allowed(connection, "seek") == MPD_SERVER_COMMAND_ALLOWED)
		mpd_player_seek(connection, mpd_status_get_elapsed_song_time(connection)+n);
	return FALSE;
}

int seek_ns(int n)
{
	return seek_ps(-n);
}

void song_fastforward()
{
	seek_ps(1);
}
void song_fastbackward()
{
	seek_ps(-1);
}
void repeat_toggle()
{
	mpd_player_set_repeat(connection, !mpd_player_get_repeat(connection));
}
void random_toggle()
{
	mpd_player_set_random(connection, !mpd_player_get_random(connection));
}
void volume_up()
{
	mpd_status_set_volume(connection, mpd_status_get_volume(connection)+5);
}
void volume_down()
{
	mpd_status_set_volume(connection, mpd_status_get_volume(connection)-5);
}
/*****************************************************************
 * Preferences
 */
static void outputs_toggled(GtkCellRendererToggle *cell, gchar *path_str, GtkTreeView *view)
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

static void create_outputs_tree()
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
static void update_outputs_settings()
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
		while(data != NULL && data->output_dev->id != -10)
		{
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 
					0, data->output_dev->enabled?TRUE:FALSE, 
					1, data->output_dev->name, 
					2, data->output_dev->id, -1);
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


static void update_server_stats()
{
	if(server_pref_xml == NULL) return;
	if(mpd_check_connected(connection))
	{
		gchar *temp;
		temp = g_strdup_printf("%i", mpd_stats_get_total_songs(connection));
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_songs")), 
				temp);
		g_free(temp);
		temp = g_strdup_printf("%i", mpd_stats_get_total_artists(connection));
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_artists")), 
				temp);
		g_free(temp);
		temp = g_strdup_printf("%i", mpd_stats_get_total_albums(connection));
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_albums")), 
				temp);
		g_free(temp);
		temp = g_strdup_printf(_("%i day%s %02i hour%s %02i minute%s"),
				mpd_stats_get_uptime(connection)/86400,
				((mpd_stats_get_uptime(connection)/86400) != 1)? "s":"",
				(mpd_stats_get_uptime(connection)%86400)/3600,
				((mpd_stats_get_uptime(connection)%86400)/3600 != 1)? "s":"",
				(mpd_stats_get_uptime(connection)%3600)/60,
				((mpd_stats_get_uptime(connection)%3600)/60 != 1)? "s":""
				);
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_uptime")),
				temp);
		g_free(temp);
		temp = g_strdup_printf(_("%i day%s %02i hour%s %02i minute%s"), 
				mpd_stats_get_playtime(connection)/86400,
				((mpd_stats_get_playtime(connection)/86400) != 1)? "s":"",
				(mpd_stats_get_playtime(connection)%86400)/3600,
				((mpd_stats_get_playtime(connection)%86400)/3600 != 1)? "s":"",
				(mpd_stats_get_playtime(connection)%3600)/60,
				((mpd_stats_get_playtime(connection)%3600)/60 != 1)? "s":""
				);

		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_playtime")), 
				temp);
		g_free(temp);
	}
	else
	{
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_songs")),
				_("N/A"));
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_artists")),
				_("N/A"));
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_albums")),
				_("N/A"));
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_uptime")),
				_("N/A"));
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(server_pref_xml, "ss_label_playtime")),
				_("N/A"));
	}



}
void xfade_enable_toggled(GtkToggleButton *but)
{

	int bool1  = gtk_toggle_button_get_active(but);
	gtk_widget_set_sensitive(glade_xml_get_widget(server_pref_xml, "sb_fade_time"), bool1);
	if(bool1)
	{
		int fade_time = gtk_spin_button_get_value_as_int(
				GTK_SPIN_BUTTON(glade_xml_get_widget(server_pref_xml, "sb_fade_time")));
		mpd_status_set_crossfade(connection, fade_time);
	}
	else
	{
		mpd_status_set_crossfade(connection, 0);
	}
}

void xfade_time_changed(GtkSpinButton *but)
{
	int fade_time = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(glade_xml_get_widget(server_pref_xml, "sb_fade_time")));
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
					glade_xml_get_widget(server_pref_xml, "cb_fading"))))
	{
		return;
	}
	mpd_status_set_crossfade(connection, fade_time);
}
static void xfade_update()
{
	if(mpd_status_get_crossfade(connection) > 0)
	{
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(
					glade_xml_get_widget(server_pref_xml, "sb_fade_time")),
				mpd_status_get_crossfade(connection));
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
				glade_xml_get_widget(server_pref_xml, "cb_fading")),
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





static void server_pref_destroy(GtkWidget *container)
{
	if(server_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(server_pref_xml, "server-vbox");
		gtk_container_remove(GTK_CONTAINER(container),vbox);
		g_object_unref(server_pref_xml);
		server_pref_xml = NULL;
	}
}
static void server_pref_construct(GtkWidget *container)
{
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
	server_pref_xml = glade_xml_new(path, "server-vbox",NULL);
	g_free(path);

	if(server_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(server_pref_xml, "server-vbox");
		create_outputs_tree();
		update_outputs_settings();
		update_server_stats();
		if(!mpd_check_connected(connection))
		{
			gtk_widget_set_sensitive(vbox,FALSE);
			gtk_widget_show(glade_xml_get_widget(server_pref_xml, "hb_warning_mesg"));
		}
		else
		{
			gtk_widget_set_sensitive(vbox,TRUE);
			gtk_widget_hide(glade_xml_get_widget(server_pref_xml, "hb_warning_mesg"));
		}

		if(mpd_status_get_crossfade(connection) == 0)
		{
			gtk_toggle_button_set_active((GtkToggleButton *)
					glade_xml_get_widget(server_pref_xml, "cb_fading"), FALSE);
			gtk_widget_set_sensitive(glade_xml_get_widget(server_pref_xml, "sb_fade_time"),
					FALSE);
		}
		else {
			gtk_toggle_button_set_active((GtkToggleButton *)
					glade_xml_get_widget(server_pref_xml, "cb_fading"), TRUE);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(
						glade_xml_get_widget(server_pref_xml,"sb_fade_time")),
					mpd_status_get_crossfade(connection));
			gtk_widget_set_sensitive(glade_xml_get_widget(server_pref_xml, "sb_fade_time"),
					TRUE);

		}
		gtk_container_add(GTK_CONTAINER(container),vbox);
		glade_xml_signal_autoconnect(server_pref_xml);
	}
}

/**************************************************
 * Connection Preferences *
 */
/* this function is called from the main loop, it makes sure stuff is up-to-date(r) */




static void preferences_update()
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
	GtkComboBox *combo = (GtkComboBox *)glade_xml_get_widget(connection_pref_xml, "cb_profiles");
	GtkTreeIter iter;
	GtkListStore *store = (GtkListStore *)gtk_combo_box_get_model(combo);
	if(gtk_combo_box_get_active_iter(combo,&iter))
	{
		gchar *value= NULL, *uid = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &uid, 1,&value, -1);
		cfg_set_single_value_as_string(profiles, uid,"password",
			(char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(connection_pref_xml, "entry_auth"))));
		g_free(uid);
		g_free(value);
	}
}

void auth_enable_toggled(GtkToggleButton *but)
{
	GtkComboBox *combo = (GtkComboBox *) glade_xml_get_widget(connection_pref_xml, "cb_profiles");
	GtkTreeIter iter;
	GtkTreeModel *store = gtk_combo_box_get_model(combo);
	if(gtk_combo_box_get_active_iter(combo,&iter))
	{
		char *value= NULL, *uid = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &uid, 1,&value, -1);
		cfg_set_single_value_as_int(profiles, uid, "useauth",gtk_toggle_button_get_active(but));
		gtk_widget_set_sensitive(glade_xml_get_widget(connection_pref_xml, "entry_auth"), 
				cfg_get_single_value_as_int_with_default(profiles, uid,"useauth",0));	
		g_free(uid);
		g_free(value);
		printf("blub\n");
	}
}

void update_preferences_name(void)
{
	GtkComboBox *combo = (GtkComboBox *) glade_xml_get_widget(connection_pref_xml, "cb_profiles");
	GtkTreeIter iter;
	GtkTreeModel *store = gtk_combo_box_get_model(combo);
	if(gtk_combo_box_get_active_iter(combo,&iter))
	{
		char *value= NULL, *uid = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &uid, 1,&value, -1);
		cfg_set_single_value_as_string(profiles,uid,"name", 
				(char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(connection_pref_xml, "name_entry"))));
		g_free(uid);
		g_free(value);
		value = (char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(connection_pref_xml, "name_entry")));
		gtk_list_store_set(GTK_LIST_STORE(store), &iter, 1, value, -1);
		pl3_update_profiles_menu();
	}
}

void update_preferences_hostname(void)
{
	GtkComboBox *combo = (GtkComboBox *) glade_xml_get_widget(connection_pref_xml, "cb_profiles");
	GtkTreeIter iter;
	GtkTreeModel *store = gtk_combo_box_get_model(combo);
	if(gtk_combo_box_get_active_iter(combo,&iter))
	{
		char *value= NULL, *uid = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &uid, 1,&value, -1);
		cfg_set_single_value_as_string(profiles,uid,"hostname", 
				(char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(connection_pref_xml, "hostname_entry"))));
		g_free(uid);
		g_free(value);
	}
}
void update_preferences_portnumber(void)
{

	GtkComboBox *combo = (GtkComboBox *) glade_xml_get_widget(connection_pref_xml, "cb_profiles");
	GtkTreeIter iter;
	GtkTreeModel *store = gtk_combo_box_get_model(combo);
	if(gtk_combo_box_get_active_iter(combo,&iter))
	{
		char *value= NULL, *uid = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &uid, 1,&value, -1);
		cfg_set_single_value_as_int(profiles, uid, "portnumber",
				gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(connection_pref_xml, "port_spin"))));
		g_free(uid);
		g_free(value);
	}
}
void update_preferences_information(void)
{
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
	if(connection_pref_xml)
	{
		GtkComboBox *combo = (GtkComboBox *) glade_xml_get_widget(connection_pref_xml, "cb_profiles");
		GtkTreeIter iter;
		GtkTreeModel *store = gtk_combo_box_get_model(combo);
		if(gtk_combo_box_get_active_iter(combo,&iter))
		{
			char *uid = NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &uid, -1);                  	
			if(uid)
			{
				connection_set_current_profile(uid);
				if(!mpd_check_connected(connection))
				{
					if(!connect_to_mpd());
				}
				g_free(uid);
				pl3_update_profiles_menu();
			}
		}
	}
	else {
			if(!mpd_check_connected(connection))
			{
				if(!connect_to_mpd());
			}  
	}
}

void preferences_window_disconnect(GtkWidget *but)
{
	/* set that user doesn't want to connect */
	gmpc_connected = FALSE;
	debug_printf(DEBUG_INFO,"**DEBUG** disconnect\n");    
	mpd_disconnect(connection);
}

static void connection_pref_destroy(GtkWidget *container)
{
	if(connection_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(connection_pref_xml, "connection-vbox");
		gtk_container_remove(GTK_CONTAINER(container),vbox);
		g_object_unref(connection_pref_xml);
		connection_pref_xml = NULL;
	}
}

void connection_profiles_changed(GtkComboBox *combo, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *store = gtk_combo_box_get_model(combo);
	if(gtk_combo_box_get_active_iter(combo,&iter))
	{
		char *value= NULL, *uid = NULL, *string;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &uid, 1,&value, -1);
		/**
		 * Set name
		 */
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(connection_pref_xml, "name_entry")), value);
		/**
		 * Set hostname
		 */
		string = cfg_get_single_value_as_string_with_default(profiles,uid,"hostname","localhost");
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(connection_pref_xml, "hostname_entry")), string);
		cfg_free_string(string);
		/**
		 * Set port number 
		 */
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(connection_pref_xml, "port_spin")), 
				cfg_get_single_value_as_int_with_default(profiles,uid,"portnumber",6600));

		/**
		 * Set password check, and entry
		 */
		string = cfg_get_single_value_as_string_with_default(profiles, uid,"password", "");
		gtk_toggle_button_set_active((GtkToggleButton *)
				glade_xml_get_widget(connection_pref_xml, "ck_auth"), 
				cfg_get_single_value_as_int_with_default(profiles, uid, "useauth", 0));
		gtk_widget_set_sensitive(glade_xml_get_widget(connection_pref_xml, "entry_auth"), 
				cfg_get_single_value_as_int_with_default(profiles, uid, "useauth", 0));
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(connection_pref_xml, "entry_auth")),string);
		cfg_free_string(string);

		/**
		 * Only enable the rmeove button when there is more then 1 profile
		 */
		if(gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store),NULL) >1)
		{
			gtk_widget_set_sensitive(glade_xml_get_widget(connection_pref_xml, "remove_butt"), TRUE);
		}
		else
		{
			gtk_widget_set_sensitive(glade_xml_get_widget(connection_pref_xml, "remove_butt"), FALSE);
		}
		g_free(value);
		g_free(uid);
	}

}

void connection_add_profile(void)
{
	GtkComboBox *combo = (GtkComboBox *) glade_xml_get_widget(connection_pref_xml, "cb_profiles");
	GtkTreeIter iter;
	GtkTreeModel *store = gtk_combo_box_get_model(combo);
	gchar *value = g_strdup_printf("%u", g_random_int());
	gtk_list_store_append(GTK_LIST_STORE(store), &iter);
	gtk_list_store_set(GTK_LIST_STORE(store), &iter, 0, value, 1, "Name", -1);
	cfg_set_single_value_as_string(profiles,value,"name","Name");	
	g_free(value);
	gtk_combo_box_set_active_iter(GTK_COMBO_BOX(glade_xml_get_widget(connection_pref_xml, "cb_profiles")),&iter);
	pl3_update_profiles_menu();
}

void connection_remove_profile(void)
{
	GtkComboBox *combo = (GtkComboBox *) glade_xml_get_widget(connection_pref_xml, "cb_profiles");
	GtkTreeIter iter;
	GtkTreeModel *store = gtk_combo_box_get_model(combo);
	if(gtk_combo_box_get_active_iter(combo,&iter))
	{
		char *value= NULL, *uid = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &uid, 1,&value, -1);
		cfg_remove_class(profiles,uid);
		gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
		g_free(uid);
		g_free(value);
		gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(connection_pref_xml, "cb_profiles")),0);
		pl3_update_profiles_menu();
		connection_set_current_profile(NULL);
	}
}

static void connection_pref_construct(GtkWidget *container)
{
	gchar *def_profile = NULL;
	conf_mult_obj *mult, *iter;
	GtkCellRenderer *renderer = NULL;
	GtkListStore *store = NULL;
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
	connection_pref_xml = glade_xml_new(path, "connection-vbox",NULL);
	g_free(path);


	/**
	 * Profile selector
	 * uid, name
	 */
	def_profile = connection_get_current_profile();
	store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_combo_box_set_model(GTK_COMBO_BOX(glade_xml_get_widget(connection_pref_xml, "cb_profiles")), GTK_TREE_MODEL(store));
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(glade_xml_get_widget(connection_pref_xml, "cb_profiles")), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(glade_xml_get_widget(connection_pref_xml, "cb_profiles")), renderer, "text", 1, NULL);

	mult = cfg_get_class_list(profiles);
	if(mult)
	{
		int i = 0;
		iter = mult;
		do{
			GtkTreeIter piter;
			gchar *value = cfg_get_single_value_as_string_with_default(profiles, iter->key, "name", "Name");
			gtk_list_store_append(store, &piter);
			gtk_list_store_set(store, &piter, 0,iter->key, 1,value,-1);
			if(!strcmp(iter->key, def_profile))
			{	
				gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(connection_pref_xml, "cb_profiles")),i);
			}
			g_free(value);
			iter = iter->next;
			i++;
		}while(iter);
		cfg_free_multiple(mult);
	}
	else{
		GtkTreeIter piter;
		gchar *value = cfg_get_single_value_as_string_with_default(profiles, "Default", "name", "Default");
		gtk_list_store_append(store, &piter);
		gtk_list_store_set(store, &piter, 0,"Default", 1,value,-1);
		g_free(value);
		gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(connection_pref_xml, "cb_profiles")),0);
	}


	connection_profiles_changed(GTK_COMBO_BOX(glade_xml_get_widget(connection_pref_xml, "cb_profiles")),NULL);


	//	update_auth_settings();
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(connection_pref_xml, "timeout_spin")), 
			(gdouble)cfg_get_single_value_as_float_with_default(config,"connection", "timeout",DEFAULT_TIMEOUT));
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
void connection_set_current_profile(const char *uid)
{
	if(current)
	{
		g_free(current);
		current = NULL;
	}
	if(uid)
	{
		current = g_strdup(uid);
		cfg_set_single_value_as_string(config, "connection", "currentprofile",(char *)uid);
	}
}
char *connection_get_current_profile()
{
	gchar *value = NULL;
	/**
	 * Some hack to make it work even if the default value removed 
	 */
	if(!current)
	{
		current = cfg_get_single_value_as_string_with_default(config, "connection", "currentprofile", "Default");
		value = cfg_get_single_value_as_string(profiles, current, "name");
		/* if the default doesn't exists, start looking for the first profile */
		if(!value)
		{
			conf_mult_obj *mult = cfg_get_class_list(profiles);
			if(mult){
				current = g_strdup(mult->key);
				/**
				 * update saved value
				 */
				cfg_set_single_value_as_string(config, "connection", "currentprofile",current);
			}
			cfg_free_multiple(mult);
		}
		else {
			g_free(value);
		}

	}
	return current;
}

void connection_set_password(char *password)
{
	gchar *profile = connection_get_current_profile();
	/**
	 * if NULL, or length 0, then disable, else set
	 */ 
	if(password && password[0] != '\0')
	{
		cfg_set_single_value_as_int(profiles, profile, "useauth", TRUE);
		cfg_set_single_value_as_string(profiles, profile, "password", password);

	}
	else
	{
		cfg_set_single_value_as_int(profiles, profile, "useauth", FALSE);
	}
	g_free(profile);
}

int connection_use_auth()
{
	gchar *profile = connection_get_current_profile();
	int retv = cfg_get_single_value_as_int_with_default(profiles, profile, "useauth",0);
	return retv;
}

char *connection_get_hostname()
{
	gchar *profile = connection_get_current_profile();
	gchar *retv = cfg_get_single_value_as_string_with_default(profiles,profile,"hostname","localhost");
	return retv;
}
int connection_get_port()
{
	gchar *profile = connection_get_current_profile();
	int retv = cfg_get_single_value_as_int_with_default(profiles,profile,"portnumber", 6600);
	return retv;
}
char *connection_get_password()
{
	gchar *profile = connection_get_current_profile();
	gchar *retv = cfg_get_single_value_as_string_with_default(profiles, profile,"password", "");
	return retv;
}


