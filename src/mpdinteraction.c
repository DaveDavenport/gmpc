#include <stdio.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <string.h>
#include "main.h"
#include "config1.h"
#include "mpdinteraction.h"

G_LOCK_DEFINE (connecting_lock);

/* Server Settings plugin */
static void server_pref_construct(GtkWidget *);
static void server_pref_destroy(GtkWidget *);

/* Connection settings plugin */
static void connection_pref_construct(GtkWidget *container);
static void connection_pref_destroy(GtkWidget *container);

static void ServerStatusChangedCallback(MpdObj *mi, ChangedStatusType what, void *userdata);

static GladeXML *server_pref_xml = NULL;
gmpcPrefPlugin server_gpp = {
	server_pref_construct,
	server_pref_destroy
};

/* Glade prototypes, would be static without glade */
void xfade_enable_toggled(GtkToggleButton *but);
void xfade_time_changed(GtkSpinButton *but);
void entry_auth_changed(GtkEntry *entry);
void auth_enable_toggled(GtkToggleButton *but);

void preferences_window_autoconnect(GtkToggleButton *tog);
void preferences_window_connect(GtkWidget *but);
void preferences_window_disconnect(GtkWidget *but);
void update_preferences_name(GtkWidget *entry);
void update_preferences_hostname(GtkWidget *entry);
void update_preferences_portnumber(GtkWidget *wid);
void update_preferences_information(GtkWidget *wid);
void connection_profiles_changed(GtkComboBox *combo, gpointer data);
void connection_add_profile(GtkWidget *but);
void connection_remove_profile(GtkWidget *but);
void submenu_artist_clicked(GtkWidget *item);
void submenu_album_clicked(GtkWidget *item);
void submenu_genre_clicked(GtkWidget *item);
void submenu_dir_clicked(GtkWidget *item);

gmpcPlugin server_plug = {
	N_("Server Settings"), 	/** name */
	{1,1,1},			/** Version */
	GMPC_INTERNALL,		/** Plugin Type */
	0,					/** Internal Id */
	NULL,				/** path to plugin */
	NULL,				/** init */
        NULL,                           /** Destroy */
	NULL,				/** browser ext */
	ServerStatusChangedCallback,	/** status changed */
	NULL,
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


gmpcPrefPlugin connection_gpp = {
	connection_pref_construct,
	connection_pref_destroy
};

gmpcPlugin connection_plug = {
	N_("Connection"),       /* name */
	{1,1,1},                /* version */
	GMPC_INTERNALL,         /* type */
	0,                      /* id */
        NULL,                   /* path */
	NULL,                   /* init function */
	NULL,                   /* destroy function */
	NULL,                   /* browser */
	NULL,                   /* status changed */
	NULL,                   /* connection changed */
	&connection_gpp,        /* preferences */
	NULL,                   /* metadata */
	NULL,                   /* get_enabled */
	NULL                    /* set_enabled */
};

extern GladeXML *pl3_xml;
guint connecting_pulse = 0;
gboolean connecting_pulse_callback(void);
gboolean connecting_pulse_callback(void)
{
	if(pl3_xml)
	{
		GtkProgressBar *pb = (GtkProgressBar *)glade_xml_get_widget(pl3_xml, "pl3_progressbar");
		gtk_progress_bar_pulse(pb);
	}
	return TRUE;
}
/* this function doesnt use the start/stop_mpd_action because it the user doesnt want to see that */

int update_mpd_status()
{
	if(!mpd_check_connected(connection)) return TRUE;
	mpd_status_update(connection);

	return TRUE;
}

static int connected_to_mpd(mpd_Connection *mpd_conn)
{
	gtk_widget_hide(glade_xml_get_widget(pl3_xml, "pl3_progressbar"));
	g_source_remove(connecting_pulse);
	connecting_pulse = 0;
	G_UNLOCK(connecting_lock);
	printf("unlocking\n");
	if(connection)
	{
		mpd_connect_real(connection, mpd_conn);
	}

	return FALSE;
}
static void connection_thread(void)
{
	mpd_Connection *conn = mpd_newConnection(connection_get_hostname(),connection_get_port(), DEFAULT_TIMEOUT);
	g_idle_add((GSourceFunc)connected_to_mpd,conn);
	return;
}

/* the functiont that connects to mpd */
int connect_to_mpd()
{
	char *string = NULL;
    if(!G_TRYLOCK(connecting_lock))
    {
        debug_printf(DEBUG_ERROR, "Allready busy connecting to mpd.. not doing anything");
        return FALSE;
    }
	/**
	 * Set Hostname
	 */
	string = connection_get_hostname();
	mpd_set_hostname(connection,string);
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
	}
	else
	{
		mpd_set_password(connection,"");
	}
/*
	if(mpd_connect(connection) < 0)
	{
		debug_printf(DEBUG_INFO,"Connection failed\n");
		return TRUE;
	}
*/
	g_thread_create((GThreadFunc)connection_thread, NULL, FALSE,NULL);
	connecting_pulse = g_timeout_add(200,(GSourceFunc)(connecting_pulse_callback),NULL);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(glade_xml_get_widget(pl3_xml, "pl3_progressbar")), _("Connecting"));
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_progressbar"));

	/* Set the title 
	update_mpd_status();
	mpd_stats_update(connection);
	*/
	/* set that user wants to connect */
	gmpc_connected = TRUE;

	return FALSE;
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
		gtk_tree_model_get(model, &iter, ENABLED_COL, &state, ID_COL, &id,-1);
		state = !state;
		mpd_server_set_output_device(connection, id, state);

		gtk_list_store_set(GTK_LIST_STORE(model), &iter, ENABLED_COL, state, -1);
	}
	gtk_tree_path_free(path);
}

