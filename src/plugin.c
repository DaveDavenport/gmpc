#include <glib.h>
#include <gtk/gtk.h>
#include <gmodule.h>
#include "main.h"

int plugin_load(char *path, char *file)
{
	GModule *handle;
	gmpcPlugin *plug = NULL;
	gchar *string = NULL;
	gchar *full_path = g_strdup_printf("%s/%s", path, file);	
	if(path == NULL)
	{
		return 1;
	}
	handle = g_module_open(full_path, G_MODULE_BIND_LAZY);
	g_free(full_path);
	if (!handle) {
		fprintf (stderr, "%s\n", g_module_error());

		return 1;
	}
	string = g_strndup(file, strlen(file)-3);
	if(!g_module_symbol(handle, string, (gpointer)&plug)){
		fprintf (stderr, "%s\n", g_module_error());
		g_free(string);
		g_module_close(handle);
		return 1;
	}
	g_free(string);
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
		
			if(!plugin_load(path,dirname))
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
		}
		g_dir_close(dir);
	}
}
