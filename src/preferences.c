#include <gtk/gtk.h>
#include <glade/glade.h>
#include <string.h>
#include <time.h>
#include "main.h"
#include "playlist3.h"
#include "tag-browser.h"
#include "config1.h"
extern config_obj *config;

enum
{
        ENABLED_COL,
	NAME_COL,
        ID_COL,
        N_COLUMNS
};



GladeXML *xml_preferences_window;
gboolean running = 0, connected = 0;
void popup_timeout_changed();
void tray_enable_toggled(GtkToggleButton *but);
void preferences_update();
void popup_enable_toggled(GtkToggleButton *but);
void popup_position_changed(GtkOptionMenu *om);
void update_auth_settings();
void preferences_window_connect(GtkWidget *but);
void preferences_window_disconnect(GtkWidget *but);
void update_popup_settings();
void update_tray_settings();
void update_server_settings();
void auth_enable_toggled(GtkToggleButton *but);
void entry_auth_changed(GtkEntry *entry);
void xfade_time_changed(GtkSpinButton *but);
void xfade_enable_toggled(GtkToggleButton *bug);
void set_display_settings();
void update_display_settings();
void create_outputs_tree();
void outputs_toggled(GtkCellRendererToggle *cell, gchar *path_str, GtkTreeView *view);
void update_outputs_settings();
void create_osb_tree();
void osb_add_edit_source();
int update_osb_tree();
void update_server_stats();
/* advanced id3 browser */
void pref_id3b_init();
void pref_id3b_fill();

void create_preferences_window()
{
	GtkWidget *dialog;
	char *string = NULL;
	if(running)
	{
		if(xml_preferences_window == NULL)
		{
			running = 0;
		} 
		else
		{
			dialog = glade_xml_get_widget(xml_preferences_window, "preferences_window");
			gtk_window_present(GTK_WINDOW(dialog));
			return;
		}
	}
	xml_preferences_window = glade_xml_new(GLADE_PATH"gmpc.glade", "preferences_window", NULL);
	/* check for errors and axit when there is no gui file */
	if(xml_preferences_window == NULL)  g_error("Couldnt initialize GUI. Please check installation\n");


	/* set info from struct */
	/* hostname */
	string = cfg_get_single_value_as_string_with_default(config, "connection","hostname","localhost");
	gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml_preferences_window, "hostname_entry")), string);
	cfg_free_string(string);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "port_spin")), 
			cfg_get_single_value_as_int_with_default(config, "connection","portnumber",6600));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "timeout_spin")), 
			cfg_get_single_value_as_float_with_default(config,"connection", "timeout",1.0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_preferences_window, "ck_autocon")), 
			cfg_get_single_value_as_int_with_default(config,"connection", "autoconnect", 0));
	create_outputs_tree();
	create_osb_tree();
	update_osb_tree();
	dialog = glade_xml_get_widget(xml_preferences_window, "preferences_window");
	gtk_widget_show_all(GTK_WIDGET(dialog));
	running = 1;

	update_server_settings();

	/* set the right sensitive stuff */
	if(!mpd_ob_check_connected(connection))
	{
		gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "bt_con"), TRUE);
		gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "bt_dis"), FALSE);	    
	}
	else
	{
		gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "bt_con"), FALSE);
		gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "bt_dis"), TRUE);	    
	}
	update_popup_settings();
	update_tray_settings();
	update_auth_settings();
	update_server_stats();
	pref_id3b_init();

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_preferences_window, "osb_ck_enable")), 
			cfg_get_single_value_as_int_with_default(config,"osb", "enable", 0));        
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_preferences_window, "csl_ck_enable")), 
			cfg_get_single_value_as_int_with_default(config,"playlist", "custom_stream_enable", 1));                          	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_preferences_window, "ck_of")), 
			cfg_get_single_value_as_int_with_default(config,"playlist", "open-to-position", 0));                          	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_preferences_window, "ck_ps")), 
			cfg_get_single_value_as_int_with_default(config,"playlist3", "st_cur_song", 0));                          		

