/*
 * Copyright (C) 2004-2006 Qball Cow <qball@sarine.nl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <string.h>
#include <glade/glade.h>
#include <config.h>
#include "main.h"
#include "misc.h"
#include "id3info.h"

extern config_obj *cover_index;
GladeXML *xml_id3_window = NULL;
GList *songs = NULL;
static void set_text (GList * node);

/* Glade declarations, without glade these would be static*/
void id3_status_update(void);
void id3_save_artist_txt(void);
void id3_save_album_txt(void);
void remove_id3_window(void);
void id3_next_song(void);
void id3_last_song(void);
void id3_info_clear_album_image(void);
void id3_album_image_file_selector(GtkFileChooser *);
void id3_reget_album_art(void);
void id3_info_clear_artist_image(void);
void id3_artist_image_file_selector(GtkFileChooser *);
void id3_reget_artist_art(void);
void id3_save_song_lyric(void);
void id3_reget_lyric_txt(void);
void id3_reget_artist_txt(void);
void id3_reget_album_txt(void);
void id3_del_album_txt(void);
void id3_del_artist_txt(void);
void id3_del_song_lyric(void);
static void art_set_from_path(GtkWidget *metaimage, int type, GtkWidget *vbox);

/**
 * The Functions 
 */
static void art_set_from_path(GtkWidget *metaimage, int type, GtkWidget *vbox)
{
    gtk_widget_set_sensitive(vbox, TRUE);
}

void id3_del_album_txt(void)
{
    if(songs == NULL || songs->data == NULL)
        return;
    mpd_Song *sg = songs->data;
    gchar *temp = g_strdup_printf("albumtxt:%s", sg->album);
    cfg_set_single_value_as_string(cover_index, sg->artist,temp,"");
    q_free(temp);

    GtkTextBuffer *buffer = NULL;
    GtkTextView *tv = NULL;
    /** Get the text view */
    tv = (GtkTextView *)glade_xml_get_widget(xml_id3_window, "album_tv");
    buffer = gtk_text_view_get_buffer(tv);
    gtk_text_buffer_set_text(buffer, "", -1);
}

void id3_save_album_txt()
{
    /**
     * Safety check
     */
    if(songs != NULL && songs->data != NULL && ((mpd_Song *)(songs->data))->artist && ((mpd_Song *)(songs->data))->album)
    {
        GtkWidget *parent = glade_xml_get_widget(xml_id3_window, "id3_info_window");
        GtkWidget *fs_dialog = gtk_file_chooser_dialog_new(_("Save Album Text"), 
                GTK_WINDOW(parent),GTK_FILE_CHOOSER_ACTION_SAVE, 
                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
        gchar *filename = NULL, *temp = NULL;
        /* get s direct pointer */
        mpd_Song *sg = songs->data;

        /* Get stored path */
        temp = g_strdup_printf("albumtxt:%s", sg->album);
        filename = cfg_get_single_value_as_string(cover_index, sg->artist,temp);
        if(filename)
        {
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fs_dialog),filename);
            cfg_free_string(filename);
        }

        switch(gtk_dialog_run(GTK_DIALOG(fs_dialog)))
        {
            case GTK_RESPONSE_OK:
                {
                    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs_dialog));
                    /**
                     * Implement saving and setting db 
                     */
                    {
                        char *content =NULL; 
                        GtkTextIter end, start;
                        GtkTextBuffer *buffer = NULL;
                        GtkTextView *tv = NULL;
                        /** Get the text view */
                        tv = (GtkTextView *)glade_xml_get_widget(xml_id3_window, "album_tv");
                        buffer = gtk_text_view_get_buffer(tv);
                        /* get start and end */
                        gtk_text_buffer_get_start_iter(buffer, &start);
                        gtk_text_buffer_get_end_iter(buffer, &end);
                        /** get content */
                        content = gtk_text_buffer_get_text(buffer,&start, &end,TRUE); 
                        if(g_file_set_contents(filename, content, -1,NULL))
                        {
                            cfg_set_single_value_as_string(cover_index, sg->artist,temp,filename);
                        }

                        /* free content */
                        q_free(content);
                    }
                    q_free(filename);
                }
            default:
                break;
        }
        gtk_widget_destroy(fs_dialog);	
        q_free(temp);
    }
}

