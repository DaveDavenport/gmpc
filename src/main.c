/*
 *Copyright (C) 2004 Qball Cow <Qball@qballcow.nl>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs.h>
#include "libmpdclient.h"
#include "config1.h"
#include "playlist2.h"
#include "playlist3.h"
#include "main.h"
#include "strfsong.h"
#include "mm-keys.h"


/*
 * the xml fle pointer to the player window 
 */
GladeXML *xml_main_window = NULL;

/*
 * set this true to get a little bit more of debug information 
 */
int debug = FALSE;

int update_interface ();
/*
 * the ID of the update timeout
 */
guint update_timeout = 0;
void init_stock_icons ();
extern GtkListStore *pl2_store;

config_obj *config = NULL;



	void
main_trigger_update ()
{
	if (info.connection != NULL)
	{
		update_mpd_status ();
		update_interface ();
	}
}


/*
 * sets default values in the main struct's 
 */
	void
set_default_values ()
{
	/*
	 * the mpd status struct used  in the whole program 
	 */
	info.status = NULL;
	/*
	 * the mpd connection 
	 */
	info.connection = NULL;
	/*
	 * status about the connection 
	 */
	info.stats = NULL;
	/*
	 * connection lock, to prevent to functions to use the connection concurrent 
	 */
	info.conlock = TRUE;
	/*
	 * playlist number this is to check if the playlist changed 
	 */
	info.playlist_id = -1;
	info.playlist_length = 0;
	info.playlist_playtime = 0;
	/*
	 * the state, if the state changes I know I have to update some stuff 
	 */
	info.state = -1;
	/*
	 * the volume if the volume change I also have to update some stuff 
	 */
	info.volume = -1;
	/*
	 * the current song 
	 */
	info.song = -1;
	info.old_pos = -1;
	/*
	 * tray icon 
	 */
	info.hidden = FALSE;
	info.mpdSong = NULL;

	/* */
	info.sb_hidden = FALSE;

	/*
	 * updating 
	 */
	info.updating = FALSE;
}



int main (int argc, char **argv)
{
	MmKeys *keys = NULL;
	gchar *url = NULL;
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif
	set_default_values ();
	/*
	 * load config 
	 */
	url = g_strdup_printf("%s/.gmpc.xml", g_getenv("HOME"));
	config = cfg_open(url);
	g_free(url);
	/* test case: */
	if(config == NULL)
	{
		printf("Failed to save/load configuration\n");
		return 1;
	}
	/*
	 * initialize gtk 
	 */
	gtk_init (&argc, &argv);
	gnome_vfs_init ();

	/*
	 * stock icons 
	 */
	init_stock_icons ();


	/*
	 * create the main window, This is done before anything else (but after command line check) 
	 */
	create_player ();
	init_playlist2 ();

	/*
	 * create timeouts 
	 */
	/*
	 * get the status every 1/2 second should be enough 
	 */
	gtk_timeout_add (500, (GSourceFunc) update_mpd_status, NULL);
	update_timeout =
		gtk_timeout_add (5000, (GSourceFunc) update_interface, NULL);

	if (cfg_get_single_value_as_int_with_default(config, "tray-icon", "enable",1))
		create_tray_icon ();
	update_interface ();

	/*
	 * Keys
	 */
	keys = mmkeys_new();
	g_signal_connect(G_OBJECT(keys), "mm_playpause", G_CALLBACK(play_song), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_next", G_CALLBACK(next_song), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_prev", G_CALLBACK(prev_song), NULL);
	g_signal_connect(G_OBJECT(keys), "mm_stop", G_CALLBACK(stop_song), NULL);


	/*
	 * run the main loop 
	 */
	gtk_main ();

	return 0;
}


