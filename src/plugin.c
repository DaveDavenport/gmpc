/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpcwiki.sarine.nl/
 
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

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gmodule.h>
#include "main.h"
#include "metadata.h"


gmpcPlugin **plugins = NULL;
int num_plugins = 0;

gmpcPlugin * plugin_get_from_id(int id)
{
    int pos = plugin_get_pos(id);
    g_assert_cmpint(pos, <, num_plugins);
	return plugins[pos];
}


int plugin_get_pos(int id)
{
	return id&(PLUGIN_ID_MARK-1);
}
static int plugin_validate(gmpcPlugin *plug)
{
    int i;
    if(plug == NULL)
    {
        debug_printf(DEBUG_ERROR, "plug != NULL failed");
        return FALSE;
    }
    if(plug->name == NULL)
    {
        debug_printf(DEBUG_ERROR, "Plugin has no name.");
        return FALSE;
    }
    for(i=0;i<num_plugins;i++)
    {
        if(strcmp(gmpc_plugin_get_name(plugins[i]), plug->name) == 0)
        {
            debug_printf(DEBUG_ERROR, "Plugin with same name allready exists.");
            return FALSE;
        }
    }
    if(plug->set_enabled == NULL || plug->get_enabled == NULL)
    {
        debug_printf(DEBUG_ERROR, "%s: set_enabled == NULL || get_enabled == NULL failed",plug->name);
        return FALSE;
    }
    if(plug->plugin_type&GMPC_PLUGIN_PL_BROWSER)
    {
        if(plug->browser == NULL)
        {   
            debug_printf(DEBUG_ERROR, "%s: plugin_type&GMPC_PLUGIN_PL_BROWSER && plugin->browser != NULL Failed",plug->name);
            return FALSE;
        }
        if(plug->browser->cat_key_press != NULL)
        {
            debug_printf(DEBUG_ERROR, "Plugin %s implements a cat_key_press event handler that is deprecated", plug->name);
        }
    }
    if(plug->plugin_type&GMPC_PLUGIN_META_DATA)
    {
        if(plug->metadata == NULL)
        {   
            debug_printf(DEBUG_ERROR, "%s: plugin_type&GMPC_PLUGIN_META_DATA && plugin->metadata != NULL Failed",plug->name);
            return FALSE;                                                                                             
        }
        if(plug->metadata->get_priority == NULL)
        {   
            debug_printf(DEBUG_ERROR, "%s: plugin_type&GMPC_PLUGIN_META_DATA && plugin->metadata->get_priority != NULL Failed",plug->name);
            return FALSE;                                                                                             
        }
        if(plug->metadata->set_priority == NULL)
        {
            debug_printf(DEBUG_ERROR, "%s: plugin_type&GMPC_PLUGIN_META_DATA && plugin->metadata->set_priority != NULL Failed",plug->name);
            return FALSE;                                                                                             
        }
        if(plug->metadata->get_image == NULL)
        {   
            debug_printf(DEBUG_ERROR, "%s: plugin_type&GMPC_PLUGIN_META_DATA && plugin->metadata->get_image != NULL Failed",plug->name);
            return FALSE;                                                                                             
        }
    }
    /* if there is a browser field, check validity */
    if(plug->browser)
    {
        if((plug->browser->selected && plug->browser->unselected == NULL) ||(plug->browser->selected  == NULL && plug->browser->unselected))
        {
            debug_printf(DEBUG_ERROR, "%s: If a plugin provides a browser pane, it needs both selected and unselected",plug->name); 
            return FALSE;
        }
    }
    /* if there is a pref window withouth both construct/destroy, give an error */
    if(plug->pref)
    {
        if(!(plug->pref->construct && plug->pref->destroy))
        {
            debug_printf(DEBUG_ERROR, "%s: If a plugin has a preferences pane, it needs both construct and destroy", plug->name);
        }
    }

    return TRUE;
}