void id3_del_artist_txt(void)
{
    if(songs != NULL && songs->data != NULL && ((mpd_Song*)songs->data)->artist != NULL)
    { 
        mpd_Song *sg = songs->data;
        cfg_set_single_value_as_string(cover_index, sg->artist,"biography","");
        GtkTextBuffer *buffer = NULL;
        GtkTextView *tv = NULL;
        /** Get the text view */
        tv = (GtkTextView *)glade_xml_get_widget(xml_id3_window, "artist_tv");
        buffer = gtk_text_view_get_buffer(tv);
        gtk_text_buffer_set_text(buffer, "",-1);
    }
}

void id3_save_artist_txt()
{
    /**
     * Safety check
     */
    if(songs != NULL && songs->data != NULL && ((mpd_Song*)songs->data)->artist)
    {
        GtkWidget *parent = glade_xml_get_widget(xml_id3_window, "id3_info_window");
        GtkWidget *fs_dialog = gtk_file_chooser_dialog_new(_("Save Artist Text"), 
                GTK_WINDOW(parent),GTK_FILE_CHOOSER_ACTION_SAVE, 
                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
        gchar *filename = NULL;
        mpd_Song *sg = songs->data;

        filename = cfg_get_single_value_as_string(cover_index, sg->artist,"biography");
        if(filename)
        {
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fs_dialog),filename);
            cfg_free_string(filename);
        }
        switch(gtk_dialog_run(GTK_DIALOG(fs_dialog)))
        {
            case GTK_RESPONSE_OK:
                {
                    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs_dialog));
                    /**
                     * Implement saving and setting db 
                     */
                    {
                        char *content =NULL; 
                        GtkTextIter end, start;
                        GtkTextBuffer *buffer = NULL;
                        GtkTextView *tv = NULL;
                        /** Get the text view */
                        tv = (GtkTextView *)glade_xml_get_widget(xml_id3_window, "artist_tv");
                        buffer = gtk_text_view_get_buffer(tv);
                        /* get start and end */
                        gtk_text_buffer_get_start_iter(buffer, &start);
                        gtk_text_buffer_get_end_iter(buffer, &end);
                        /** get content */
                        content = gtk_text_buffer_get_text(buffer,&start, &end,TRUE); 
                        if(g_file_set_contents(filename, content, -1,NULL))
                        {
                            cfg_set_single_value_as_string(cover_index, sg->artist, "biography",filename);
                        }

                        /* free content */
                        q_free(content);
                    }
                    q_free(filename);
                }
            default:
                break;
        }
        gtk_widget_destroy(fs_dialog);
    }
}


