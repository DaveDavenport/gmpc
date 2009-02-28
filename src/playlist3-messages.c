/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
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
#include <time.h>
#include "main.h"
#include "playlist3.h"

extern int pl3_zoom;
static gboolean error_visible = FALSE;
static ErrorLevel last_error_level = ERROR_INFO;
guint timeout_callback = 0;
GtkListStore *message_list = NULL;
void message_window_open(void);
void message_window_destroy(GtkWidget *win);
static GIOChannel *log_file = NULL;

static const char *error_levels[3] = {
	N_("Info"),
	N_("Warning"),
	N_("Critical")
};
void playlist3_message_destroy(void)
{
	g_io_channel_flush(log_file, NULL);
	g_io_channel_unref(log_file);
}

void playlist3_message_init(void)
{
	if(!message_list)
	{
		GError *error = NULL;
		gchar *path = gmpc_get_user_path("gmpc.log");
		message_list = gtk_list_store_new(3, G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING);

		log_file = g_io_channel_new_file(path, "a", &error);
		if(error)
		{
			g_error("Failed to log file: '%s'", error->message);
		}
		q_free(path);

		g_io_channel_flush(log_file, NULL);
	}
}
void playlist3_show_error_message(const gchar *message, ErrorLevel el)
{
	gchar text[64];
	struct tm *lt;
	GtkWidget *event;
	GtkWidget *label = NULL;
	GtkTreeIter iter;
	time_t t = time(NULL);
	ErrorLevel level;
	const gchar *image_name;
	gchar *string;
	playlist3_message_init();
	gtk_list_store_prepend(message_list, &iter);
	gtk_list_store_set(message_list, &iter, 0, (gint64)t, 2, message,-1);


	lt = localtime(&t);
	strftime(text, 64,"%d/%m/%Y-%H:%M:%S", lt);

	string = g_strdup_printf("%s:%s:%s\n",text,error_levels[el], message);
	g_io_channel_write_chars(log_file, string, -1, NULL, NULL);
	q_free(string);
	g_io_channel_flush(log_file, NULL);


	level = cfg_get_single_value_as_int_with_default(config, "Default","min-error-level", ERROR_INFO);
	switch(el)
	{
		case ERROR_CRITICAL:
			image_name = GTK_STOCK_DIALOG_ERROR;
			break;
		case ERROR_WARNING:
			image_name = GTK_STOCK_DIALOG_WARNING;
			break;
		case ERROR_INFO:
		default:
			image_name = GTK_STOCK_DIALOG_INFO;
			break;
	}
	gtk_list_store_set(message_list, &iter, 1, image_name,-1);
	if(el < level)
	{
		return;
	}
	if(error_visible)
	{
		/* higher level errors are not overwritten by lower level errors */
		if(el < last_error_level)
		{
			return;
		}
		playlist3_close_error();
		if(timeout_callback)
		{
			g_source_remove(timeout_callback);
		}
		timeout_callback = 0;

	}
	/* store last level */
	last_error_level = el;
	if(pl3_xml && pl3_zoom != PLAYLIST_MINI)
	{
		GList *list, *siter; 
		label = gtk_image_new_from_stock(image_name, GTK_ICON_SIZE_BUTTON);

		event = (GtkWidget *) glade_xml_get_widget(pl3_xml, "error_hbox");

		/* right image */

		gtk_box_pack_start(GTK_BOX(event), label, FALSE, TRUE, 0);
		label = gtk_label_new("") ;
		gtk_label_set_markup(GTK_LABEL(label),message);
		gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
		gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
		gtk_box_pack_start(GTK_BOX(event), label, TRUE, TRUE, 0);



		list = gtk_container_get_children(GTK_CONTAINER(event));

		event = (GtkWidget *) glade_xml_get_widget(pl3_xml, "error_event");
		for(siter = list; siter; siter = g_list_next(siter)) {
			gtk_widget_modify_fg(GTK_WIDGET(siter->data),GTK_STATE_NORMAL, &(event->style->fg[GTK_STATE_NORMAL]));
			gtk_widget_modify_text(GTK_WIDGET(siter->data),GTK_STATE_NORMAL, &(event->style->text[GTK_STATE_NORMAL]));
		}

		//event = (GtkWidget *) glade_xml_get_widget(pl3_xml, "error_event");
		gtk_widget_show_all(event);
		/* Error */
		error_visible = TRUE;
		timeout_callback = g_timeout_add_seconds(5, (GSourceFunc)playlist3_close_error, NULL);
	}else{
		error_visible = FALSE;
	}
}
/* Indicates if a widget is allready added */
static gboolean widget_added = FALSE;

