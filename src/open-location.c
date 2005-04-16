#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <strings.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs.h>
#include "libmpdclient.h"
#include "main.h"
#include "misc.h"
#include "open-location.h"

#define MAX_PLAYLIST_SIZE 10000

GladeXML *ol_xml = NULL;
GnomeVFSAsyncHandle *handle = NULL;
GnomeVFSAsyncHandle *handle1 = NULL;
/* this is so I won't return while busy */
gint working = FALSE;
gint add_anyway = FALSE;

static GtkTargetEntry drag_types[] =
{
	{ "text/plain", 0, 100},
};

void ol_destroy()
{
	if(working) return;
	if(ol_xml != NULL)
	{
		gtk_widget_destroy(glade_xml_get_widget(ol_xml, "add_location"));
		g_object_unref(ol_xml);
	}
	ol_xml = NULL;
}
void ol_file_close()
{
	printf("test\n");
}

void ol_file_read(GnomeVFSAsyncHandle *hand, GnomeVFSResult result, gchar *buffer)
{
	if(result == GNOME_VFS_OK)
	{
		gchar **list = g_strsplit(buffer, "\n",0);
		if(list != NULL && list[0] != NULL)
		{
			if(!strncmp(list[0], "[playlist]", 10))
			{
				int i =1;
				while(list[i] != NULL)
				{
					if(!strncmp(list[i], "File", 4))
					{
						mpd_ob_playlist_add(connection, &list[i][6]);
					}

					i++;
				}              
				working = FALSE;		
				ol_destroy();	

			}
			else {
				int i =0;
				while(list[i] != NULL)
				{
					if(!strncasecmp(list[i], "http://", 7))
					{
						mpd_ob_playlist_add(connection, list[i]);
					}

					i++;
				}
				working = FALSE;		
				ol_destroy();	           			
			}
		}
		g_strfreev(list);
		printf("gkkkn\n");
//		gnome_vfs_async_close(hand, (GnomeVFSAsyncCloseCallback)ol_file_close, NULL);         				
	}
	else
	{
		gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(ol_xml, "label_message")),
				_("<span size=\"x-small\"><i>Failed to read the file.</i></span>"));       			
		working = FALSE;
		printf("fdaljdlk\n");
		gtk_widget_set_sensitive(glade_xml_get_widget(ol_xml, "add_location"),TRUE);
//		gnome_vfs_async_close(hand, (GnomeVFSAsyncCloseCallback)ol_file_close, NULL);         		
	}       


	g_free(buffer);


}	

void ol_file_opened(GnomeVFSAsyncHandle *hand, GnomeVFSResult result, gpointer data)
{
	gint size = GPOINTER_TO_INT(data);

	if(result == GNOME_VFS_OK)
	{
		gchar *buffer = g_malloc0(size+sizeof(char));
		gnome_vfs_async_read(hand, buffer, size,(GnomeVFSAsyncReadCallback)ol_file_read, NULL); 
	}
	else
	{
		g_print("Error found:%s\n", gnome_vfs_result_to_string(result));
		gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(ol_xml, "label_message")),
				_("<span size=\"x-small\"><i>Failed to open file.</i></span>"));       			
		gtk_widget_set_sensitive(glade_xml_get_widget(ol_xml, "add_location"),TRUE);
		working = FALSE;
	}
}


/* grab the type of file the user entered */
void ol_get_fileinfo(GnomeVFSAsyncHandle *hand,GList *results)
{
	GnomeVFSGetFileInfoResult *r = results->data;

	if(r->result == GNOME_VFS_OK)
	{
		g_print("Got mime-type: %s\n", r->file_info->mime_type);
		/* m3u file */
		if(
				!g_utf8_collate(r->file_info->mime_type, "audio/x-scpls") ||
				!g_utf8_collate(r->file_info->mime_type, "audio/x-mpegurl") ||			
				!g_utf8_collate(r->file_info->mime_type, "audio/m3u") ||			
				!g_utf8_collate(r->file_info->mime_type, "audio/pls") ||			
				!g_utf8_collate(r->file_info->mime_type, "text/plain")) /* plain text isnt a stream, so we are gonna try to parse it */
		{
			GnomeVFSURI *uri = gnome_vfs_uri_dup(r->uri);
			gint size = r->file_info->size;
			g_print("found m3u file  size: %i \n",(gint)size);
			if(size == 0) size = MAX_PLAYLIST_SIZE;
			gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(ol_xml, "label_message")),
					_("<span size=\"x-small\"><i>Found playlist file</i></span>"));
			gnome_vfs_async_open_uri(&handle, uri,GNOME_VFS_OPEN_READ,GNOME_VFS_PRIORITY_DEFAULT,
					(GnomeVFSAsyncOpenCallback) ol_file_opened, GINT_TO_POINTER(size));


		}
		/* audio file */
		else if(r->file_info->flags == 0)
		{
			gtk_widget_set_sensitive(glade_xml_get_widget(ol_xml, "add_location"),TRUE);
			g_print("stream found\n");
			mpd_ob_playlist_add(connection, (char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(ol_xml, "entry_stream"))));
			working = FALSE;
			ol_destroy();
		}
		else
		{
			gtk_widget_set_sensitive(glade_xml_get_widget(ol_xml, "add_location"),TRUE);
			gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(ol_xml, "label_message")),
					_("<span size=\"x-small\"><i>Unkown file found.</i></span>"));       			
			working = FALSE;
		}
	}
	else
	{
		gtk_widget_set_sensitive(glade_xml_get_widget(ol_xml, "add_location"),TRUE);
		gtk_widget_hide(glade_xml_get_widget(ol_xml, "hbox4"));
		gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(ol_xml, "label_message")),                          	
				_("<i>Failed to grab fileinfo.</i>\n<span size=\"larger\">Add anyway?</span>"));       			
		working = FALSE;
		add_anyway = TRUE;
	}
}



