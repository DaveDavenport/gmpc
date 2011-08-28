/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@gmpclient.org>
 * Project homepage: http://gmpclient.org/

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
#include <string.h>
#include <time.h>
#include <config.h>
#include "main.h"
#include "playlist3.h"
#include "misc.h"
#include "preferences.h"

#define LOG_DOMAIN "Preferences"
#define PLUGIN_STATS -200
void preferences_show_pref_window(int plugin_id);
static void plugin_stats_construct(GtkWidget *);
static void plugin_stats_destroy(GtkWidget *);
GtkBuilder *plugin_stat_xml = NULL;
/* About "plugin" */

/* End About */
GtkListStore *plugin_store_unfiltered = NULL;
GtkTreeModel *plugin_store= NULL;

GtkBuilder *xml_preferences_window = NULL;
gboolean running = 0;

/* Glade Prototypes, without glade these would be static */
void preferences_window_connect(GtkWidget * but);
void preferences_window_disconnect(GtkWidget * but);
void auth_enable_toggled(GtkToggleButton * but);
void entry_auth_changed(GtkEntry * entry);
void create_preferences_window(void);
void preferences_window_destroy(void);

int plugin_last;


static void pref_plugin_changed(void)
{
    GtkTreeSelection *sel =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(gtk_builder_get_object(xml_preferences_window, "plugin_tree")));
    GtkTreeModel *model = GTK_TREE_MODEL(plugin_store);
    GtkTreeIter iter;
    int id = 0;
    if (plugin_last >= 0)
    {
        gmpc_plugin_preferences_destroy(plugins[plugin_last],
            (GtkWidget *) gtk_builder_get_object(xml_preferences_window,
            "plugin_container"));
        plugin_last = -1;

    } else if (plugin_last == PLUGIN_STATS)
    {
        plugin_stats_destroy((GtkWidget *) gtk_builder_get_object(xml_preferences_window, "plugin_container"));
    }
    if (gtk_tree_selection_get_selected(sel, &model, &iter))
    {
        gtk_tree_model_get(GTK_TREE_MODEL(plugin_store), &iter, 0, &id, -1);
        if (id >= 0 && gmpc_plugin_has_preferences(plugins[id]))
        {
            char *buf = NULL;
            const gchar *translation_domain = gmpc_plugin_get_translation_domain(plugins[id]);
            if (!gmpc_plugin_is_internal(plugins[id]))
            {
                const int *version = gmpc_plugin_get_version(plugins[id]);
                if (version != NULL)
                {
                    buf = g_strdup_printf("<span size=\"xx-large\"><b>%s</b></span>\n<i>%s: %i.%i.%i</i>",
                        #if defined(ENABLE_NLS) &&  GLIB_CHECK_VERSION(2,18,0)
                        g_dgettext(translation_domain, gmpc_plugin_get_name(plugins[id])),
                        #else
                        gmpc_plugin_get_name(plugins[id]),
                        #endif
                        _("Plugin version"), version[0], version[1], version[2]);
                } else
                buf = g_strdup_printf("<span size=\"xx-large\"><b>%s</b></span>",
                        gmpc_plugin_get_name(plugins[id]));

            } else
            {
                buf = g_strdup_printf("<span size=\"xx-large\"><b>%s</b></span>",
                    N_(gmpc_plugin_get_name(plugins[id])));
            }

            gmpc_plugin_preferences_construct(plugins[id],
                (GtkWidget *) gtk_builder_get_object(xml_preferences_window,
                "plugin_container"));
            plugin_last = id;
            gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(xml_preferences_window, "plugin_label")), buf);
            q_free(buf);
            return;
        } else if (id == PLUGIN_STATS)
        {
            gchar *value = g_markup_printf_escaped("<span size=\"xx-large\" weight=\"bold\">%s</span>", _("Plugins"));
            gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(xml_preferences_window, "plugin_label")), value);
            g_free(value);

            plugin_stats_construct((GtkWidget *) gtk_builder_get_object(xml_preferences_window, "plugin_container"));
            plugin_last = id;
            return;
        }
    }
    gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(xml_preferences_window, "plugin_label")),
        "<span size=\"xx-large\"><b>Nothing Selected</b></span>");
}

/**
 * Filter out disable drows
 */
