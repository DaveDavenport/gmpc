/* gmpc-serverstats (GMPC plugin)
 * Copyright (C) 2006-2012 Qball Cow <qball@gmpclient.org>
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
#include <gdk/gdkkeysyms.h>
#include <config.h>
#include "main.h"
#include "plugin.h"
#include "playlist3-messages.h"
#include "playlist3.h"
#include "misc.h"

gmpcPlugin statistics_plugin;
enum
{
	SERVERSTATS_MPD_VERSION,
	SERVERSTATS_MPD_UPTIME,
	SERVERSTATS_MPD_PLAYTIME,
	SERVERSTATS_MPD_DB_PLAYTIME,
	SERVERSTATS_MPD_DB_ARTISTS,
	SERVERSTATS_MPD_DB_ALBUMS,
	SERVERSTATS_MPD_DB_SONGS,
	SERVERSTATS_MPD_URLHANDLERS,
	SERVERSTATS_MPD_TAG_TYPES,
	SERVERSTATS_NUM_FIELDS
};

static GtkWidget *serverstats_sw = NULL, *serverstats_tree = NULL;
static GtkWidget *serverstats_labels[SERVERSTATS_NUM_FIELDS];
static gboolean cancel_query = FALSE;

/**
 * Playlist browser functions 
 */

static void serverstats_clear(void)
{
	int i;
	for (i = 0; i < SERVERSTATS_NUM_FIELDS; i++)
	{
		gtk_label_set_text(GTK_LABEL(serverstats_labels[i]), "");
	}

}

static void serverstats_update(void)
{
	gchar **handlers = NULL;
	gchar *value = NULL;

	serverstats_clear();
	if (!mpd_check_connected(connection))
		return;
	mpd_stats_update(connection);
	/** Version */
	value = mpd_server_get_version(connection);
	gtk_label_set_text(GTK_LABEL(serverstats_labels[SERVERSTATS_MPD_VERSION]), value);
	free(value);
	/** Uptime  */
	value = format_time_real(mpd_stats_get_uptime(connection), "");
	gtk_label_set_text(GTK_LABEL(serverstats_labels[SERVERSTATS_MPD_UPTIME]), value);
	g_free(value);
	/** Playtime*/
	value = format_time_real(mpd_stats_get_playtime(connection), "");
	gtk_label_set_text(GTK_LABEL(serverstats_labels[SERVERSTATS_MPD_PLAYTIME]), value);
	g_free(value);
	/** DB Playtime*/
	value = format_time_real(mpd_stats_get_db_playtime(connection), "");
	gtk_label_set_text(GTK_LABEL(serverstats_labels[SERVERSTATS_MPD_DB_PLAYTIME]), value);
	g_free(value);
	/** DB ARTIST*/
	value = g_strdup_printf("%i", mpd_stats_get_total_artists(connection));
	gtk_label_set_text(GTK_LABEL(serverstats_labels[SERVERSTATS_MPD_DB_ARTISTS]), value);
	g_free(value);
	/** DB ALBUMS*/
	value = g_strdup_printf("%i", mpd_stats_get_total_albums(connection));
	gtk_label_set_text(GTK_LABEL(serverstats_labels[SERVERSTATS_MPD_DB_ALBUMS]), value);
	g_free(value);
	/** DB SONGS*/
	value = g_strdup_printf("%i", mpd_stats_get_total_songs(connection));
	gtk_label_set_text(GTK_LABEL(serverstats_labels[SERVERSTATS_MPD_DB_SONGS]), value);
	g_free(value);
	/** URL_HANDLERS*/
	handlers = mpd_server_get_url_handlers(connection);
	if (handlers)
	{
		value = g_strjoinv(",", handlers);
		g_strfreev(handlers);
		handlers = NULL;
	} else
		value = g_strdup("N/A");
	gtk_label_set_text(GTK_LABEL(serverstats_labels[SERVERSTATS_MPD_URLHANDLERS]), value);
	g_free(value);

	if (mpd_server_check_version(connection, 0, 13, 0))
		handlers = mpd_server_get_tag_types(connection);
	if (handlers)
	{
		value = g_strjoinv(", ", handlers);
		g_strfreev(handlers);
		handlers = NULL;
	} else
		value = g_strdup("N/A");
	gtk_label_set_text(GTK_LABEL(serverstats_labels[SERVERSTATS_MPD_TAG_TYPES]), value);
	g_free(value);

}

