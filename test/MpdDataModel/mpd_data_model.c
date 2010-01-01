/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2010 Qball Cow <qball@sarine.nl>
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
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "gmpc-mpddata-model.h"


/** 
 * Stuff to make linker happy 
 */
GObject *config = NULL;
GObject *gmw = NULL;
GObject *gmpc_signals = NULL;

const gchar *meta_data_get_uri(gpointer input)
{
    return NULL;
}

int cfg_get_single_value_as_int_with_default(GObject *config, const gchar *a, const gchar *b, int def)
{
    return def;
}

gchar * cfg_get_single_value_as_string_with_default(GObject *config, const gchar *a, const gchar *b, gchar* def)
{
    return g_strdup(def);
}
char * 	gmpc_signals_get_browser_markup	(GObject * self)
{
    return NULL;
}
void gmpc_meta_watcher_get_meta_path_callback()
{
}
void playlist3_show_error_message()
{
}
/**
 * Create test list 
 */
MpdData * get_test_list_songs(int length)
{
    MpdData *retv = NULL;
    int i = 0;
    for(;i<length;i++)
    {
        retv = mpd_new_data_struct_append(retv);
        retv->type = MPD_DATA_TYPE_SONG;
        retv->song = mpd_newSong();
        retv->song->file = g_strdup_printf("%i", i);
        retv->song->title = g_strdup_printf("%i", i);
        retv->song->pos = i;
        retv->song->time = i;
    }
    return mpd_data_get_first(retv);
}
MpdData * get_test_list_tags(int length, gboolean reverse)
{
    MpdData *retv = NULL;
    int i = 0;
    for(;i<length;i++)
    {
        retv = mpd_new_data_struct_append(retv);
        retv->type = MPD_DATA_TYPE_TAG;
        retv->tag= g_strdup_printf("%08i", (reverse)?length-i-1:i);
        retv->tag_type = 0;
    }
    return mpd_data_get_first(retv);
}
void test_songs_data_slow()
{
    GtkTreePath *path;
    GtkTreeIter iter;
    int length = 10000;
    int j = 0;
    MpdData *list = get_test_list_tags(length, FALSE);
    MpdData *list2 = get_test_list_tags(length-10, TRUE);
    GmpcMpdDataModel *model = gmpc_mpddata_model_new();
    g_assert(list != NULL);
    g_assert(list2 != NULL);
    g_assert(model != NULL);
    
    gmpc_mpddata_model_set_mpd_data_slow(GMPC_MPDDATA_MODEL(model), list);
    /* this should merge the 2 list, ignoring duplicates */
    gmpc_mpddata_model_set_mpd_data_slow(GMPC_MPDDATA_MODEL(model), list2);

    for(j=0; j< length-10;j++){
        path = gtk_tree_path_new_from_indices(j, -1);
        gchar *pos;

        g_assert(gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path));
        gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, MPDDATA_MODEL_COL_SONG_TITLE, &pos,-1);
        /* pos always has a 1 offset. this is because if you want to show the index number, you want to start at 1 */
        g_assert(pos != NULL);
        g_assert_cmpint(atoi(pos), ==, j);
        g_free(pos);
        g_assert_cmpint(gmpc_mpddata_model_get_pos(model, &iter), ==, j);
        gtk_tree_path_free(path);
    }
    /* Not allowed to be next iter */
    path = gtk_tree_path_new_from_indices(j, -1);
    g_assert(!gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path));
    gtk_tree_path_free(path);

    g_object_unref(model);
}

/**
 * This tests the function that turns a GtkTreePAth into an iter.
 * With the has_up
 */
void test_songs_path_to_iter_with_up()
{
    int length = 10000;
    int j = 0;
    MpdData *list = get_test_list_songs(length);
    GmpcMpdDataModel *model = gmpc_mpddata_model_new();
    g_assert(list != NULL);
    g_assert(model != NULL);
    gmpc_mpddata_model_set_has_up(model, TRUE);
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(model), list);
    /* Skip the first, as that is up */
    for(j=1; j< length;j++){
        GtkTreePath *path = gtk_tree_path_new_from_indices(j, -1);
        GtkTreeIter iter;
        int pos;
        mpd_Song *song;
        g_assert(gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path));
        gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, MPDDATA_MODEL_COL_SONG_POS, &pos,MPDDATA_MODEL_COL_MPDSONG, &song,-1);
        /* pos always has a 1 offset minus the _up_ offset. this is because if you want to show the index number, you want to start at 1 */
        g_assert_cmpint(pos, ==, j);
        /* song pos should match */
        g_assert_cmpint(song->pos+1, == , j);

        g_assert_cmpint(gmpc_mpddata_model_get_pos(model, &iter), ==, j);
        gtk_tree_path_free(path);
    }


    g_object_unref(model);
}