static void create_outputs_tree(void)
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
	g_signal_connect(GTK_OBJECT(cell), "toggled",	GTK_SIGNAL_FUNC(outputs_toggled), tree);

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
static void update_outputs_settings(void)
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
static void xfade_update(void)
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

/**
 * Helper functions
 */
void play_path(const gchar *path)
{
	if(path)
	{
		if(mpd_server_check_command_allowed(connection, "playlistfind")== MPD_SERVER_COMMAND_ALLOWED)
		{
			MpdData *data = NULL;
			mpd_playlist_search_start(connection,TRUE);
			mpd_playlist_search_add_constraint(connection, MPD_TAG_ITEM_FILENAME,path);
			data = mpd_playlist_search_commit(connection);
			if(data)
			{
				mpd_player_play_id(connection, data->song->id);
				mpd_data_free(data);
				return;
			}
		}
		if(mpd_server_check_command_allowed(connection, "addid") == MPD_SERVER_COMMAND_ALLOWED){
			int songid = mpd_playlist_add_get_id(connection, (gchar *)path);
			if(songid >= 0) {
				mpd_player_play_id(connection, songid);
			}
		}
	}
}

void add_artist(const gchar *artist)
{
	MpdData *data = NULL;
	/* Check artist */
	g_return_if_fail(artist != NULL);

	mpd_database_search_start(connection,TRUE);
	mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST,artist);
	data = mpd_database_search_commit(connection);
	if(data)
	{
		for(;data;data = mpd_data_get_next(data))
		{
			mpd_playlist_queue_add(connection, data->song->file);
		}
		mpd_playlist_queue_commit(connection);
	}
}
void add_album(const gchar *artist,const gchar *album)
{
	MpdData *data = NULL;
	/* Check artist */
	g_return_if_fail(artist != NULL);
	g_return_if_fail(album != NULL);

	mpd_database_search_start(connection,TRUE);
	mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST,artist);
	mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ALBUM,album);
	data = mpd_database_search_commit(connection);
	if(data)
	{
		for(;data;data = mpd_data_get_next(data))
		{
			mpd_playlist_queue_add(connection, data->song->file);
		}
		mpd_playlist_queue_commit(connection);
	}
}
void add_genre(const gchar *genre)
{
	MpdData *data = NULL;
	/* Check artist */
	g_return_if_fail(genre != NULL);

	mpd_database_search_start(connection,TRUE);
	mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_GENRE,genre);
	data = mpd_database_search_commit(connection);
	if(data)
	{
		for(;data;data = mpd_data_get_next(data))
		{
			mpd_playlist_queue_add(connection, data->song->file);
		}
		mpd_playlist_queue_commit(connection);
	}
}

void add_directory(const gchar *path)
{
	gchar *dirpath = g_path_get_dirname(path);
	MpdData *data =  mpd_database_get_directory(connection,dirpath);
	for(;data;data = mpd_data_get_next(data))
	{
		if(data->type == MPD_DATA_TYPE_SONG)
		{
			mpd_playlist_queue_add(connection, data->song->file);
		}
	}
	mpd_playlist_queue_commit(connection);
	g_free(dirpath);
}