void plugin_add(gmpcPlugin *plug, int plugin)
{
	/* set plugin id */
	plug->id = num_plugins|((plugin)?PLUGIN_ID_MARK:PLUGIN_ID_INTERNALL);
	/* put it in the list */
	num_plugins++;
	plugins = g_realloc(plugins,(num_plugins+1)*sizeof(gmpcPlugin **));
	plugins[num_plugins-1] = plug;
	plugins[num_plugins] = NULL;
	
	if(plug->plugin_type&GMPC_PLUGIN_META_DATA)
	{
		meta_data_add_plugin(plug);
	}
}
static int plugin_load(char *path, const char *file)
{
	GModule *handle;
	int *api_version=0;
	gmpcPlugin *plug = NULL;
	gchar *string = NULL;
	gchar *full_path = NULL;
	if(path == NULL)
	{
		return 1;
	}
	full_path = g_strdup_printf("%s%c%s", path,G_DIR_SEPARATOR, file);
	debug_printf(DEBUG_INFO, "plugin_load: trying to load plugin %s", full_path);

	handle = g_module_open(full_path, G_MODULE_BIND_LOCAL);
	q_free(full_path);
	if (!handle) {
		gchar *message = g_strdup_printf("Failed to load plugin:\n%s:%s",file, g_module_error());
		debug_printf (DEBUG_ERROR, "plugin_load: module failed to load: %s:%s\n", file, g_module_error());
		show_error_message(message, FALSE);
		q_free(message);
		return 1;
	}
	if(!g_module_symbol(handle, "plugin_api_version", (gpointer)&api_version)){
		gchar *message = g_strdup_printf("Failed to load plugin:\n%s:%s",file, g_module_error());
		debug_printf(DEBUG_ERROR, "plugin_load: symbol failed to bind: %s:%s\n",file, g_module_error());
		show_error_message(message,FALSE);
		q_free(string);
		q_free(message);
		g_module_close(handle);
		return 1;
	}
	if(*api_version != PLUGIN_API_VERSION)
	{
		gchar *message = g_strdup_printf("Plugin '%s' has the wrong api version.\nPlugin api is %i, but we need %i",
				file, *api_version, PLUGIN_API_VERSION);
		debug_printf(DEBUG_ERROR, "Plugin '%s' has the wrong api version.\nPlugin api is %i, but we need %i",
			       file, *api_version, PLUGIN_API_VERSION);
	//	show_error_message(message,FALSE);
        playlist3_show_error_message(message, ERROR_WARNING);
        q_free(string);
		q_free(message);
		g_module_close(handle);
		return 1;
	}

	if(!g_module_symbol(handle, "plugin", (gpointer)&plug)){
		gchar *message = g_strdup_printf("Failed to load plugin:\n%s:%s",file, g_module_error());
		debug_printf(DEBUG_ERROR, "plugin_load: symbol failed to bind: %s:%s\n",file, g_module_error());
//		show_error_message(message,FALSE);
        playlist3_show_error_message(message, ERROR_CRITICAL);
        q_free(string);
		q_free(message);
		g_module_close(handle);
		return 1;
	}
	if(plug == NULL)
	{
		debug_printf(DEBUG_WARNING, "plugin load: unknown type of plugin.\n");
		g_module_close(handle);
		return 1;
	}
    if(!plugin_validate(plug))
    {
        debug_printf(DEBUG_ERROR, "Faled to validate plugin: %s\n", file);
        g_module_close(handle);
        return 1;
    }
    /* set path, plugins might want this for images and glade files. */
    plug->path = g_strdup(path);
    /* add the plugin to the list */
    plugin_add(plug,1);
    return 0;
}
static gboolean __show_plugin_load_error(gpointer data)
{
    playlist3_show_error_message(_("One or more plugins failed to load, see help->messages for more information"), ERROR_CRITICAL);
    return FALSE;
}