void ol_add_location()
{
	GList *list = NULL;
	GnomeVFSURI *uri;
	if(strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(ol_xml, "entry_stream")))) == 0 ) return;
	if(add_anyway)
	{
		mpd_ob_playlist_add(connection, (char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(ol_xml, "entry_stream"))));
		ol_destroy();
		return;
	}
	uri = gnome_vfs_uri_new(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(ol_xml, "entry_stream"))));
	if(uri == NULL)
	{
		g_print("Invalid link\n");
		gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(ol_xml, "label_message")),
				_("<span size=\"x-small\"><i>Invalid link format</i></span>"));
		return;
	}
	list = g_list_append(NULL,uri);
	gtk_widget_set_sensitive(glade_xml_get_widget(ol_xml, "add_location"),FALSE);
	working = TRUE;

	gnome_vfs_async_get_file_info(&handle,  
			list,
			GNOME_VFS_FILE_INFO_GET_MIME_TYPE,
			GNOME_VFS_PRIORITY_DEFAULT,
			(GnomeVFSAsyncGetFileInfoCallback)ol_get_fileinfo,
			NULL);
}


void ol_replace_location()
{
	mpd_ob_playlist_clear(connection);
	ol_add_location();
}

/* data I got from a drag */
void ol_drag_data_recieved(GtkWidget *window, GdkDragContext *context,
		gint x, gint y, GtkSelectionData *selection_data,
		guint info, guint time)

{
	GList *p = NULL, *list = NULL;
	g_print("Drag start\n");
	list = gnome_vfs_uri_list_parse ((const char *)selection_data->data);
	p = list;

	while (p != NULL)
	{
		g_print("%s\n",gnome_vfs_uri_to_string ((const GnomeVFSURI*)(p->data),GNOME_VFS_URI_HIDE_NONE)); 
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(ol_xml, "entry_stream")),
				gnome_vfs_uri_to_string ((const GnomeVFSURI*)(p->data),GNOME_VFS_URI_HIDE_NONE)                     	
				);
		p = p->next;
	}

/*	gnome_vfs_uri_list_free (list);*/

	gtk_drag_finish(context, TRUE, FALSE, time);
}



void ol_create_url(GtkWidget *wid,char *url)
{
	/* check if allready open */
	if(ol_xml != NULL)
	{
		gtk_widget_show_all(
				glade_xml_get_widget(ol_xml, "add_location"));
		gtk_window_present(GTK_WINDOW(
					glade_xml_get_widget(ol_xml, "add_location")));
		return;          	
	}

	add_anyway = FALSE;
	/* create glade file, and set parent */
	ol_xml = glade_xml_new(GLADE_PATH"open-location.glade", "add_location",NULL);
	gtk_window_set_transient_for(GTK_WINDOW(glade_xml_get_widget(ol_xml, "add_location")), GTK_WINDOW(wid));

	/* Accept drops from outside */
	gtk_drag_dest_set(glade_xml_get_widget(ol_xml, "add_location"), GTK_DEST_DEFAULT_ALL, drag_types, 1, GDK_ACTION_COPY);
	gtk_drag_dest_set(glade_xml_get_widget(ol_xml, "entry_stream"), GTK_DEST_DEFAULT_ALL, drag_types, 1, GDK_ACTION_COPY);

	/* set image with custom stock */
	gtk_image_set_from_stock(GTK_IMAGE(glade_xml_get_widget(ol_xml, "image")), "media-stream", GTK_ICON_SIZE_DIALOG);
	if(url != NULL)
	{
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(ol_xml, "entry_stream")),url);
	}

	/* set correct signals */
	g_signal_connect (G_OBJECT (glade_xml_get_widget(ol_xml, "add_location")), "drag_data_received",
			G_CALLBACK (ol_drag_data_recieved),
			NULL);
	/*	g_signal_connect (G_OBJECT (glade_xml_get_widget(ol_xml, "entry_stream")), "drag_data_received",
		G_CALLBACK (ol_drag_data_recieved),
		NULL);
		*/
	glade_xml_signal_autoconnect(ol_xml);	
}

void ol_create(GtkWidget *wid)
{
	ol_create_url(wid, NULL);

}
