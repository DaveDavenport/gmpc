/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2010 Qball Cow <qball@sarine.nl>
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

struct _Playlist3MessagePlugin
{
        GmpcPluginBase  parent_instance;
        Playlist3MessagePluginPrivate *priv;
};

struct _Playlist3MessagePluginClass
{
        GmpcPluginBaseClass parent_class;
};
static const char *error_levels[3] = {
	N_("Info"),
	N_("Warning"),
	N_("Critical")
};

/**
 * Private data structure 
 */
typedef struct _Playlist3MessagePluginPrivate {
	gboolean error_visible;
	ErrorLevel last_error_level;
	guint timeout_callback;
	GtkListStore *message_list;
	GIOChannel *log_file;
} _Playlist3MessagePluginPrivate;


void copy_to_clipboard(GtkButton *button, GtkBuilder *xml);

static void playlist3_message_destroy(Playlist3MessagePlugin *self)
{
	g_io_channel_flush(self->priv->log_file, NULL);
	g_io_channel_unref(self->priv->log_file);
}

static void playlist3_message_init(Playlist3MessagePlugin *self)
{
	if(!self->priv->message_list)
	{
		GError *error = NULL;
		gchar *path = gmpc_get_user_path("gmpc.log");
		self->priv->message_list = gtk_list_store_new(3, G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING);

		self->priv->log_file = g_io_channel_new_file(path, "a", &error);
		if(error)
		{
			g_error("Failed to log file: '%s'", error->message);
		}
		q_free(path);

		g_io_channel_flush(self->priv->log_file, NULL);
	}
}
void playlist3_message_show(Playlist3MessagePlugin *self, const gchar *message, ErrorLevel el)
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
	playlist3_message_init(self);
	gtk_list_store_prepend(self->priv->message_list, &iter);
	gtk_list_store_set(self->priv->message_list, &iter, 0, (gint64)t, 2, message,-1);


	lt = localtime(&t);
	strftime(text, 64,"%d/%m/%Y-%H:%M:%S", lt);

	string = g_strdup_printf("%s:%s:%s\n",text,error_levels[el], message);
	g_io_channel_write_chars(self->priv->log_file, string, -1, NULL, NULL);
	q_free(string);
	g_io_channel_flush(self->priv->log_file, NULL);


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
	gtk_list_store_set(self->priv->message_list, &iter, 1, image_name,-1);
	if(el < level)
	{
		return;
	}
	if(self->priv->error_visible)
	{
		/* higher level errors are not overwritten by lower level errors */
		if(el < self->priv->last_error_level)
		{
			return;
		}
		playlist3_close_error();
		if(self->priv->timeout_callback)
		{
			g_source_remove(self->priv->timeout_callback);
		}
		self->priv->timeout_callback = 0;

	}
	/* store last level */
	self->priv->last_error_level = el;
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
		if(list) g_list_free(list);

		gtk_widget_show_all(event);
		/* Error */
		self->priv->error_visible = TRUE;
		self->priv->timeout_callback = g_timeout_add_seconds(5, (GSourceFunc)playlist3_message_close, self);
	}else{
		self->priv->error_visible = FALSE;
	}
}
/* Indicates if a widget is allready added */
static gboolean widget_added = FALSE;

void playlist3_message_add_widget(Playlist3MessagePlugin *self, GtkWidget *widget)
{
	GtkWidget *event = (GtkWidget *) glade_xml_get_widget(pl3_xml, "error_hbox");
	/* Avoid adding more then one widget */
	if(widget_added) return;
	widget_added = TRUE;

	gtk_box_pack_end(GTK_BOX(event), widget, FALSE, TRUE, 0);
	gtk_widget_show_all(event);
}

gboolean playlist3_message_close(Playlist3MessagePlugin *self)
{
	/* reset */
	widget_added = FALSE;
	if(self->priv->error_visible)
	{
		self->priv->error_visible = FALSE;
		g_source_remove(self->priv->timeout_callback);


		if(pl3_xml)
		{
			GtkWidget *event = (GtkWidget *) glade_xml_get_widget(pl3_xml, "error_event");
			gtk_widget_hide(event);
			event = (GtkWidget *) glade_xml_get_widget(pl3_xml, "error_hbox");
			gtk_container_foreach(GTK_CONTAINER(event), (GtkCallback)(gtk_widget_destroy), NULL);
		}
	}
	self->priv->timeout_callback = 0;
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
/**
 * The list of messages
 */

void message_window_destroy(GtkWidget *win,GdkEvent *event, GtkBuilder *message_xml);
void message_window_destroy(GtkWidget *win,GdkEvent *event, GtkBuilder *message_xml)
{
	gtk_widget_destroy(win);
	g_object_unref(message_xml);
	message_xml = NULL;
}
void copy_to_clipboard(GtkButton *button, GtkBuilder *xml)
{
	GtkWidget *tree= (GtkWidget *) gtk_builder_get_object(xml, "message_tree");
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	GtkClipboard *gcb = NULL;
	GList *list = NULL, *liter = NULL;
	GString *str = g_string_new("");
	printf("Copy to clipboard\n");
	if(gtk_tree_selection_count_selected_rows(selection) == 0)
	{
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter_first(model, &iter)) {
			do{
				GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
				list = g_list_prepend(list, path);
			}while(gtk_tree_model_iter_next(model, &iter));
		}	
		list = g_list_reverse(list);
	}else{
			list = gtk_tree_selection_get_selected_rows(selection, NULL);
	}
	for(liter = g_list_first(list); liter; liter = g_list_next(liter))
	{
		gchar *message = NULL;
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(model, &iter, liter->data))
		{
			gtk_tree_model_get(model,&iter,2, &message, -1);
			str = g_string_append(str, message);
			str = g_string_append(str, "\n");
			g_free(message);
		}
	}

	gcb = gtk_widget_get_clipboard(GTK_WIDGET(button), GDK_SELECTION_CLIPBOARD);
	printf("Set clipboard: %s\n", str->str);
	gtk_clipboard_set_text(gcb, str->str, str->len);
	

	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (list);
	g_string_free(str, TRUE);
}