void plugin_load_dir(gchar *path)
{
    GDir *dir = g_dir_open(path, 0, NULL);
    int failure = 0;
    if(dir)
    {
        const gchar *dirname = NULL;
        while((dirname = g_dir_read_name(dir)) != NULL)
        {
            gchar *full_path = g_strdup_printf("%s%c%s",path,G_DIR_SEPARATOR,dirname);
            /* Make sure only to load plugins */
            if(g_str_has_suffix(dirname, G_MODULE_SUFFIX))
            {
                if(plugin_load(path,dirname))
                {
                    failure = 1;
                    debug_printf(DEBUG_ERROR, "Failed to load plugin: %s\n", dirname);
                }
            }
            else
            {
                debug_printf(DEBUG_INFO, "File not loaded, wrong extention, should be: '%s'", G_MODULE_SUFFIX);
            }
            q_free(full_path);
        }
        g_dir_close(dir);
    }
    if(failure)
    {
        gtk_init_add(__show_plugin_load_error, NULL);
    }
}

/**
 * gmpcPlugin
 */

void gmpc_plugin_destroy(gmpcPlugin *plug)
{
    if(plug->destroy)
    {
        plug->destroy();
    }
}
void gmpc_plugin_init(gmpcPlugin *plug)
{
    if(plug->init)
    {
        plug->init();
    }
}
void gmpc_plugin_status_changed(gmpcPlugin *plug, MpdObj *mi, ChangedStatusType what)
{
    if(plug->mpd_status_changed)
    {
        plug->mpd_status_changed(mi,what,NULL);
    }
}
const char *gmpc_plugin_get_name(gmpcPlugin *plug)
{
    g_assert(plug->name != NULL);
    return plug->name;
}
void gmpc_plugin_save_yourself(gmpcPlugin *plug)
{
    if(plug->save_yourself)
    {
        plug->save_yourself();
    }
}
gboolean gmpc_plugin_get_enabled(gmpcPlugin *plug)
{
    if(plug->get_enabled)
    {
        return plug->get_enabled();
    }
    return TRUE;
}
void gmpc_plugin_set_enabled(gmpcPlugin *plug, gboolean enabled)
{
    if(plug->set_enabled)
    {
        plug->set_enabled(enabled);
    }
}

gchar *gmpc_plugin_get_data_path(gmpcPlugin *plug)
{
#ifdef WIN32
    debug_printf(DEBUG_INFO, "path: %s\n", plug->path);
    return g_strdup(plug->path);
#else
    gchar *url = NULL;
    gchar *homedir = gmpc_get_user_path(""); 
    if(strncmp(plug->path,homedir ,strlen(homedir)) == 0)
    {
        url = g_strdup(plug->path);
    }else{
       /* Ok it is a homedir */
        url = g_build_path(G_DIR_SEPARATOR_S, PACKAGE_DATA_DIR, "gmpc","plugins",NULL);
    }

    printf("found url: %s (%s:%s)\n", url, plug->path, homedir);
    g_free(homedir);
    return url;
#endif
}

void gmpc_plugin_mpd_connection_changed(gmpcPlugin *plug, MpdObj *mi, int connected, gpointer data)
{
    g_assert(plug != NULL);
    if(plug->mpd_connection_changed != NULL)
    {
        plug->mpd_connection_changed(mi,connected,data);
    }
}

gboolean gmpc_plugin_is_browser(gmpcPlugin *plug)
{
    return ((plug->plugin_type&GMPC_PLUGIN_PL_BROWSER) != 0);
}
void gmpc_plugin_browser_unselected(gmpcPlugin *plug, GtkWidget *container)
{
    if(gmpc_plugin_is_browser(plug)) {
        g_assert(plug->browser != NULL);
        g_assert(plug->browser->unselected != NULL);
        plug->browser->unselected(container);
    }
}
void gmpc_plugin_browser_selected(gmpcPlugin *plug, GtkWidget *container)
{
    if(gmpc_plugin_is_browser(plug)) {
        g_assert(plug->browser != NULL);
        g_assert(plug->browser->selected != NULL);
        plug->browser->selected(container);
    }
}

void gmpc_plugin_browser_add(gmpcPlugin *plug, GtkWidget *cat_tree)
{
    if(gmpc_plugin_is_browser(plug)) {
        g_assert(plug->browser != NULL);
        if(plug->browser->add)
        {
            plug->browser->add(cat_tree);
        }
    }
}