static void id3_txt_fetched(mpd_Song *song,MetaDataResult ret, char *path,gpointer data)
{
    int type = GPOINTER_TO_INT(data)&META_QUERY_DATA_TYPES;
    GtkWidget *view, *container;
    mpd_Song *current = NULL;
    if(songs == NULL || song == NULL) return;
    if(songs->data== NULL) return;
    current = songs->data;
    switch(type)
    {
        case META_ARTIST_TXT:
            view = glade_xml_get_widget(xml_id3_window, "artist_tv");
            container = glade_xml_get_widget(xml_id3_window, "artist_container");
            break;
        case META_ALBUM_TXT:
            view = glade_xml_get_widget(xml_id3_window, "album_tv");
            container = glade_xml_get_widget(xml_id3_window, "album_container");
            break;
        default:
            view = glade_xml_get_widget(xml_id3_window, "lyric_tv");
            container = glade_xml_get_widget(xml_id3_window, "lyric_container");
            break;
    }
    if(current->file)
    {
        if(!strcmp(current->file,song->file))
        {
            if(ret == META_DATA_FETCHING)
            {
                gtk_text_buffer_set_text(GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view))),_("Working...."), -1);
                gtk_widget_set_sensitive(GTK_WIDGET(container), FALSE);
            }
            else if(ret == META_DATA_AVAILABLE)
            {
                gsize size;
                char *content = NULL;

                g_file_get_contents(path, &content, &size,NULL);
                gtk_text_buffer_set_text(GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view))),content, size);
                q_free(content);

                gtk_widget_set_sensitive(GTK_WIDGET(container), TRUE);
            }
            else if(ret == META_DATA_UNAVAILABLE)
            {
                gtk_text_buffer_set_text(GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view))),"", -1);
                gtk_widget_set_sensitive(GTK_WIDGET(container), TRUE);
            }
        }
    }
}
void id3_del_song_lyric(void)
{
    if(songs != NULL && songs->data != NULL&& ((mpd_Song*)songs->data)->title && ((mpd_Song*)songs->data)->artist)
    {
        mpd_Song *sg = songs->data;
        char *temp = g_strdup_printf("lyrics:%s", sg->title);    
        cfg_set_single_value_as_string(cover_index, sg->artist,temp,"");
        q_free(temp);
        GtkTextBuffer *buffer = NULL;
        GtkTextView *tv = NULL;
        /** Get the text view */
        tv = (GtkTextView *)glade_xml_get_widget(xml_id3_window, "lyric_tv");
        buffer = gtk_text_view_get_buffer(tv);
        gtk_text_buffer_set_text(buffer, "",-1);
    }
}
void id3_save_song_lyric()
{
    /**
     * Safety check
     */
    if(songs != NULL && songs->data != NULL&& ((mpd_Song*)songs->data)->title && ((mpd_Song*)songs->data)->artist)
    {
        GtkWidget *parent = glade_xml_get_widget(xml_id3_window, "id3_info_window");
        GtkWidget *fs_dialog = gtk_file_chooser_dialog_new(_("Save Lyric"), 
                GTK_WINDOW(parent),GTK_FILE_CHOOSER_ACTION_SAVE, 
                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
        mpd_Song *sg = songs->data;

        /* get old value */
        char *temp = g_strdup_printf("lyrics:%s", sg->title);    
        gchar *filename = cfg_get_single_value_as_string(cover_index, sg->artist,temp);
        if(filename)
        {
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fs_dialog),filename);
            cfg_free_string(filename);
        }

        switch(gtk_dialog_run(GTK_DIALOG(fs_dialog)))
        {
            case GTK_RESPONSE_OK:
                {
                    gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs_dialog));
                    /**
                     * Implement saving and setting db 
                     */
                    {
                        char *content =NULL; 
                        GtkTextIter end, start;
                        GtkTextBuffer *buffer = NULL;
                        GtkTextView *tv = NULL;
                        mpd_Song *sg = songs->data;
                        /** Get the text view */
                        tv = (GtkTextView *)glade_xml_get_widget(xml_id3_window, "lyric_tv");
                        buffer = gtk_text_view_get_buffer(tv);
                        /* get start and end */
                        gtk_text_buffer_get_start_iter(buffer, &start);
                        gtk_text_buffer_get_end_iter(buffer, &end);
                        /** get content */
                        content = gtk_text_buffer_get_text(buffer,&start, &end,TRUE); 
                        if(g_file_set_contents(filename, content, -1,NULL))
                        {
                            cfg_set_single_value_as_string(cover_index, sg->artist, temp,filename);
                        }

                        /* free content */
                        q_free(content);
                    }
                    q_free(filename);
                }
            default:
                break;
        }
        gtk_widget_destroy(fs_dialog);
        q_free(temp);
    }
}

void remove_id3_window ()
{
    GtkWidget *window =
        glade_xml_get_widget (xml_id3_window, "id3_info_window");
    /* destroy and free memory */
    if (window)
        gtk_widget_destroy (window);
    if (xml_id3_window != NULL)
        g_object_unref (xml_id3_window);
    xml_id3_window = NULL;

    /* Clean up list and free it */ 
    songs = g_list_first(songs);
    while ((songs = g_list_next (songs)))
    {
        mpd_freeSong (songs->data);
        songs->data = NULL;
    }
    g_list_free (songs);
    songs = NULL;
}


