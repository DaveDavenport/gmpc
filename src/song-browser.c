#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <glade/glade.h>
#include "libmpdclient.h"
#include "strfsong.h"
#include "main.h"
#include "misc.h"
#include "song-browser.h"

GladeXML *sb_xml = NULL;
GtkTreeStore *sb_store = NULL;
void sb_fill_browser(); 

int last_db = 0;


void sb_close()
{
	gtk_widget_destroy(glade_xml_get_widget(sb_xml, "song_browser"));
	gtk_tree_store_clear(sb_store);
	g_object_unref(sb_store);
	g_object_unref(sb_xml);
	sb_store = NULL;
	sb_xml = NULL;
}


void sb_row_activated()
{
	GtkTreeView *tree = GTK_TREE_VIEW(glade_xml_get_widget(sb_xml, "treeview"));
	GtkWidget *cb = glade_xml_get_widget(sb_xml, "cb_type");
	GtkTreeSelection *selection = gtk_tree_view_get_selection(tree);	
	GtkTreeModel *model = GTK_TREE_MODEL(sb_store);
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);
	GList *add_list = NULL;
	int i = gtk_combo_box_get_active(GTK_COMBO_BOX(cb));
	/* if now rows selected (how we activated it then? but just to be safe) return */
	if(rows == NULL) return;

	if(i == 0)
	{
		GList *node = g_list_first(rows);		
		char *name = NULL;
		GtkTreeIter iter;
		/* step to all the selected songs/directories */
		do{
			GtkTreePath *path = node->data;
			gtk_tree_model_get_iter(GTK_TREE_MODEL(sb_store), &iter, path);
			gtk_tree_model_get(GTK_TREE_MODEL(sb_store), &iter, SB_FPATH, &name, -1);
			/* add them to the add list */	
			add_list = g_list_append(add_list, g_strdup(name));

		}while((node = g_list_next(node)) != NULL);

	}
	else if(i == 1)
	{
		gint type  = 0;
		gchar *name, *album;
		GtkTreeIter iter;
		GList *node = g_list_first(rows);
		do
		{
			mpd_InfoEntity *ent = NULL;
			GtkTreePath *path = node->data;
			gtk_tree_model_get_iter(GTK_TREE_MODEL(sb_store), &iter, path);			
			gtk_tree_model_get(GTK_TREE_MODEL(sb_store), &iter, SB_TYPE, &type,SB_FPATH,&name,SB_DPATH, &album, -1);
			if(type == 0)/* adding a song */
			{
				add_list = g_list_append(add_list, g_strdup(name));
			}
			else if(type == 1 || type ==3) /* artist */
			{
				/* fetch all songs by this artist from mpd and add them to the add-list */
//				mpd_sendSearchCommand(info.connection, MPD_TABLE_ARTIST, name);
				mpd_sendFindCommand(info.connection, MPD_TABLE_ARTIST, name);
				while((ent = mpd_getNextInfoEntity(info.connection)) != NULL)
				{
					add_list = g_list_append(add_list, g_strdup(ent->info.song->file));
					mpd_freeInfoEntity(ent);
				}	
				mpd_finishCommand(info.connection);	
			}
			else if (type ==2) /* album */
			{
				/* fetch all songs by this album and check if the artist is right. from mpd and add them to the add-list */
//				mpd_sendSearchCommand(info.connection, MPD_TABLE_ALBUM, album);
				mpd_sendFindCommand(info.connection, MPD_TABLE_ALBUM, album);
				while((ent = mpd_getNextInfoEntity(info.connection)) != NULL)
				{
					if(!g_utf8_collate(ent->info.song->artist, name))
					{
						add_list = g_list_append(add_list, g_strdup(ent->info.song->file));	
					}
					mpd_freeInfoEntity(ent);
				}	
				mpd_finishCommand(info.connection);	
			}
		}
		while((node = g_list_next(node)) != NULL);
	}

	/* if there are items in the add list add them to the playlist */
	if(add_list != NULL)
	{
		mpd_sendCommandListBegin(info.connection);
		GList *song = g_list_first(add_list);
		do{
			mpd_sendAddCommand(info.connection, song->data);
		}while((song = g_list_next(song)) != NULL);
		mpd_sendCommandListEnd(info.connection);
		mpd_finishCommand(info.connection);     	
		g_list_foreach (add_list,(GFunc) g_free, NULL);
		g_list_free(add_list);
	}

	g_list_foreach (rows,(GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows);
}

void sb_replace_songs()
{
	mpd_sendClearCommand(info.connection);
	mpd_finishCommand(info.connection);
	sb_row_activated();
}



