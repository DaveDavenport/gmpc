/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpcwiki.sarine.nl/
 
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
#include <string.h>
#include <libmpd/libmpd.h>
#include <libmpd/debug_printf.h>
#include "main.h"
#include "playlist3.h"
#include "gmpc_easy_download.h"

/** in gmpc */
void pl3_option_menu_activate(); 

/***
 * Parse PLS files:
 */
static void url_parse_pls_file(const char *data, int size)
{
	int i=0;
	int songs = 0;
	gchar **tokens = g_regex_split_simple("\n", data,G_REGEX_MULTILINE,G_REGEX_MATCH_NEWLINE_ANY); // g_strsplit(data, "\n", -1);
	if(tokens)
	{
		for(i=0;tokens[i];i++)
		{
			/* Check for File */
			if(!strncmp(tokens[i], "File", 4))
			{
				int del = 0;
				/* split the string, look for delimiter = */
				for(del = 3; tokens[i][del] != '\0' && tokens[i][del] != '=';del++);
				/** if delimiter is found, and the url behind it starts with http:// add it*/
				if(tokens[i][del] == '=' && strncmp(&tokens[i][del+1], "http://", 7) == 0)
				{
					printf("Adding '%s'\n", &tokens[i][del+1]);		
					mpd_playlist_add(connection, &tokens[i][del+1]);	
					songs++;
				}
			}
		}
		g_strfreev(tokens);
	}
	if(songs)
	{
		char *string = g_strdup_printf(_("Added %i %s"), songs, ngettext("stream", "streams", songs));
		pl3_push_statusbar_message(string);
		q_free(string);
	}
}
/***
 * Parse EXTM3U Files:
 */
static void url_parse_extm3u_file(const char *data, int size)
{
	int i=0;
	int songs = 0;
	gchar **tokens = g_strsplit(data, "\n", -1);
	if(tokens)
	{
		for(i=0;tokens[i];i++)
		{
			/* Check for File */
			if(!strncmp(tokens[i], "http://", 7))
			{
				printf("Adding %s\n", tokens[i]);
				mpd_playlist_add(connection, tokens[i]);		
				songs++;
			}
		}
		g_strfreev(tokens);
	}
	if(songs)
	{
		char *string = g_strdup_printf(_("Added %i %s"), songs, ngettext("stream", "streams", songs));
		pl3_push_statusbar_message(string);
		q_free(string);
	}
}

/**
 * Check url for correctness
 */
static gboolean url_validate_url(const gchar *text)
{
	/** test if text has a length */
	if(!text || text[0] == '\0')
		return FALSE;
	/* you need at least 8 chars to form http://<url> */
	if(strlen(text) < 8)
		return FALSE;
	/* must start with http */
	if(strncmp(text, "http://", 7))
		return FALSE;

	return TRUE;
}
/**
 * Handle user input
 */
static void url_entry_changed(GtkEntry *entry, GtkWidget *add_button)
{
	const gchar *text = gtk_entry_get_text(entry);
	gtk_widget_set_sensitive(add_button, url_validate_url(text));
}
static void url_progress_callback(int size, int total,GtkProgressBar *pb)
{
	if(total > 0)
	{
		gtk_progress_bar_set_fraction(pb, size/(double)total);
	}
	else{
		gtk_progress_bar_pulse(pb);
	}
	while(gtk_events_pending())
		gtk_main_iteration();
}
static int url_check_binary(const char *data,const int size)
{
/*	int i=0;*/
	int binary = FALSE;
    /*
	for(i=0;i < size;i++) {
		if((unsigned int)data[i] > 127) binary = TRUE;
	}
    */
    binary = !g_utf8_validate(data, size,NULL);
	if(binary)
		printf("Binary data found\n");
	return binary;
}

