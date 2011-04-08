/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/

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
#include "playlist3.h"
#include "main.h"
#include "cmd.h"
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

/* On activate */
void show_command_line_activate(GtkWidget *entry, gpointer data)
{
	const char *command  = gtk_entry_get_text(GTK_ENTRY(entry));	

	gmpc_easy_command_do_query(GMPC_EASY_COMMAND(gmpc_easy_command),
			command);
	gtk_widget_hide(entry);
	/* Clear the entry */
	gtk_entry_set_text(GTK_ENTRY(entry), "");
}

/* On escape close and backspace when empty */
gboolean show_command_line_key_press_event(
                    GtkWidget *entry,
                    GdkEventKey *event,
                    gpointer data)
{
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
		/* Only need todo this once */
		init = 1;
	}
	/* Show the entry and grab focus */
	gtk_widget_show(entry);
	gtk_widget_grab_focus(entry);
 }