/**
 * This tests the function that turns a GtkTreePAth into an iter.
 */
void test_songs_path_to_iter()
{
    int length = 10000;
    int j = 0;
    MpdData *list = get_test_list_songs(length);
    GmpcMpdDataModel *model = gmpc_mpddata_model_new();

    g_assert(list != NULL);
    g_assert(model != NULL);
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(model), list);

    for(j=0; j< length;j++){
        GtkTreePath *path = gtk_tree_path_new_from_indices(j, -1);
        GtkTreeIter iter;
        int pos;
        mpd_Song *song;
        g_assert(gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path));
        gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, MPDDATA_MODEL_COL_SONG_POS, &pos,MPDDATA_MODEL_COL_MPDSONG, &song,-1);
        /* pos always has a 1 offset. this is because if you want to show the index number, you want to start at 1 */
        g_assert_cmpint(pos-1, ==, j);
        /* song pos should match */
        g_assert_cmpint(song->pos, == , j);

        g_assert_cmpint(gmpc_mpddata_model_get_pos(model, &iter), ==, j);
        gtk_tree_path_free(path);
    }


    g_object_unref(model);
}

void test_songs_total_playtime()
{
    long unsigned playtime = 0;
    int length = 10000;
    int j = 0;
    MpdData *list = get_test_list_songs(length);
    GmpcMpdDataModel *model = gmpc_mpddata_model_new();

    g_assert(list != NULL);
    g_assert(model != NULL);
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(model), list);
    
    playtime = gmpc_mpddata_model_get_playtime(GMPC_MPDDATA_MODEL(model));
    g_assert(playtime == 49995000); 

    g_object_unref(model);
}

void test_songs_num_rows()
{

    long unsigned playtime = 0;
    int length = 10000;
    int j = 0;
    MpdData *list = get_test_list_songs(length);
    GmpcMpdDataModel *model = gmpc_mpddata_model_new();

    g_assert(list != NULL);
    g_assert(model != NULL);
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(model), list);

    g_assert(model->num_rows == length);
    g_assert(gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), NULL) == length);

    g_object_unref(model);
}

void test_songs_num_rows_with_up()
{

    long unsigned playtime = 0;
    int length = 10000;
    int j = 0;
    MpdData *list = get_test_list_songs(length);
    GmpcMpdDataModel *model = gmpc_mpddata_model_new();

    g_assert(list != NULL);
    g_assert(model != NULL);

    gmpc_mpddata_model_set_has_up(model, TRUE);
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(model), list);

    g_assert(model->num_rows == length+1);
    g_assert(gtk_tree_model_iter_n_children(GTK_TREE_MODEL(model), NULL) == length+1);

    g_object_unref(model);
}

void test_iter_children()
{
    long unsigned playtime = 0;
    int length = 10000;
    int j = 0;
    GtkTreeIter iter, parent;
    MpdData *list = get_test_list_songs(length);
    GmpcMpdDataModel *model = gmpc_mpddata_model_new();

    g_assert(model != NULL);
    g_assert(list != NULL);

    g_assert(!gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &iter, NULL));


    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(model), list);

    g_assert(gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &parent, NULL));
    /* but no child */
    g_assert(!gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &iter, &parent));

    g_object_unref(model);
}
void test_iter_children_has_up()
{
    long unsigned playtime = 0;
    GtkTreeIter iter, parent;
    GmpcMpdDataModel *model = gmpc_mpddata_model_new();

    g_assert(model != NULL);

    g_assert(!gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &iter, NULL));
    gmpc_mpddata_model_set_has_up(model, TRUE);
    g_assert(!gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &parent, NULL));

    /* but no child */
    g_assert(!gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &iter, &parent));

    g_object_unref(model);
}
int main(int argc, char **argv)
{
    GtkTreeIter iter;
    g_type_init();
    g_test_init(&argc, &argv, NULL);
    
    g_test_add_func("/GmpcMpdDataModel/num_rows", test_songs_num_rows);
    g_test_add_func("/GmpcMpdDataModel/num_rows_with_up", test_songs_num_rows_with_up);
    g_test_add_func("/GmpcMpdDataModel/path_to_iter", test_songs_path_to_iter);
    g_test_add_func("/GmpcMpdDataModel/path_to_iter_with_up", test_songs_path_to_iter_with_up);
    g_test_add_func("/GmpcMpdDAtaModel/test_songs_data_slow", test_songs_data_slow);
    g_test_add_func("/GmpcMpdDataModel/total_playtime", test_songs_total_playtime);
    g_test_add_func("/GmpcMpdDataModel/iter_children",  test_iter_children);
    g_test_add_func("/GmpcMpdDataModel/iter_children_has_up",  test_iter_children_has_up);

    return g_test_run();
}