void ServerStatusChangedCallback(MpdObj *mi, ChangedStatusType what, void *userdata)
{
	if(!server_pref_xml)return;
	if(what&MPD_CST_CROSSFADE)
	{
		xfade_update();
	}
    if(what&MPD_CST_OUTPUT)
    {
        update_outputs_settings();
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
	q_free(path);

	if(server_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(server_pref_xml, "server-vbox");
		create_outputs_tree();
		update_outputs_settings();
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
static void gmpc_profiles_changed_pref_win(GmpcProfiles *prof,const int changed, const int col, const char * id, GladeXML *xml)
{
	if(changed == PROFILE_ADDED)
	{
		GtkComboBox *combo = (GtkComboBox *) glade_xml_get_widget(xml, "cb_profiles");
		GtkTreeIter iter;
		const char *name = gmpc_profiles_get_name(prof, id);
		GtkTreeModel *store = gtk_combo_box_get_model(combo);
		gtk_list_store_append(GTK_LIST_STORE(store), &iter);
		gtk_list_store_set(GTK_LIST_STORE(store), &iter, 0, id, 1, name,-1);
	}
	if(changed == PROFILE_REMOVED)
	{
		/* TODO: */

	}
	if(changed == PROFILE_COL_CHANGED)
	{
		GtkTreeIter iter;
		GtkComboBox *combo = (GtkComboBox *) glade_xml_get_widget(xml, "cb_profiles");
		GtkListStore *store = (GtkListStore *)gtk_combo_box_get_model(combo);
		/* tell it to update all the information in the view. 
		 * might be to much work, so check id*/
		gchar *uid;
		if(gtk_combo_box_get_active_iter(combo,&iter) && id)
		{

			gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &uid, -1);
			if(strcmp(uid, id) == 0)
			{
				connection_profiles_changed(combo, NULL);
			}
			g_free(uid);
		}
	}
}

static void gmpc_connection_changed_pref_win(GmpcConnection *object, MpdObj *mi, int connected, GladeXML *xml)
{ 
	if(!connected)
	{
		gtk_widget_set_sensitive(glade_xml_get_widget(xml, "bt_con"), TRUE);
		gtk_widget_set_sensitive(glade_xml_get_widget(xml, "bt_dis"), FALSE);
	}
	else
	{
		gtk_widget_set_sensitive(glade_xml_get_widget(xml, "bt_con"), FALSE);
		gtk_widget_set_sensitive(glade_xml_get_widget(xml, "bt_dis"), TRUE);
	}
}
void entry_auth_changed(GtkEntry *entry)
{
	GladeXML *xml = glade_get_widget_tree(GTK_WIDGET(entry));
	GtkWidget *vbox = glade_xml_get_widget(xml, "connection-vbox");
	gulong *a = g_object_get_data(G_OBJECT(vbox),"profile-signal-handler"); 
	GtkComboBox *combo = (GtkComboBox *)glade_xml_get_widget(xml, "cb_profiles");
	GtkTreeIter iter;
	GtkListStore *store = (GtkListStore *)gtk_combo_box_get_model(combo);
	if(gtk_combo_box_get_active_iter(combo,&iter))
	{
		gchar *value= NULL, *uid = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &uid, 1,&value, -1);
		g_signal_handler_block(G_OBJECT(gmpc_profiles), *a);
		gmpc_profiles_set_password(gmpc_profiles, uid, (char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml, "entry_auth"))));
		g_signal_handler_unblock(G_OBJECT(gmpc_profiles), *a);
		q_free(uid);
		q_free(value);
	}
}

void auth_enable_toggled(GtkToggleButton *but)
{
	GladeXML *xml = glade_get_widget_tree(GTK_WIDGET(but));
	GtkWidget *vbox = glade_xml_get_widget(xml, "connection-vbox");
	gulong *a = g_object_get_data(G_OBJECT(vbox),"profile-signal-handler"); 
	GtkComboBox *combo = (GtkComboBox *) glade_xml_get_widget(xml, "cb_profiles");
	GtkTreeIter iter;
	GtkTreeModel *store = gtk_combo_box_get_model(combo);
	if(gtk_combo_box_get_active_iter(combo,&iter))
	{
		char *value= NULL, *uid = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &uid, 1,&value, -1);
		g_signal_handler_block(G_OBJECT(gmpc_profiles), *a);

		gmpc_profiles_set_do_auth(gmpc_profiles, uid, gtk_toggle_button_get_active(but));
		gtk_widget_set_sensitive(glade_xml_get_widget(xml, "entry_auth"), 
				gmpc_profiles_get_do_auth(gmpc_profiles, uid));	

		g_signal_handler_unblock(G_OBJECT(gmpc_profiles), *a);
		q_free(uid);
		q_free(value);
	}
}