void id3_status_update()
{
    mpd_Song *song = NULL;
    if(xml_id3_window == NULL) return;
    if(songs == NULL) return;
    song =  songs->data;
    if(song == NULL) return;
    if(song->id == mpd_player_get_current_song_id(connection) && song->id != MPD_SONG_NO_ID)
    {
        char *temp = g_strdup_printf("%i kbps", mpd_status_get_bitrate(connection));
        gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_id3_window,"bitrate_label")),temp);
        q_free(temp);

    }
    else
    {
        gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml_id3_window,"bitrate_label")),"<i>only available for playing song</i>");
    }
    if (song->time != MPD_SONG_NO_TIME)
    {
        gint min = (int) (song->time / 60);
        gint sec = song->time - min * 60;
        gchar *buf1 =NULL;
        if(song->id == mpd_player_get_current_song_id(connection))
        {
            buf1 = g_strdup_printf ("%02i:%02i/%02i:%02i",
                    mpd_status_get_elapsed_song_time(connection)/60,
                    mpd_status_get_elapsed_song_time(connection)%60,
                    min, sec);
        }
        else
        {
            buf1= g_strdup_printf ("%02i:%02i", min, sec);
        }
        gtk_label_set_text (GTK_LABEL
                (glade_xml_get_widget
                 (xml_id3_window, "length_label")), buf1);
        g_free (buf1);
    }
    else
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "length_label")), "");
    }

}

static void create_window (int song)
{
    char *path;
    mpd_Song *songstr = mpd_playlist_get_song(connection, song);

    if(songstr == NULL)
    {
        return;
    }
    path = gmpc_get_full_glade_path("gmpc.glade");	
    xml_id3_window = glade_xml_new (path,"id3_info_window", NULL);
    q_free(path);
    /* check for errors and axit when there is no gui file */
    if (xml_id3_window == NULL)
    {
        g_error ("Couldnt initialize GUI. Please check installation\n");
    }
    gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")),META_ALBUM_ART);
    g_signal_connect(G_OBJECT(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")), "image_changed",
            G_CALLBACK(art_set_from_path), 
            glade_xml_get_widget(xml_id3_window, "album_vbox"));
    gmpc_metaimage_set_size(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")),300);

    gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")),META_ARTIST_ART);
    gmpc_metaimage_set_size(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")),300);                 	
    g_signal_connect(G_OBJECT(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")), "image_changed",
            G_CALLBACK(art_set_from_path), 
            glade_xml_get_widget(xml_id3_window, "artist_vbox"));                                                    	
    glade_xml_signal_autoconnect (xml_id3_window);

    songs = g_list_append (songs, songstr);
    set_text (songs);
}

static void set_text (GList * node)
{
    mpd_Song *song;
    if (node == NULL)
    {
        remove_id3_window ();
        return;
    }
    song = node->data;
    gtk_widget_set_sensitive(glade_xml_get_widget(xml_id3_window, "album_vbox"), FALSE);
    gmpc_metaimage_update_cover_from_song(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")),song);
    gtk_widget_set_sensitive(glade_xml_get_widget(xml_id3_window, "artist_vbox"), FALSE);
    gmpc_metaimage_update_cover_from_song(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")),song);


    if (song->artist != NULL)
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "artist_label")), song->artist);
        gtk_widget_show(glade_xml_get_widget(xml_id3_window, "artist_vbox"));
        gtk_widget_show(glade_xml_get_widget(xml_id3_window, "artist_container"));
    }
    else
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "artist_label")), "");
        gtk_widget_hide(glade_xml_get_widget(xml_id3_window, "artist_vbox"));
        gtk_widget_hide(glade_xml_get_widget(xml_id3_window, "artist_container"));
    }
    if (song->title != NULL)
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "title_label")), song->title);
        gtk_widget_show(glade_xml_get_widget(xml_id3_window, "lyric_container"));
    }
    else
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "title_label")), "");
        gtk_widget_hide(glade_xml_get_widget(xml_id3_window, "lyric_container"));
    }
    if (song->album != NULL)
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "album_label")), song->album);
        gtk_widget_show(glade_xml_get_widget(xml_id3_window, "album_vbox"));
        gtk_widget_show(glade_xml_get_widget(xml_id3_window, "album_container"));
    }
    else if (song->name != NULL)
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "album_label")), song->name);
        gtk_widget_hide(glade_xml_get_widget(xml_id3_window, "album_vbox"));
        gtk_widget_hide(glade_xml_get_widget(xml_id3_window, "album_container"));
    }
    else
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "album_label")), "");
        gtk_widget_hide(glade_xml_get_widget(xml_id3_window, "album_vbox"));
        gtk_widget_hide(glade_xml_get_widget(xml_id3_window, "album_container"));
    }
    if (song->date != NULL)
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "date_label")), song->date);
    }
    else
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "date_label")), "");
    }

    if (song->track != NULL)
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "track_label")), song->track);
    }
    else
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "track_label")), "");
    }
    if (song->genre != NULL)
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "genre_label")), song->genre);
    }
    else
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "genre_label")), "");
    }
    if (song->disc != NULL)
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "disc_label")), song->disc);
    }
    else
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "disc_label")), "");
    }

    if (song->comment != NULL)
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "comment_label")), song->comment);
    }
    else                                                                                                   	
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "comment_label")), "");
    }                                                                                                      	


    if (song->composer != NULL)
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "composer_label")), song->composer);
    }
    else
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "composer_label")), "");
    }
    if (song->file != NULL)
    {
        gchar *buf1 = g_path_get_basename (song->file);
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "filename_label")), buf1);
        g_free (buf1);
    }
    else
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "filename_label")), "");
    }
    if (song->file != NULL)
    {
        gchar *buf1 = g_path_get_dirname (song->file);
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "path_label")), buf1);
        g_free (buf1);
    }
    else
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "path_label")), "");
    }

    if(song->id == mpd_player_get_current_song_id(connection) && song->id != MPD_SONG_NO_ID)
    {
        char *temp = g_strdup_printf("%i kbps", mpd_status_get_bitrate(connection));
        gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_id3_window,"bitrate_label")),temp);
        q_free(temp);

    }
    else
    {
        gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml_id3_window,"bitrate_label")),"<i>only available for playing song</i>");
    }

    if (song->time != MPD_SONG_NO_TIME)
    {
        gint min = (int) (song->time / 60);
        gint sec = song->time - min * 60;
        gchar *buf1 = g_strdup_printf ("%02i:%02i", min, sec);
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "length_label")), buf1);
        g_free (buf1);
    }
    else
    {
        gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "length_label")), "");
    }
    if (g_list_previous (songs) == NULL)
    {
        gtk_widget_set_sensitive (glade_xml_get_widget(xml_id3_window, "button_back"), FALSE);
    }
    else
    {
        gtk_widget_set_sensitive (glade_xml_get_widget(xml_id3_window, "button_back"), TRUE);
    }
    if (g_list_next (songs) == NULL)
    {
        gtk_widget_set_sensitive (glade_xml_get_widget(xml_id3_window, "button_next"), FALSE);
    }
    else
    {
        gtk_widget_set_sensitive (glade_xml_get_widget(xml_id3_window, "button_next"), TRUE);
    }

    if(song){
        meta_data_get_path_callback(song, META_ARTIST_TXT, (MetaDataCallback)id3_txt_fetched, 
                GINT_TO_POINTER(META_ARTIST_TXT));
        meta_data_get_path_callback(song, META_ALBUM_TXT, (MetaDataCallback)id3_txt_fetched, 
                GINT_TO_POINTER(META_ALBUM_TXT));
        meta_data_get_path_callback(song, META_SONG_TXT, (MetaDataCallback)id3_txt_fetched, 
                GINT_TO_POINTER(META_SONG_TXT));
    }
}



