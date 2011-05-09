/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@gmpclient.org>
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
#include <glib/gstdio.h>
#include "playlist3.h"
#include "main.h"
#include "cmd.h"


#define LOG_DOMAIN "GUI.cmd"
/**
 * Easy command interface 
 */

/* icon release */
void show_command_line_icon_release(
            GtkWidget *entry,
            GtkEntryIconPosition pos,
            GdkEvent *event,
            gpointer data)
{

	/* The clear icon */
	if ( pos == GTK_ENTRY_ICON_SECONDARY )
	{
		/* clear and hide */
		gtk_widget_hide(entry);
		gtk_entry_set_text(GTK_ENTRY(entry), "");
	}
	/* Primary */
	else if ( pos == GTK_ENTRY_ICON_PRIMARY )
	{
		/* open the help window */
		gmpc_easy_command_help_window(gmpc_easy_command, NULL);

	}
}


/* List<string> history */
static GList *history = NULL;

/* weak List<string> current */
static GList *current = NULL;
static GList *last = NULL;
static gint history_length = 0;
static gchar *entry_current = NULL;

static void show_command_line_history_destroy(void)
{
	gchar *path = gmpc_get_user_path("cmd-history.txt");
	/* Store results */
	FILE *fp = g_fopen(path,"w");
	if(fp != NULL)
	{
		GList *first = g_list_first(history);
		for(;first != NULL; first = g_list_next(first))
		{
			fputs(first->data, fp);
			fputc('\n', fp);
		}

		fclose(fp);
	}
	g_free(path);
	current = NULL;
    last = NULL;
    g_list_foreach(history, (GFunc)g_free, NULL);
    g_list_free(history);
    history = NULL;
    g_free(entry_current);
    entry_current = NULL;
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Cleanup Command Line history");
}

/* On activate */
void show_command_line_activate(GtkWidget *entry, gpointer data)
{
	const char *command  = gtk_entry_get_text(GTK_ENTRY(entry));	

	gmpc_easy_command_do_query(GMPC_EASY_COMMAND(gmpc_easy_command),
			command);

	if(last == NULL || g_utf8_collate(last->data, command) != 0) 
	{
		history = g_list_prepend(history, g_strdup(command));
		/* if no last, current is last */ 
		if(last == NULL) {
			last = history;
		}
		history_length++;
	}
		/* Clear the current selected entry */
    current = NULL;
    if(history_length > 100) {
        GList *temp;
        temp = g_list_previous(last);;
        g_free(last->data);
        history = g_list_delete_link(history, last);
        last = temp;
        history_length--;
    }

	gtk_widget_hide(entry);
	/* Clear the entry */
	gtk_entry_set_text(GTK_ENTRY(entry), "");

}

void show_command_line_entry_changed( 
                    GtkWidget *entry,
                    gpointer data)
{
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

    if(current != NULL && g_utf8_collate(text, current->data) == 0) return;


    if(entry_current) g_free(entry_current);
    entry_current = NULL;
    if(text) entry_current = g_strdup(text);
    current = NULL;
}

/* On escape close and backspace when empty */
gboolean show_command_line_key_press_event(
                    GtkWidget *entry,
                    GdkEventKey *event,
                    gpointer data)
{

    /* Go back in history */
    if(event->keyval == GDK_Up)
    {
        if(current == NULL) current = g_list_first(history);
        else if(current->next != NULL) current = g_list_next(current);

        if(current != NULL) {
            gtk_entry_set_text(GTK_ENTRY(entry), (char *)(current->data));
        }
        return TRUE;
    }
    /* Go forward in history */
    else if (event->keyval == GDK_Down)
    {
        if(current != NULL) {
            current = g_list_previous(current);

            if(current) {
                gtk_entry_set_text(GTK_ENTRY(entry), (char *)(current->data));
            }else if (entry_current) {
                gtk_entry_set_text(GTK_ENTRY(entry), entry_current); 
            }else{
                /* Fallback */
                gtk_entry_set_text(GTK_ENTRY(entry),""); 
            }
        }
        return TRUE;
    }
    else
	if(event->keyval == GDK_Escape)
	{
		gtk_widget_hide(entry);
		/* Clear the entry */
		gtk_entry_set_text(GTK_ENTRY(entry), "");
		return TRUE;
	}
	if(gtk_entry_get_text_length(GTK_ENTRY(entry)) == 0 && 
	    event->keyval == GDK_BackSpace) 
	{
		gtk_widget_hide(entry);
		return TRUE;
	}

	return FALSE;
}

 /* Call from the menu */