void update_preferences_name(GtkWidget *entry)
{
	GladeXML *xml = glade_get_widget_tree(GTK_WIDGET(entry));
	GtkWidget *vbox = glade_xml_get_widget(xml, "connection-vbox");
	gulong *a = g_object_get_data(G_OBJECT(vbox),"profile-signal-handler"); 
	GtkComboBox *combo = (GtkComboBox *) glade_xml_get_widget(xml, "cb_profiles");
	GtkTreeIter iter;
	GtkTreeModel *store = gtk_combo_box_get_model(combo);
	if(gtk_combo_box_get_active_iter(combo,&iter))
	{
		char *value= NULL, *uid = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &uid, 1,&value, -1);
		g_signal_handler_block(G_OBJECT(gmpc_profiles), *a);

		gmpc_profiles_set_name(gmpc_profiles, uid,
				(char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml, "name_entry"))));
		g_signal_handler_unblock(G_OBJECT(gmpc_profiles), *a);

		q_free(uid);
		q_free(value);
		value = (char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml, "name_entry")));
		gtk_list_store_set(GTK_LIST_STORE(store), &iter, 1, value, -1);
	}
}

void update_preferences_hostname(GtkWidget *entry)
{
	GladeXML *xml = glade_get_widget_tree(GTK_WIDGET(entry));
	GtkComboBox *combo = (GtkComboBox *) glade_xml_get_widget(xml, "cb_profiles");
	GtkWidget *vbox = glade_xml_get_widget(xml, "connection-vbox");
	gulong *a = g_object_get_data(G_OBJECT(vbox),"profile-signal-handler"); 
	GtkTreeIter iter;
	GtkTreeModel *store = gtk_combo_box_get_model(combo);

	if(gtk_combo_box_get_active_iter(combo,&iter))
	{
		char *value= NULL, *uid = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &uid, 1,&value, -1);
		g_signal_handler_block(G_OBJECT(gmpc_profiles), *a);

		gmpc_profiles_set_hostname(gmpc_profiles, uid,
				(char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml, "hostname_entry"))));
		g_signal_handler_unblock(G_OBJECT(gmpc_profiles), *a);

		q_free(uid);
		q_free(value);
	}
}
void update_preferences_portnumber(GtkWidget *wid)
{
	GladeXML *xml = glade_get_widget_tree(GTK_WIDGET(wid));
	GtkWidget *vbox = glade_xml_get_widget(xml, "connection-vbox");
	gulong *a = g_object_get_data(G_OBJECT(vbox),"profile-signal-handler"); 
	GtkComboBox *combo = (GtkComboBox *) glade_xml_get_widget(xml, "cb_profiles");
	GtkTreeIter iter;
	GtkTreeModel *store = gtk_combo_box_get_model(combo);
	if(gtk_combo_box_get_active_iter(combo,&iter))
	{
		char *value= NULL, *uid = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &uid, 1,&value, -1);
		g_signal_handler_block(G_OBJECT(gmpc_profiles), *a);
		gmpc_profiles_set_port(gmpc_profiles, uid,
				gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(xml, "port_spin"))));
		g_signal_handler_unblock(G_OBJECT(gmpc_profiles), *a);
		q_free(uid);
		q_free(value);
	}
}
void update_preferences_information(GtkWidget *wid)
{
	GladeXML *xml = glade_get_widget_tree(GTK_WIDGET(wid));
	cfg_set_single_value_as_float(config,"connection", "timeout",
			(float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(glade_xml_get_widget(xml,"timeout_spin"))));

	/* update timeout live */
	if(mpd_check_connected(connection))
	{
		mpd_set_connection_timeout(connection, 
				(float)gtk_spin_button_get_value(GTK_SPIN_BUTTON(glade_xml_get_widget(xml, "timeout_spin"))));
	}
}

void preferences_window_autoconnect(GtkToggleButton *tog)
{
	cfg_set_single_value_as_int(config, "connection", "autoconnect", gtk_toggle_button_get_active(tog));
}

