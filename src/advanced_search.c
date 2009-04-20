#include <config.h>
#include <gtk/gtk.h>
#include "main.h"
#include "misc.h"
#include "advanced_search.h"

static GRegex *search_regex = NULL;
void advanced_search_init(void)
{
    int i =0;
    GString *string = g_string_new("(");
    for(i=0;i<MPD_TAG_NUM_OF_ITEM_TYPES;i++)
    {
        if(mpd_server_tag_supported(connection,i))
        {
            g_string_append(string, mpdTagItemKeys[i]);
            if(i< (MPD_TAG_NUM_OF_ITEM_TYPES-1))
                g_string_append(string,"|");
        }
    }
    g_string_append(string, ")[ ]*[=:][ ]*|[ ]*(\\|\\|)[ ]*");
    search_regex = g_regex_new(string->str, G_REGEX_CASELESS, 0, NULL);
    g_string_free(string, TRUE);
}
void advanced_search_update_taglist(void)
{
    g_regex_unref(search_regex);
    advanced_search_init();
}
void advanced_search_destroy(void)
{
    if(search_regex) g_regex_unref(search_regex);
    search_regex = NULL;
}

MpdData * advanced_search(const gchar *query, int in_playlist)
{
    MpdData *data = NULL, *data_t = NULL;
    gchar **text = g_regex_split(search_regex, query, 0);
    int i= 0, found=0,type;
    for(i=0; text && text[i] ;i++)
    {
        if(strcmp(text[i], "||") == 0){
            if(in_playlist)
                data = mpd_playlist_search_commit(connection);
            else
                data = mpd_database_search_commit(connection);
            data_t = mpd_data_concatenate(data_t, data);
            data = NULL;
            found = FALSE;
            continue;
        }

        if(text[i][0] == '\0')continue;

        type = mpd_misc_get_tag_by_name(g_strstrip(text[i]));
        if(type != MPD_TAG_NOT_FOUND && text[i+1])
        {
            gchar **split = tokenize_string(text[i+1]);
            int j;
            for(j=0;split && split[j];j++)
            {
                if(!found){
                    if(in_playlist)
                        mpd_playlist_search_start(connection, FALSE);
                    else
                        mpd_database_search_start(connection, FALSE);
                    found= 1;
                }
                if(in_playlist)
                    mpd_playlist_search_add_constraint(connection, type,split[j]);
                else
                    mpd_database_search_add_constraint(connection,type,split[j]);
            }
            if(split)g_strfreev(split);
            i++;
        }
        else 
        {
            gchar **split = tokenize_string(text[i]);
            int j;
            for(j=0;split && split[j];j++)
            {
                if(!found){
                    if(in_playlist)
                        mpd_playlist_search_start(connection, FALSE);
                    else
                        mpd_database_search_start(connection, FALSE);
                    found = 1;
                }
                if(in_playlist)
                    mpd_playlist_search_add_constraint(connection,MPD_TAG_ITEM_ANY,split[j]);
                else
                    mpd_database_search_add_constraint(connection,MPD_TAG_ITEM_ANY,split[j]);
            }
            if(split)g_strfreev(split);
        }
    }
    if(found){
            if(in_playlist)
                data = mpd_playlist_search_commit(connection);
            else
                data = mpd_database_search_commit(connection);
            data_t = mpd_data_concatenate(data_t, data);
    }

    data_t = misc_mpddata_remove_duplicate_songs(data_t);
    return data_t;
}
/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