static void parse_data(const char *data, guint size, const char *text)
{
    if(url_check_binary(data, size))
    {
        debug_printf(DEBUG_INFO,"Adding url\n", text);
        mpd_playlist_add(connection, (char *)text);
        pl3_push_statusbar_message(_("Added 1 stream"));
    }
    /** pls file: */
    else if(!strncasecmp(data, "[playlist]",10))
    {
        debug_printf(DEBUG_INFO,"Detected a PLS\n");
        url_parse_pls_file(data, size);
    }
    /** Extended M3U file */
    else if (!strncasecmp(data, "#EXTM3U", 7))
    {
        debug_printf(DEBUG_INFO,"Detected a Extended M3U\n");
        url_parse_extm3u_file(data, size);
    }
    /** Hack to detect most non-extended m3u files */
    else if (!strncasecmp(data, "http://", 7))
    {
        debug_printf(DEBUG_INFO,"Might be a M3U, or generic list\n");
        url_parse_extm3u_file(data, size);
    }
    /** Assume Binary file */
    else
    {
        debug_printf(DEBUG_INFO,"Adding url: %s\n", text);
        mpd_playlist_add(connection, (char *)text);
        pl3_push_statusbar_message(_("Added 1 stream"));
    }
}
static void url_fetcher_download_callback(GEADAsyncHandler *handle, const GEADStatus status, gpointer user_data)
{
    const gchar *uri = gmpc_easy_handler_get_uri(handle);
    if(status == GEAD_DONE)
    {
        GtkWidget *dialog = user_data;
        goffset length;
        const char *data = gmpc_easy_handler_get_data(handle, &length);
    parse_data(data,(guint)length,uri);
        if(dialog)
        {
            gtk_dialog_response(GTK_DIALOG(gtk_widget_get_toplevel(dialog)), GTK_RESPONSE_CANCEL);
        }
        gmpc_easy_async_free_handler(handle);
    }
    else if (status == GEAD_CANCELLED)
    {
        GtkWidget *dialog = user_data;
        printf("Download cancelled\n");
        if(dialog)
        {
            gtk_widget_hide(dialog);
            gtk_widget_set_sensitive(gtk_widget_get_toplevel(dialog), TRUE);
        }
        gmpc_easy_async_free_handler(handle);

    }
    else if (status == GEAD_PROGRESS)
    {
        goffset length;
        goffset total = gmpc_easy_handler_get_content_size(handle);
        const char *data =  gmpc_easy_handler_get_data(handle, &length);
        if(user_data)
        {
            GtkWidget *progress = user_data;
            if(total > 0){
                gdouble prog = (length/(double)total);
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), prog);
                printf("%f\n", prog);
            }
            else{
                gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progress));
            }
        }
        if(length > 12*1024) {
            printf("Cancel to much data to handle, assume binary\n");
            parse_data(data,(guint)length,uri);
            gmpc_easy_async_cancel(handle);
            if(user_data)
                gtk_dialog_response(GTK_DIALOG(gtk_widget_get_toplevel(GTK_WIDGET(user_data))), GTK_RESPONSE_CANCEL);
            printf("done\n");
        }
    }
    else
    {
        GtkWidget *dialog = user_data;
        if(user_data)
            gtk_dialog_response(GTK_DIALOG(gtk_widget_get_toplevel(dialog)), GTK_RESPONSE_CANCEL);
        gmpc_easy_async_free_handler(handle);
    }
}
void url_start(void)
{
	/**
	 * Setup the Dialog
	 */
	GtkWidget *vbox = NULL, *label = NULL, *entry=NULL,*ali = NULL, *progress = NULL;
	GtkWidget *pl3_win = playlist3_get_window(); 
	GtkWidget *add_button = NULL;
	GtkWidget *dialog = gtk_dialog_new_with_buttons
		("Open URL", NULL,
		 GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
		 NULL);

	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(pl3_win));
	/* Add buttons */
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL); /** CANCEL BUTTON */
	add_button = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_ADD, GTK_RESPONSE_OK); /** ADD BUTTON */

	/* set default state */
	gtk_widget_set_sensitive(add_button, FALSE);
	/**
	 * Setup widgets in dialog
	 */
	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), vbox);

	/* set hig margins */
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 9);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 3);
	/**
	 * Setup the label
	 */
	label = gtk_label_new(_("Enter an url")); 
	ali = gtk_alignment_new(0,0.5,0,0);
	gtk_container_add(GTK_CONTAINER(ali), label);
	gtk_box_pack_start(GTK_BOX(vbox), ali, FALSE, TRUE, 0);
	/**
	 * Setup the entry box
	 */
	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, TRUE, 0);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(url_entry_changed), add_button);
/*	if(url)
		gtk_entry_set_text(GTK_ENTRY(entry), url);
        */
	progress = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox), progress, FALSE, TRUE, 0);

	gtk_widget_show_all(dialog);
	gtk_widget_hide(progress);

	while(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) 
	{
        GEADAsyncHandler *handler;
		const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
		/*  In any case, don't download more then 2 kbyte */
		gtk_widget_show(progress);
		gtk_widget_set_sensitive(dialog, FALSE);
        handler = gmpc_easy_async_downloader(text, url_fetcher_download_callback, progress);
	}	

	gtk_widget_destroy(dialog);
}



void url_start_real(const gchar *url)
{
    gmpc_easy_async_downloader(url, url_fetcher_download_callback, NULL);
}