typedef struct _ss_str
{
	int total;
	int tag;
	int hits;
	MpdData *data;
	GtkTreeModel *model;
	GtkWidget *box, *pb;
	long unsigned max_i;
} ss_str;

static gboolean serverstats_idle_handler(ss_str * s)
{
	GtkTreeIter iter;
	MpdDBStats *stats = NULL;
	if (s->data == NULL || !mpd_check_connected(connection) || cancel_query)
	{

		if (gtk_tree_model_get_iter_first(s->model, &iter))
		{
			do
			{
				guint d;
				gulong i;
				gchar *value = NULL;
				gtk_tree_model_get(s->model, &iter, 0, &i, -1);
				d = (guint) 100 *(i / (double)s->max_i);
				value = format_time_real(i, "");
				gtk_list_store_set(GTK_LIST_STORE(s->model), &iter, 2, d, 3, value, -1);
				g_free(value);
			} while (gtk_tree_model_iter_next(s->model, &iter));
		}

		if (s->data)
			mpd_data_free(s->data);
		gtk_tree_view_set_model(GTK_TREE_VIEW(serverstats_tree), s->model);
		gtk_tree_view_set_search_column(GTK_TREE_VIEW(serverstats_tree), 1);
		gtk_widget_set_sensitive(GTK_WIDGET(s->box), TRUE);
		gtk_widget_hide(gtk_widget_get_parent(s->pb));

		if (cancel_query)
			gtk_list_store_clear(GTK_LIST_STORE(s->model));
		g_free(s);
		cancel_query = FALSE;
		return FALSE;
	}
	mpd_database_search_stats_start(connection);
	mpd_database_search_add_constraint(connection, s->tag, s->data->tag);

	stats = mpd_database_search_stats_commit(connection);
	if (stats)
	{
		gtk_list_store_prepend(GTK_LIST_STORE(s->model), &iter);
		gtk_list_store_set(GTK_LIST_STORE(s->model), &iter, 0, (unsigned long)(stats->playTime), 1, s->data->tag, -1);
		s->max_i = MAX(s->max_i, stats->playTime);

		mpd_database_search_free_stats(stats);
	}
	/* limit the amount of updating to 0.2 % */
	if ((int)((1000 * s->hits) / s->total) % 5 == 0)
	{
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(s->pb), s->hits / (double)s->total);
	}
	s->hits++;
	s->data = mpd_data_get_next(s->data);
	return TRUE;
}

static void serverstats_combo_changed(GtkComboBox * box, GtkWidget * pb)
{
	ss_str *s;
	int hits, total;
	gulong max_i;
	MpdData *node, *data;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(serverstats_tree));
	int tag = gtk_combo_box_get_active(box);

	if (!mpd_check_connected(connection))
		return;
	if (!mpd_server_check_version(connection, 0, 13, 0))
	{
		playlist3_show_error_message("This feature is not supported in mpd older then version 0.13.0.", ERROR_WARNING);
		return;
	}
	/* reset the cancel flag */
	cancel_query = FALSE;
	/* show progress bar */

	gtk_widget_show_all(gtk_widget_get_parent(pb));

	/** make the combo box insensitive and remove the model from the treeview */
	gtk_tree_view_set_model(GTK_TREE_VIEW(serverstats_tree), NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(box), FALSE);

	gtk_list_store_clear(GTK_LIST_STORE(model));
	mpd_database_search_field_start(connection, tag);
	data = mpd_database_search_commit(connection);
	max_i = 0;

	hits = 0;
	total = 0;
	for (node = mpd_data_get_first(data); node != NULL; node = (MpdData *) mpd_data_get_next_real(node, FALSE))
		total++;
	s = g_malloc0(sizeof(*s));
	s->total = total;
	s->model = model;
	s->data = data;
	s->hits = 0;
	s->tag = tag;
	s->pb = pb;
	s->box = GTK_WIDGET(box);
	g_idle_add((GSourceFunc) serverstats_idle_handler, s);

}