void song_browser_create()
{
	GtkCellRenderer *renderer;
	GtkWidget *tree;
	GtkTreeViewColumn *column;          	
	if(sb_xml != NULL)
	{
		gtk_widget_show_all(
				glade_xml_get_widget(sb_xml, "song_browser"));
		gtk_window_present(GTK_WINDOW(
					glade_xml_get_widget(sb_xml, "song_browser")));
		return;          	
	}
	sb_xml = glade_xml_new(GLADE_PATH"add-browser.glade", "song_browser",NULL);

	if(sb_store == NULL)
	{
		sb_store = gtk_tree_store_new(SB_NROWS,
				GTK_TYPE_STRING, /* full path*/
				GTK_TYPE_STRING, /* display string */
				GTK_TYPE_INT,	/* 1 for folder 0 for song */
				GTK_TYPE_STRING  /* stock -id*/
				);
	}

	tree = glade_xml_get_widget(sb_xml, "treeview");
	/* set selection mode */
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(
				gtk_tree_view_get_selection(GTK_TREE_VIEW(tree))), GTK_SELECTION_MULTIPLE);

	/* set filter */
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree),
			GTK_TREE_MODEL(sb_store));

	/* draw the column with the songs */


	/* insert the column in the tree */
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column,renderer, FALSE);
	gtk_tree_view_column_set_attributes(
			column,renderer,
			"stock-id", SB_PIXBUF,
			NULL);
	/* draw the column with the songs */
	renderer = gtk_cell_renderer_text_new();

	/* insert the column in the tree */
	gtk_tree_view_column_pack_end(GTK_TREE_VIEW_COLUMN(column), renderer, TRUE);
	gtk_tree_view_column_set_attributes(
			GTK_TREE_VIEW_COLUMN(column),
			renderer,
			"text", SB_DPATH,

			NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), GTK_TREE_VIEW_COLUMN(column));

	gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(sb_xml, "cb_type")),0);


	sb_fill_browser();
	glade_xml_signal_autoconnect(sb_xml);

}

void sb_row_expanded(GtkTreeView *tree, GtkTreeIter *parent, GtkTreePath *path)
{
	GtkWidget *cb = glade_xml_get_widget(sb_xml, "cb_type");
	gint type;
	mpd_InfoEntity *ent = NULL;
	gchar *artist;
	GtkTreeIter iter,child2;
	/* only for id3 */
	if(gtk_combo_box_get_active(GTK_COMBO_BOX(cb)) == 0) return;


	gtk_tree_model_get(GTK_TREE_MODEL(sb_store),parent, SB_TYPE, &type, SB_FPATH, &artist, -1);
	if(type == 1)
	{
		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(sb_store), &iter, parent))
		{
			gchar *album;
			do{                                                                                                   
				gtk_tree_model_get(GTK_TREE_MODEL(sb_store), &iter, SB_DPATH, &album,-1);
//				mpd_sendSearchCommand(info.connection, MPD_TABLE_ALBUM, album);
				mpd_sendFindCommand(info.connection, MPD_TABLE_ALBUM, album);
				while((ent = mpd_getNextInfoEntity(info.connection)) !=NULL)
				{
					if(ent->info.song->artist != NULL)
					{
						if(!g_utf8_collate(ent->info.song->artist, artist))
						{
							gchar buffer[1024];
							strfsong(buffer, 1024, preferences.markup_main_display, ent->info.song);
							gtk_tree_store_append(sb_store, &child2, &iter);
							gtk_tree_store_set(sb_store, &child2,                			
									SB_FPATH,ent->info.song->file,	
									SB_DPATH, buffer,
									SB_TYPE, 0,
									SB_PIXBUF, "media-audiofile",
									-1);
						}

					}
					mpd_freeInfoEntity(ent);
				} 	
			}while(gtk_tree_model_iter_next(GTK_TREE_MODEL(sb_store), &iter));                                      	
		}
		else
		{
//		mpd_sendSearchCommand(info.connection, MPD_TABLE_ARTIST,artist);
		mpd_sendFindCommand(info.connection, MPD_TABLE_ARTIST,artist);	
		while((ent = mpd_getNextInfoEntity(info.connection)) !=NULL)
		{
			if(ent->info.song->album == NULL || strlen(ent->info.song->album) == 0)
			{
				gchar buffer[1024];
				strfsong(buffer, 1024, preferences.markup_main_display, ent->info.song);
				gtk_tree_store_append(sb_store, &iter, parent);
				gtk_tree_store_set(sb_store, &iter,                			
						SB_FPATH,ent->info.song->file,	
						SB_DPATH, buffer,
						SB_TYPE, 0,
						SB_PIXBUF, "media-audiofile",
						-1);

			}
			mpd_freeInfoEntity(ent);
		}
		}
		/* we only need to fill it once */
		/* so I now make it type 3 */
		gtk_tree_store_set(sb_store,parent, SB_TYPE, 3, -1);
	}
}