void preferences_window_connect(GtkWidget *but)
{
	GladeXML *xml = glade_get_widget_tree(GTK_WIDGET(but));
	if(xml)
	{
		GtkComboBox *combo = (GtkComboBox *) glade_xml_get_widget(xml, "cb_profiles");
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
				q_free(uid);
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
	GtkWidget *widget = gtk_bin_get_child(GTK_BIN(container));
	if(widget)
	{
		GladeXML *xml = glade_get_widget_tree(widget);
		if(xml)
		{
			gtk_container_remove(GTK_CONTAINER(container),widget);
			g_object_unref(xml);
		}
	}
	else
	{
		debug_printf(DEBUG_ERROR, "No child widget to destroy\n");
	}
}
void connection_profiles_changed(GtkComboBox *combo, gpointer data)
{
	GladeXML *xml = glade_get_widget_tree(GTK_WIDGET(combo));
	GtkTreeIter iter;
	GtkTreeModel *store = gtk_combo_box_get_model(combo);
	if(gtk_combo_box_get_active_iter(combo,&iter))
	{
		char *value= NULL, *uid = NULL, *string;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &uid, 1,&value, -1);


		/**
		 * Set name
		 */
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml, "name_entry")), value);
		/**
		 * Set hostname
		 */
		string = g_strdup(gmpc_profiles_get_hostname(gmpc_profiles, uid));
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml, "hostname_entry")), 
				string);
		g_free(string);
		/**
		 * Set port number 
		 */
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(xml, "port_spin")), 
				gmpc_profiles_get_port(gmpc_profiles, uid));

		/**
		 * Set password check, and entry
		 */
		string = g_strdup(gmpc_profiles_get_password(gmpc_profiles, uid));
		gtk_toggle_button_set_active((GtkToggleButton *)
				glade_xml_get_widget(xml, "ck_auth"), 
				gmpc_profiles_get_do_auth(gmpc_profiles, uid));
		gtk_widget_set_sensitive(glade_xml_get_widget(xml, "entry_auth"), 
				gmpc_profiles_get_do_auth(gmpc_profiles, uid));

		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml, "entry_auth")),/*string);*/
			string);
		g_free(string);

		/**
		 * Only enable the rmeove button when there is more then 1 profile
		 */
		if(gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store),NULL) >1)
		{
			gtk_widget_set_sensitive(glade_xml_get_widget(xml, "remove_butt"), TRUE);
		}
		else
		{
			gtk_widget_set_sensitive(glade_xml_get_widget(xml, "remove_butt"), FALSE);
		}
		q_free(value);
		q_free(uid);
	}

}

void connection_add_profile(GtkWidget *but)
{
	gchar *value = NULL;
	GladeXML *xml = glade_get_widget_tree(GTK_WIDGET(but));
	GtkWidget *vbox = glade_xml_get_widget(xml, "connection-vbox");
	gulong *a = g_object_get_data(G_OBJECT(vbox),"profile-signal-handler"); 
	GtkComboBox *combo = (GtkComboBox *) glade_xml_get_widget(xml, "cb_profiles");
	GtkTreeIter iter;
	GtkTreeModel *store = gtk_combo_box_get_model(combo);

	g_signal_handler_block(G_OBJECT(gmpc_profiles), *a);
	value = gmpc_profiles_create_new_item(gmpc_profiles, NULL); /*g_strdup_printf("%u", g_random_int());*/
	gtk_list_store_append(GTK_LIST_STORE(store), &iter);
	gtk_list_store_set(GTK_LIST_STORE(store), &iter, 0, value, 1, "Name", -1);
	gtk_combo_box_set_active_iter(GTK_COMBO_BOX(glade_xml_get_widget(xml, "cb_profiles")),&iter);
	g_signal_handler_unblock(G_OBJECT(gmpc_profiles), *a);
}

void connection_remove_profile(GtkWidget *but)
{
	GladeXML *xml = glade_get_widget_tree(GTK_WIDGET(but));
	GtkComboBox *combo = (GtkComboBox *) glade_xml_get_widget(xml, "cb_profiles");
	GtkTreeIter iter;
	GtkTreeModel *store = gtk_combo_box_get_model(combo);
	if(gtk_combo_box_get_active_iter(combo,&iter))
	{
		char *value= NULL, *uid = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &uid, 1,&value, -1);
		gmpc_profiles_remove_item(gmpc_profiles, uid);
		gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
		q_free(uid);
		q_free(value);
		gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(xml, "cb_profiles")),0);
	}
}
static void destroy_profile_signal_handler(gpointer box)
{
	gulong *a = box;
	g_signal_handler_disconnect(G_OBJECT(gmpc_profiles), *a);
	g_free(a);
}
static void destroy_connection_signal_handler(gpointer box)
{
	gulong *a = box;
	g_signal_handler_disconnect(G_OBJECT(gmpcconn), *a);
	g_free(a);
}