int update_interface ()
{

	/*
	 * update the preferences menu, I call this as soon as possible so the preferences menu can detect update 
	 */
	preferences_update ();

	/*
	 * check if there is an connection. (that is when connection == NULL) 
	 */
	if (info.connection == NULL)
	{
		/*
		 * update the popup 
		 */
		//update_popup ();
		if (!cfg_get_single_value_as_int_with_default(config, "connection", "autoconnect", 0))
		{
			return TRUE;
		}
		/*
		 * connect to mpd if that fails return this function 
		 */
		if (connect_to_mpd ())
		{
			/*
			 * make sure this is all set correct. just a little security 
			 */

			info.connection = NULL;
			info.conlock = TRUE;
			return TRUE;
		}
		else
		{
			info.conlock = FALSE;
			gtk_timeout_remove (update_timeout);
			update_timeout =
				gtk_timeout_add (400,
						(GSourceFunc)
						update_interface, NULL);
		}
	}
	/*
	 * now start updating the rest 
	 */
	/*
	 * check if busy 
	 */
	if (info.conlock)
		return TRUE;

	/* check if the database is being updated */
	if(info.status->updatingDb != info.updating)
	{
		if(info.status->updatingDb)
		{
			msg_push_popup(_("MPD is updating the database, please wait"));
		}
		else
		{
			msg_pop_popup();
		}
		info.updating = info.status->updatingDb;
	}

	/*
	 * ok save to update interface, no need to lock (yet) 
	 */
	if (info.song != info.status->song)
	{
		mpd_InfoEntity *ent = NULL;
		if (info.mpdSong != NULL)
		{
			mpd_freeSong (info.mpdSong);
			info.mpdSong = NULL;
		}
		mpd_sendCurrentSongCommand (info.connection);
		ent = mpd_getNextInfoEntity (info.connection);
		if (ent != NULL)
		{
			info.mpdSong = mpd_songDup (ent->info.song);
			mpd_finishCommand (info.connection);
			mpd_freeInfoEntity (ent);
		}
	}

	/*
	 * tray update 
	 */
	update_tray_icon ();

	/*
	 * check for new playlist and load it if needed 
	 */
	if (info.playlist_id != info.status->playlist)
	{
		mpd_InfoEntity *ent = NULL;
		/*
		 * so I don't have to check all the time 
		 */
		gint old_length = 0;
		GtkTreeIter iter;
		gchar buffer[1024];

		old_length = info.playlist_length;


		mpd_sendPlChangesCommand (info.connection, info.playlist_id);

		ent = mpd_getNextInfoEntity (info.connection);
		while (ent != NULL)
		{
			/*
			 * decide wether to update or to add 
			 */
			if (ent->info.song->pos < old_length)
			{
				/*
				 * needed for getting the row 
				 */
				gchar *path =
					g_strdup_printf ("%i",
							ent->info.song->pos);
				if (gtk_tree_model_get_iter_from_string
						(GTK_TREE_MODEL (pl2_store), &iter, path))
				{
					/* overwriting existing entry */
					gint weight = PANGO_WEIGHT_NORMAL;
					gint time=0;
					if (ent->info.song->id ==
							info.status->songid)
					{
						weight = PANGO_WEIGHT_ULTRABOLD;
					}
					/* get old time */
					gtk_tree_model_get(GTK_TREE_MODEL(pl2_store), &iter, SONG_TIME, &time, -1);
					if(time != MPD_SONG_NO_TIME)
					{
						info.playlist_playtime -= time;
					}
					if(ent->info.song->time != MPD_SONG_NO_TIME)
					{
						info.playlist_playtime += ent->info.song->time;
					}
					strfsong (buffer, 1024,
							cfg_get_single_value_as_string_with_default(config, "playlist","markup", DEFAULT_PLAYLIST_MARKUP),
							ent->info.song);
					gtk_list_store_set (pl2_store, &iter,
							SONG_ID,
							ent->info.song->
							id, SONG_POS,
							ent->info.song->
							pos, SONG_TITLE,
							buffer,
							WEIGHT_ENABLE,
							TRUE, WEIGHT_INT,
							weight,
							SONG_STOCK_ID,
							(strstr(ent->info.song->
								file,"://") ==
							 NULL) ?
							"media-audiofile"
							: "media-stream",
							SONG_TIME,
							ent->info.song->time,
							-1);
				}
			}
			else
			{
				if(ent->info.song->time != MPD_SONG_NO_TIME)
				{
					info.playlist_playtime += ent->info.song->time;
				}


				gtk_list_store_append (pl2_store, &iter);
				strfsong (buffer, 1024,
						//						preferences.markup_playlist,
						cfg_get_single_value_as_string_with_default(config, "playlist","markup", DEFAULT_PLAYLIST_MARKUP),
						ent->info.song);
				gtk_list_store_set (pl2_store, &iter, SONG_ID,
						ent->info.song->id,
						SONG_POS,
						ent->info.song->pos,
						SONG_TITLE, buffer,
						WEIGHT_ENABLE, TRUE,
						WEIGHT_INT,
						PANGO_WEIGHT_NORMAL,
						SONG_STOCK_ID,
						(strstr(ent->info.song->file, "://") ==
						 NULL) ? "media-audiofile"
						: "media-stream", 
						SONG_TIME, ent->info.song->time,
						-1);
			}
			mpd_freeInfoEntity (ent);
			ent = mpd_getNextInfoEntity (info.connection);
		}
		while (info.status->playlistLength < old_length)
		{
			gchar *path = g_strdup_printf ("%i", old_length - 1);
			if (gtk_tree_model_get_iter_from_string
					(GTK_TREE_MODEL (pl2_store), &iter, path))
			{
				gint time = 0;
				gtk_tree_model_get(GTK_TREE_MODEL(pl2_store), &iter, SONG_TIME, &time, -1);
				if(time != MPD_SONG_NO_TIME)
				{
					info.playlist_playtime -= time;
				}                                                      				
				gtk_list_store_remove (pl2_store, &iter);
			}
			g_free (path);
			old_length--;
		}
		//pl2_highlight_song ();


		info.status->song = -1;
	}
	info.playlist_length = info.status->playlistLength;
	/*
	 * update the playlist 
	 */
	pl3_update ();

	/*
	 * update the player window 
	 */
	if (update_player ())
	{
		/*
		 * error return 
		 */
		return TRUE;
	}

	/*
	 * return (must be true to keep timeout going) 
	 */
	/*
	 * set these to the good value. So there only updated when changed 
	 */
	info.playlist_id = info.status->playlist;
	if (info.status->state != MPD_STATUS_STATE_UNKNOWN)
		info.song = info.status->song;
	if (info.status->state == MPD_STATUS_STATE_STOP)
		info.song = -1;
	return TRUE;
}



