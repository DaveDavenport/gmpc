/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2012 Qball Cow <qball@gmpclient.org>
 * Project homepage: http://gmpclient.org/
 
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

#include <gtk/gtk.h>
#include <config.h>
#include <main.h>
#include <bug-information.h>
#include <sqlite3.h>
#ifdef HAVE_UNIQUE
#include <unique/uniqueversion.h>
#endif
#include "revision.h"

static void bug_information_generate_message(GtkTextBuffer * buffer)
{
	int i;
	gchar *temp;
	GtkTextIter iter;
	GtkTextTag *bold_tag, *larger_tag;
	GtkTextTagTable *table;

	/* get buffer's tag table */
	table = gtk_text_buffer_get_tag_table(buffer);
	/* Create bold tag */
	bold_tag = gtk_text_tag_new("bold");
	g_object_set(G_OBJECT(bold_tag), "weight", PANGO_WEIGHT_BOLD, NULL);
	/* add bold_tag to tag table */
	gtk_text_tag_table_add(table, bold_tag);

	/* Create bold tag */
	larger_tag = gtk_text_tag_new("larger");
	g_object_set(G_OBJECT(larger_tag), "scale", PANGO_SCALE_X_LARGE, NULL);
	/* add larger_tag to tag table */
	gtk_text_tag_table_add(table, larger_tag);

	/* Get the start */
	gtk_text_buffer_get_start_iter(buffer, &iter);

	/* insert program name */
	gtk_text_buffer_insert_with_tags(buffer, &iter, "Gnome Music Player Client\n", -1, bold_tag, larger_tag, NULL);
	/* insert copyright */
	gtk_text_buffer_insert_with_tags(buffer, &iter, GMPC_COPYRIGHT "\n\n", -1, bold_tag, NULL);

	/* insert tagline */
	gtk_text_buffer_insert_with_tags(buffer, &iter, "Tagline:\t", -1, bold_tag, NULL);
	gtk_text_buffer_insert(buffer, &iter, GMPC_TAGLINE "\n", -1);

	/* insert version */
	gtk_text_buffer_insert_with_tags(buffer, &iter, "Version:\t", -1, bold_tag, NULL);
	temp = g_strdup_printf("%i.%i.%i\n", GMPC_MAJOR_VERSION, GMPC_MINOR_VERSION, GMPC_MICRO_VERSION);
	gtk_text_buffer_insert(buffer, &iter, temp, -1);
	g_free(temp);

	/* insert revision */
	gtk_text_buffer_insert_with_tags(buffer, &iter, "Revision:\t", -1, bold_tag, NULL);
	temp = g_strdup_printf("%s\n", revision);
	gtk_text_buffer_insert(buffer, &iter, temp, -1);
	g_free(temp);

	/** support libs */
	gtk_text_buffer_insert_with_tags(buffer, &iter, "\nSupport libraries:\n", -1, bold_tag, larger_tag, NULL);

	/* libmpd */
	gtk_text_buffer_insert_with_tags(buffer, &iter, "Libmpd:\t", -1, bold_tag, NULL);
	temp = g_strdup_printf("%i.%i.%i\n", LIBMPD_MAJOR_VERSION, LIBMPD_MINOR_VERSION, LIBMPD_MICRO_VERSION);
	gtk_text_buffer_insert(buffer, &iter, temp, -1);
	g_free(temp);

	/* gtk+-2.0 */
	gtk_text_buffer_insert_with_tags(buffer, &iter, "Compile time Gtk+-2.0:\t", -1, bold_tag, NULL);
	temp = g_strdup_printf("%i.%i.%i\n", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
	gtk_text_buffer_insert(buffer, &iter, temp, -1);
	g_free(temp);

	gtk_text_buffer_insert_with_tags(buffer, &iter, "Runtime Gtk+-2.0:\t", -1, bold_tag, NULL);
	temp = g_strdup_printf("%i.%i.%i\n", gtk_major_version, gtk_minor_version, gtk_micro_version);
	gtk_text_buffer_insert(buffer, &iter, temp, -1);
	g_free(temp);

	/* glib-2.0 */
	gtk_text_buffer_insert_with_tags(buffer, &iter, "Glib-2.0:\t", -1, bold_tag, NULL);
	temp = g_strdup_printf("%i.%i.%i\n", GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
	gtk_text_buffer_insert(buffer, &iter, temp, -1);
	g_free(temp);

	/* glib-2.0 */
	gtk_text_buffer_insert_with_tags(buffer, &iter, "Runtime sqlite3:\t", -1, bold_tag, NULL);
	gtk_text_buffer_insert(buffer, &iter, sqlite3_libversion(), -1);

	gtk_text_buffer_insert_with_tags(buffer, &iter, "\nCompile time sqlite3:\t", -1, bold_tag, NULL);
	gtk_text_buffer_insert(buffer, &iter, SQLITE_VERSION, -1);

#ifdef HAVE_UNIQUE
	gtk_text_buffer_insert_with_tags(buffer, &iter, "\nLibunique:\t", -1, bold_tag, NULL);
	temp = g_strdup_printf("%s\n", UNIQUE_VERSION_S);
	gtk_text_buffer_insert(buffer, &iter, temp, -1);
	g_free(temp);
#endif	
	/* platform */
	gtk_text_buffer_insert_with_tags(buffer, &iter, "\nPlatform:\t", -1, bold_tag, NULL);
#ifdef WIN32
	gtk_text_buffer_insert(buffer, &iter, "Windows\n", -1);
	gtk_text_buffer_insert_with_tags(buffer, &iter, "Windows version:\t", -1, bold_tag, NULL);
	temp = g_strdup_printf("%i\n", g_win32_get_windows_version());
	gtk_text_buffer_insert(buffer, &iter, temp, -1);
	g_free(temp);
#else
#ifdef OSX
	gtk_text_buffer_insert(buffer, &iter, "Mac OsX\n", -1);
#else
	gtk_text_buffer_insert(buffer, &iter, "*nix\n", -1);
#endif
#endif

	/** compile flags*/
	gtk_text_buffer_insert_with_tags(buffer, &iter, "\nCompile flags:\n", -1, bold_tag, larger_tag, NULL);

	gtk_text_buffer_insert_with_tags(buffer, &iter, "\nNLS Support:\t", -1, bold_tag, NULL);
#ifdef ENABLE_NLS
	gtk_text_buffer_insert(buffer, &iter, "Enabled", -1);
#else
	gtk_text_buffer_insert(buffer, &iter, "Disabled", -1);
#endif

	gtk_text_buffer_insert_with_tags(buffer, &iter, "\nMultimedia Keys:\t", -1, bold_tag, NULL);
#ifdef ENABLE_MMKEYS
	gtk_text_buffer_insert(buffer, &iter, "Enabled", -1);
#else
	gtk_text_buffer_insert(buffer, &iter, "Disabled", -1);
#endif

    gtk_text_buffer_insert_with_tags(buffer, &iter, "\nAppIndicator Support:\t", -1, bold_tag, NULL);
#ifdef HAVE_APP_INDICATOR
    gtk_text_buffer_insert(buffer, &iter, "Enabled", -1);
#else
    gtk_text_buffer_insert(buffer, &iter, "Disabled", -1);
#endif

	gtk_text_buffer_insert_with_tags(buffer, &iter, "\nLibspiff support:\t", -1, bold_tag, NULL);
#ifdef SPIFF
	gtk_text_buffer_insert(buffer, &iter, "Enabled", -1);
#else
	gtk_text_buffer_insert(buffer, &iter, "Disabled", -1);
#endif

	gtk_text_buffer_insert_with_tags(buffer, &iter, "\nLibunique support:\t", -1, bold_tag, NULL);
#ifdef HAVE_UNIQUE
	gtk_text_buffer_insert(buffer, &iter, "Enabled", -1);
#else
	gtk_text_buffer_insert(buffer, &iter, "Disabled", -1);
#endif

	/** Plugins */
	if (num_plugins > 0)
	{
		gtk_text_buffer_insert_with_tags(buffer, &iter, "\n\nExternal Plugins:\n", -1, bold_tag, larger_tag, NULL);
		for (i = 0; i < num_plugins; i++)
		{
			if (!gmpc_plugin_is_internal(plugins[i]))
			{
				const gchar *name = gmpc_plugin_get_name(plugins[i]);
				const int *version = gmpc_plugin_get_version(plugins[i]);
				if(gmpc_plugin_get_enabled(plugins[i])) {
					gtk_text_buffer_insert(buffer, &iter, "☑ ", -1);
				}else  {
					gtk_text_buffer_insert(buffer, &iter, "☐ ", -1);
				}
				gtk_text_buffer_insert_with_tags(buffer, &iter, name, -1, bold_tag, NULL);
				temp = g_strdup_printf("\t%i.%i.%i\n", version[0], version[1], version[2]);
				gtk_text_buffer_insert(buffer, &iter, temp, -1);
				g_free(temp);
			}
		}
	}
	if (num_plugins > 0)
	{
		gtk_text_buffer_insert_with_tags(buffer, &iter, "\n\nMetadata Plugins:\n", -1, bold_tag, larger_tag, NULL);
		for (i = 0; i < num_plugins; i++)
		{
			if (gmpc_plugin_is_metadata(plugins[i]))
			{
				const gchar *name = gmpc_plugin_get_name(plugins[i]);
				const int *version = gmpc_plugin_get_version(plugins[i]);
				if(gmpc_plugin_get_enabled(plugins[i])) {
					gtk_text_buffer_insert(buffer, &iter, "☑ ", -1);
				}else  {
					gtk_text_buffer_insert(buffer, &iter, "☐ ", -1);
				}
				gtk_text_buffer_insert_with_tags(buffer, &iter, name, -1, bold_tag, NULL);
				temp = g_strdup_printf("\t%i.%i.%i\n", version[0], version[1], version[2]);
				gtk_text_buffer_insert(buffer, &iter, temp, -1);
				g_free(temp);
			}
		}
	}

	if (mpd_check_connected(connection))
	{
		gchar **handlers;
		/** Plugins */
		gtk_text_buffer_insert_with_tags(buffer, &iter, "\nMusic Player Daemon:\n", -1, bold_tag, larger_tag, NULL);

		/* Version */
		gtk_text_buffer_insert_with_tags(buffer, &iter, "Version:\t", -1, bold_tag, NULL);
		temp = mpd_server_get_version(connection);
		gtk_text_buffer_insert(buffer, &iter, temp, -1);
		g_free(temp);

		/* total songs */
		gtk_text_buffer_insert_with_tags(buffer, &iter, "\nSongs:\t", -1, bold_tag, NULL);
		temp = g_strdup_printf("%i", mpd_stats_get_total_songs(connection));
		gtk_text_buffer_insert(buffer, &iter, temp, -1);
		g_free(temp);

		/* hostname */
		gtk_text_buffer_insert_with_tags(buffer, &iter, "\nHostname:\t", -1, bold_tag, NULL);
		temp = connection_get_hostname();
		gtk_text_buffer_insert(buffer, &iter, temp, -1);
		/* handlers */
		gtk_text_buffer_insert_with_tags(buffer, &iter, "\nUrl handlers:\t", -1, bold_tag, NULL);
		handlers = mpd_server_get_url_handlers(connection);
		if (handlers)
		{
			temp = g_strjoinv(",", handlers);
			g_strfreev(handlers);
			handlers = NULL;
		} else
			temp = g_strdup("N/A");
		gtk_text_buffer_insert(buffer, &iter, temp, -1);
		g_free(temp);

	}
}

/**
 * Shows window with usefull information for a bug report.
 */

void bug_information_window_new(GtkWidget * window)
{
	GtkWidget *dialog;
	GtkTextBuffer *buffer;
	GtkWidget *scrolled_window, *text_view;
	PangoTabArray *tab_array;

	/* Basic dialog with a close button */
	dialog = gtk_dialog_new_with_buttons(_("Bug information"),
										 (window) ? (GtkWindow *) playlist3_get_window() : NULL,
										 GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
										 GTK_STOCK_CLOSE, GTK_RESPONSE_YES, NULL);

	/* Set default window size */
	gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 800);
	/* The buffer that holds the "report" */
	buffer = gtk_text_buffer_new(NULL);

	bug_information_generate_message(buffer);

	/* View to show the textbuffer */
	text_view = gtk_text_view_new();
	/* setup textview */
	/* set tabarray */
	tab_array = pango_tab_array_new_with_positions(1, TRUE, PANGO_TAB_LEFT, 500);
	gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(text_view), TRUE);
	gtk_text_view_set_tabs(GTK_TEXT_VIEW(text_view), tab_array);
	pango_tab_array_free(tab_array);

	/* not editable */
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
	/* set margins */
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_view), 12);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text_view), 12);

	/* Add the text buffer */
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view), buffer);

	/* scrolled window, this allows the text view to scroll */
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	/* setup scrolled window */
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	/* add text view to scrolled_window */
	gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
	/* add scrolled_window to dialog */
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scrolled_window, TRUE, TRUE, 0);

	/* Add dialogs response handler */
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
	/* show dialog */
	gtk_widget_show_all(dialog);
	if (window == NULL)
	{
		gtk_dialog_run(GTK_DIALOG(dialog));
	}
}


/**
 * print text to information for a bug report.
 */
void bug_information_file_new(FILE *fp)
{
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	gchar *output;
	/* The buffer that holds the "report" */
	buffer = gtk_text_buffer_new(NULL);

	bug_information_generate_message(buffer);

	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	output = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
	if(output)
	{
		fprintf(fp, "%s\n", output);
     	g_free(output);
	}
	g_object_unref(buffer);
}