static void connection_pref_construct(GtkWidget *container)
{
	gulong *a;
	gchar *def_profile = NULL;
	GList *mult, *iter;
	GtkWidget *vbox = NULL;
	GtkCellRenderer *renderer = NULL;
	GtkListStore *store = NULL;
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
	GladeXML *connection_pref_xml = glade_xml_new(path, "connection-vbox",NULL);
	q_free(path);

	vbox = glade_xml_get_widget(connection_pref_xml, "connection-vbox");
	/**
	 * Profile selector
	 * uid, name
	 */
	def_profile = gmpc_profiles_get_current(gmpc_profiles);
	store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_combo_box_set_model(GTK_COMBO_BOX(glade_xml_get_widget(connection_pref_xml, "cb_profiles")), GTK_TREE_MODEL(store));
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(glade_xml_get_widget(connection_pref_xml, "cb_profiles")), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(glade_xml_get_widget(connection_pref_xml, "cb_profiles")), renderer, "text", 1, NULL);

	mult = gmpc_profiles_get_profiles_ids(gmpc_profiles); 
	if(mult)
	{
		int i = 0;
		iter = mult;
		do{
			GtkTreeIter piter;
			const gchar *value = gmpc_profiles_get_name(gmpc_profiles, (char *)iter->data); 
			gtk_list_store_append(store, &piter);
			gtk_list_store_set(store, &piter, 0,iter->data, 1,value,-1);
			if(!strcmp((char *)(iter->data), def_profile))
			{	
				gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(connection_pref_xml, "cb_profiles")),i);
			}
			i++;
		}while((iter = g_list_next(iter)));
		g_list_foreach(mult, (GFunc)g_free, NULL);
		g_list_free(mult);
	}
	else{
		/*	GtkTreeIter piter;
			gchar *value = cfg_get_single_value_as_string_with_default(profiles, "Default", "name", "Default");
			gtk_list_store_append(store, &piter);
			gtk_list_store_set(store, &piter, 0,"Default", 1,value,-1);
			q_free(value);
			gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(connection_pref_xml, "cb_profiles")),0);
			*/
	}
	q_free(def_profile);

	connection_profiles_changed(GTK_COMBO_BOX(glade_xml_get_widget(connection_pref_xml, "cb_profiles")),NULL);


	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(connection_pref_xml, "timeout_spin")), 
			(gdouble)cfg_get_single_value_as_float_with_default(config,"connection", "timeout",DEFAULT_TIMEOUT));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(connection_pref_xml, "ck_autocon")), 
			cfg_get_single_value_as_int_with_default(config,"connection", "autoconnect", 0));


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


	a = g_malloc0(sizeof(*a));
	*a= g_signal_connect(G_OBJECT(gmpc_profiles), "changed",
			G_CALLBACK(gmpc_profiles_changed_pref_win), connection_pref_xml);
	g_object_set_data_full(G_OBJECT(vbox), "profile-signal-handler", a, destroy_profile_signal_handler);

	a = g_malloc0(sizeof(*a));
	*a= g_signal_connect(G_OBJECT(gmpcconn), "connection-changed",
			G_CALLBACK(gmpc_connection_changed_pref_win), connection_pref_xml);
	g_object_set_data_full(G_OBJECT(vbox), "conn-signal-handler", a, destroy_connection_signal_handler);

}
void connection_set_current_profile(const char *uid)
{
	gmpc_profiles_set_current(gmpc_profiles, uid);
}
void connection_set_password(char *password)
{
	gchar *profile = gmpc_profiles_get_current(gmpc_profiles);
	/**
	 * if NULL, or length 0, then disable, else set
	 */ 
	if(password && password[0] != '\0')
	{
	  gmpc_profiles_set_password(gmpc_profiles, profile, password);
	  gmpc_profiles_set_do_auth(gmpc_profiles, profile, TRUE);

	}
	else
	{
	  gmpc_profiles_set_password(gmpc_profiles, profile, NULL);
	  gmpc_profiles_set_do_auth(gmpc_profiles, profile, FALSE);
	}
	q_free(profile);
}