#ifdef ENABLE_GNOME_VFS
	gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "vbox_stream"),TRUE);	
	gtk_widget_hide(glade_xml_get_widget(xml_preferences_window, "hbox_stream"));	

#endif 
	
	glade_xml_signal_autoconnect(xml_preferences_window);	

}

void set_browser_format()
{
	char *string = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
	char *format = edit_song_markup(string);
	cfg_free_string(string);
	if(format != NULL)
	{
		cfg_set_single_value_as_string(config, "playlist","browser_markup",format);
	}
	g_free(format);
}

void set_playlist_format()
{
	char *string = cfg_get_single_value_as_string_with_default(config, "playlist", "markup",DEFAULT_PLAYLIST_MARKUP);
	char *format = edit_song_markup(string);
	cfg_free_string(string);
	if(format != NULL)
	{
		cfg_set_single_value_as_string(config, "playlist","markup",format);
	}
	g_free(format);
}

void set_player_format()
{
	char *string = cfg_get_single_value_as_string_with_default(config, "player", "display_markup",	DEFAULT_PLAYER_MARKUP);
	char *format = edit_song_markup(string);
	cfg_free_string(string);

	if(format != NULL)
	{
		cfg_set_single_value_as_string(config, "player","display_markup",format);
	}
	g_free(format);
}


void set_display_default_sd()
{
	gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml_preferences_window, "en_sd")), DEFAULT_PLAYER_MARKUP);
}

/* destory the preferences window */
void preferences_window_destroy()
{
	GtkWidget *dialog = glade_xml_get_widget(xml_preferences_window, "preferences_window");
	gtk_widget_destroy(dialog);
	g_object_unref(xml_preferences_window);
	xml_preferences_window = NULL;
	running = 0;
}

void update_preferences_information()
{
	cfg_set_single_value_as_string(config,"connection","hostname", 
			(char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml_preferences_window, "hostname_entry"))));
	cfg_set_single_value_as_int(config, "connection", "portnumber",
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "port_spin"))));
	cfg_set_single_value_as_float(config,"connection","timeout",
			gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "timeout_spin"))));
	/* update timeout live */
	if(mpd_ob_check_connected(connection))
	{
		mpd_ob_set_connection_timeout(connection, 
			gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "timeout_spin"))));
	}
}

void preferences_window_autoconnect(GtkToggleButton *tog)
{
	cfg_set_single_value_as_int(config, "connection", "autoconnect", gtk_toggle_button_get_active(tog));
}

void preferences_window_connect(GtkWidget *but)
{
	debug_printf(DEBUG_INFO,"*DEBUG** connect\n");
	if(!mpd_ob_check_connected(connection))
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
	/*	gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_preferences_window, "db_lu")),"n/a");*/
}

/* this function is called from the main loop, it makes sure stuff is up-to-date(r) */
void preferences_update()
{
	if(!running)return;

	
	if(mpd_ob_check_connected(connection) != connected)
	{
		update_server_stats();
		if(!mpd_ob_check_connected(connection))
		{
			gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "bt_con"), TRUE);
			gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "bt_dis"), FALSE);	    
			gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "vb_server_set"), FALSE);
			gtk_widget_show(glade_xml_get_widget(xml_preferences_window, "hb_warning_mesg"));
		}
		else
		{
			gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "bt_con"), FALSE);
			gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "bt_dis"), TRUE);	   
			gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "vb_server_set"), TRUE);
			gtk_widget_hide(glade_xml_get_widget(xml_preferences_window, "hb_warning_mesg"));
		}
		update_outputs_settings();
		connected = mpd_ob_check_connected(connection);
	} 
}

void popup_enable_toggled(GtkToggleButton *but)
{
	cfg_set_single_value_as_int(config, "tray-icon", "do-popup", gtk_toggle_button_get_active(but));
}