void init_stock_icons ()
{
	GtkIconFactory *factory;
	GdkPixbuf *pb;
	GtkIconSet *set;
	factory = gtk_icon_factory_new ();

	/*
	 * add media-audiofile 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-audiofile.png",
			NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-audiofile", set);
	g_object_unref (G_OBJECT (pb));

	/*
	 * add media-stream 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-stream.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-stream", set);
	g_object_unref (G_OBJECT (pb));

	/*
	 * add media-artist 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-artist.svg", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-artist", set);
	g_object_unref (G_OBJECT (pb));

	/*
	 * add media-album 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-album.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-album", set);
	g_object_unref (G_OBJECT (pb));

	/*
	 * add media-play 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-play.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-play", set);
	g_object_unref (G_OBJECT (pb));

	/*
	 * add media-stop 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-stop.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-stop", set);
	g_object_unref (G_OBJECT (pb));

	/*
	 * add media-next 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-next.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-next", set);
	g_object_unref (G_OBJECT (pb));

	/*
	 * add media-prev 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-prev.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-prev", set);
	g_object_unref (G_OBJECT (pb));

	/*
	 * add media-pause 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-pause.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-pause", set);
	g_object_unref (G_OBJECT (pb));

	/*
	 * add player-shuffle 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "player-shuffle.png",
			NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-random", set);
	g_object_unref (G_OBJECT (pb));

	/*
	 * add playerrepeat 
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "player-repeat.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);
	gtk_icon_factory_add (factory, "media-repeat", set);
	g_object_unref (G_OBJECT (pb));


	/*
	 * add icecast
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "icecast.png", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb); 
	gtk_icon_factory_add (factory, "icecast", set);
	g_object_unref (G_OBJECT (pb));


	/*
	 * add media playlist
	 */
	pb = gdk_pixbuf_new_from_file (PIXMAP_PATH "media-playlist.svg", NULL);
	set = gtk_icon_set_new_from_pixbuf (pb);                        	
	gtk_icon_factory_add (factory, "media-playlist", set);
	g_object_unref (G_OBJECT (pb));                
	gtk_icon_factory_add_default (factory);
}