static void playlist3_message_window_open(Playlist3MessagePlugin *self)
{
	GtkBuilder *message_xml = NULL;
	GtkWidget *win,*pl3_win = playlist3_get_window(); 
	GtkBuilder *xml;
	GtkCellRenderer *renderer;
	GtkWidget *tree;
	gchar *path;
	path = gmpc_get_full_glade_path("playlist-message-window.ui");
	message_xml = xml = gtk_builder_new();
	gtk_builder_add_from_file(xml, path, NULL);
	q_free(path);
	playlist3_message_init(self);

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
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(self->priv->message_list));

	gtk_widget_show(win);

	gtk_builder_connect_signals(xml, xml);
}


/**
 * Turn this into a plugin. This is a test implementation. 
 * This is not usefull-as-is
 */
static void playlist3_message_plugin_class_init (Playlist3MessagePluginClass *klass);
GType playlist3_message_plugin_get_type(void);

static int *playlist3_message_plugin_get_version(GmpcPluginBase *plug, int *length)
{
	static int version[3] = {0,0,1};
	if(length) *length = 3;
	return (int *)version;
}

static const char *playlist3_message_plugin_get_name(GmpcPluginBase *plug)
{
	return "Playlist3 Messages";
}

static void playlist3_message_plugin_finalize(GObject *obj) {
	Playlist3MessagePluginClass * klass = (g_type_class_peek (playlist3_message_plugin_get_type()));
	gpointer parent_class = g_type_class_peek_parent (klass);
	playlist3_message_destroy((Playlist3MessagePlugin *)obj);

	if(((Playlist3MessagePlugin *)obj)->priv){
		Playlist3MessagePluginPrivate *priv = ((Playlist3MessagePlugin *)obj)->priv;
		if(priv->message_list)
			g_object_unref(priv->message_list);
		g_free(priv);
		((Playlist3MessagePlugin *)obj)->priv = NULL;
	}
	if(parent_class)
		G_OBJECT_CLASS(parent_class)->finalize(obj);
}
static GObject *playlist3_message_plugin_constructor(GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	Playlist3MessagePluginClass * klass;
	Playlist3MessagePlugin *self;
	GObjectClass * parent_class;
	klass = (g_type_class_peek (playlist3_message_plugin_get_type()));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	self = (Playlist3MessagePlugin *) parent_class->constructor (type, n_construct_properties, construct_properties);

	/* setup private structure */
	self->priv = g_malloc0(sizeof(Playlist3MessagePluginPrivate));
	self->priv->error_visible = FALSE;
	self->priv->last_error_level = ERROR_INFO;
	self->priv->timeout_callback = 0;
	self->priv->message_list = NULL;
	self->priv->log_file = NULL;

	/* Make it an internal plugin */
	GMPC_PLUGIN_BASE(self)->plugin_type = GMPC_INTERNALL;
	playlist3_message_init(self);

	return G_OBJECT(self);
}

static void playlist3_message_plugin_class_init (Playlist3MessagePluginClass *klass)
{
	/* Connect destroy and construct */
	G_OBJECT_CLASS(klass)->finalize =		playlist3_message_plugin_finalize;
	G_OBJECT_CLASS(klass)->constructor =	playlist3_message_plugin_constructor;
	
	/* Connect plugin functions */
	GMPC_PLUGIN_BASE_CLASS(klass)->get_version = playlist3_message_plugin_get_version;
	GMPC_PLUGIN_BASE_CLASS(klass)->get_name =	 playlist3_message_plugin_get_name;

}
GType playlist3_message_plugin_get_type(void) {
	static GType playlist3_message_plugin_type_id = 0;
	if(playlist3_message_plugin_type_id == 0) {
		static const GTypeInfo info = {
			.class_size = sizeof(Playlist3MessagePluginClass),
			.class_init = (GClassInitFunc)playlist3_message_plugin_class_init,
			.instance_size = sizeof(Playlist3MessagePlugin),
			.n_preallocs = 0
		};
		playlist3_message_plugin_type_id = g_type_register_static(GMPC_PLUGIN_TYPE_BASE, "Playlist3MessagesPlugin", &info, 0);
	}
	return playlist3_message_plugin_type_id;
}

Playlist3MessagePlugin * playlist3_message_plugin_new(void) {
	return g_object_newv(playlist3_message_plugin_get_type(), 0, NULL);
}

/**
 * Old compatibility functions
 */
void playlist3_show_error_message(const gchar *message, ErrorLevel el)
{
	playlist3_message_show(pl3_messages, message, el);
}

void playlist3_close_error(void)
{
	playlist3_message_close(pl3_messages);
}

void playlist3_error_add_widget(GtkWidget *widget)
{
	playlist3_message_add_widget(pl3_messages, widget);
}

void message_window_open(void)
{
	playlist3_message_window_open(pl3_messages);
}
/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