void popup_position_changed(GtkOptionMenu *om)
{
	cfg_set_single_value_as_int(config, "tray-icon", "popup-location", gtk_option_menu_get_history(om));
}

void popup_timeout_changed()
{
	cfg_set_single_value_as_int(config, "tray-icon", "popup-timeout", 
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "popup_timeout"))));
}

void update_popup_settings()
{
	gtk_toggle_button_set_active((GtkToggleButton *)
			glade_xml_get_widget(xml_preferences_window, "ck_popup_enable"),
			cfg_get_single_value_as_int_with_default(config, "tray-icon", "do-popup", 1));
	gtk_option_menu_set_history((GtkOptionMenu *)
			glade_xml_get_widget(xml_preferences_window, "om_popup_position"),
			cfg_get_single_value_as_int_with_default(config, "tray-icon", "popup-location", 0));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "popup_timeout")),
			cfg_get_single_value_as_int_with_default(config, "tray-icon", "popup-timeout", 5));
}


void update_server_settings()
{
	if(!mpd_ob_check_connected(connection))
	{
		gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "vb_server_set"), FALSE);
		gtk_widget_show(glade_xml_get_widget(xml_preferences_window, "hb_warning_mesg"));
		return;
	}
	else 
	{
		gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "vb_server_set"), TRUE);
		gtk_widget_hide(glade_xml_get_widget(xml_preferences_window, "hb_warning_mesg"));
	}	

	if(mpd_ob_status_get_crossfade(connection) == 0)
	{
		gtk_toggle_button_set_active((GtkToggleButton *)
				glade_xml_get_widget(xml_preferences_window, "cb_fading"), FALSE);
		gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "sb_fade_time"), FALSE);
	}
	else {
		gtk_toggle_button_set_active((GtkToggleButton *)
				glade_xml_get_widget(xml_preferences_window, "cb_fading"), TRUE);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "sb_fade_time")), mpd_ob_status_get_crossfade(connection));
		gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "sb_fade_time"), TRUE);

	}
	update_outputs_settings();
}

/* this sets all the settings in the notification area preferences correct */
void update_tray_settings()
{
	gtk_toggle_button_set_active((GtkToggleButton *)
			glade_xml_get_widget(xml_preferences_window, "ck_tray_enable"), 
			cfg_get_single_value_as_int_with_default(config, "tray-icon", "enable", DEFAULT_TRAY_ICON_ENABLE));
	gtk_toggle_button_set_active((GtkToggleButton *)
			glade_xml_get_widget(xml_preferences_window, "ck_hide_player"), 
			cfg_get_single_value_as_int_with_default(config, "player", "hide-startup", DEFAULT_HIDE_ON_STARTUP));

}

void tray_enable_toggled(GtkToggleButton *but)
{
	g_print("changing tray icon %i\n", gtk_toggle_button_get_active(but));
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





void entry_auth_changed(GtkEntry *entry)
{
	cfg_set_single_value_as_string(config, "connection","password",
			(char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml_preferences_window, "entry_auth"))));
}

void auth_enable_toggled(GtkToggleButton *but)
{
	cfg_set_single_value_as_int(config, "connection", "useauth",gtk_toggle_button_get_active(but));
	gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "entry_auth"), 
			cfg_get_single_value_as_int_with_default(config, "connection","useauth",0));	
}

void xfade_enable_toggled(GtkToggleButton *but)
{

	int bool1  = gtk_toggle_button_get_active(but);
	gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "sb_fade_time"), bool1);
	if(bool1)
	{
		int fade_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "sb_fade_time")));	
		mpd_ob_status_set_crossfade(connection, fade_time);
	}
	else 
	{
		mpd_ob_status_set_crossfade(connection, 0);
	}	
}

