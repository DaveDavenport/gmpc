#include <gtk/gtk.h>
#include <string.h>
#include <glade/glade.h>
#include <time.h>
#include "libmpdclient.h"
#include "main.h"

/* this should be something configurable by the user */
/* or .f.e window size dependent */
gint max_string_length = 32;


GladeXML *xml_playlist_window = NULL;
void update_information_tab();

/* playlist only functions */
void refresh_information_tab();
void do_server_side_search();
void load_songs_with_filter();
void fill_playlist_tree();
void playlist_row_activated(GtkWidget *wid);
void clear_playlist();
void shuffle_songs();
void save_playlist();
void delete_playlist();
void playlist_row_selected(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col);
void delete_selected_songs();
void load_directories(gchar *oldp);
void directory_row_selected(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col);
int file_browser_mouse_event(GtkWidget *tree, GdkEventButton *event);
int file_browser_activate(GtkWidget *tree);
void fill_artist_tree();
void file_browser_add_button();
void expand_artist_tree();
void collapse_artist_tree();
void add_selected_id3_browser();
void add_selected_search();
int hide_playlist_view();

char * shorter_string(const char *long_string)
{
	char *ret_val = NULL;
	int strl = 0;
	if(long_string == NULL)
	{
		return NULL;
	}
	strl = g_utf8_strlen(long_string, -1);
	/* this should be configurable? */
	if(strl > max_string_length)
	{
	ret_val = g_strndup(long_string, max_string_length +1);
	ret_val[max_string_length-3] = ret_val[max_string_length-2] = ret_val[max_string_length-1] = '.';
	ret_val[max_string_length] = '\0';
	}
	else ret_val = g_strdup(long_string);
	return ret_val;

}


void destroy_playlist(GtkWidget *wid)
{
	if(!info.playlist_running) return;
	/* clear the current playlist.. allready got this buffered.. no need to buffer it twice */
	gtk_list_store_clear(info.cur_list);
	gtk_list_store_clear(info.dir_list);
	gtk_list_store_clear(info.file_list);
	gtk_list_store_clear(info.search_list);
	/* destroy the playlist */
	gtk_widget_destroy(gtk_widget_get_toplevel(wid));
	info.playlist_running = FALSE;
}

gint track_sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer data)
{
	gint column = GPOINTER_TO_INT(data);
	gchar *ca=NULL, *cb=NULL;
	gint ia=-1, ib = -1;
	if(a== NULL || b == NULL) return 0;
	gtk_tree_model_get(model, a, column, &ca, -1);
	gtk_tree_model_get(model, b, column, &cb, -1);
	if(ca != NULL) ia = (int)g_ascii_strtod(ca, NULL);
	if(cb != NULL) ib = (int)g_ascii_strtod(cb, NULL);
	/* still don't know if its needed or not :-/ */
	g_free(ca);
	g_free(cb);	
	return ia-ib; 
}


void clear_playlist_buffer()
{
	/* Check all "buffer" and if there full empty and unref them. */
	if(info.playlist_list != NULL) 	g_object_unref(info.playlist_list);
	if(info.cur_list != NULL) 		g_object_unref(info.cur_list);
	if(info.dir_list != NULL)		g_object_unref(info.dir_list);
	if(info.file_list != NULL) 		g_object_unref(info.file_list);
	if(info.id3_songs_list != NULL)	g_object_unref(info.id3_songs_list);
	if(info.id3_album_list != NULL)	g_object_unref(info.id3_album_list);
	if(info.search_list != NULL)	g_object_unref(info.search_list);
	/* set all pointers to NULL so I can test at a later state */
	info.playlist_list = NULL;
	info.cur_list = NULL;
	info.dir_list = NULL;
	info.file_list = NULL;
	info.id3_songs_list = NULL;
	info.id3_album_list = NULL;
	info.search_list = NULL;
}

