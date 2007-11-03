#include <glib.h>
#include <gtk/gtk.h>
#include <gmodule.h>
#include "main.h"
#include "metadata.h"


gmpcPlugin **plugins = NULL;
int num_plugins = 0;

gmpcPlugin * plugin_get_from_id(int id)
{
	return plugins[plugin_get_pos(id)];
}


int plugin_get_pos(int id)
{
	return id&(PLUGIN_ID_MARK-1);
}
static int plugin_validate(gmpcPlugin *plug)
{
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
		show_error_message(message,FALSE);
		q_free(string);
		q_free(message);
		g_module_close(handle);
		return 1;
	}

	if(!g_module_symbol(handle, "plugin", (gpointer)&plug)){
		gchar *message = g_strdup_printf("Failed to load plugin:\n%s:%s",file, g_module_error());
		debug_printf(DEBUG_ERROR, "plugin_load: symbol failed to bind: %s:%s\n",file, g_module_error());
		show_error_message(message,FALSE);
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

void plugin_load_dir(gchar *path)
{
    GDir *dir = g_dir_open(path, 0, NULL);
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
}