void show_command_line(void)
 {
	static int init = 1;
	GtkWidget *entry =  GTK_WIDGET(gtk_builder_get_object(
	                                pl3_xml, 
	                                "special_command_entry"));

	/* Tell gcc this is not likely to happen (only once) */
	if(G_UNLIKELY(init == 1))
	{
		GtkTreeModel		*model;
		GtkCellRenderer		*ren;
		GtkEntryCompletion	*comp;
		FILE				*fp;
		gchar 				*path;
		/**
		 * Setup completion on the entry box.
		 * Completion should match what is done in the Easycommand popup 
		 */

		/* steal pointer to model from easycommand */
		model = (GtkTreeModel *)gmpc_easy_command->store;

		/* Create completion */
		comp = gtk_entry_completion_new();

		/* Attach model to completion */	
		gtk_entry_completion_set_model(comp, model);
		/* Column 1 holds the 'to match' text */
		gtk_entry_completion_set_text_column(comp, 1);
		/* Enable all features that might be usefull to the user */
		gtk_entry_completion_set_inline_completion(comp, TRUE);
		gtk_entry_completion_set_inline_selection(comp, TRUE);
		gtk_entry_completion_set_popup_completion(comp, TRUE);
		/* Use the match function from GmpcEasyCommand */
		gtk_entry_completion_set_match_func(comp, 
				(GtkEntryCompletionMatchFunc)gmpc_easy_command_completion_function, 
				g_object_ref(gmpc_easy_command),
				g_object_unref);


		/* setup looks */
		/* Icon */
		ren = gtk_cell_renderer_pixbuf_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(comp), ren, FALSE);
		gtk_cell_layout_reorder(GTK_CELL_LAYOUT(comp),ren, 0);
		/* Support both icon-name and stock-id. */
		gtk_cell_layout_add_attribute(
		                GTK_CELL_LAYOUT(comp),
		                ren,
		                "icon-name", 6);
		gtk_cell_layout_add_attribute(
		                GTK_CELL_LAYOUT(comp),
		                ren,
		                "stock-id", 7);
		/* hint */
		ren = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(comp), ren, FALSE);
		gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(comp),ren, "text", 5);
		g_object_set(G_OBJECT(ren), "foreground", "grey", NULL);

		/* Add completion to the entry */
		gtk_entry_set_completion(GTK_ENTRY(entry),comp); 

        /* Make sure the history gets cleaned on destroy */
        g_signal_connect(G_OBJECT(entry), 
                        "destroy", 
                        G_CALLBACK(show_command_line_history_destroy),
                        NULL);



		/* Store results */
		path = gmpc_get_user_path("cmd-history.txt");
		fp = g_fopen(path,"r");
		if(fp != NULL)
		{
			char buffer[512];
			while(fgets(buffer, 512, fp) != NULL) 
			{
				if(buffer[strlen(buffer)-1] == '\n') buffer[strlen(buffer)-1] = '\0';
				if(buffer[0] != '\0')
					history = g_list_prepend(history, g_strdup(buffer));
			}
			history = g_list_reverse(history);
			last = history;
			fclose(fp);
		}
		g_free(path);
		/* Only need todo this once */
		init = 0;
	}
	/* Show the entry and grab focus */
	gtk_widget_show(entry);
	gtk_widget_grab_focus(entry);
 }