void sb_fill_browser_id3()
{
	mpd_InfoEntity *ent = NULL;
	GtkTreeIter iter, parent;
	gchar *string;
	if(info.connection == NULL) return;

	mpd_sendListCommand(info.connection, MPD_TABLE_ARTIST,NULL);

	while((string = mpd_getNextArtist(info.connection)) != NULL)
	{
		gtk_tree_store_append(sb_store, &iter, NULL);    	
		gtk_tree_store_set(sb_store, &iter, 
				SB_FPATH,string,
				SB_DPATH,string,
				SB_TYPE, 1,
				SB_PIXBUF, "media-artist",
				-1);
		g_free(string);
	}
	/* get the first iter */
	if(gtk_tree_model_iter_children(GTK_TREE_MODEL(sb_store), &parent, NULL))
	{
		do
		{
			gchar *artist;
			int nalbum = 0;
			gtk_tree_model_get(GTK_TREE_MODEL(sb_store), &parent, SB_FPATH, &artist,-1);
			mpd_sendListCommand(info.connection, MPD_TABLE_ALBUM, artist);
			while((string = mpd_getNextAlbum(info.connection)) != NULL)
			{
				gtk_tree_store_append(sb_store, &iter, &parent);    	
				gtk_tree_store_set(sb_store, &iter, 
						SB_FPATH,artist,
						SB_DPATH,string,                        			
						SB_TYPE, 2,                             			
						SB_PIXBUF, "media-album",                          		
						-1);                                    			
				g_free(string);
				nalbum++;
			}
			while(gtk_events_pending()) gtk_main_iteration();
			/* check if the users hasn't closed the interface */
			if(sb_xml == NULL) return;
			
			if(nalbum == 0)
			{
				mpd_sendSearchCommand(info.connection, MPD_TABLE_ARTIST,artist);
				mpd_sendFindCommand(info.connection, MPD_TABLE_ARTIST,artist);
				while((ent = mpd_getNextInfoEntity(info.connection)) !=NULL)
				{
					if(ent->info.song->album == NULL || strlen(ent->info.song->album) == 0)
					{
						gchar buffer[1024];
						strfsong(buffer, 1024, preferences.markup_main_display, ent->info.song);
						gtk_tree_store_append(sb_store, &iter, &parent);
						gtk_tree_store_set(sb_store, &iter,                			
								SB_FPATH,ent->info.song->file,	
								SB_DPATH, buffer,
								SB_TYPE, 0,
								SB_PIXBUF, "media-audiofile",
								-1);

					}
					mpd_freeInfoEntity(ent);
				}
			}
		}while(gtk_tree_model_iter_next(GTK_TREE_MODEL(sb_store), &parent));

	}
}

void sb_fill_browser_file(char *path, GtkTreeIter *parent)
{
	mpd_InfoEntity *ent = NULL;
	GtkTreeIter iter;
	if(info.connection == NULL) return;
	mpd_sendLsInfoCommand(info.connection, path);

	ent = mpd_getNextInfoEntity(info.connection);
	while(ent != NULL)
	{
		if(ent->type == MPD_INFO_ENTITY_TYPE_DIRECTORY)
		{
			gchar *basename = g_path_get_basename(ent->info.directory->path);
			gtk_tree_store_append(sb_store, &iter, parent);
			gtk_tree_store_set(sb_store, &iter, 
					SB_FPATH,ent->info.directory->path,
					SB_DPATH, basename,
					SB_TYPE, 1,
					SB_PIXBUF, "gtk-open",
					-1);

			g_free(basename);
		}
		else if (ent->type == MPD_INFO_ENTITY_TYPE_SONG)
		{	
			gchar buffer[1024];
			strfsong(buffer, 1024, preferences.markup_main_display, ent->info.song);
			gtk_tree_store_append(sb_store, &iter, parent);
			gtk_tree_store_set(sb_store, &iter,                			
					SB_FPATH,ent->info.song->file,
					SB_DPATH, buffer,
					SB_TYPE, 0,
					SB_PIXBUF, "media-audiofile",
					-1);

		}
		mpd_freeInfoEntity(ent);


		ent = mpd_getNextInfoEntity(info.connection);
	}

	while(gtk_events_pending()) gtk_main_iteration();
	/* check if the users hasn't closed the interface */
	if(sb_xml == NULL) return;

	if(gtk_tree_model_iter_children(GTK_TREE_MODEL(sb_store), &iter,parent))
	{
		do{
			gint type;
			gchar *name;
			gtk_tree_model_get(GTK_TREE_MODEL(sb_store), &iter, SB_TYPE, &type,SB_FPATH, &name, -1);
			if(type == 1)
			{
				sb_fill_browser_file(name, &iter);
			}
		}while(gtk_tree_model_iter_next(GTK_TREE_MODEL(sb_store), &iter));
	}
}


void sb_fill_browser()
{
	GtkWidget *cb = glade_xml_get_widget(sb_xml, "cb_type");
	int i = gtk_combo_box_get_active(GTK_COMBO_BOX(cb));
	gtk_tree_store_clear(sb_store);
	last_db = info.stats->dbUpdateTime;
	if( i == 0)
	{
		/* fill by filename */
		sb_fill_browser_file("/", NULL);

	}
	else if (i == 1)
	{
		/* fill by id3 */	
		sb_fill_browser_id3();
	}
}


void update_song_browser()
{
	if(sb_xml == NULL)
	{
		return;
	}
	if(info.stats->dbUpdateTime != last_db)
	{
		g_print("updating browser\n");
		sb_fill_browser();		
	}
}