void xfade_time_changed(GtkSpinButton *but)
{
	int fade_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "sb_fade_time")));	
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_preferences_window, "cb_fading"))))
	{
		return;
	}
	mpd_ob_status_set_crossfade(connection, fade_time);
}



/* this sets all the settings in the authentification area preferences correct */
void update_auth_settings()
{
	char *string = 	cfg_get_single_value_as_string_with_default(config, "connection","password", "");
	gtk_toggle_button_set_active((GtkToggleButton *)
			glade_xml_get_widget(xml_preferences_window, "ck_auth"), 
			cfg_get_single_value_as_int_with_default(config, "connection", "useauth", 0));
	gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "entry_auth"), 
			cfg_get_single_value_as_int_with_default(config, "connection", "useauth", 0));
	gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml_preferences_window, "entry_auth")),string);
	cfg_free_string(string);
}

void osb_enable_tb(GtkToggleButton *but)
{

	int bool1  = gtk_toggle_button_get_active(but);
	if(bool1 != cfg_get_single_value_as_int_with_default(config, "osb", "enable", 0))
	{
		cfg_set_single_value_as_int(config, "osb","enable", bool1);
		pl3_reinitialize_tree();
	}
}


void csl_enable_tb(GtkToggleButton *but)
{
	int bool1  = gtk_toggle_button_get_active(but);
	if(bool1 != cfg_get_single_value_as_int_with_default(config,  "playlist","custom_stream_enable", 0))
	{                                                                   	
		cfg_set_single_value_as_int(config, "playlist","custom_stream_enable", bool1);
		pl3_reinitialize_tree();
	}
}
void cur_song_center_enable_tb(GtkToggleButton *but)
{
	int bool1  = gtk_toggle_button_get_active(but);
	cfg_set_single_value_as_int(config, "playlist3","st_cur_song", bool1);
}
void open_to_position_enable_tb(GtkToggleButton *but)
{
	int bool1  = gtk_toggle_button_get_active(but);
	cfg_set_single_value_as_int(config, "playlist","open-to-position", bool1);
}

void create_osb_tree()
{
	GtkListStore *model;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *col;
	GtkTreeView *tree;

	tree = GTK_TREE_VIEW(glade_xml_get_widget(xml_preferences_window, "tree_osb"));
	model = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
	gtk_tree_view_set_model(tree, GTK_TREE_MODEL(model));
	g_object_unref(G_OBJECT(model));

	cell = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(col, cell, TRUE);
	gtk_tree_view_column_add_attribute(col, cell, "text", 0);
	gtk_tree_view_column_set_title(col, "Name:");
	gtk_tree_view_append_column(tree, col);

	cell = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(col, cell, TRUE);
	gtk_tree_view_column_add_attribute(col, cell, "text", 1);
	gtk_tree_view_column_set_title(col, "Uri");
	gtk_tree_view_append_column(tree, col);
}