int gmpc_plugin_browser_cat_right_mouse_menu(gmpcPlugin *plug, GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event)
{
    if(gmpc_plugin_is_browser(plug)) {
        g_assert(plug->browser != NULL);
        if(plug->browser->cat_right_mouse_menu != NULL)
        {
            return plug->browser->cat_right_mouse_menu(menu,type,tree,event);
        }
    }
    return 0;
}

int gmpc_plugin_browser_key_press_event(gmpcPlugin *plug, GtkWidget *mw, GdkEventKey *event, int type)
{
    if(gmpc_plugin_is_browser(plug)) {
        g_assert(plug->browser != NULL);
        if(plug->browser->key_press_event != NULL)
        {
            return plug->browser->key_press_event(mw,event,type);
        }
    }
    return 0;
}

int gmpc_plugin_browser_add_go_menu(gmpcPlugin *plug, GtkWidget *menu)
{
    if(gmpc_plugin_is_browser(plug)) {
        g_assert(plug->browser != NULL);
        if(plug->browser->add_go_menu != NULL)
        {
            return plug->browser->add_go_menu(menu);
        }
    }
    return 0;
}

int gmpc_plugin_browser_song_list_option_menu(gmpcPlugin *plug, GmpcMpdDataTreeview *tree, GtkMenu *menu) 
{
    if(gmpc_plugin_is_browser(plug))
    {
        g_assert(plug->browser != NULL);
        if(plug->browser->song_list_option_menu)
        {
            return plug->browser->song_list_option_menu(tree, menu);
        }
    }
    return 0;
}

gboolean gmpc_plugin_browser_has_integrate_search(gmpcPlugin *plug)
{
    if(gmpc_plugin_is_browser(plug))
    {
        return plug->browser->integrate_search != NULL;
    }
    return FALSE;
}
MpdData *gmpc_plugin_browser_integrate_search(gmpcPlugin *plug, const int search_field, const gchar * query, GError **error)
{
    if(!gmpc_plugin_browser_has_integrate_search(plug)) return NULL;
    return plug->browser->integrate_search(search_field,query,error); 
}

gboolean gmpc_plugin_has_preferences(gmpcPlugin *plug)
{
    return (plug->pref != NULL);
}

void gmpc_plugin_preferences_construct(gmpcPlugin *plug,GtkWidget *wid)
{
    if(gmpc_plugin_has_preferences(plug))
    {
        g_assert(plug->pref != NULL);
        g_assert(plug->pref->construct);
        plug->pref->construct(wid);
    }
}

void gmpc_plugin_preferences_destroy(gmpcPlugin *plug,GtkWidget *wid)
{
    if(gmpc_plugin_has_preferences(plug))
    {
        g_assert(plug->pref != NULL);
        g_assert(plug->pref->destroy);
        plug->pref->destroy(wid);
    }
}


gboolean gmpc_plugin_is_internal(gmpcPlugin *plug)
{
    return (((plug->plugin_type)&GMPC_INTERNALL) != 0);
}

const int * gmpc_plugin_get_version(gmpcPlugin *plug)
{
    return (int *)plug->version;
}

int gmpc_plugin_get_type(gmpcPlugin *plug)
{
    return plug->plugin_type;
}

int gmpc_plugin_get_id(gmpcPlugin *plug)
{
    return plug->id;
}

gboolean gmpc_plugin_is_metadata(gmpcPlugin *plug)
{
    return (plug->metadata != NULL);
}

int gmpc_plugin_metadata_get_priority(gmpcPlugin *plug)
{
    if(gmpc_plugin_is_metadata(plug))
    {
        return plug->metadata->get_priority();
    }
    return 100;
}

void gmpc_plugin_metadata_set_priority(gmpcPlugin *plug, int priority)
{
    if(gmpc_plugin_is_metadata(plug))
    {
        return plug->metadata->set_priority(priority);
    }
}

int gmpc_plugin_metadata_get_image(gmpcPlugin *plug, mpd_Song *song, MetaDataType type, char **path)
{
    *path = NULL;
    if(gmpc_plugin_is_metadata(plug))
    {
        return plug->metadata->get_image(song, type, path);
    }
    return META_DATA_UNAVAILABLE;
}

