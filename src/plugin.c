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

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gmodule.h>
#include "vala/gmpc-plugin.h"
#include "main.h"
#include "metadata.h"


gmpcPluginParent **plugins = NULL;
int num_plugins = 0;

typedef struct _gmpcPluginParent {
    gmpcPlugin *old;
    GmpcPluginBase *new;
}_gmpcPluginParent;

gmpcPluginParent * plugin_get_from_id(int id)
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
    gmpcPluginParent *parent = g_malloc0(sizeof(*parent));
    parent->old = plug;
    parent->new = NULL;
	/* set plugin id */
	plug->id = num_plugins|((plugin)?PLUGIN_ID_MARK:PLUGIN_ID_INTERNALL);
	/* put it in the list */
	num_plugins++;
	plugins = g_realloc(plugins,(num_plugins+1)*sizeof(gmpcPlugin **));
	plugins[num_plugins-1] = parent;
	plugins[num_plugins] = NULL;
	
	if(plug->plugin_type&GMPC_PLUGIN_META_DATA)
	{
		meta_data_add_plugin(parent);
	}
}
void plugin_add_new(GmpcPluginBase *plug, int plugin)
{
    gmpcPluginParent *parent = g_malloc0(sizeof(*parent));
    parent->new = plug;
    parent->old = NULL;
	/* set plugin id */
	plug->id = num_plugins|((plugin)?PLUGIN_ID_MARK:PLUGIN_ID_INTERNALL);
	/* put it in the list */
	num_plugins++;
	plugins = g_realloc(plugins,(num_plugins+1)*sizeof(gmpcPlugin **));
	plugins[num_plugins-1] = parent;
	plugins[num_plugins] = NULL;
}