static void serverstats_header_style_changed(GtkWidget * vbox, GtkStyle * style, GtkWidget * vp)
{
	gtk_widget_modify_bg(vp, GTK_STATE_NORMAL, &(GTK_WIDGET(vbox)->style->base[GTK_STATE_NORMAL]));
}

static void cancel_clicked(GtkWidget * cancel, gpointer data)
{
	cancel_query = TRUE;
}

static void serverstats_add_entry(GtkWidget * table, int i, const char *name, int stats)
{
	char *markup;
	GtkWidget *label;
	/** Mpd Uptime */
	label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.0);
	gtk_misc_set_padding(GTK_MISC(label), 12, 0);
	markup = g_markup_printf_escaped("<b>%s:</b>", name);
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, i, i + 1, GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	label = serverstats_labels[stats] = gtk_label_new("");
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 1, 2, i, i + 1, GTK_EXPAND | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
}

static void serverstats_init(void)
{
	/** Get an allready exposed widgets to grab theme colors from. */
	GtkWidget *colw = (GtkWidget *) playlist3_get_category_tree_view();
	GtkWidget *label = NULL;
	GtkWidget *table = NULL;
	GtkWidget *hbox = NULL;
	GtkWidget *event = NULL;
	GtkWidget *serverstats_vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *serverstats_event;
	gchar *markup = NULL;
	int i = 0;

	serverstats_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(serverstats_sw), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(serverstats_sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	serverstats_event = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(serverstats_event), TRUE);
	gtk_container_add(GTK_CONTAINER(serverstats_event), serverstats_vbox);

	/* wrap in event box to set bg color */
	event = gtk_event_box_new();
	gtk_widget_set_app_paintable(event, TRUE);
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(event), TRUE);
	gtk_widget_set_state(GTK_WIDGET(event), GTK_STATE_SELECTED);
	g_signal_connect(G_OBJECT(event), "expose-event", G_CALLBACK(misc_header_expose_event), NULL);

	gtk_widget_modify_bg(serverstats_event, GTK_STATE_NORMAL, &(colw->style->base[GTK_STATE_NORMAL]));
	g_signal_connect(G_OBJECT(serverstats_vbox), "style-set", G_CALLBACK(serverstats_header_style_changed),
					 serverstats_event);

	/* set label and padding */
	hbox = gtk_hbox_new(FALSE, 6);
	label = gtk_image_new_from_icon_name("mpd", GTK_ICON_SIZE_DND);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	markup = g_markup_printf_escaped("<span size='xx-large' weight='bold'>%s</span>", _("Server Information"));
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(event), hbox);

	gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
	gtk_box_pack_start(GTK_BOX(serverstats_vbox), event, FALSE, TRUE, 0);
	gtk_widget_show_all(event);

	label = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(serverstats_vbox), label, FALSE, TRUE, 0);
	gtk_widget_show(label);
	/**
     * Data list 
     */
	table = gtk_table_new(SERVERSTATS_NUM_FIELDS + 2, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_container_set_border_width(GTK_CONTAINER(table), 12);

	/** Database */
	label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	markup = g_markup_printf_escaped("<span size='x-large' weight='bold'>%s</span>", _("Server"));
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);
	gtk_table_attach(GTK_TABLE(table), label, 0, 2, 0, 1, GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

	i = 1;
	/** Mpd version */
	serverstats_add_entry(table, i++, _("Version"), SERVERSTATS_MPD_VERSION);

	serverstats_add_entry(table, i++, _("Uptime"), SERVERSTATS_MPD_UPTIME);
	serverstats_add_entry(table, i++, _("Time Playing"), SERVERSTATS_MPD_PLAYTIME);

	/** Database */
	label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	markup = g_markup_printf_escaped("<span size='x-large' weight='bold'>%s</span>", _("Database"));
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);
	gtk_table_attach(GTK_TABLE(table), label, 0, 2, i, i + 1, GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	i++;

	/** Mpd Playtime */
	serverstats_add_entry(table, i++, _("Total Playtime"), SERVERSTATS_MPD_DB_PLAYTIME);
	/** Mpd Artists*/
	serverstats_add_entry(table, i++, _("Number of artists"), SERVERSTATS_MPD_DB_ARTISTS);
	/** Mpd Albums */
	serverstats_add_entry(table, i++, _("Number of albums"), SERVERSTATS_MPD_DB_ALBUMS);
	/** Mpd Songs */
	serverstats_add_entry(table, i++, _("Number of songs"), SERVERSTATS_MPD_DB_SONGS);
	/** Mpd Songs */
	serverstats_add_entry(table, i++, _("URL Handlers"), SERVERSTATS_MPD_URLHANDLERS);
	/** Mpd Songs */
	serverstats_add_entry(table, i++, _("Tag Types"), SERVERSTATS_MPD_TAG_TYPES);
	/** Stats */
	label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	markup = g_markup_printf_escaped("<span size='x-large' weight='bold'>%s</span>", _("Tag statistics"));
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);
	gtk_table_attach(GTK_TABLE(table), label, 0, 2, i, i + 1, GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	i++;
	gtk_widget_show_all(table);

	/**
     * Stats treeview
     */
	{
		GtkWidget *combo = NULL;
		GtkWidget *sw = NULL, *cancel;
		GtkListStore *store;
		GtkCellRenderer *renderer;
		GtkWidget *pb = gtk_progress_bar_new();
		combo = gtk_combo_box_new_text();
		for (i = 0; i < MPD_TAG_NUM_OF_ITEM_TYPES - 1; i++)
		{
			if (mpd_server_tag_supported(connection, i))
			{
				gtk_combo_box_append_text(GTK_COMBO_BOX(combo), mpdTagItemKeys[i]);
			}
		}

		gtk_table_attach(GTK_TABLE(table), combo, 0, 2, 12, 13, GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
		gtk_widget_show(combo);

		hbox = gtk_hbox_new(FALSE, 6);
		cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
		g_signal_connect(G_OBJECT(cancel), "clicked", G_CALLBACK(cancel_clicked), NULL);
		gtk_box_pack_start(GTK_BOX(hbox), pb, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(hbox), cancel, FALSE, TRUE, 0);

		gtk_table_attach(GTK_TABLE(table), hbox, 0, 2, 13, 14, GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
		g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(serverstats_combo_changed), pb);

		sw = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_container_set_border_width(GTK_CONTAINER(sw), 6);

		store = gtk_list_store_new(4, G_TYPE_ULONG, G_TYPE_STRING, G_TYPE_ULONG, G_TYPE_STRING);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), 0, GTK_SORT_DESCENDING);
		serverstats_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(serverstats_tree), FALSE);
		gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(serverstats_tree), TRUE);
		renderer = gtk_cell_renderer_text_new();
		g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, "width-chars", 30,
					 NULL);
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(serverstats_tree), -1, "", renderer, "text", 1, NULL);
		renderer = gtk_cell_renderer_progress_new();
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(serverstats_tree),
													-1, "", renderer, "value", 2, "text", 3, NULL);

		gtk_tree_view_set_search_column(GTK_TREE_VIEW(serverstats_tree), 1);

		gtk_container_add(GTK_CONTAINER(sw), serverstats_tree);

		gtk_table_attach(GTK_TABLE(table), sw, 0, 2, 14, 15, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
		gtk_widget_show_all(sw);
	}
	/**
     * Add table
     */
	gtk_box_pack_start(GTK_BOX(serverstats_vbox), table, TRUE, TRUE, 0);
	/* maintain my own reference to the widget, so it won't get destroyed removing 
	 * from view
	 */
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(serverstats_sw), serverstats_event);
	gtk_widget_show(serverstats_vbox);
	gtk_widget_show(serverstats_event);
	gtk_widget_show(serverstats_sw);
	g_object_ref_sink(serverstats_sw);
}