static gboolean pref_model_filter_func(GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    int plugin_index = -1;
    gtk_tree_model_get(model, iter, 0, &plugin_index, -1);
    if(plugin_index >=0 )
    {
        g_assert(plugin_index < num_plugins);
        if(gmpc_plugin_has_preferences(plugins[plugin_index]))
        {
            if(gmpc_plugin_get_enabled(plugins[plugin_index]))
            {
                return TRUE;
            }
        }
    }
    /* Special case */
    else if(plugin_index == PLUGIN_STATS)
    {
        return TRUE;
    }
    return FALSE;
}

/**
 * If a plugin state changed (enabled/disable) this forces the preferences
 * window to update
 * the list.
 */
void preferences_window_update(void)
{
    if(xml_preferences_window != NULL)
    {
        gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(plugin_store));
    }
}

void create_preferences_window(void)
{
    GError *error = NULL;
    GtkWidget *pl3_win = playlist3_get_window();
    GtkWidget *dialog;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkWidget *label;
    int i = 0;
    char *string = NULL;

    if (running)
    {
        if (xml_preferences_window == NULL)
        {
            running = 0;
        } else
        {
            dialog = (GtkWidget *) gtk_builder_get_object(xml_preferences_window, "preferences_window");
            gtk_window_present(GTK_WINDOW(dialog));
            return;
        }
    }
    plugin_last = -1;
    string = gmpc_get_full_glade_path("preferences.ui");
    xml_preferences_window = gtk_builder_new();
    gtk_builder_add_from_file(xml_preferences_window, string, &error);
    q_free(string);
    if (error)
    {
        g_log(LOG_DOMAIN, G_LOG_LEVEL_ERROR, "Failed to load preferences.ui: %s", error->message);
        g_error_free(error);
    }
    /* set info from struct */
    /* hostname */
    dialog = (GtkWidget *) gtk_builder_get_object(xml_preferences_window, "preferences_window");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(pl3_win));
    gtk_widget_show_all(GTK_WIDGET(dialog));
    running = 1;

    plugin_store_unfiltered = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING);
    /* Create a filtered list that hides the disabled plugins */
    plugin_store = gtk_tree_model_filter_new(GTK_TREE_MODEL(plugin_store_unfiltered), NULL);

    gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(plugin_store),
            pref_model_filter_func, NULL, NULL);

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Plugins"));
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer, "markup", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(gtk_builder_get_object(xml_preferences_window, "plugin_tree")), column);

    g_signal_connect(G_OBJECT
        (gtk_tree_view_get_selection
        (GTK_TREE_VIEW(gtk_builder_get_object(xml_preferences_window, "plugin_tree")))), "changed",
        G_CALLBACK(pref_plugin_changed), NULL);
    gtk_tree_view_set_model(GTK_TREE_VIEW(gtk_builder_get_object(xml_preferences_window, "plugin_tree")),
        GTK_TREE_MODEL(plugin_store));

    /* internals */
    for (i = 0; i < num_plugins; i++)
    {
        if (gmpc_plugin_has_preferences(plugins[i]))
        {
            if (gmpc_plugin_is_internal(plugins[i]))
            {
                GtkTreeIter iter;
                gtk_list_store_append(GTK_LIST_STORE(plugin_store_unfiltered), &iter);
                gtk_list_store_set(GTK_LIST_STORE(plugin_store_unfiltered), &iter, 0, i,
                    #if defined(ENABLE_NLS) &&  GLIB_CHECK_VERSION(2,18,0)
                    1, _(gmpc_plugin_get_name(plugins[i])),
                    #else
                    1, gmpc_plugin_get_name(plugins[i]),
                    #endif
                    -1);
            }
        }
    }
    // Select the first row
    // TODO: Move this outside the loop.
    if (gtk_tree_selection_count_selected_rows
            (gtk_tree_view_get_selection
                    (GTK_TREE_VIEW(gtk_builder_get_object(xml_preferences_window, "plugin_tree")))) == 0)
    {
        GtkTreeIter iter;
        if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(plugin_store), &iter))
        {
            gtk_tree_selection_select_iter(gtk_tree_view_get_selection
                    (GTK_TREE_VIEW
                     (gtk_builder_get_object(xml_preferences_window, "plugin_tree"))),
                    &iter);
        }
    }
    /* plugins */
    {
        GtkTreeIter iter;
        gchar *value = g_markup_printf_escaped("<b>%s:</b>", _("Plugins"));

        gtk_list_store_append(GTK_LIST_STORE(plugin_store_unfiltered), &iter);
        gtk_list_store_set(GTK_LIST_STORE(plugin_store_unfiltered), &iter, 0, PLUGIN_STATS, 1, value, -1);
        g_free(value);
        for (i = 0; i < num_plugins; i++)
        {
            if (gmpc_plugin_has_preferences(plugins[i]) && !gmpc_plugin_is_internal(plugins[i]))
            {
                const gchar *translation_domain = gmpc_plugin_get_translation_domain(plugins[i]);
                gtk_list_store_append(GTK_LIST_STORE(plugin_store_unfiltered), &iter);
                gtk_list_store_set(GTK_LIST_STORE(plugin_store_unfiltered), &iter, 0, i,
                    #if defined(ENABLE_NLS) &&  GLIB_CHECK_VERSION(2,18,0)
                    1, g_dgettext(translation_domain, gmpc_plugin_get_name(plugins[i])),
                    #else
                    1, gmpc_plugin_get_name(plugins[i]),
                    #endif
                    -1);
            }
        }
    }
    {

        GtkWidget *widget = (GtkWidget *) gtk_builder_get_object(xml_preferences_window,
            "eventbox_background");

        gtk_widget_modify_bg(widget, GTK_STATE_NORMAL, &(dialog->style->base[GTK_STATE_NORMAL]));
    }

    label = (GtkWidget *) gtk_builder_get_object(xml_preferences_window, "plugin_label_box");
    gtk_widget_set_app_paintable(label, TRUE);
    g_signal_connect(G_OBJECT(label), "expose-event", G_CALLBACK(misc_header_expose_event), NULL);
    gtk_widget_set_state(GTK_WIDGET(label), GTK_STATE_SELECTED);

    gtk_widget_show(dialog);
    gtk_builder_connect_signals(xml_preferences_window, NULL);
}