static int plugin_load(char *path, const char *file)
{
    gpointer function;
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

	if(g_module_symbol(handle, "plugin_get_type", (gpointer)&function)){
        GmpcPluginBase *new;
        GType (*get_type)(void);
        get_type = function;
        new = g_object_newv(get_type(),0,NULL);
        
        if(!new) return 1;
        new->path = g_strdup(path);
        plugin_add_new(new, 1);
        return 0;
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
        playlist3_show_error_message(message, ERROR_WARNING);
        q_free(string);
		q_free(message);
		g_module_close(handle);
		return 1;
	}
	if(!g_module_symbol(handle, "plugin", (gpointer)&(plug))){
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

void gmpc_plugin_destroy(gmpcPluginParent *plug)
{
    if(plug->new)
    {
        g_object_unref(plug->new);
        return;
    }
    if(plug->old->destroy)
    {
        plug->old->destroy();
    }
}
void gmpc_plugin_init(gmpcPluginParent *plug)
{
    if(plug->new)
    {

        return;
    }
    if(plug->old->init)
    {
        plug->old->init();
    }
}
void gmpc_plugin_status_changed(gmpcPluginParent *plug, MpdObj *mi, ChangedStatusType what)
{
    if(plug->new) return;
    if(plug->old->mpd_status_changed)
    {
        plug->old->mpd_status_changed(mi,what,NULL);
    }
}
const char *gmpc_plugin_get_name(gmpcPluginParent *plug)
{
    /* if new plugin, use that method */
    if(plug->new) {
        return gmpc_plugin_base_get_name(plug->new);
    }
    g_assert(plug->old->name != NULL);
    return plug->old->name;
}
void gmpc_plugin_save_yourself(gmpcPluginParent *plug)
{
    if(plug->new)
    {
        gmpc_plugin_base_save_yourself(plug->new);
        return;
    }
    if(plug->old->save_yourself)
    {
        plug->old->save_yourself();
    }
}
gboolean gmpc_plugin_get_enabled(gmpcPluginParent *plug)
{
    if(plug->new){
        return gmpc_plugin_base_get_enabled(plug->new);
    }
    if(plug->old->get_enabled)
    {
        return plug->old->get_enabled();
    }
    return TRUE;
}
void gmpc_plugin_set_enabled(gmpcPluginParent *plug, gboolean enabled)
{
    if(plug->new){
        return gmpc_plugin_base_set_enabled(plug->new,enabled);
    }
    if(plug->old->set_enabled)
    {
        plug->old->set_enabled(enabled);
    }
}

const gchar * gmpc_plugin_get_translation_domain(gmpcPluginParent *plug)
{
    if(plug->new)
    {
        return plug->new->translation_domain;
    }
    if(plug->old && plug->old->get_translation_domain) {
        return plug->old->get_translation_domain();
    }
    return NULL;
}
gchar *gmpc_plugin_get_data_path(gmpcPlugin *plug)
{
#ifdef WIN32
    gchar *url = g_win32_get_package_installation_directory_of_module(NULL);
    gchar *retv = g_build_path(G_DIR_SEPARATOR_S, url, "share", "gmpc", "plugins", NULL);
    return retv;//g_strdup(plug->path);
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

    g_free(homedir);
    return url;
#endif
}

void gmpc_plugin_mpd_connection_changed(gmpcPluginParent *plug, MpdObj *mi, int connected, gpointer data)
{
    g_assert(plug != NULL);

    if(plug->new) return;
    if(plug->old->mpd_connection_changed != NULL)
    {
        plug->old->mpd_connection_changed(mi,connected,data);
    }
}

gboolean gmpc_plugin_is_browser(gmpcPluginParent *plug)
{
    if(plug->new) {
        return GMPC_PLUGIN_IS_BROWSER_IFACE(plug->new); 
    }
    return ((plug->old->plugin_type&GMPC_PLUGIN_PL_BROWSER) != 0);
}
void gmpc_plugin_browser_unselected(gmpcPluginParent *plug, GtkWidget *container)
{
    if(gmpc_plugin_is_browser(plug)) {
        if(plug->new) {
            gmpc_plugin_browser_iface_browser_unselected((GmpcPluginBrowserIface *)plug->new, container);
            return;
        }
        g_assert(plug->old->browser != NULL);
        g_assert(plug->old->browser->unselected != NULL);
        plug->old->browser->unselected(container);
    }
}
void gmpc_plugin_browser_selected(gmpcPluginParent *plug, GtkWidget *container)
{
    if(gmpc_plugin_is_browser(plug)) {
        if(plug->new) {
            gmpc_plugin_browser_iface_browser_selected((GmpcPluginBrowserIface *)plug->new, container);
            return;
        }
        g_assert(plug->old->browser != NULL);
        g_assert(plug->old->browser->selected != NULL);
        plug->old->browser->selected(container);
    }
}

void gmpc_plugin_browser_add(gmpcPluginParent *plug, GtkWidget *cat_tree)
{
    if(gmpc_plugin_is_browser(plug)) {
        if(plug->new) {
            gmpc_plugin_browser_iface_browser_add((GmpcPluginBrowserIface *)plug->new, cat_tree);
            return;
        }
        g_assert(plug->old->browser != NULL);
        if(plug->old->browser->add)
        {
            plug->old->browser->add(cat_tree);
        }
    }
}

int gmpc_plugin_browser_cat_right_mouse_menu(gmpcPluginParent *plug, GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event)
{
    if(gmpc_plugin_is_browser(plug)) {
        if(plug->new){
            if(type == plug->new->id)
                return gmpc_plugin_browser_iface_browser_option_menu((GmpcPluginBrowserIface *)plug->new, GTK_MENU(menu));
            return 0;
        }
        g_assert(plug->old->browser != NULL);
        if(plug->old->browser->cat_right_mouse_menu != NULL)
        {
            return plug->old->browser->cat_right_mouse_menu(menu,type,tree,event);
        }
    }
    return 0;
}

int gmpc_plugin_browser_key_press_event(gmpcPluginParent *plug, GtkWidget *mw, GdkEventKey *event, int type)
{
    if(gmpc_plugin_is_browser(plug)) {
        if(plug->new) {
            /* not going to be implemented */
            return 0;
        }
        g_assert(plug->old->browser != NULL);
        if(plug->old->browser->key_press_event != NULL)
        {
            return plug->old->browser->key_press_event(mw,event,type);
        }
    }
    return 0;
}

int gmpc_plugin_browser_add_go_menu(gmpcPluginParent *plug, GtkWidget *menu)
{
    if(gmpc_plugin_is_browser(plug)) {
        if(plug->new) {
            return gmpc_plugin_browser_iface_browser_add_go_menu((GmpcPluginBrowserIface *)plug->new, GTK_MENU(menu));
        }
        g_assert(plug->old->browser != NULL);
        if(plug->old->browser->add_go_menu != NULL)
        {
            return plug->old->browser->add_go_menu(menu);
        }
    }
    return 0;
}

int gmpc_plugin_browser_song_list_option_menu(gmpcPluginParent *plug, GmpcMpdDataTreeview *tree, GtkMenu *menu) 
{
    if(gmpc_plugin_is_browser(plug))
    {
        if(plug->new) {
            /* TODO */
            return 0;
        }
        g_assert(plug->old->browser != NULL);
        if(plug->old->browser->song_list_option_menu)
        {
            return plug->old->browser->song_list_option_menu(tree, menu);
        }
    }
    return 0;
}

gboolean gmpc_plugin_browser_has_integrate_search(gmpcPluginParent *plug)
{
    if(gmpc_plugin_is_browser(plug))
    {
        return plug->old->browser->integrate_search != NULL;
    }
    return FALSE;
}
MpdData *gmpc_plugin_browser_integrate_search(gmpcPluginParent *plug, const int search_field, const gchar * query, GError **error)
{
    if(!gmpc_plugin_browser_has_integrate_search(plug)) return NULL;
    return plug->old->browser->integrate_search(search_field,query,error); 
}

gboolean gmpc_plugin_browser_integrate_search_field_supported(gmpcPluginParent *plug, const int search_field)
{
    if(!gmpc_plugin_browser_has_integrate_search(plug)) return FALSE;
	if(plug->old->browser->integrate_search_field_supported == NULL) return TRUE;
    return plug->old->browser->integrate_search_field_supported(search_field); 
}

gboolean gmpc_plugin_has_preferences(gmpcPluginParent *plug)
{
    if(plug->new) {
        return GMPC_PLUGIN_IS_PREFERENCES_IFACE(plug->new);
    }
    return (plug->old->pref != NULL);
}

void gmpc_plugin_preferences_construct(gmpcPluginParent *plug,GtkWidget *wid)
{
    if(gmpc_plugin_has_preferences(plug))
    {
        if(plug->new) {
            gmpc_plugin_preferences_iface_pane_construct(GMPC_PLUGIN_PREFERENCES_IFACE(plug->new), GTK_CONTAINER(wid));
            return;
        }
        g_assert(plug->old->pref != NULL);
        g_assert(plug->old->pref->construct);
        plug->old->pref->construct(wid);
    }
}

void gmpc_plugin_preferences_destroy(gmpcPluginParent *plug,GtkWidget *wid)
{
    if(gmpc_plugin_has_preferences(plug))
    {
        if(plug->new) {
            gmpc_plugin_preferences_iface_pane_destroy(GMPC_PLUGIN_PREFERENCES_IFACE(plug->new), GTK_CONTAINER(wid));
            return;
        }
        g_assert(plug->old->pref != NULL);
        g_assert(plug->old->pref->destroy);
        plug->old->pref->destroy(wid);
    }
}


gboolean gmpc_plugin_is_internal(gmpcPluginParent *plug)
{
    if(plug->new) {
        return (((plug->new->plugin_type)&GMPC_INTERNALL) != 0);
;
    }
    return (((plug->old->plugin_type)&GMPC_INTERNALL) != 0);
}

const int * gmpc_plugin_get_version(gmpcPluginParent *plug)
{
    if(plug->new) {
        gint length;
        return (int *)gmpc_plugin_base_get_version(plug->new, &length);
    }
    return (int *)plug->old->version;
}

int gmpc_plugin_get_type(gmpcPluginParent *plug)
{
    if(plug->new) {
        return plug->new->plugin_type;
    }
    return plug->old->plugin_type;
}

int gmpc_plugin_get_id(gmpcPluginParent *plug)
{
    if(plug->new) {
        return plug->new->id;
    }
    return plug->old->id;
}

gboolean gmpc_plugin_is_metadata(gmpcPluginParent *plug)
{
    return (plug->old->metadata != NULL);
}

int gmpc_plugin_metadata_get_priority(gmpcPluginParent *plug)
{
    if(gmpc_plugin_is_metadata(plug))
    {
        return plug->old->metadata->get_priority();
    }
    return 100;
}

void gmpc_plugin_metadata_set_priority(gmpcPluginParent *plug, int priority)
{
    if(gmpc_plugin_is_metadata(plug))
    {
        return plug->old->metadata->set_priority(priority);
    }
}

int gmpc_plugin_metadata_get_image(gmpcPluginParent *plug, mpd_Song *song, MetaDataType type, char **path)
{
    *path = NULL;
    if(gmpc_plugin_is_metadata(plug))
    {
        return plug->old->metadata->get_image(song, type, path);
    }
    return META_DATA_UNAVAILABLE;
}


gint  gmpc_plugin_tool_menu_integration (gmpcPluginParent  *plug, GtkMenu *menu)
{
    if(plug->new) {
        if(GMPC_PLUGIN_IS_TOOL_MENU_IFACE(plug->new)){
            return gmpc_plugin_tool_menu_iface_tool_menu_integration(GMPC_PLUGIN_TOOL_MENU_IFACE(plug->new), menu);
        }
        return 0;
    }
    if(plug->old->tool_menu_integration)
        return plug->old->tool_menu_integration(menu);
    return 0;
}

