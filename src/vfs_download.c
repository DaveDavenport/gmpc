#include <gtk/gtk.h>
#include <string.h>
#include "vfs_download.h"

#define BUF_SIZE 512

void file_close()
{
/*	g_print("file closed\n");*/
}


void transfer_stopped(GtkWidget *dialog, gint response, dl_info *di)
{
	if(di->handle != NULL)
	{
		gnome_vfs_async_cancel(di->handle);
	}

	g_print("Stopping transfer\n");
	gtk_widget_destroy(di->dialog);
	g_free(di->data);
	g_free(di);
}

void start_transfer(gchar *link, void *(*function)(gchar *buffer, gpointer data),gpointer data,GtkWidget *parent_window)
{
	dl_info *di = g_malloc(sizeof(dl_info));
	di->link = g_strdup(link);
	di->function = function;
	di->handle = NULL;
	di->data = data;
	di->file = NULL;
	di->dialog =  gtk_message_dialog_new(GTK_WINDOW(parent_window), 
			GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_INFO, GTK_BUTTONS_CANCEL,  "Fetching information");
	di->prog_bar = gtk_progress_bar_new();
	gtk_box_pack_end(GTK_BOX(GTK_DIALOG(di->dialog)->vbox), di->prog_bar, FALSE, TRUE,6);
	gtk_widget_show_all(di->dialog);


	g_signal_connect(G_OBJECT(di->dialog), "response", G_CALLBACK(transfer_stopped),di);

	get_file(di);
}





void file_read(GnomeVFSAsyncHandle *handle, GnomeVFSResult result, gchar *buffer,
		GnomeVFSFileSize size,GnomeVFSFileSize read,dl_info *di)
{
	if(result == GNOME_VFS_OK)
	{
		g_string_append_len(di->file,buffer,read);
		memset(buffer, BUF_SIZE,'\0');
		gnome_vfs_async_read(handle, buffer, BUF_SIZE,(GnomeVFSAsyncReadCallback)file_read, di);
		gtk_progress_bar_pulse(GTK_PROGRESS_BAR(di->prog_bar));	
	}
	else
	{
		g_string_append_len(di->file,buffer,read);
		g_free(buffer);
		gnome_vfs_async_close(handle,(GnomeVFSAsyncCloseCallback)file_close,NULL);

		di->function(di->file->str,di->data);
		g_string_free(di->file,TRUE);
		gtk_widget_destroy(di->dialog);
		g_free(di->link);
		g_free(di);

	}
}

void file_opened(GnomeVFSAsyncHandle *handle, GnomeVFSResult result, dl_info *di)
{

	if(result == GNOME_VFS_OK)
	{
		gchar *buffer = g_malloc0(BUF_SIZE+sizeof(char));
		di->file = g_string_new("");
		gnome_vfs_async_read(handle, buffer, BUF_SIZE,(GnomeVFSAsyncReadCallback)file_read, di); 
	}
	else
	{
		/* FIXME: Do some error handling */
		gtk_widget_destroy(di->dialog);
		g_free(di);
	}
}


void get_file(dl_info *di)
{


	/* Try to open the file asyncr. */
	gnome_vfs_async_open(&di->handle,di->link,
			GNOME_VFS_OPEN_READ,GNOME_VFS_PRIORITY_DEFAULT,
			(GnomeVFSAsyncOpenCallback) file_opened, di);      	
}