static guint timeout_source = 0;

static void serverstats_selected(GtkWidget * container)
{
	if (serverstats_sw == NULL)
	{
		serverstats_init();
	}
	serverstats_update();
	gtk_container_add(GTK_CONTAINER(container), serverstats_sw);
	gtk_widget_show(serverstats_sw);
	if (timeout_source)
		g_source_remove(timeout_source);
	timeout_source = g_timeout_add_seconds(5, (GSourceFunc) serverstats_update, NULL);
}

static void serverstats_unselected(GtkWidget * container)
{
	if (timeout_source)
		g_source_remove(timeout_source);
	timeout_source = 0;
	gtk_container_remove(GTK_CONTAINER(container), serverstats_sw);
}

/**
 * public function
 */
static void serverinformation_popup_close(GtkWidget * dialog, gint response_id, gpointer data)
{
	int width, height;
	/* Save windows size */
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	cfg_set_single_value_as_int(config, "serverstats", "dialog-width", width);
	cfg_set_single_value_as_int(config, "serverstats", "dialog-height", height);

	/* Remove info window, and keep it */
	serverstats_unselected(GTK_DIALOG(dialog)->vbox);

	/* destroy dialog */
	gtk_widget_destroy(dialog);

}

static void serverstats_connection_changed(MpdObj * mi, int connect, void *usedata)
{
	if (!connect && serverstats_tree)
	{
		GtkWidget *dialog = gtk_widget_get_parent(serverstats_sw);
		if (dialog)
			gtk_dialog_response(GTK_DIALOG(gtk_widget_get_toplevel(dialog)), 0);
		if (serverstats_sw)
		{
			gtk_widget_destroy(serverstats_sw);
			serverstats_sw = NULL;
			serverstats_tree = NULL;
		}
	}
}

