#include <glib.h>
#include <gtk/gtk.h>
#include <gmodule.h>
#include "main.h"





int plugin_load(char *path)
{
	GModule *handle;
	gmpcPlugin *plug = NULL;
	if(path == NULL)
	{
		return 1;
	}
	handle = g_module_open(path, G_MODULE_BIND_LAZY);
	if (!handle) {
		fprintf (stderr, "%s\n", g_module_error());
		return 1;
	}

	if(!g_module_symbol(handle, "plugin", (gpointer)&plug)){
		fprintf (stderr, "%s\n", g_module_error());
		g_module_close(handle);
		return 1;
	}

	if(plug == NULL)
	{
		fprintf (stderr, "failed to get plug\n");
		g_module_close(handle);
		return 1;
	}
	plug->id = num_plugins|PLUGIN_ID_MARK;
	num_plugins++;
	plugins = g_realloc(plugins,(num_plugins+1)*sizeof(gmpcPlugin **));
	plugins[num_plugins-1] = plug;
	plugins[num_plugins] = NULL;
	return 0;
}

void load_plugins_from_dir(gchar *path)
{
	GDir *dir = g_dir_open(path, 0, NULL);
	if(dir)
	{
		const gchar *dirname = NULL;
		while((dirname = g_dir_read_name(dir)) != NULL)
		{
			gchar *full_path = g_strdup_printf("%s/%s", path, dirname);	
			if(!plugin_load(full_path))
			{
				printf("%i. plugin '%s' loaded ",plugins[num_plugins-1]->id^PLUGIN_ID_MARK, plugins[num_plugins-1]->name);
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
			g_free(full_path);
		}
		g_dir_close(dir);
	}
}
