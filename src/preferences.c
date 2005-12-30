#include <gtk/gtk.h>
#include <glade/glade.h>
#include <string.h>
#include <time.h>
#include <config.h>

#include "plugin.h"

#include "main.h"
#include "playlist3.h"
#include "playlist3-tag-browser.h"
#include "config1.h"

#define PLUGIN_STATS -200

void plugin_stats_construct(GtkWidget *container);
void plugin_stats_destroy(GtkWidget *container);
GladeXML *plugin_stat_xml = NULL;
/* About "plugin" */
void about_pref_construct(GtkWidget *container);
void about_pref_destroy(GtkWidget *container);
GladeXML *about_pref_xml = NULL;
gmpcPrefPlugin about_gpp = {
	about_pref_construct,
	about_pref_destroy
};

gmpcPlugin about_plug = {
	"About",
	{1,1,1},
	GMPC_INTERNALL,
	0,
	NULL, /* initialize */
	NULL,
	NULL,
	NULL,
	NULL,
	&about_gpp
};
/* End About */
GtkListStore *plugin_store = NULL;
GladeXML *xml_preferences_window = NULL;
gboolean running = 0, connected = 0;

void update_auth_settings();
void preferences_window_connect(GtkWidget *but);
void preferences_window_disconnect(GtkWidget *but);
void auth_enable_toggled(GtkToggleButton *but);
void entry_auth_changed(GtkEntry *entry);
void set_display_settings();
void update_display_settings();

int plugin_last;

void pref_plugin_changed()
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW (glade_xml_get_widget(xml_preferences_window, "plugin_tree")));
	GtkTreeModel *model = GTK_TREE_MODEL(plugin_store);
	GtkTreeIter iter;
	int id = 0;
	if(plugin_last >= 0)
	{
		plugins[plugin_last]->pref->destroy(glade_xml_get_widget(xml_preferences_window, "plugin_container"));
		plugin_last = -1;

	}
	else if(plugin_last == PLUGIN_STATS)
	{
		plugin_stats_destroy(glade_xml_get_widget(xml_preferences_window, "plugin_container"));
	}
	if(gtk_tree_selection_get_selected(sel, &model, &iter))
	{
		gtk_tree_model_get(GTK_TREE_MODEL(plugin_store), &iter, 0, &id, -1);
		if(id >= 0 && plugins[id]->pref)
		{
			if(plugins[id]->pref->construct)
			{
				char *buf = NULL;
				if(plugins[id]->plugin_type != GMPC_INTERNALL)
				{
					buf = g_strdup_printf("<span size=\"xx-large\"><b>%s</b></span>\n<i>Plugin version: %i.%i.%i</i>", 
							plugins[id]->name,
							plugins[id]->version[0],plugins[id]->version[1], plugins[id]->version[2]);
				}
				else
				{
					buf = g_strdup_printf("<span size=\"xx-large\"><b>%s</b></span>",
							plugins[id]->name);
				}

				plugins[id]->pref->construct(glade_xml_get_widget(xml_preferences_window, "plugin_container"));
				plugin_last = id;
				gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml_preferences_window, "plugin_label")),buf);
				g_free(buf);
				return;
			}
		}
		else if(id == PLUGIN_STATS)
		{
			gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml_preferences_window, 
						"plugin_label")),
					_("<span size=\"xx-large\"><b>Plugins</b></span>"));

			plugin_stats_construct(glade_xml_get_widget(xml_preferences_window,
						"plugin_container"));
			plugin_last = id;
			return;
		}
	}
	gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml_preferences_window, "plugin_label")),
			"<span size=\"xx-large\"><b>Nothing Selected</b></span>");
}

