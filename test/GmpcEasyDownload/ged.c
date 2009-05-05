#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>
#include "../../src/gmpc_easy_download.h"

gpointer config = NULL;

void playlist3_show_error_message(const char *message, int el)
{

}
/**
 * Dummy config system 
 */
int cfg_get_single_value_as_int_with_default(gpointer *config, const gchar *a, const gchar *b, int def)
{
    return def;
}
gchar * cfg_get_single_value_as_string_with_default(gpointer *config, const gchar *a, const gchar *b, gchar* def)
{
    return g_strdup(def);
}
gchar *cfg_get_single_value_as_string(gpointer *config, const gchar *a, const gchar *b)
{
    return NULL;
}
void cfg_set_single_value_as_string(gpointer *config, const gchar *a, const gchar *b, gchar *def)
{
}

void cfg_set_single_value_as_int(gpointer *config, const gchar *a, const gchar *b, int def )
{
}

void downloaded(const GEADAsyncHandler *handle, GEADStatus status, gpointer user_data)
{
    if(status == GEAD_PROGRESS) return;
    else if(status == GEAD_DONE){
        gsize length;
        const guchar *data = gmpc_easy_handler_get_data(handle, &length); 
        printf("Download: %li bytes\n", length);
    }else{
        g_error("Download failed\n");
    }

    gtk_main_quit();
}
int main(int argc, char **argv)
{
    GEADAsyncHandler *handle;
    if(!g_thread_supported())
        g_thread_init(NULL);
    gtk_init(&argc, &argv);
    handle = gmpc_easy_async_downloader("http://www.google.nl", downloaded, NULL);
    gtk_main();
    return EXIT_FAILURE;
}