void playlist3_error_add_widget(GtkWidget *widget)
{
	GtkWidget *event = (GtkWidget *) glade_xml_get_widget(pl3_xml, "error_hbox");
	/* Avoid adding more then one widget */
	if(widget_added) return;
	widget_added = TRUE;

	gtk_box_pack_end(GTK_BOX(event), widget, FALSE, TRUE, 0);
	gtk_widget_show_all(event);
}

gboolean playlist3_close_error(void)
{
	/* reset */
	widget_added = FALSE;
	if(error_visible)
	{
		error_visible = FALSE;
		g_source_remove(timeout_callback);


		if(pl3_xml)
		{
			GtkWidget *event = (GtkWidget *) glade_xml_get_widget(pl3_xml, "error_event");
			gtk_widget_hide(event);
			event = (GtkWidget *) glade_xml_get_widget(pl3_xml, "error_hbox");
			gtk_container_foreach(GTK_CONTAINER(event), (GtkCallback)(gtk_widget_destroy), NULL);
		}
	}
	timeout_callback = 0;
	return FALSE;
}
static void message_cell_data_func(GtkTreeViewColumn *tree_column,
		GtkCellRenderer *cell,
		GtkTreeModel *tree_model,
		GtkTreeIter *iter,
		gpointer data)
{
	time_t t;
	gint64 id;
	gchar text[64];
	struct tm *lt;
	gtk_tree_model_get(tree_model, iter, 0,&id, -1);
	/* gtk_list_store only knows the type int64, not time_T
	 * so lets do some casting)
	 */
	t = (time_t) id;
	lt = localtime(&t);
	strftime(text, 64,"%H:%M:%S", lt);
	g_object_set(G_OBJECT(cell), "text",text,NULL);
}
static GtkBuilder *message_xml = NULL;
void message_window_open(void)
{
	GtkWidget *win,*pl3_win = playlist3_get_window(); 
	GtkBuilder *xml;
	GtkCellRenderer *renderer;
	GtkWidget *tree;
	gchar *path;
	path = gmpc_get_full_glade_path("playlist-message-window.ui");
	message_xml = xml = gtk_builder_new();
	gtk_builder_add_from_file(xml, path, NULL);
	q_free(path);
	playlist3_message_init();

	/* set transient */
	win = (GtkWidget *) gtk_builder_get_object(xml, "message_window");
	gtk_window_set_transient_for(GTK_WINDOW(win), GTK_WINDOW(pl3_win));
	gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER_ON_PARENT);

	tree= (GtkWidget *) gtk_builder_get_object(xml, "message_tree");
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1,"", renderer, "stock-id", 1,NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(tree), -1,_("Time"), renderer,(GtkTreeCellDataFunc)message_cell_data_func, NULL,NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1,_("Message"), renderer, "markup", 2,NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(message_list));

	gtk_widget_show(win);
	//glade_xml_signal_autoconnect(xml);
	gtk_builder_connect_signals(xml, NULL);
}

void message_window_destroy(GtkWidget *win)
{

	gtk_widget_destroy(win);
	g_object_unref(message_xml);
	message_xml = NULL;
}

/* vim: noexpandtab ts=4 sw=4 sts=4 tw=120: */