/* destory the preferences window */
G_MODULE_EXPORT void preferences_window_destroy(void)
{
    GtkWidget *dialog = (GtkWidget *) gtk_builder_get_object(xml_preferences_window, "preferences_window");
    if (plugin_last >= 0)
    {
        gmpc_plugin_preferences_destroy(plugins[plugin_last],
            (GtkWidget *) gtk_builder_get_object(xml_preferences_window,
            "plugin_container"));
        plugin_last = -1;

    } else if (plugin_last == PLUGIN_STATS)
    {
        plugin_stats_destroy((GtkWidget *) gtk_builder_get_object(xml_preferences_window, "plugin_container"));
    }
    gtk_widget_destroy(dialog);
    g_object_unref(xml_preferences_window);
    g_object_unref(plugin_store);
    plugin_store = NULL;
    xml_preferences_window = NULL;
    running = 0;
}


static void pref_plugin_enabled(GtkCellRendererToggle * rend, gchar * path, GtkListStore * store)
{
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path))
    {
        int toggled;
        gmpcPluginParent *plug = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &toggled, 3, &plug, -1);
        {
            gmpc_plugin_set_enabled(plug, !toggled);

            if (GMPC_PLUGIN_IS_SIDEBAR_IFACE(plug->new))
            {
                if (gmpc_plugin_get_enabled(plug)) {
                    gmpc_sidebar_plugins_enable((GmpcPluginSidebarIface*)plug->new);
                }
                else
                {
                    gmpc_sidebar_plugins_disable((GmpcPluginSidebarIface*)plug->new);
                }

            }


            preferences_window_update();
            gtk_list_store_set(store, &iter, 0, gmpc_plugin_get_enabled(plug), -1);
        }
    }
}