void osb_del_source()
{
	GtkTreeView *tree = GTK_TREE_VIEW(glade_xml_get_widget(xml_preferences_window, "tree_osb"));
	GtkTreeModel *model = gtk_tree_view_get_model(tree);                                         	
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)tree);
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(selec, &model, &iter))
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(glade_xml_get_widget(xml_preferences_window,"preferences_window")),
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_WARNING,
				GTK_BUTTONS_YES_NO,
				"Are you sure you want to delete this source?");

		switch(gtk_dialog_run(GTK_DIALOG(dialog)))
		{
			case GTK_RESPONSE_YES:
				{
					char *id;
					gtk_tree_model_get(model, &iter, 0, &id, -1);
					cfg_del_multiple_value(config, "osb", "streams",id);
					gtk_list_store_remove((gpointer)model, &iter);
					pl3_reinitialize_tree();
				}
			default:
				break;
		}
		gtk_widget_destroy(dialog);
	}
}
void osb_add_source()
{
	osb_add_edit_source(0);
}
void osb_edit_source()
{
	osb_add_edit_source(1);
}
void osb_add_edit_source(int edit)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	if(edit)
	{
		GtkTreeView *tree = GTK_TREE_VIEW(glade_xml_get_widget(xml_preferences_window, "tree_osb"));
		GtkTreeModel *model = gtk_tree_view_get_model(tree);                                         	
		GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)tree);
		GtkTreeIter iter;
		gchar *id, *path;
		if(gtk_tree_selection_get_selected(selec, &model, &iter))
		{
			gxml =glade_xml_new (GLADE_PATH "playlist3.glade", "osb_add_dialog", NULL);
			dialog =  glade_xml_get_widget(gxml, "osb_add_dialog");
			gtk_tree_model_get(model, &iter, 0, &id,1, &path, -1);
			gtk_widget_set_sensitive(glade_xml_get_widget(gxml, "entry_name"),FALSE);
			gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(gxml, "entry_name")),id);
			gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(gxml, "entry_url")),path);
		}
		else
		{
			return;
		}
	}
	else
	{
		gxml =glade_xml_new (GLADE_PATH "playlist3.glade", "osb_add_dialog", NULL);
		dialog =  glade_xml_get_widget(gxml, "osb_add_dialog");
	}

	switch(gtk_dialog_run(GTK_DIALOG(dialog)))
	{
		case GTK_RESPONSE_YES:
			{
				const gchar *key = gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(gxml, "entry_name")));
				const gchar *value = gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(gxml, "entry_url")));
				cfg_set_multiple_value_as_string(config,"osb", "streams", (gchar *)key, (gchar *)value);
				pl3_reinitialize_tree();
				update_osb_tree();
			}
		default:
			break;

	}

	gtk_widget_destroy(dialog);
	g_object_unref(gxml);
}



int update_osb_tree()
{
	GtkTreeView *tree;
	GtkTreeModel *model;
	GtkTreeIter iter;
	conf_mult_obj *list;
	tree = GTK_TREE_VIEW(glade_xml_get_widget(xml_preferences_window, "tree_osb"));
	model = gtk_tree_view_get_model(tree);

	gtk_list_store_clear((gpointer)model);
	list = cfg_get_multiple_as_string(config, "osb", "streams");
	if(list != NULL)
	{
		conf_mult_obj *data = list;
		do{
			if(data->key != NULL && data->value != NULL)
			{
				gtk_list_store_append((gpointer)model, &iter);
				gtk_list_store_set((gpointer)model, &iter, 
						0, data->key,
						1, data->value,-1);
			}
			data = data->next;
		}while(data  != NULL);
		cfg_free_multiple(list);
	}
	return 0;
}


void create_outputs_tree()
{
	GtkListStore *model;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *col;
	GtkTreeView *tree;

	tree = GTK_TREE_VIEW(glade_xml_get_widget(xml_preferences_window, "tv_outputs"));
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
		mpd_ob_server_set_output_device(connection, id, state);

		gtk_list_store_set(GTK_LIST_STORE(model), &iter, ENABLED_COL, state, -1);
	}
	gtk_tree_path_free(path);
}


