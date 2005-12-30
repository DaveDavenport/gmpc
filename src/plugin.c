#include <glib.h>
#include <gtk/gtk.h>
#include <gmodule.h>
#include "main.h"

int plugin_get_pos(int id)
{
	return id&(PLUGIN_ID_MARK-1);
}

void plugin_add(gmpcPlugin *plug, int plugin)
{
	/* set plugin id */
	plug->id = num_plugins|((plugin)?PLUGIN_ID_MARK:PLUGIN_ID_INTERNALL);
	printf("plugin %i %s\n", plug->id, plug->name);
	/* put it in the list */
	num_plugins++;
	plugins = g_realloc(plugins,(num_plugins+1)*sizeof(gmpcPlugin **));
	plugins[num_plugins-1] = plug;
	plugins[num_plugins] = NULL;

}
int plugin_load(char *path, const char *file)
{
	GModule *handle;
	gmpcPlugin *plug = NULL;
	gchar *string = NULL;
	gchar *full_path = g_strdup_printf("%s/%s", path, file);
	debug_printf(DEBUG_INFO, "plugin_load: trying to load plugin %s", full_path);
	if(path == NULL)
	{
		return 1;
	}
	handle = g_module_open(full_path, G_MODULE_BIND_LOCAL);
	g_free(full_path);
	if (!handle) {
		gchar *message = g_strdup_printf("Failed to load plugin:\n<i>%s</i>", g_module_error());
		debug_printf (DEBUG_ERROR, "plugin_load: module failed to load: %s\n", g_module_error());
		show_error_message(message);
		g_free(message);
		return 1;
	}
	if(!g_module_symbol(handle, "plugin", (gpointer)&plug)){
		gchar *message = g_strdup_printf("Failed to load plugin:\n<i>%s</i>", g_module_error());
		debug_printf(DEBUG_ERROR, "plugin_load: symbol failed to bind: %s\n", g_module_error());
		show_error_message(message);
		g_free(string);
		g_free(message);
		g_module_close(handle);
		return 1;
	}
	if(plug == NULL)
	{
		debug_printf(DEBUG_WARNING, "plugin load: unkown type of plugin.\n");
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
			gchar *full_path = g_strdup_printf("%s/%s",path,dirname);
			printf("%s\n", full_path);
			if(g_file_test(full_path, G_FILE_TEST_IS_REGULAR))
			{
				if(!plugin_load(path,dirname))
				{
					printf("%i. plugin '%s' loaded ",
							plugins[num_plugins-1]->id,
							plugins[num_plugins-1]->name);
					switch(plugins[num_plugins-1]->plugin_type){
						case GMPC_PLUGIN_DUMMY:
							printf("type: dummy\n");
							break;
						case GMPC_PLUGIN_PL_BROWSER:
							printf("type: playlist browser\n");
							break;
						case GMPC_PLUGIN_NO_GUI:
							printf("type: no gui\n");
							break;
						default:
							printf("type: unkown\n");
							break;
					}
				}
			}
			g_free(full_path);
		}
		g_dir_close(dir);
	}
}