int connection_use_auth()
{
  int retv;
	gchar *profile = gmpc_profiles_get_current(gmpc_profiles);
	retv  = gmpc_profiles_get_do_auth(gmpc_profiles, profile);
  q_free(profile);
  return retv;
}

char *connection_get_hostname()
{
	gchar *profile = gmpc_profiles_get_current(gmpc_profiles);
	gchar *retv  = gmpc_profiles_get_hostname(gmpc_profiles, profile);
    g_free(profile);

	return retv;
}
int connection_get_port()
{
	int retv;
	gchar *profile = gmpc_profiles_get_current(gmpc_profiles);
	retv  = gmpc_profiles_get_port(gmpc_profiles, profile);
	q_free(profile);
	return retv;
}
char *connection_get_password()
{
	gchar *profile = gmpc_profiles_get_current(gmpc_profiles);
	gchar *retv  = gmpc_profiles_get_password(gmpc_profiles, profile);
	g_free(profile);
	return retv;
}

/**
 * Helper menu functions *
 */
void submenu_artist_clicked(GtkWidget *item)
{
	gchar *artist = g_object_get_data(G_OBJECT(item), "artist");
	add_artist(artist);	
}
void submenu_album_clicked(GtkWidget *item)
{
	gchar *artist = g_object_get_data(G_OBJECT(item), "artist");
	gchar *album = g_object_get_data(G_OBJECT(item), "album");
	add_album(artist,album);	
}
void submenu_genre_clicked(GtkWidget *item)
{
	gchar *genre = g_object_get_data(G_OBJECT(item), "genre");
	add_genre(genre);	
}

void submenu_dir_clicked(GtkWidget *item)
{
	gchar *dir = g_object_get_data(G_OBJECT(item), "path");
	add_directory(dir);	
}
void submenu_for_song(GtkWidget *menu, mpd_Song *song)
{
	GtkWidget *sitem;
	GtkWidget *item;
	GtkWidget *smenu;
	smenu  = gtk_menu_new();
	if(song->artist && song->album) 
	{
		/* Add all from album */
		sitem = gtk_image_menu_item_new_with_label(_("All from album"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(sitem), gtk_image_new_from_icon_name("media-album", GTK_ICON_SIZE_MENU));
		g_object_set_data_full(G_OBJECT(sitem), "artist", g_strdup(song->artist), g_free);
		g_object_set_data_full(G_OBJECT(sitem), "album", g_strdup(song->album), g_free);
		g_signal_connect(G_OBJECT(sitem), "activate", G_CALLBACK(submenu_album_clicked), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(smenu), sitem);
		gtk_widget_show(sitem);

	}
	if(song->artist )
	{
		/* Add all from artist */
		sitem = gtk_image_menu_item_new_with_label(_("All from artist"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(sitem), gtk_image_new_from_icon_name("media-artist", GTK_ICON_SIZE_MENU));
		g_object_set_data_full(G_OBJECT(sitem), "artist", g_strdup(song->artist), g_free);
		g_signal_connect(G_OBJECT(sitem), "activate", G_CALLBACK(submenu_artist_clicked), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(smenu), sitem);
		gtk_widget_show(sitem);
	}
	if(song->genre)
	{
		/* Add all from genre */
		sitem = gtk_menu_item_new_with_label(_("All from genre"));
		g_object_set_data_full(G_OBJECT(sitem), "genre", g_strdup(song->genre), g_free);
		g_signal_connect(G_OBJECT(sitem), "activate", G_CALLBACK(submenu_genre_clicked), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(smenu), sitem);
		gtk_widget_show(sitem);

	}
	/* Add all from file */
	sitem = gtk_image_menu_item_new_with_label(_("All from same directory"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(sitem), gtk_image_new_from_icon_name("gtk-directory",GTK_ICON_SIZE_MENU));
	g_object_set_data_full(G_OBJECT(sitem), "path", g_strdup(song->file), g_free);
	g_signal_connect(G_OBJECT(sitem), "activate", G_CALLBACK(submenu_dir_clicked), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(smenu), sitem);
	gtk_widget_show(sitem);
	/* Create sub menu */
	/* Add */
	item = gtk_menu_item_new_with_label(_("Add more"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), smenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);
	
	gtk_widget_show(smenu);

}