void create_playlist()
{
	GtkWidget *tree;
	GtkCellRenderer *renderer;
	if(info.playlist_running)
	{
		gtk_window_present(GTK_WINDOW(glade_xml_get_widget(xml_playlist_window, "playlist_window")));
		return;
	}
	info.playlist_running = TRUE;
	xml_playlist_window = glade_xml_new(GLADE_PATH"gmpc.glade", "playlist_window", NULL);
	/* check for errors and axit when there is no gui file */
	if(xml_playlist_window == NULL)  g_error("Couldnt initialize GUI. Please check installation\n");
	glade_xml_signal_autoconnect(xml_playlist_window);	

	/* playlist treeview */
	if(info.playlist_list == NULL)
	{
		if(debug)g_print("**DEBUG** Creating playlist list store \n");
		info.playlist_list = gtk_list_store_new(1, GTK_TYPE_STRING);
		g_object_ref(info.playlist_list);
		if(info.connection != NULL) fill_playlist_tree();
	}
	tree = glade_xml_get_widget(xml_playlist_window, "playlist_treeview");
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree),GTK_TREE_MODEL(info.playlist_list));

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_text_set_fixed_height_from_font ((GtkCellRendererText *)renderer, 1);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Playlist", renderer, "text", 0, NULL);
	gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column((GtkTreeView*)tree, 0), 0);
	/* the following function doesnt do what I want.. to bad */
	gtk_tree_sortable_set_sort_column_id
		((GtkTreeSortable *)info.playlist_list,
		 0,
		 GTK_SORT_ASCENDING);
	/* set the search function */
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), 0);

	/** songs in the current playlist **/
	if(info.cur_list == NULL)
	{
		if(debug)g_print("**DEBUG** Creating current_playlist list store \n");
		/* make sure it doesnt get unreffed by gtk.. ref it again.. */
		info.cur_list = gtk_list_store_new(7, GTK_TYPE_INT,GTK_TYPE_STRING, GTK_TYPE_STRING, GTK_TYPE_STRING, G_TYPE_BOOLEAN, GTK_TYPE_STRING, GTK_TYPE_STRING);
		g_object_ref(info.cur_list);
	}

	tree = glade_xml_get_widget(xml_playlist_window, "current_playlist");
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree),GTK_TREE_MODEL(info.cur_list));
	gtk_tree_selection_set_mode((GtkTreeSelection *)gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), GTK_SELECTION_MULTIPLE);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Track", renderer, 
					"text", 6,"background-set", 4, "background", 5, NULL);
	gtk_tree_view_column_set_resizable(gtk_tree_view_get_column((GtkTreeView*)tree, 0), TRUE);
	
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Title", renderer, 
					"text", 1,"background-set", 4, "background", 5, NULL);
	gtk_tree_view_column_set_resizable(gtk_tree_view_get_column((GtkTreeView*)tree, 1), TRUE);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Artist", renderer, 
					"text", 2,"background-set", 4, "background", 5, NULL);
	gtk_tree_view_column_set_resizable(gtk_tree_view_get_column((GtkTreeView*)tree, 2), TRUE);


	/* Set search on title
	 * I dont think artist is realy needed.. this is just to lookup quickly a song f.e. after a filter
	 */
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), 1);

	/*********************FILE BROWSER *****************************/
	/* directory */
	if(info.dir_list == NULL)
	{
		info.dir_list = gtk_list_store_new(2, GTK_TYPE_STRING, GTK_TYPE_STRING);
		g_object_ref(info.dir_list);
	}
	tree = glade_xml_get_widget(xml_playlist_window, "treeview_dirs");
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree),GTK_TREE_MODEL(info.dir_list));
	gtk_tree_selection_set_mode((GtkTreeSelection *)gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), GTK_SELECTION_MULTIPLE);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_text_set_fixed_height_from_font ((GtkCellRendererText *)renderer, 1);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Directory", renderer, "text", 1, NULL);
	gtk_tree_view_column_set_sizing(gtk_tree_view_get_column((GtkTreeView*)tree, 0), GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_max_width(gtk_tree_view_get_column((GtkTreeView*)tree, 0),200);
	/* set the search function to search in the directories the way the users sees it */
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), 1);
	/* files tree */
	if(info.file_list == NULL) 
	{
		info.file_list = gtk_list_store_new(5, GTK_TYPE_STRING,GTK_TYPE_STRING, GTK_TYPE_STRING, GTK_TYPE_STRING, GTK_TYPE_STRING);	
		g_object_ref(info.file_list);
	}
	tree = glade_xml_get_widget(xml_playlist_window, "treeview_files");

	gtk_tree_view_set_model(GTK_TREE_VIEW(tree),GTK_TREE_MODEL(info.file_list));
	gtk_tree_selection_set_mode((GtkTreeSelection *)gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), GTK_SELECTION_MULTIPLE);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_text_set_fixed_height_from_font ((GtkCellRendererText *)renderer, 1);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Track", renderer, "text", 4, NULL);
	gtk_tree_view_column_set_resizable(gtk_tree_view_get_column((GtkTreeView*)tree, 0), TRUE);
	gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column((GtkTreeView*)tree, 0), 4);
	gtk_tree_sortable_set_sort_func((GtkTreeSortable *)info.file_list, 4, (GtkTreeIterCompareFunc)track_sort_func, GINT_TO_POINTER(4), NULL);
	gtk_tree_sortable_set_sort_column_id((GtkTreeSortable *)info.file_list, 4, GTK_SORT_ASCENDING);



	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_text_set_fixed_height_from_font ((GtkCellRendererText *)renderer, 1);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Title", renderer, "text", 1, NULL);
	gtk_tree_view_column_set_resizable(gtk_tree_view_get_column((GtkTreeView*)tree, 1), TRUE);
	gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column((GtkTreeView*)tree, 1), 1);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_text_set_fixed_height_from_font ((GtkCellRendererText *)renderer, 1);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Artist", renderer, "text", 2, NULL);
	gtk_tree_view_column_set_resizable(gtk_tree_view_get_column((GtkTreeView*)tree, 2), TRUE);
	gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column((GtkTreeView*)tree, 2), 2);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_text_set_fixed_height_from_font ((GtkCellRendererText *)renderer, 1);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Album", renderer, "text", 3, NULL);
	gtk_tree_view_column_set_resizable(gtk_tree_view_get_column((GtkTreeView*)tree, 3), TRUE);
	gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column((GtkTreeView*)tree, 3), 3);
	/* set the search to search on the title column.. I think that is the most usefull one.. I dont want to wind up
	 * making my own search routine..
	 * Users should use advanced search if they realy need to find anything 
	 */    
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), 1);




	/* Show the filter stuff if it was showed in the last session*/
	gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml_playlist_window, "search_entry")), info.filter_entry);
	gtk_option_menu_set_history(GTK_OPTION_MENU(glade_xml_get_widget(xml_playlist_window, "search_optionmenu")), info.filter_field);
	if(info.show_filter)
	{
		gtk_arrow_set((GtkArrow *)glade_xml_get_widget(xml_playlist_window, "filter_arrow"),  GTK_ARROW_DOWN, GTK_SHADOW_IN);
		gtk_widget_show_all(glade_xml_get_widget(xml_playlist_window, "filterbox"));

	}

	/**************************** The ID3 Browser *********************************/
	if(info.id3_songs_list == NULL)
	{
		if(debug)g_print("**DEBUG** Creating id3 list store \n");
		/* make sure it doesnt get unreffed by gtk.. ref it again.. */
		info.id3_songs_list = gtk_list_store_new(4,GTK_TYPE_STRING, GTK_TYPE_STRING, GTK_TYPE_STRING, GTK_TYPE_STRING);
		g_object_ref(info.id3_songs_list);
	}

	tree = glade_xml_get_widget(xml_playlist_window, "id3_songs_tree");
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree),GTK_TREE_MODEL(info.id3_songs_list));
	gtk_tree_selection_set_mode((GtkTreeSelection *)gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), GTK_SELECTION_MULTIPLE);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_text_set_fixed_height_from_font ((GtkCellRendererText *)renderer, 1);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Track", renderer, "text", 3, NULL);
	gtk_tree_view_column_set_resizable(gtk_tree_view_get_column((GtkTreeView*)tree, 0), TRUE);
	gtk_tree_view_column_set_max_width(gtk_tree_view_get_column((GtkTreeView*)tree, 0), 250);    
	gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column((GtkTreeView*)tree, 0), 3);
	/* the following function doesnt do what I want.. to bad */
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_text_set_fixed_height_from_font ((GtkCellRendererText *)renderer, 1);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Title", renderer, "text", 1, NULL);
	gtk_tree_view_column_set_resizable(gtk_tree_view_get_column((GtkTreeView*)tree, 1), TRUE);
	gtk_tree_view_column_set_max_width(gtk_tree_view_get_column((GtkTreeView*)tree, 1), 250);    
	gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column((GtkTreeView*)tree, 1), 1);

	gtk_tree_sortable_set_sort_func((GtkTreeSortable *)info.id3_songs_list, 3, (GtkTreeIterCompareFunc)track_sort_func, GINT_TO_POINTER(3), NULL);
	/* set the search on the title .. we allready know the album and the artist.. so title is the logical choise */
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), 1);

	gtk_tree_sortable_set_sort_column_id
		((GtkTreeSortable *)info.id3_songs_list,
		 3,
		 GTK_SORT_ASCENDING);



	if(info.id3_album_list == NULL)
	{
		info.id3_album_list = gtk_tree_store_new(2, GTK_TYPE_STRING, GTK_TYPE_STRING);
		g_object_ref(info.id3_songs_list);	    
		if(info.connection != NULL) fill_artist_tree();
	}
	tree = glade_xml_get_widget(xml_playlist_window, "id3_artist_tree");

	gtk_tree_view_set_model(GTK_TREE_VIEW(tree),GTK_TREE_MODEL(info.id3_album_list));
	gtk_tree_selection_set_mode((GtkTreeSelection *)gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), GTK_SELECTION_MULTIPLE);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_text_set_fixed_height_from_font ((GtkCellRendererText *)renderer, 1);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Artist", renderer, "text", 0, NULL);
	gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column((GtkTreeView*)tree, 0), 0);
	/* the following function doesnt do what I want.. to bad */
	gtk_tree_sortable_set_sort_column_id
		((GtkTreeSortable *)info.id3_album_list,
		 0,
		 GTK_SORT_ASCENDING);
	/* Set the search.. shamefully it doesnt expand the view..  to bad */
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), 0);                                             

	/* Search window stuff */
	if(info.search_list== NULL) 
	{
		if(debug)g_print("**DEBUG** create empty search list\n");
		info.search_list= gtk_list_store_new(4, GTK_TYPE_STRING,GTK_TYPE_STRING, GTK_TYPE_STRING, GTK_TYPE_STRING);	
		g_object_ref(info.search_list);
	}
	tree = glade_xml_get_widget(xml_playlist_window, "search_tree");

	gtk_tree_view_set_model(GTK_TREE_VIEW(tree),GTK_TREE_MODEL(info.search_list));
	gtk_tree_selection_set_mode((GtkTreeSelection *)gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), GTK_SELECTION_MULTIPLE);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_text_set_fixed_height_from_font ((GtkCellRendererText *)renderer, 1);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Title", renderer, "text", 1, NULL);
	gtk_tree_view_column_set_sizing(gtk_tree_view_get_column((GtkTreeView*)tree, 0), GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_text_set_fixed_height_from_font ((GtkCellRendererText *)renderer, 1);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Artist", renderer, "text", 2, NULL);
	gtk_tree_view_column_set_sizing(gtk_tree_view_get_column((GtkTreeView*)tree, 1), GTK_TREE_VIEW_COLUMN_AUTOSIZE);	
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_text_set_fixed_height_from_font ((GtkCellRendererText *)renderer, 1);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "Album", renderer, "text", 3, NULL);
	gtk_tree_view_column_set_sizing(gtk_tree_view_get_column((GtkTreeView*)tree, 2), GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	/* set the search to search on the title column.. I think that is the most usefull one.. I dont want to wind up
	 * making my own search routine..
	 * Users should use advanced search if they realy need to find anything 
	 */    
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), 1);


	/* load the data into the playlist */
	if(info.connection != NULL) 	load_songs_with_filter();
	if(info.connection != NULL) 
	{
		if(strlen(info.path) == 0)load_directories(NULL);
		else
		{
			gchar *buf = g_path_get_dirname(info.path);
			gchar *buf2 = g_strdup_printf("Current Directory: %s", info.path);
			gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_playlist_window, "path_label")), buf2);
			load_directories(buf);
			g_free(buf);
			g_free(buf2);
		}
	}
	info.playlist_view_hidden = !info.playlist_view_hidden;
	hide_playlist_view();
	
	/* ok.. now show the main window */
	gtk_widget_show(glade_xml_get_widget(xml_playlist_window, "playlist_window"));
	/* update the information tab */
	update_information_tab();
}