static void plugin_stats_construct(GtkWidget * container)
{
    gchar *path = gmpc_get_full_glade_path("preferences-plugins.ui");
    plugin_stat_xml = gtk_builder_new();
    gtk_builder_add_from_file(plugin_stat_xml, path, NULL);
    q_free(path);
    if (plugin_stat_xml)
    {
        GtkWidget *tree = (GtkWidget *) gtk_builder_get_object(plugin_stat_xml, "plugin_stats_tree");
        GtkListStore *store = NULL;
        GtkTreeIter iter;
        GtkCellRenderer *renderer = NULL;
        int i = 0;
        GtkWidget *vbox = (GtkWidget *) gtk_builder_get_object(plugin_stat_xml, "plugin_stats_vbox");

        /**
         * new
         */
        store = gtk_list_store_new(5, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING);

        gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
        renderer = gtk_cell_renderer_toggle_new();
        g_object_set_data(G_OBJECT(renderer), "editable", GINT_TO_POINTER(1));
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, _("Enabled"), renderer, "active", 0, NULL);
        g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(pref_plugin_enabled), store);
        renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, _("Name"), renderer, "text", 1, NULL);
        renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, _("Function"), renderer, "text", 2, NULL);
        renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, _("Version"), renderer, "text", 4, NULL);
        for (i = 0; i < num_plugins; i++)
        {
            if (!gmpc_plugin_is_internal(plugins[i]))
            {
                const gchar *translation_domain = gmpc_plugin_get_translation_domain(plugins[i]);
                const int *ver = gmpc_plugin_get_version(plugins[i]);
                gchar *version = (ver) ? g_strdup_printf("%i.%i.%i", ver[0], ver[1], ver[2]) : g_strdup("n/a");
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, 0, TRUE,
                    #if defined(ENABLE_NLS) &&  GLIB_CHECK_VERSION(2,18,0)
                    1, g_dgettext(translation_domain, gmpc_plugin_get_name(plugins[i])),
                    #else
                    1, gmpc_plugin_get_name(plugins[i]),
                    #endif
                    3, (plugins[i]), 4, version, -1);
                g_free(version);
                if (gmpc_plugin_get_enabled(plugins[i]))
                {
                    gtk_list_store_set(store, &iter, 0, TRUE, -1);
                } else
                gtk_list_store_set(store, &iter, 0, FALSE, -1);
                switch (gmpc_plugin_get_type(plugins[i]))
                {
                    case GMPC_PLUGIN_DUMMY:
                        gtk_list_store_set(store, &iter, 2, _("Dummy"), -1);
                        break;
                    case GMPC_PLUGIN_PL_BROWSER:
                        gtk_list_store_set(store, &iter, 2, _("Browser Extension"), -1);
                        break;
                    case GMPC_PLUGIN_META_DATA:
                        gtk_list_store_set(store, &iter, 2, _("Metadata Provider"), -1);
                        break;
                    case GMPC_PLUGIN_PL_BROWSER | GMPC_PLUGIN_META_DATA:
                        gtk_list_store_set(store, &iter, 2, _("Metadata Provider and Browser Extension"), -1);
                        break;
                    case GMPC_PLUGIN_NO_GUI:
                        gtk_list_store_set(store, &iter, 2, _("Misc."), -1);
                        break;
                    case GMPC_INTERNALL:
                    case GMPC_DEPRECATED:
                    default:
                        gtk_list_store_set(store, &iter, 2, _("Unknown"), -1);
                        break;

                }
                if (GMPC_PLUGIN_IS_SIDEBAR_IFACE((plugins[i])->new))
                {
                    gtk_list_store_set(store, &iter, 2, _("Sidebar Extension"), -1);
                }
            }

        }

        gtk_container_add(GTK_CONTAINER(container), vbox);
    }

}


static void plugin_stats_destroy(GtkWidget * container)
{
    if (plugin_stat_xml)
    {
        GtkWidget *vbox = (GtkWidget *) gtk_builder_get_object(plugin_stat_xml, "plugin_stats_vbox");
        GtkTreeModel *model =
            gtk_tree_view_get_model(GTK_TREE_VIEW(gtk_builder_get_object(plugin_stat_xml, "plugin_stats_tree")));
        gtk_container_remove(GTK_CONTAINER(container), vbox);
        if (model)
            g_object_unref(model);
        g_object_unref(plugin_stat_xml);
        plugin_stat_xml = NULL;
    }
}


void preferences_show_pref_window(int plugin_id)
{
    GtkTreeIter iter;
    if (xml_preferences_window == NULL)
        create_preferences_window();
    if (plugin_store)
    {
        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(plugin_store), &iter))
        {
            do
            {
                int pos;
                gtk_tree_model_get(GTK_TREE_MODEL(plugin_store), &iter, 0, &pos, -1);
                if (pos == plugin_get_pos(plugin_id))
                {
                    GtkTreeSelection *sel =
                        gtk_tree_view_get_selection(GTK_TREE_VIEW
                        (gtk_builder_get_object(xml_preferences_window, "plugin_tree")));
                    gtk_tree_selection_select_iter(sel, &iter);
                    return;
                }
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(plugin_store), &iter));
        }
    }

}
