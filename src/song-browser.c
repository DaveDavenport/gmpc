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
		sb_store == gtk_tree_store_new(SB_NROWS,
				GTK_TYPE_STRING, /* full path*/
				GTK_TYPE_STRING, /* display string */
				GTK_TYPE_INT,	/* 1 for folder 0 for song */
				GDK_TYPE_PIXBUF);
	}

	tree = glade_xml_get_widget(sb_xml, "treeview");
	/* set selection mode */
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(
		gtk_tree_view_get_selection(GTK_TREE_VIEW(tree))), GTK_SELECTION_MULTIPLE);

	/* set filter */
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree),
			GTK_TREE_MODEL(sb_store));

	/* draw the column with the songs */
	renderer = gtk_cell_renderer_text_new();

	/* insert the column in the tree */
	column = gtk_tree_view_column_new_with_attributes( 
			"Playlist", renderer, 
			"text", SB_DPATH,
			NULL);

	/* draw the column with the songs */
	renderer = gtk_cell_renderer_pixbuf_new();

	/* insert the column in the tree */
	gtk_tree_view_column_pack_start(GTK_TREE_VIEW_COLUMN(column), renderer, FALSE);
	gtk_tree_view_column_set_attributes(
			GTK_TREE_VIEW_COLUMN(column),
			renderer, 
			"pixbuf", SB_DPATH,
			NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), GTK_TREE_VIEW_COLUMN(column));

}