void fill_playlist_tree()
{
	mpd_InfoEntity *entity;

	if(info.conlock) return;
	info.conlock = TRUE;
	gtk_list_store_clear(info.playlist_list);
	mpd_sendLsInfoCommand(info.connection,"");
	if(check_for_errors())
	{
		return;
	}
	while((entity = mpd_getNextInfoEntity(info.connection))) {
		if(check_for_errors())
		{
			return;
		}
		if(entity->type== MPD_INFO_ENTITY_TYPE_PLAYLISTFILE) {
			mpd_PlaylistFile * pl = entity->info.playlistFile;
			GtkTreeIter iter;
			gtk_list_store_append(info.playlist_list, &iter);
			gtk_list_store_set(info.playlist_list, &iter, 0, pl->path, -1);
		}
		mpd_freeInfoEntity(entity);
	}
	info.conlock = FALSE;
}



void load_songs_with_filter()
{
	gchar *entryb,*buf = NULL;
	GPatternSpec *spec;
	GtkTreeIter iter;
	mpd_Song *song;
	int option = gtk_option_menu_get_history(GTK_OPTION_MENU(glade_xml_get_widget(xml_playlist_window, "search_optionmenu")));
	GtkWidget *entry = glade_xml_get_widget(xml_playlist_window, "search_entry");
	int i=0, rate = 50;
	GList *node = NULL;
	gtk_list_store_clear(info.cur_list);	
	/* if there is a playlistlength set the update rate to every 2.5 % */
	if(info.status->playlistLength != -1) rate = (int)(info.status->playlistLength/40 + 1);

	strncpy(info.filter_entry, gtk_entry_get_text(GTK_ENTRY(entry)), 256);
	info.filter_field = option;	
	/* create the compare spec.  all uppercast */
	entryb = g_strdup_printf("*%s*",  gtk_entry_get_text(GTK_ENTRY(entry)));
	buf = g_utf8_strup(entryb, -1);
	spec = g_pattern_spec_new(buf);
	g_free(entryb);
	g_free(buf);

	node = g_list_nth(info.playlist, i);
	while(node)
	{
		song = node->data;
		buf = NULL; /* if an tag can't be converted propperly .  I wanna know */
		if(info.show_filter)
		{
			if(option == 0  && song->title != NULL)buf = g_strdup(song->title);
			else if(option == 1 && song->artist != NULL) buf = g_strdup(song->artist);
			else if (option == 2 && song->album != NULL) buf = g_strdup(song->album);
			else if (option == 3 && song->file != NULL) buf = g_strdup(song->file);
			else if(option == 4)
			{
				GString *string = g_string_new("");
				if(song->title != NULL) g_string_append(string, song->title);
				if(song->artist != NULL) g_string_append(string, song->artist);
				if(song->album != NULL) g_string_append(string, song->album);						
				if(song->file != NULL) g_string_append(string, song->file);
				buf = g_strdup(string->str);
				g_string_free(string, TRUE);
			}
			if(buf == NULL) buf = g_strdup("!#@$"); /* oeps */
			else
			{
				gchar *temp = g_utf8_strup(buf, -1);
				g_free(buf);
				buf = temp;
			}
		}
		else buf = g_strdup("");
		if(!info.show_filter || g_pattern_match_string(spec, buf))
		{
			gtk_list_store_append(info.cur_list, &iter);
			if(song->title != NULL)
			{
				gchar *short_string = shorter_string(song->title);
				if(short_string == NULL) g_print("ARG\n");
				gtk_list_store_set(info.cur_list, &iter,0,i,5,"green", 1, short_string,-1);
				g_free(short_string);
				if(song->artist != NULL)
				{
				short_string = shorter_string(song->artist);
				gtk_list_store_set(info.cur_list, &iter, 2, short_string, -1);
				g_free(short_string);
				}
				if(song->track != NULL)
				{
				gtk_list_store_set(info.cur_list, &iter, 6, song->track, -1);
				}
				gtk_list_store_set(info.cur_list, &iter,4,(info.status->song == i && 
						 info.status->state != MPD_STATUS_STATE_STOP && 
						 info.status->state != MPD_STATUS_STATE_UNKNOWN)? TRUE:FALSE,  -1);
				
			}
			else {
				gchar *buf1 = g_path_get_basename(song->file);
				gchar *short_string = shorter_string(buf1);
				g_free(buf1);
				gtk_list_store_set(info.cur_list, &iter, 0,i,5,"green",1,short_string,4,(info.status->song == i && info.status->state != MPD_STATUS_STATE_STOP && info.status->state != MPD_STATUS_STATE_UNKNOWN)? TRUE:FALSE, -1);
				g_free(short_string);
			}			
		}

		g_free(buf);
		i++;
		node = g_list_nth(info.playlist, i);
	}
	g_free(spec);
}


