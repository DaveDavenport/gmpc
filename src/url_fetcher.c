#include <stdio.h>
#include <string.h>
#include <glade/glade.h>
#include <libmpd/libmpd.h>
#include <libmpd/debug_printf.h>
#include  "plugin.h"
#include "main.h"
#include "gmpc_easy_download.h"

/** in gmpc */
void pl3_option_menu_activate(); 
void pl3_push_statusbar_message(char *mesg);	



gmpcPlugin url_plugin = {
	.name = "Url Parser Plugin",
	.version = {0,0,3},
	.plugin_type = GMPC_PLUGIN_PL_BROWSER,
};
/***
 * Parse PLS files:
 */
static void url_parse_pls_file(const char *data, int size)
{
	int i=0;
	int songs = 0;
	gchar **tokens = g_strsplit(data, "\n", -1);
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
					printf("Adding %s\n", &tokens[i][del+1]);		
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
static int url_check_binary(char *data, int size)
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

static void parse_data(gmpc_easy_download_struct dld, const char *text)
{
    if(url_check_binary(dld.data, dld.size))
    {
        debug_printf(DEBUG_INFO,"Adding url\n", text);
        mpd_playlist_add(connection, (char *)text);
        pl3_push_statusbar_message(_("Added 1 stream"));
    }
    /** pls file: */
    else if(!strncasecmp(dld.data, "[playlist]",10))
    {
        debug_printf(DEBUG_INFO,"Detected a PLS\n");
        url_parse_pls_file(dld.data, dld.size);
    }
    /** Extended M3U file */
    else if (!strncasecmp(dld.data, "#EXTM3U", 7))
    {
        debug_printf(DEBUG_INFO,"Detected a Extended M3U\n");
        url_parse_extm3u_file(dld.data, dld.size);
    }
    /** Hack to detect most non-extended m3u files */
    else if (!strncasecmp(dld.data, "http://", 7))
    {
        debug_printf(DEBUG_INFO,"Might be a M3U, or generic list\n");
        url_parse_extm3u_file(dld.data, dld.size);
    }
    /** Assume Binary file */
    else
    {
        debug_printf(DEBUG_INFO,"Adding url: %s\n", text);
        mpd_playlist_add(connection, (char *)text);
        pl3_push_statusbar_message(_("Added 1 stream"));
    }
}
void url_start(void)
{
	/**
	 * Setup the Dialog
	 */
	GtkWidget *vbox = NULL, *label = NULL, *entry=NULL,*ali = NULL, *progress = NULL;
	GtkWidget *pl3_win = glade_xml_get_widget(pl3_xml, "pl3_win");
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
		const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
		/*  In any case, don't download more then 2 kbyte */
		gmpc_easy_download_struct dld = {NULL, 0, 4096,(ProgressCallback)url_progress_callback, progress};
		gtk_widget_show(progress);
		gtk_widget_set_sensitive(dialog, FALSE);

		if(gmpc_easy_download(text, &dld) && dld.size)
		{
            parse_data(dld,text);
            gmpc_easy_download_clean(&dld);
			gtk_widget_destroy(dialog);
			return;
		} else {
			/**
			 * Failed to contact %s
			 * Show an error dialog
			 */
			/* Make a copy so we can safely destroy the widget */
			gchar *url = g_strdup(text);
			/* Destroy old popup */
			gtk_widget_destroy(dialog);
			/* Create info dialog */
			dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_INFO,GTK_BUTTONS_CLOSE,
				_("Failed to contact url: '%s'"), url);
			q_free(url);
			/* run it */
			gtk_dialog_run(GTK_DIALOG(dialog));
		}
	}	
	gtk_widget_destroy(dialog);
}



void url_start_real(const gchar *url)
{
	gmpc_easy_download_struct dld = {NULL, 0, 4096,NULL, NULL};
	if(gmpc_easy_download(url, &dld) && dld.size)
	{
        parse_data(dld,url);
        gmpc_easy_download_clean(&dld);
	} else {
		/**
		 * Failed to contact %s
		 * Show an error dialog
		 */
        gchar *mes = g_strdup_printf("%s: %s",_("Failed to parse the playlist link"), url);
	    playlist3_show_error_message(mes, ERROR_CRITICAL);
        q_free(mes);
	}
}