void update_server_stats()
{
	if(xml_preferences_window == NULL) return;
	if(mpd_ob_check_connected(connection))
	{
		gchar *temp;
		temp = g_strdup_printf("%i", mpd_ob_stats_get_total_songs(connection));
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_preferences_window, "ss_label_songs")), temp);
		g_free(temp);
		temp = g_strdup_printf("%i", mpd_ob_stats_get_total_artists(connection));
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_preferences_window, "ss_label_artists")), temp);
		g_free(temp);
		temp = g_strdup_printf("%i", mpd_ob_stats_get_total_albums(connection));
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_preferences_window, "ss_label_albums")), temp);
		g_free(temp);
		temp = g_strdup_printf(_("%i day%s %02i hour%s %02i minute%s"), 
				mpd_ob_stats_get_uptime(connection)/86400,((mpd_ob_stats_get_uptime(connection)/86400) > 1)? "s":"",
				(mpd_ob_stats_get_uptime(connection)%86400)/3600,((mpd_ob_stats_get_uptime(connection)%86400)/3600 > 1)? "s":"",
				(mpd_ob_stats_get_uptime(connection)%3600)/60,((mpd_ob_stats_get_uptime(connection)%3600)/60 > 1)? "s":""
				);
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_preferences_window, "ss_label_uptime")), temp);
		g_free(temp);                                                                                        		
		temp = g_strdup_printf(_("%i day%s %02i hour%s %02i minute%s"), 
				mpd_ob_stats_get_playtime(connection)/86400,((mpd_ob_stats_get_playtime(connection)/86400) > 1)? "s":"",
				(mpd_ob_stats_get_playtime(connection)%86400)/3600,((mpd_ob_stats_get_playtime(connection)%86400)/3600 > 1)? "s":"",
				(mpd_ob_stats_get_playtime(connection)%3600)/60,((mpd_ob_stats_get_playtime(connection)%3600)/60 > 1)? "s":""
				);                                                                     		
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_preferences_window, "ss_label_playtime")), temp);		
		g_free(temp);
		gtk_widget_hide(glade_xml_get_widget(xml_preferences_window, "hbox_connected"));			
	}
	else
	{
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_preferences_window, "ss_label_songs")), "N/A");
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_preferences_window, "ss_label_artists")), "N/A");
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_preferences_window, "ss_label_albums")), "N/A");
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_preferences_window, "ss_label_uptime")), "N/A");
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_preferences_window, "ss_label_playtime")), "N/A");		
		gtk_widget_show(glade_xml_get_widget(xml_preferences_window, "hbox_connected"));		
	}



}

void update_outputs_settings()
{
	GtkTreeIter iter;
	GtkListStore *store;
	GtkFrame *frame;

	frame = GTK_FRAME(glade_xml_get_widget(xml_preferences_window, "frm_outputs"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(glade_xml_get_widget(xml_preferences_window, "tv_outputs"))));
	gtk_list_store_clear(store);
	if(mpd_ob_check_connected(connection) && mpd_ob_server_check_version(connection, 0,12,0))
	{
		MpdData *data = mpd_ob_server_get_output_devices(connection);
		while(data != NULL && data->value.output_dev->id != -10)
		{
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 
					0, data->value.output_dev->enabled?TRUE:FALSE, 
					1, data->value.output_dev->name, 
					2, data->value.output_dev->id, -1);
			data = mpd_ob_data_get_next(data);
		}
		gtk_widget_set_sensitive(GTK_WIDGET(frame), TRUE);
		gtk_widget_show_all(GTK_WIDGET(frame));            		
	}
	else
	{
		gtk_widget_set_sensitive(GTK_WIDGET(frame), FALSE);
	}
}


/****************************************************************/
/* 		Advanced Browser				*/
/****************************************************************/
void pref_id3b_row_remove()
{
	GtkWidget *tree = glade_xml_get_widget(xml_preferences_window,"id3b_tree");
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(selection,&model,&iter))
	{             
		gchar *title;
		gtk_tree_model_get(model,&iter,0,&title,-1);		
		cfg_del_multiple_value(config, "playlist", "advbrows",title);
		pref_id3b_fill();
		pl3_custom_tag_browser_reload();
	}
	
}