void playlist_row_activated(GtkWidget *wid)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(wid));
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(info.cur_list);
	if(info.conlock) return;
	if(gtk_tree_selection_count_selected_rows(selection) > 0)
	{
		int song;
		GList *list = gtk_tree_selection_get_selected_rows(selection, &model);
		GtkTreePath *path = list->data;
		gtk_tree_model_get_iter(model,&iter, path);
		/* free the list.. I just want to play the first one */
		g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free (list);
		gtk_tree_model_get(model, &iter, 0, &song, -1);
		mpd_sendPlayCommand(info.connection, song);
		mpd_finishCommand(info.connection);
		check_for_errors();
	}
}


/* when the user clicks on the filter toggle I want it to popup */
void filter_toggle()
{
	if(info.show_filter)
	{
		info.show_filter= FALSE;
		gtk_arrow_set((GtkArrow *)glade_xml_get_widget(xml_playlist_window, "filter_arrow"),  GTK_ARROW_LEFT, GTK_SHADOW_IN);
		gtk_widget_hide_all(glade_xml_get_widget(xml_playlist_window, "filterbox"));
		if(strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml_playlist_window, "search_entry")))))load_songs_with_filter();
	}
	else 
	{
		info.show_filter= TRUE;
		gtk_arrow_set((GtkArrow *)glade_xml_get_widget(xml_playlist_window, "filter_arrow"),  GTK_ARROW_DOWN, GTK_SHADOW_IN);
		gtk_widget_show_all(glade_xml_get_widget(xml_playlist_window, "filterbox"));    
		if(strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml_playlist_window, "search_entry")))))load_songs_with_filter();
	}
}

/* clears the current playlist */
void clear_playlist()
{
	if(info.conlock) return;
	info.conlock = TRUE;
	mpd_sendClearCommand(info.connection);
	if(check_for_errors()) 
	{
		return;
	}
	mpd_finishCommand(info.connection);    
	if(check_for_errors()) return;
	info.conlock = FALSE;
}

/** shuffles the current playlist **/
void shuffle_songs()
{
	if(info.conlock) return;
	info.conlock = TRUE;
	mpd_sendShuffleCommand(info.connection);
	if(check_for_errors())
	{
		return;
	}
	mpd_finishCommand(info.connection);    	
	if(check_for_errors()) return;
	info.conlock = FALSE;
}


/* this function makes sure everything is up to date in the playlist..  this is called from the normall loop */
void update_playlist()
{
	if(!info.playlist_running) return;
	/* update on playlist change */
	if(info.playlist_id != info.status->playlist)
	{
		load_songs_with_filter();
		/* update teh information tab */
		update_information_tab();
	}
	/* update hightlighted song */
	if((info.status->song != info.song && info.song != -1) || 
	    (info.state != info.status->state &&  info.status->state != MPD_STATUS_STATE_PAUSE && info.state != MPD_STATUS_STATE_PAUSE))
	{
		GtkTreeIter iter;
		GtkTreeModel *model = GTK_TREE_MODEL(info.cur_list);
		int i = 0;
		if(gtk_tree_model_get_iter_first(model, &iter))
			do
			{
				if(info.status->state != MPD_STATUS_STATE_STOP && info.status->state != MPD_STATUS_STATE_UNKNOWN)
				{
					gtk_tree_model_get(model, &iter, 0, &i, -1);
					if(i == info.status->song) gtk_list_store_set(info.cur_list, &iter, 4, TRUE,-1); 
					else  gtk_list_store_set(info.cur_list, &iter, 4, FALSE,-1);
				}
				else  gtk_list_store_set(info.cur_list, &iter, 4, FALSE,-1);
			}
			while (gtk_tree_model_iter_next(model, &iter));
	}
}


/* save's the playlist (name is from entry box */
void save_playlist()
{
	GtkTreeIter iter;
	GladeXML *esf;
	GtkWidget *dialog;
	GtkWidget *entry;
	GtkTreeModel *model = GTK_TREE_MODEL(info.playlist_list);
	const gchar *buf;
	if(info.conlock) return;
	info.conlock = TRUE;
	esf =  glade_xml_new(GLADE_PATH"gmpc.glade", "savedialog", NULL);
	dialog = glade_xml_get_widget(esf, "savedialog");
	entry = glade_xml_get_widget(esf, "name_entry");
	switch(gtk_dialog_run(GTK_DIALOG(dialog)))
	{
		case GTK_RESPONSE_OK:
			buf = gtk_entry_get_text(GTK_ENTRY(entry));
			break;
		default:
			gtk_widget_destroy(dialog);
			g_object_unref(esf);
			info.conlock = FALSE;
			return;
	}


	if(gtk_tree_model_get_iter_first(model, &iter))
	{
		do	
		{
			char *buf1;
			gtk_tree_model_get(model, &iter, 0, &buf1, -1);
			if(!strcmp(buf1, buf))
			{
				mpd_sendRmCommand(info.connection, (char *)buf);
				if(check_for_errors())
				{
					gtk_widget_destroy(dialog);
					g_object_unref(esf);
					return;
				}
				mpd_finishCommand(info.connection);
				if(check_for_errors())
				{
					gtk_widget_destroy(dialog);
					g_object_unref(esf);
					return;
				}
			}
		}while(gtk_tree_model_iter_next(model, &iter));
	}
	mpd_sendSaveCommand(info.connection, (char *)buf);
	if(check_for_errors())
	{
		gtk_widget_destroy(dialog);
		g_object_unref(esf);
		return;
	}
	mpd_finishCommand(info.connection);    
	if(check_for_errors())
	{
		gtk_widget_destroy(dialog);
		g_object_unref(esf);
		return;
	}   
	gtk_widget_destroy(dialog);
	g_object_unref(esf);
	fill_playlist_tree();
	info.conlock = FALSE;
	fill_playlist_tree();
}


/** remove's the selected playlist */
void delete_playlist()
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkWidget *tree = glade_xml_get_widget(xml_playlist_window, "playlist_treeview");
	GtkTreeModel *model = GTK_TREE_MODEL(info.playlist_list);
	if(info.conlock) return;
	info.conlock = TRUE;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *buf;
		gtk_tree_model_get(model, &iter, 0, &buf, -1);	
		if(debug)g_print("**DEBUG** removing %s\n", buf);
		mpd_sendRmCommand(info.connection, buf);
		if(check_for_errors())
		{
			return;
		}
		mpd_finishCommand(info.connection);    	
		if(check_for_errors())
		{
			return;
		}
		fill_playlist_tree();
	}
	else g_warning("No selection to delete");
	info.conlock = FALSE;
	fill_playlist_tree();	
}