void serverinformation_show_popup(void)
{
	GtkWidget *dialog = NULL;
	if (serverstats_sw)
	{
		GtkWidget *win = gtk_widget_get_parent(serverstats_sw);
		if (win)
		{
			gtk_window_present(GTK_WINDOW(win));
			return;
		}
	}

	dialog = gtk_dialog_new_with_buttons(_("Server Information"),
										 GTK_WINDOW(playlist3_get_window()),
										 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
										 GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);
	/* Add info window */
	serverstats_selected(GTK_DIALOG(dialog)->vbox);
	/* Restore size */
	gtk_window_resize(GTK_WINDOW(dialog),
					  cfg_get_single_value_as_int_with_default(config, "serverstats", "dialog-width", 400),
					  cfg_get_single_value_as_int_with_default(config, "serverstats", "dialog-height", 400));
	/* handle close */
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(serverinformation_popup_close), NULL);
	gtk_widget_show(dialog);

}

static void serverstats_destroy(void)
{
	serverstats_connection_changed(connection, 0, NULL);
}

/** 
 * Define the plugin structure
 */
gmpcPlugin statistics_plugin = {
	/* name */
	.name = N_("Server Information"),
	/* version */
	.version = {0, 1, 2},
	.destroy = serverstats_destroy,
	/* type */
	.plugin_type = GMPC_INTERNALL,
	/** Connection changed */
	.mpd_connection_changed = serverstats_connection_changed
};