void id3_next_song ()
{
    songs = g_list_next (songs);
    set_text (songs);
}

void id3_last_song ()
{
    songs = g_list_previous (songs);
    set_text (songs);
}



void call_id3_window_song(mpd_Song *songstr)
{
    if(songstr == NULL)
    {
        return;
    }
    if(xml_id3_window == NULL)
    {
        char *path = gmpc_get_full_glade_path("gmpc.glade");
        xml_id3_window = glade_xml_new (path, "id3_info_window", NULL);
        q_free(path);
        gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")),META_ALBUM_ART);
        g_signal_connect(G_OBJECT(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")), "image_changed",
                G_CALLBACK(art_set_from_path), 
                glade_xml_get_widget(xml_id3_window, "album_vbox"));
        gmpc_metaimage_set_size(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")),300);

        gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")),META_ARTIST_ART);
        gmpc_metaimage_set_size(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")),300);                 	
        g_signal_connect(G_OBJECT(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")), "image_changed",
                G_CALLBACK(art_set_from_path), 
                glade_xml_get_widget(xml_id3_window, "artist_vbox"));                                                    	




        /* check for errors and axit when there is no gui file */
        if (xml_id3_window == NULL)
            g_error ("Couldnt initialize GUI. Please check installation\n");
        glade_xml_signal_autoconnect (xml_id3_window);
    }

    songs = g_list_append (songs,songstr);
    songs = g_list_last (songs);
    set_text (songs);


}

void call_id3_window (int song)
{
    if (xml_id3_window == NULL)
    {
        create_window (song);
        return;
    }
    else
    {
        mpd_Song  *songstr = mpd_playlist_get_song(connection, song);
        if(songstr != NULL)
        {
            songs = g_list_append (songs,songstr);
            songs = g_list_last (songs);
            set_text (songs);
        }
        gtk_window_present(GTK_WINDOW(glade_xml_get_widget(xml_id3_window, "id3_info_window")));
    }
}

void id3_info_clear_album_image()
{
    meta_data_set_cache(songs->data, META_ALBUM_ART, META_DATA_UNAVAILABLE, NULL);
    gmpc_metaimage_set_cover_na(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")));	
    GmpcStatusChangedCallback(connection, MPD_CST_SONGID, 	NULL);
}

void id3_album_image_file_selector(GtkFileChooser *chooser)
{
    gchar *path = gtk_file_chooser_get_filename(chooser);
    if(path)
    {
        meta_data_set_cache(songs->data, META_ALBUM_ART, META_DATA_AVAILABLE, path);
        gmpc_metaimage_set_cover_from_path(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")),path);	
        GmpcStatusChangedCallback(connection, MPD_CST_SONGID, 	NULL);
        q_free(path);
    }
}
/**
 * Reget lyric
 */
void id3_reget_lyric_txt()
{
    if(songs)
    {
        if(songs->data)
        {
            meta_data_get_path_callback(songs->data, META_SONG_TXT|META_QUERY_NO_CACHE, (MetaDataCallback)id3_txt_fetched, 
                    GINT_TO_POINTER(META_SONG_TXT|META_QUERY_NO_CACHE));
        }
    }
}
void id3_reget_artist_txt()
{
    if(songs)
    {
        if(songs->data)
        {
            meta_data_get_path_callback(songs->data, META_ARTIST_TXT|META_QUERY_NO_CACHE, (MetaDataCallback)id3_txt_fetched, 
                    GINT_TO_POINTER(META_ARTIST_TXT|META_QUERY_NO_CACHE));
        }
    }
}
void id3_reget_album_txt()
{
    if(songs)
    {
        if(songs->data)
        {
            meta_data_get_path_callback(songs->data, META_ALBUM_TXT|META_QUERY_NO_CACHE, (MetaDataCallback)id3_txt_fetched, 
                    GINT_TO_POINTER(META_ALBUM_TXT|META_QUERY_NO_CACHE));
        }
    }
}

void id3_reget_album_art()
{
    if(songs)
    {
        if(songs->data)
        {

            gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")),
                    META_ALBUM_ART|META_QUERY_NO_CACHE);
            gmpc_metaimage_update_cover_from_song(
                    GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")),
                    songs->data);	
            gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")),META_ALBUM_ART);
            gtk_widget_set_sensitive(glade_xml_get_widget(xml_id3_window, "album_vbox"), FALSE);

        }
    }
}

void id3_info_clear_artist_image()
{
    meta_data_set_cache(songs->data, META_ARTIST_ART, META_DATA_UNAVAILABLE, NULL);
    gmpc_metaimage_set_cover_na(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")));	
    GmpcStatusChangedCallback(connection, MPD_CST_SONGID, 	NULL);
}

void id3_artist_image_file_selector(GtkFileChooser *chooser)
{
    gchar *path = gtk_file_chooser_get_filename(chooser);
    if(path)
    {

        gmpc_metaimage_set_cover_from_path(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")),path);	
        meta_data_set_cache(songs->data, META_ARTIST_ART, META_DATA_AVAILABLE, path);
        GmpcStatusChangedCallback(connection, MPD_CST_SONGID, 	NULL);         		
    }
}
void id3_reget_artist_art()
{
    if(songs)
    {
        if(songs->data)
        {
            gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")),
                    META_ARTIST_ART|META_QUERY_NO_CACHE);
            gmpc_metaimage_update_cover_from_song(
                    GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")),
                    songs->data);	
            gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")),
                    META_ARTIST_ART);
            gtk_widget_set_sensitive(glade_xml_get_widget(xml_id3_window, "artist_vbox"), FALSE);
        }
    }
}