void create_preferences_window()
{
	GtkWidget *dialog;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkWidget *label;
	int plugs = 0;
	int i=0;
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
	plugin_last = -1;
	string = gmpc_get_full_glade_path("gmpc.glade");
	xml_preferences_window = glade_xml_new(string, "preferences_window", NULL);
	g_free(string);
	/* check for errors and axit when there is no gui file */
	if(xml_preferences_window == NULL)  g_error("Couldnt initialize GUI. Please check installation\n");


	/* set info from struct */
	/* hostname */
	dialog = glade_xml_get_widget(xml_preferences_window, "preferences_window");
	gtk_widget_show_all(GTK_WIDGET(dialog));
	running = 1;

	plugin_store = gtk_list_store_new(2, GTK_TYPE_INT, GTK_TYPE_STRING);
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title(column, _("Plugins:"));
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column,renderer,"markup", 1, NULL);	
	gtk_tree_view_append_column (GTK_TREE_VIEW (glade_xml_get_widget(xml_preferences_window, "plugin_tree")), column);


	g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW (glade_xml_get_widget(xml_preferences_window, "plugin_tree")))), 
			"changed", G_CALLBACK(pref_plugin_changed), NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(glade_xml_get_widget(xml_preferences_window, "plugin_tree")), GTK_TREE_MODEL(plugin_store));

	/* internals */
	for(i=0; i< num_plugins; i++)
	{
		if(plugins[i]->pref != NULL)
		{
			if(plugins[i]->id&PLUGIN_ID_INTERNALL)
			{
				GtkTreeIter iter;
				gtk_list_store_append(GTK_LIST_STORE(plugin_store), &iter);
				gtk_list_store_set(GTK_LIST_STORE(plugin_store), &iter,
						0, plugin_get_pos(plugins[i]->id)/*^PLUGIN_ID_MARK*/,
						1, plugins[i]->name, -1);
				if(gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(
								GTK_TREE_VIEW(glade_xml_get_widget(xml_preferences_window, "plugin_tree")))) == 0)
				{
					gtk_tree_selection_select_iter(gtk_tree_view_get_selection(
								GTK_TREE_VIEW(glade_xml_get_widget(xml_preferences_window, "plugin_tree"))),&iter);
				}
			}
			else
			{
				plugs++;
			}
		}
	}
	/* plugins */
	if(plugs)
	{
		GtkTreeIter iter;
		gtk_list_store_append(GTK_LIST_STORE(plugin_store), &iter);
		gtk_list_store_set(GTK_LIST_STORE(plugin_store), &iter, 0,PLUGIN_STATS, 1,"<b>Plugins:</b>", -1);
		for(i=0; i< num_plugins; i++)
		{
			if(plugins[i]->pref != NULL && plugins[i]->id&PLUGIN_ID_MARK)
			{
				gtk_list_store_append(GTK_LIST_STORE(plugin_store), &iter);
				gtk_list_store_set(GTK_LIST_STORE(plugin_store), &iter,
						0, plugin_get_pos(plugins[i]->id),
						1, plugins[i]->name,
						-1);
			}
		}
	}

	label = glade_xml_get_widget(xml_preferences_window, "plugin_label_box");
	gtk_widget_modify_bg(label, GTK_STATE_NORMAL, &dialog->style->bg[GTK_STATE_SELECTED]);
	label = glade_xml_get_widget(xml_preferences_window, "plugin_label");
	gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &dialog->style->fg[GTK_STATE_SELECTED]);
	glade_xml_signal_autoconnect(xml_preferences_window);	

}
void set_display_default_sd()
{
	gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml_preferences_window, "en_sd")), DEFAULT_PLAYER_MARKUP);
}

/* destory the preferences window */
void preferences_window_destroy()
{
	GtkWidget *dialog = glade_xml_get_widget(xml_preferences_window, "preferences_window");
	if(plugin_last >= 0)
	{
		plugins[plugin_last]->pref->destroy(glade_xml_get_widget(xml_preferences_window, "plugin_container"));
		plugin_last = -1;

	}	                                                                                                     	
	else if(plugin_last == PLUGIN_STATS)
	{
		plugin_stats_destroy(glade_xml_get_widget(xml_preferences_window, "plugin_container"));
	}
	gtk_widget_destroy(dialog);
	g_object_unref(xml_preferences_window);
	xml_preferences_window = NULL;
	running = 0;
}

void about_pref_destroy(GtkWidget *container)
{
	if(about_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(about_pref_xml, "about-vbox");
		gtk_container_remove(GTK_CONTAINER(container),vbox);
		g_object_unref(about_pref_xml);
		about_pref_xml = NULL;
	}
}
void about_pref_construct(GtkWidget *container)
{
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
	about_pref_xml = glade_xml_new(path, "about-vbox",NULL);

	if(about_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(about_pref_xml, "about-vbox");
		gtk_container_add(GTK_CONTAINER(container),vbox);
	}
}


void plugin_stats_construct(GtkWidget *container)
{
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
	plugin_stat_xml = glade_xml_new(path, "plugin_stat_tb",NULL);
	g_free(path);
	if(plugin_stat_xml)
	{
		int plug_brow = 0, plug_misc =0,i=0;
		GtkWidget *vbox = glade_xml_get_widget(plugin_stat_xml, "plugin_stat_tb");
		for(i=0;i<num_plugins;i++)
		{
			if(plugins[i]->id&PLUGIN_ID_MARK)
			{
				if(plugins[i]->plugin_type == GMPC_PLUGIN_PL_BROWSER) plug_brow++;
				else if (plugins[i]->plugin_type == GMPC_PLUGIN_NO_GUI) plug_misc++;
			}
		}
		path = g_strdup_printf("%i", plug_brow+plug_misc);
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(plugin_stat_xml, "num_plug_label")),path);
		g_free(path);

		path = g_strdup_printf("%i", plug_brow);
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(plugin_stat_xml, "num_brow_label")),path);
		g_free(path);

		path = g_strdup_printf("%i", plug_misc);
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(plugin_stat_xml, "num_misc_label")),path);
		g_free(path);

		gtk_container_add(GTK_CONTAINER(container),vbox);
	}

}
void plugin_stats_destroy(GtkWidget *container)
{
	if(plugin_stat_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(plugin_stat_xml, "plugin_stat_tb");
		gtk_container_remove(GTK_CONTAINER(container),vbox);
		g_object_unref(plugin_stat_xml);
		plugin_stat_xml = NULL;
	}
}