/* load the selected playlist */
void playlist_row_selected(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col)
{
	GtkTreeIter iter;
	char *buf;
	if(info.conlock) return;
	info.conlock = TRUE;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(info.playlist_list),  &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(info.playlist_list), &iter, 0, &buf, -1);
	mpd_sendClearCommand(info.connection);
	if(check_for_errors()) return;
	mpd_finishCommand(info.connection);
	if(check_for_errors()) return;
	mpd_sendLoadCommand(info.connection, buf);
	if(check_for_errors()) return;
	mpd_finishCommand(info.connection);
	if(check_for_errors()) return;
	info.conlock = FALSE;
}


/* these 2 function delete all selected songs backwards..  */    
void delete_song(GtkTreeModel *model, GtkTreePath *path)
{
	GtkTreeIter iter;
	gint value; 
	mpd_Song *song = NULL;
	GList *list;
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, 0, &value, -1);
	/* delete it in the locale playlist 2.. so we keep in sync */
	list = g_list_nth(info.playlist, value);
	song = list->data;
	mpd_freeSong(song);
	info.playlist = g_list_remove_link(info.playlist, list);
	g_list_free_1(list);
	mpd_sendDeleteCommand(info.connection, value);
}

void delete_selected_songs()
{
	int i = 0;
	GtkTreeModel *model = GTK_TREE_MODEL(info.cur_list);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(xml_playlist_window, "current_playlist")));
	if(info.conlock) return;
	info.conlock = TRUE;

	if((i = gtk_tree_selection_count_selected_rows(selection)) > 0)
	{
		GList *list = NULL;
		mpd_sendCommandListBegin(info.connection);
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		list = g_list_last(list);
		do{
			delete_song(model, list->data);
		}while((list = g_list_previous(list)));
		mpd_sendCommandListEnd(info.connection);
		mpd_finishCommand(info.connection);
		/* free list */
		g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
	/* get latest playlist_id.. and make the current.. because playlist is up to date */
	if(check_for_errors()) return;    
	info.status = mpd_getStatus(info.connection);
	if(check_for_errors()) return;    
	info.playlist_id = info.status->playlist;

	info.conlock = FALSE;    
	load_songs_with_filter();
}

/* the select all and select none function */
void select_all_current_songs(GtkWidget *menu, GtkWidget *tree)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));    
	gtk_tree_selection_select_all (selection);
}
void select_no_current_songs(GtkWidget *menu, GtkWidget *tree)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));    
	gtk_tree_selection_unselect_all (selection);    
}

void show_song_info()
{
	int i = 0;
	GtkTreeModel *model = GTK_TREE_MODEL(info.cur_list);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(xml_playlist_window, "current_playlist")));
	if((i = gtk_tree_selection_count_selected_rows(selection)) > 0)
	{
		GList *list = NULL;
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		list = g_list_last(list);
		do{
			GtkTreeIter iter;
			int value;
			gtk_tree_model_get_iter(model, &iter,(GtkTreePath *)list->data);
			gtk_tree_model_get(model, &iter, 0, &value, -1);
			call_id3_window(value);
		}while((list = g_list_previous(list)));
		/* free list */
		g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
	if(check_for_errors()) return;    
	info.conlock = FALSE;    


}

int current_playlist_mouse_event(GtkWidget *tree, GdkEventButton *event)
{
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	if(event->button != 3) return FALSE;
	if(gtk_tree_selection_count_selected_rows(selection) > 0)
	{
		GtkWidget *ms;
		GtkWidget *menus[6];
		ms = gtk_menu_new();
		menus[0] = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE,NULL);
		menus[1] = gtk_image_menu_item_new_with_label("Select All");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menus[1]), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
		menus[2] = gtk_image_menu_item_new_with_label("Select None");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menus[2]), gtk_image_new_from_stock(GTK_STOCK_REMOVE, 	GTK_ICON_SIZE_MENU));
		menus[3] = gtk_image_menu_item_new_with_label("Show song info");
		menus[4] = gtk_image_menu_item_new_with_label("Clear playlist");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menus[4]), gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU));
		menus[5] = gtk_image_menu_item_new_with_label("Shuffle playlist");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menus[5]), gtk_image_new_from_stock(GTK_STOCK_CONVERT, GTK_ICON_SIZE_MENU));
		g_signal_connect(G_OBJECT(menus[0]), "activate", G_CALLBACK(delete_selected_songs), NULL);
		g_signal_connect(G_OBJECT(menus[1]), "activate", G_CALLBACK(select_all_current_songs), tree);
		g_signal_connect(G_OBJECT(menus[2]), "activate", G_CALLBACK(select_no_current_songs), tree);
		g_signal_connect(G_OBJECT(menus[3]), "activate", G_CALLBACK(show_song_info), NULL);
		g_signal_connect(G_OBJECT(menus[5]), "activate", G_CALLBACK(shuffle_songs), NULL);
		g_signal_connect(G_OBJECT(menus[4]), "activate", G_CALLBACK(clear_playlist), tree);
		gtk_menu_append(GTK_MENU(ms), menus[1]);		
		gtk_menu_append(GTK_MENU(ms), menus[2]);
		gtk_menu_append(GTK_MENU(ms), gtk_separator_menu_item_new());
		gtk_menu_append(GTK_MENU(ms), menus[0]);
		gtk_menu_append(GTK_MENU(ms), menus[4]);
		gtk_menu_append(GTK_MENU(ms), menus[5]);
		gtk_menu_append(GTK_MENU(ms), gtk_separator_menu_item_new());
		gtk_menu_append(GTK_MENU(ms), menus[3]);
		gtk_widget_show_all(ms);	
		gtk_menu_popup(GTK_MENU(ms), NULL, NULL, NULL, NULL, event->button, event->time);
		gtk_widget_show_all(ms);
		return TRUE;
	}
	return FALSE;
}

int hide_playlist_view()
{
	GtkWidget *vbox = glade_xml_get_widget(xml_playlist_window, "playlist_vbox");
	if(info.playlist_view_hidden == FALSE)
	{
		gtk_widget_hide(vbox);
		gtk_arrow_set(GTK_ARROW(glade_xml_get_widget(xml_playlist_window, "arrow1")), 
				GTK_ARROW_DOWN, GTK_SHADOW_NONE);
		info.playlist_view_hidden = TRUE;
	}
	else {
		gtk_widget_show(vbox);
		gtk_arrow_set(GTK_ARROW(glade_xml_get_widget(xml_playlist_window, "arrow1")), 
				GTK_ARROW_LEFT, GTK_SHADOW_NONE);                             	
		info.playlist_view_hidden = FALSE;
	}
	return TRUE;
}