void pref_id3b_row_changed(GtkTreeView *tree)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(tree);
	GtkTreeModel *model = gtk_tree_view_get_model(tree);
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(selection,&model,&iter))
	{
		gchar *format, *title;
		gchar ** tk_format = NULL;
		gtk_tree_model_get(model,&iter,0,&title,1,&format, -1);
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml_preferences_window, "id3b_entry")),title);
		tk_format = g_strsplit(format, "|",0);
		if(tk_format ==NULL)
		{                                     		
			debug_printf(DEBUG_INFO,"pref_id3b_row_changed: failed to split\n");
			return;
		}
		else
		{		
			gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb1")),0);
			gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb2")),0);
			gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb3")),0);
		
			if(tk_format[0] != NULL)		
			{
				gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb1")),
						mpd_misc_get_tag_by_name(tk_format[0])+1);
				if(tk_format[1] != NULL)
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb2")),
							mpd_misc_get_tag_by_name(tk_format[1])+1);
					if(tk_format[2] != NULL)
					{
						gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb3")),
								mpd_misc_get_tag_by_name(tk_format[2])+1);		
					}
				}
			}
			g_strfreev(tk_format);

		}
	}
	else
	{
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml_preferences_window, "id3b_entry")),"");
		gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb1")),0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb2")),0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb3")),0);
	}
}

void pref_id3b_add_entry()
{
	GString *format = g_string_new("");
	if(gtk_combo_box_get_active(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb1"))))
	{
		GtkTreeIter iter;
		char *string;
		if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb1")),&iter))
		{
			gtk_tree_model_get(GTK_TREE_MODEL(gtk_combo_box_get_model(
							GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb1")))),&iter, 0,&string, -1);
			g_string_append_printf(format, "%s|",string);
		}
	}
	if(gtk_combo_box_get_active(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb2"))))
	{
		GtkTreeIter iter;
		char *string;
		if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb2")),&iter))
		{
			gtk_tree_model_get(GTK_TREE_MODEL(gtk_combo_box_get_model(
							GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb2")))),&iter, 0,&string, -1);
			g_string_append_printf(format, "%s|",string);
		}
	}
	if(gtk_combo_box_get_active(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb3"))))
	{
		GtkTreeIter iter;
		char *string;
		if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb3")),&iter))
		{
			gtk_tree_model_get(GTK_TREE_MODEL(gtk_combo_box_get_model(
							GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb3")))),&iter, 0,&string, -1);
			g_string_append_printf(format, "%s|",string);
		}
	}
	if(format->len)
	{
		g_string_erase(format, format->len-1,-1);
		if(strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml_preferences_window, "id3b_entry")))))
		{
			cfg_set_multiple_value_as_string(config, "playlist", "advbrows",
					(char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml_preferences_window, "id3b_entry"))),
					format->str);
		}
	}
	g_string_free(format, TRUE);
	pref_id3b_fill();
	pl3_custom_tag_browser_reload();
}


void pref_id3b_fill()
{
	GtkWidget *tree = glade_xml_get_widget(xml_preferences_window,"id3b_tree");
	conf_mult_obj *list;
	GtkListStore *ls = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree)));
	list = cfg_get_multiple_as_string(config, "playlist", "advbrows");
	gtk_list_store_clear(ls);	
	if(list != NULL)
	{
		conf_mult_obj *data = list;
		do{
			if(strlen(data->key) && strlen(data->value))
			{
				GtkTreeIter iter;
				gtk_list_store_append(ls, &iter);
				gtk_list_store_set(ls,&iter,0,data->key,1,data->value,2,TRUE,-1);
			}
			data = data->next;
		}
		while(data);
		cfg_free_multiple(list);
	}

}

void pref_id3b_init()
{
	GtkCellRenderer *renderer = NULL;
	GtkListStore *ab_lstore = NULL;
	GtkWidget *tree = glade_xml_get_widget(xml_preferences_window,"id3b_tree");

	/* create model to store the data in */
	ab_lstore = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
	/* add model to tree */
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(ab_lstore)); 

	/* add columns */
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree),0,_("Name"), renderer,
			"text",0,
			"editable",2,
			NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree),1,_("Format"), renderer,
			"text",1,
			NULL);          
	gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb1")),0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb2")),0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(xml_preferences_window, "id3b_cb3")),0);
	pref_id3b_fill();

}
