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

void downloaded2(const GEADAsyncHandler *handle, GEADStatus status, gpointer user_data)
{
    GMainLoop *loop = user_data;
    if(status == GEAD_PROGRESS) return;
    else if(status == GEAD_DONE){
        gsize length;
        const guchar *data = gmpc_easy_handler_get_data(handle, &length); 
        printf("Download: %li bytes\n", length);

    }else{
        g_error("Download failed\n");
    }
    g_main_loop_quit(loop);
}

void downloaded(const GEADAsyncHandler *handle, GEADStatus status, gpointer user_data)
{
    GMainLoop *loop = user_data;
    if(status == GEAD_PROGRESS) return;
    else if(status == GEAD_DONE){
        GEADAsyncHandler *handle2;
        gsize length;
        const guchar *data = gmpc_easy_handler_get_data(handle, &length); 
        printf("Download: %li bytes\n", length);
        handle2 = gmpc_easy_async_downloader("http://maps.google.com", downloaded2, loop);
    }else{
        g_error("Download failed\n");
        g_main_loop_quit(loop);
    }
}
int main(int argc, char **argv)
{
    GEADAsyncHandler *handle;
    GMainLoop *loop;
    g_type_init(); 
    loop = g_main_loop_new(NULL, FALSE);
    if(!g_thread_supported())
        g_thread_init(NULL);
    handle = gmpc_easy_async_downloader("http://www.google.com", downloaded, loop);
    g_main_loop_run(loop);
    gmpc_easy_async_quit();
    g_main_loop_unref(loop);
    return EXIT_FAILURE;
}