/* load the directorie's in the current path */
void load_directories(gchar *oldp)
{
	mpd_InfoEntity *entity;
	GtkTreeIter iter;
	if(info.conlock) return;

	gtk_list_store_clear(info.file_list);
	gtk_list_store_clear(info.dir_list);
	if(oldp != NULL)if(strcmp(info.path, "/")) 
	{
		gtk_list_store_append(info.dir_list, &iter);
		gtk_list_store_set(info.dir_list, &iter, 0,oldp , 1, "../",-1);
	}

	mpd_sendLsInfoCommand(info.connection , (char *)info.path);
	if(check_for_errors()) return;
	while ((entity = mpd_getNextInfoEntity(info.connection)))
	{
		if(check_for_errors())
		{

			return;
		}	

		if(entity->type == MPD_INFO_ENTITY_TYPE_DIRECTORY)
		{	

			gchar *base;
			gtk_list_store_append(info.dir_list, &iter);
			base = g_path_get_basename(entity->info.directory->path);
			gtk_list_store_set(info.dir_list, &iter, 0,entity->info.directory->path , 1, base,-1);
			g_free(base);
		}
		else if(entity->type == MPD_INFO_ENTITY_TYPE_SONG)
		{
			gchar *short_title = NULL; 
			gtk_list_store_append(info.file_list, &iter);
			if(entity->info.song->title == NULL)
			{
				gchar *utf8 = g_path_get_basename(entity->info.song->file);
				short_title = shorter_string(utf8);
				gtk_list_store_set(info.file_list, &iter,0, entity->info.song->file,1,short_title,-1);
				g_free(short_title);
				g_free(utf8);
			}
			else
			{
				short_title =  shorter_string(entity->info.song->title);
				gtk_list_store_set(info.file_list, &iter,0, entity->info.song->file,1,short_title,-1);
				g_free(short_title);
			}
			if(entity->info.song->artist != NULL)				
			{
				short_title =  shorter_string(entity->info.song->artist);
				gtk_list_store_set(info.file_list, &iter,2,short_title,-1);
				g_free(short_title);
			}
			if(entity->info.song->album != NULL)	
			{
				short_title =  shorter_string(entity->info.song->album);
				gtk_list_store_set(info.file_list, &iter,3,short_title,-1);
				g_free(short_title);
			}
			if(entity->info.song->track != NULL)	
			{
				gtk_list_store_set(info.file_list, &iter,4,entity->info.song->track,-1);
			}                                                                               			
		}
		mpd_freeInfoEntity(entity);
	}
}

void directory_row_selected(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col)
{
	GtkTreeIter iter;
	GtkWidget *widget;
	char *buf, *new_buf, *buf1;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(info.dir_list),  &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(info.dir_list), &iter, 0, &buf1, -1);
	widget = glade_xml_get_widget(xml_playlist_window, "path_label");
	new_buf = g_strdup_printf("Current Directory: %s",  buf1);
	gtk_label_set_text(GTK_LABEL(widget), new_buf);
	buf = g_path_get_dirname(buf1);
	if(!strcmp(buf, "."))
	{
		g_free(buf);
		buf = g_strdup("/");
	}
	memset(info.path, '\0', 1024);
/*	bzero(info.path,1024);*/
	strncpy(info.path, buf1, 1024);
	load_directories(buf);
	g_free(new_buf);
	g_free(buf);
}  

void add_song_file_browser(GtkWidget *menu, GtkWidget *tree)
{
	int i = 0;
	mpd_InfoEntity *entity = NULL;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	if((i = gtk_tree_selection_count_selected_rows(selection)) > 0)
	{
		GList *list = NULL;
		GList *glist = NULL;
		GList *templist = NULL;
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		list = g_list_first(list);
		do{
			GtkTreeIter iter;
			gchar *value;
			gtk_tree_model_get_iter(model, &iter,(GtkTreePath *)list->data);
			/*** check if directory view or file view***/
			if(model == GTK_TREE_MODEL(info.dir_list))	
			{
				gchar *temp;
				gtk_tree_model_get(model, &iter, 0, &value, 1, &temp,-1);
				if(strcmp(temp, "../")) 	
				{
					if(debug)g_print("adding dir |%s|\n", value);
					mpd_sendListallCommand(info.connection, value);
					entity = mpd_getNextInfoEntity(info.connection);
					while(entity != NULL)
					{
						if(check_for_errors())
						{
							g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
							g_list_free (list);
							while((glist = g_list_next(glist))) g_free(glist->data);
							g_list_free(glist);
							if(debug)g_print("oeps\n");
							return;
						}
						if(entity->type == MPD_INFO_ENTITY_TYPE_SONG) glist = g_list_append(glist, g_strdup(entity->info.song->file));
						mpd_freeInfoEntity(entity);
						entity = mpd_getNextInfoEntity(info.connection);
					}
					/* add the songs */
					mpd_sendCommandListBegin(info.connection);
					glist = templist = g_list_first(glist);
					if(templist != NULL)do{
						mpd_sendAddCommand(info.connection,templist->data);	
					}while((templist = g_list_next(templist)));

					mpd_sendCommandListEnd(info.connection);
					mpd_finishCommand(info.connection);
					while((glist = g_list_next(glist))) g_free(glist->data);
					g_list_free(glist);
					if(check_for_errors())
					{
						g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
						g_list_free (list);
						return;
					}
				}
			}
			else if (model == GTK_TREE_MODEL(info.file_list) || model == GTK_TREE_MODEL(info.search_list))
			{

				gtk_tree_model_get(model, &iter, 0, &value, -1);
				mpd_sendAddCommand(info.connection, value);
			}
			else if (model == GTK_TREE_MODEL(info.id3_songs_list))
			{
				gtk_tree_model_get(model, &iter, 0, &value, -1);
				mpd_sendAddCommand(info.connection, value);
			}
			else if (model == GTK_TREE_MODEL(info.id3_album_list))
			{
				GtkTreeIter iter1, iter2;
				gchar *artist, *album;
				if(gtk_tree_path_get_depth((GtkTreePath *)list->data) == 1)
				{
					album = g_strdup("All");
					gtk_tree_model_get_iter(model, &iter1,(GtkTreePath *)list->data);
					gtk_tree_model_get(model, &iter1, 0,&artist, -1);
				}
				else{
					gtk_tree_model_get_iter(model, &iter2,(GtkTreePath *)list->data);
					gtk_tree_model_get(model, &iter2, 0, &album, -1);
					gtk_tree_model_iter_parent(model, &iter1, &iter2);
					gtk_tree_model_get(model, &iter1, 0,&artist, -1);
				}
				mpd_sendFindCommand(info.connection, MPD_TABLE_ARTIST, artist);
				if(!check_for_errors())
					while((entity = mpd_getNextInfoEntity(info.connection)))
					{
					if(entity->info.song->album == NULL)
					{
					if(!g_utf8_collate(album, "All"))
					{
					if(entity->type == MPD_INFO_ENTITY_TYPE_SONG && entity->info.song->file != NULL) glist = g_list_append(glist, g_strdup(entity->info.song->file));
					mpd_freeInfoEntity(entity);
					}
					
					}
					else
					if(!g_utf8_collate(album, entity->info.song->album) || !g_utf8_collate(album, "All"))
						{
							if(entity->type == MPD_INFO_ENTITY_TYPE_SONG && entity->info.song->file != NULL) glist = g_list_append(glist, g_strdup(entity->info.song->file));
							mpd_freeInfoEntity(entity);
						}
					}
				/* add the songs */
				mpd_sendCommandListBegin(info.connection);

				glist = templist = g_list_first(glist);
				if(templist != NULL)do{
					mpd_sendAddCommand(info.connection,templist->data);
				}while((templist = g_list_next(templist)));
				mpd_sendCommandListEnd(info.connection);

				while((glist = g_list_next(glist))) g_free(glist->data);
				g_list_free(glist);
				if(check_for_errors())
				{
					g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
					g_list_free (list);
					return;
				}


				g_free(album); g_free(artist);			
			}
			mpd_finishCommand(info.connection);

		}while((list = g_list_next(list)));
		/* free list */
		g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
	if(check_for_errors()) return;    
	info.conlock = FALSE;    


}

int file_browser_activate(GtkWidget *tree)
{
	add_song_file_browser(NULL, tree);    
	return TRUE;
}

void file_browser_add_button()
{
	GtkWidget *tree = glade_xml_get_widget(xml_playlist_window, "treeview_dirs");
	if(gtk_widget_is_focus(tree))
	{
		add_song_file_browser(NULL, tree);
	}
	else
	{
		tree = glade_xml_get_widget(xml_playlist_window, "treeview_files");
		add_song_file_browser(NULL, tree);    
	}	
}

int file_browser_mouse_event(GtkWidget *tree, GdkEventButton *event)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
	if(event->button != 3) return FALSE;
	if(gtk_tree_selection_count_selected_rows(selection) > 0)
	{
		GtkWidget *ms;
		GtkWidget *menus[3];
		ms = gtk_menu_new();
		menus[0] = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
		menus[1] = gtk_image_menu_item_new_with_label("Select All");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menus[1]), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
		menus[2] = gtk_image_menu_item_new_with_label("Select None");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menus[2]), gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU));
		g_signal_connect(G_OBJECT(menus[1]), "activate", G_CALLBACK(select_all_current_songs), tree);
		g_signal_connect(G_OBJECT(menus[2]), "activate", G_CALLBACK(select_no_current_songs), tree);
		g_signal_connect(G_OBJECT(menus[0]), "activate", G_CALLBACK(add_song_file_browser), tree);


		gtk_menu_append(GTK_MENU(ms), menus[1]);		
		gtk_menu_append(GTK_MENU(ms), menus[2]);
		gtk_menu_append(GTK_MENU(ms), gtk_separator_menu_item_new());
		gtk_menu_append(GTK_MENU(ms), menus[0]);
		gtk_widget_show_all(ms);	
		gtk_menu_popup(GTK_MENU(ms), NULL, NULL, NULL, NULL, event->button, event->time);
		gtk_widget_show_all(ms);
		return TRUE;
	}
	return FALSE;
}

void collapse_artist_tree()
{
	gtk_tree_view_collapse_all(GTK_TREE_VIEW(glade_xml_get_widget(xml_playlist_window, "id3_artist_tree")));
}
void expand_artist_tree()
{
	gtk_tree_view_expand_all(GTK_TREE_VIEW(glade_xml_get_widget(xml_playlist_window, "id3_artist_tree")));
}

void add_selected_id3_browser()
{

	GtkWidget * tree = glade_xml_get_widget(xml_playlist_window, "id3_artist_tree");
	if(gtk_widget_is_focus(tree))
	{
		add_song_file_browser(NULL, tree);
	}
	else
	{
		tree = glade_xml_get_widget(xml_playlist_window, "id3_songs_tree");
		add_song_file_browser(NULL, tree);
	}
}


void fill_artist_tree()
{
	gchar *buf;
	GtkTreeModel *model = GTK_TREE_MODEL(info.id3_album_list);
	GtkTreeIter iter1, iter2;
	if(debug)g_print("**DEBUG** fill artist tree\n");
	mpd_sendListCommand(info.connection, MPD_TABLE_ARTIST, NULL);
	if(check_for_errors()) return;
	while((buf = mpd_getNextArtist(info.connection)) != NULL)
	{
		gtk_tree_store_append(info.id3_album_list, &iter1, NULL);
		gtk_tree_store_set(info.id3_album_list, &iter1,0, buf,-1);
		g_free(buf);
	}
	mpd_finishCommand(info.connection);
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(info.id3_album_list), &iter1)) return;
	do{
		gtk_tree_model_get(model, &iter1, 0, &buf, -1);
		mpd_sendListCommand(info.connection,MPD_TABLE_ALBUM, buf);
		while((buf = mpd_getNextAlbum(info.connection)) != NULL)
		{
			gtk_tree_store_append(info.id3_album_list, &iter2,&iter1);
			gtk_tree_store_set(info.id3_album_list, &iter2,0, buf,-1);
			g_free(buf);
		}


	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(info.id3_album_list), &iter1));

}

void refresh_information_tab()
{
	if(info.connection == NULL) return;
	if(info.stats != NULL)
	{
		mpd_freeStats(info.stats);
	}
	info.stats = mpd_getStats(info.connection);
	mpd_finishCommand(info.connection);

	update_information_tab();
}

void update_information_tab()
{
	int day = 0, hour = 0, min = 0, sec = 0;
	gchar *temp;
	/* calculate day, hour, min and seconds */
	day = (int)(info.total_playtime/86400);
	hour = (int)(info.total_playtime/3600) - day*24;
	min = (int)(info.total_playtime/60) - hour*60 - day*1440;
	sec = info.total_playtime - min*60 - hour*3600 - day*86400;
	/* make a label */
	temp = g_strdup_printf("%id %ih %im %is", day, hour,min,sec);
	/* set the label */
	gtk_label_set_label(GTK_LABEL(glade_xml_get_widget(xml_playlist_window, "playlist_info_playtime")), temp);
	g_free(temp);

	/* set total number of songs in the playlist */
	temp = g_strdup_printf("%lu", (long unsigned)info.total_number_of_songs);
	gtk_label_set_label(GTK_LABEL(glade_xml_get_widget(xml_playlist_window, "playlist_info_nsongs")), temp);
	g_free(temp);

	/* set host label */ 
	/* FIXME no need to updat this every time, only on playlist open */
	temp = g_strdup_printf("%s:%i", preferences.host, preferences.port);
	gtk_label_set_label(GTK_LABEL(glade_xml_get_widget(xml_playlist_window, "mpd_info_host")), temp);
	g_free(temp);

	/* set version */
	temp = g_strdup_printf("%i.%i.%i", info.connection->version[0],info.connection->version[1],info.connection->version[2] );
	gtk_label_set_label(GTK_LABEL(glade_xml_get_widget(xml_playlist_window, "mpd_info_version")), temp);
	g_free(temp);

	/* set uptime */
	day =  info.stats->uptime/86400;
	hour = (int)info.stats->uptime/3600 - day*24;
	min = (int)info.stats->uptime/60 - day*1440 - hour*60;
	temp = g_strdup_printf("%id %ih %im",day, hour,min);
	gtk_label_set_label(GTK_LABEL(glade_xml_get_widget(xml_playlist_window, "mpd_info_uptime")), temp);
	g_free(temp);

	/* set time played */
	day =  info.stats->playTime/86400;
	hour = (int)info.stats->playTime/3600 - day*24;
	min = (int)info.stats->playTime/60 - day*1440 - hour*60;
	temp = g_strdup_printf("%id %ih %im",day, hour,min);
	gtk_label_set_label(GTK_LABEL(glade_xml_get_widget(xml_playlist_window, "mpd_info_music_played")), temp);
	g_free(temp);                 


	/* set time played */
	day =  info.stats->dbPlayTime/86400;
	hour = (int)info.stats->dbPlayTime/3600 - day*24;
	min = (int)info.stats->dbPlayTime/60 - day*1440 - hour*60;
	sec = info.stats->dbPlayTime - min*60 - hour*3600 - day*86400;
	temp = g_strdup_printf("%id %ih %im %is",day, hour,min, sec);
	gtk_label_set_label(GTK_LABEL(glade_xml_get_widget(xml_playlist_window, "mdb_info_playtime")), temp);
	g_free(temp);                 

	/* last update db */
	temp = g_strdup_printf("%s",ctime((time_t *)&info.stats->dbUpdateTime));
	/*(remove trailing \n*/
	temp[strlen(temp)-1] = ' ';
	gtk_label_set_label(GTK_LABEL(glade_xml_get_widget(xml_playlist_window, "mdb_info_update")), temp);
	g_free(temp);

	/* total number of songs */
	temp = g_strdup_printf("%i",info.stats->numberOfSongs);
	gtk_label_set_label(GTK_LABEL(glade_xml_get_widget(xml_playlist_window, "mdb_info_nsongs")), temp);
	g_free(temp);

	/* total number of artists */
	temp = g_strdup_printf("%i",info.stats->numberOfArtists);
	gtk_label_set_label(GTK_LABEL(glade_xml_get_widget(xml_playlist_window, "mdb_info_nartists")), temp);
	g_free(temp);

	/* total number of albums*/
	temp = g_strdup_printf("%i",info.stats->numberOfAlbums);
	gtk_label_set_label(GTK_LABEL(glade_xml_get_widget(xml_playlist_window, "mdb_info_nalbums")), temp);
	g_free(temp);				
}

void fill_id3_songs_tree(GtkWidget *tree)
{
	GtkTreeIter iter1, iter2;    
	gchar *artist;
	gchar  *album;
	GtkTreeModel *model = GTK_TREE_MODEL(info.id3_album_list);
	GtkTreeSelection *selection;
	mpd_InfoEntity *entity = NULL;
	gtk_list_store_clear(info.id3_songs_list);
	if((selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree))) == NULL) return;
	if(gtk_tree_selection_count_selected_rows(selection) > 0)
	{
		GList *list = NULL;
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		list = g_list_first(list);
		if(list != NULL)do{
			if(gtk_tree_path_get_depth((GtkTreePath *)list->data) == 1)
			{
				album = g_strdup("All");
				gtk_tree_model_get_iter(model, &iter1,(GtkTreePath *)list->data);
				gtk_tree_model_get(model, &iter1, 0,&artist, -1);
			}
			else{
				gtk_tree_model_get_iter(model, &iter2,(GtkTreePath *)list->data);
				gtk_tree_model_get(model, &iter2, 0, &album, -1);
				gtk_tree_model_iter_parent(model, &iter1, &iter2);
				gtk_tree_model_get(model, &iter1, 0,&artist, -1);
			}
			mpd_sendFindCommand(info.connection, MPD_TABLE_ARTIST, artist);
			if(!check_for_errors())	while((entity = mpd_getNextInfoEntity(info.connection)))
			{
				GtkTreeIter iter;
				if(entity->info.song->album != NULL)
				{
				if(!g_utf8_collate(album, "All") || !g_utf8_collate(album, entity->info.song->album))
				{
					gtk_list_store_append(info.id3_songs_list, &iter);
					if(entity->info.song->title != NULL )
					{
						gtk_list_store_set(info.id3_songs_list, &iter,0,entity->info.song->file,1,entity->info.song->title,3,entity->info.song->track,-1);
					}	
				}
				}
				else
				{
				if(!g_utf8_collate(album, "All"))
				{
					gtk_list_store_append(info.id3_songs_list, &iter);
					if(entity->info.song->title != NULL )
					{
						gtk_list_store_set(info.id3_songs_list, &iter,0,entity->info.song->file,1,entity->info.song->title,3,entity->info.song->track,-1);
					}	
				}
				
				
				}
				mpd_freeInfoEntity(entity);
			}
			g_free(album); g_free(artist);			
		}while((list = g_list_next(list)));
		/* free list */
		g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
}

void add_selected_search()
{
	GtkWidget *tree = glade_xml_get_widget(xml_playlist_window, "search_tree");	
	add_song_file_browser(NULL, tree);
}

void do_server_side_search()
{
	int search_type = 0;
	int i = 0;
	GtkTreeIter iter;
	const gchar *search_string;
	mpd_InfoEntity *entity = NULL;
	gtk_list_store_clear(info.search_list);

	/* get the searchtype */
	i = gtk_option_menu_get_history(GTK_OPTION_MENU(glade_xml_get_widget(xml_playlist_window, "search_option_menu")));
	switch (i)    
	{
		case 0: search_type = MPD_TABLE_TITLE;
			break;
		case 1: search_type = MPD_TABLE_ARTIST;
			break;
		case 2: search_type = MPD_TABLE_ALBUM;
			break;	
		case 3: search_type = MPD_TABLE_FILENAME;
			break;
		default:
			search_type = MPD_TABLE_ARTIST;
			break;
	}
	/* get the search string */
	search_string = gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml_playlist_window, "server_search_entry")));
	/* do the server side search */
	mpd_sendSearchCommand(info.connection, search_type, search_string);
	/* look and add them to the tree view */
	while((entity = mpd_getNextInfoEntity(info.connection)) != NULL)
	{
		gtk_list_store_append(info.search_list, &iter);
		if(entity->info.song->title == NULL)
		{
			gchar *basename = g_path_get_basename(entity->info.song->file);
			gtk_list_store_set(info.search_list, &iter, 0, entity->info.song->file, 1, basename, -1);
			g_free(basename);
		}
		else gtk_list_store_set(info.search_list, &iter, 0, entity->info.song->file, 1, entity->info.song->title, 2,entity->info.song->artist,3,entity->info.song->album, -1);
		mpd_freeInfoEntity(entity);	
	}
	mpd_finishCommand(info.connection);
}

