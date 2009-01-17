/* gmpc-serverstats (GMPC plugin)
 * Copyright (C) 2006-2009 Qball Cow <qball@sarine.nl>
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

#include <gtk/gtk.h>
#include <config.h>
#include "main.h"
#include "plugin.h"
#include "playlist3-messages.h"
#include <libmpd/libmpd-internal.h>

static void serverstats_set_enabled(int enabled);
static int serverstats_get_enabled(void);
static void serverstats_add(GtkWidget *category_tree);

static void serverstats_selected(GtkWidget *container);

static void serverstats_unselected(GtkWidget *container);

static void serverstats_plugin_init(void);
static void serverstats_connection_changed(MpdObj *mi, int connect,void *userdata);
static gchar * serverstats_format_time(unsigned long seconds);
static void serverstats_browser_save_myself(void);
static void serverstats_status_changed(MpdObj *mi, ChangedStatusType what, void *pointer);
/**
 * Browser extention 
 */

gmpcPlBrowserPlugin serverstats_gbp = {
	/** add */
	.add = serverstats_add,
	/** selected */
	.selected = serverstats_selected,
	/** unselected */
	.unselected = serverstats_unselected,
};


/** 
 * Define the plugin structure
 */
gmpcPlugin statistics_plugin = {
	/* name */
	.name = N_("Statistics"),
	/* version */
	.version        = {0,1,2},
	/* type */
	.plugin_type = GMPC_PLUGIN_PL_BROWSER,
	/* init function */
	.init = serverstats_plugin_init,
	/** playlist extention struct */
	.browser = &serverstats_gbp,
	/** Connection changed */
	.mpd_connection_changed = serverstats_connection_changed,
    .mpd_status_changed = serverstats_status_changed,
	/** enable/disable */
	.get_enabled = serverstats_get_enabled,
	.set_enabled = serverstats_set_enabled,
    /* Safe myself */
    .save_yourself = serverstats_browser_save_myself
};
enum {
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
static GtkTreeRowReference *serverstats_ref = NULL; 
static GtkWidget *serverstats_sw= NULL, *serverstats_tree = NULL,*serverstats_combo = NULL;
static GtkWidget *serverstats_labels[SERVERSTATS_NUM_FIELDS];
static gboolean cancel_query = FALSE;

/**
 * Get/Set enable 
 */

static int serverstats_get_enabled(void)
{
	return cfg_get_single_value_as_int_with_default(config, "serverstats", "enable", TRUE);
}

static void serverstats_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "serverstats", "enable", enabled);
	if(enabled)
	{
		if(serverstats_ref == NULL)
		{
			serverstats_add(GTK_WIDGET(playlist3_get_category_tree_view()));
		}
	}
	else
	{
		GtkTreePath *path = gtk_tree_row_reference_get_path(serverstats_ref);
        GtkTreeModel *model = gtk_tree_row_reference_get_model(serverstats_ref);
		if (path){
			GtkTreeIter iter;
			if (gtk_tree_model_get_iter(model, &iter, path)){
				gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
			}
			gtk_tree_path_free(path);
			gtk_tree_row_reference_free(serverstats_ref);
			serverstats_ref = NULL;
		}      
	}
}

/**
 * Playlist browser functions 
 */

static gboolean serverstats_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{

	int width = widget->allocation.width;
	int height = widget->allocation.height;
	
	gtk_paint_flat_box(widget->style, 
					widget->window, 
					GTK_STATE_SELECTED,
					GTK_SHADOW_NONE,
					NULL, 
					widget,
					"cell_odd",
					0,0,
					width,height);

	gtk_paint_focus(widget->style, widget->window, 
				GTK_STATE_NORMAL, 
				NULL, 
				widget,
				"button",
				0,0,width,height);
	return FALSE;
}
static void serverstats_add(GtkWidget *category_tree)
{
	GtkTreePath *path;
	GtkTreeModel *model = GTK_TREE_MODEL(playlist3_get_category_tree_store()); 
	GtkTreeIter iter;
    gint pos;
	/**
	 * don't do anything if we are disabled
	 */
	if(!cfg_get_single_value_as_int_with_default(config, "serverstats", "enable", TRUE)) return;
	/** 
	 * Add ourslef to the list 
	 */
	pos = cfg_get_single_value_as_int_with_default(config, "serverstats","position",2);
	playlist3_insert_browser(&iter, pos);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 
			PL3_CAT_TYPE, statistics_plugin.id,
			PL3_CAT_TITLE,"Server Statistics", 
			PL3_CAT_INT_ID, "/",
			PL3_CAT_ICON_ID, "mpd",
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
	/** 
	 * remove odl reference if exists 
	 */
	if (serverstats_ref) {
		gtk_tree_row_reference_free(serverstats_ref);
		serverstats_ref = NULL;
	}
	/**
	 * create reference to ourself in the list
	 */
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &iter);
	if (path) {
		serverstats_ref = gtk_tree_row_reference_new(model, path);
		gtk_tree_path_free(path);
	}
}
static void serverstats_browser_save_myself(void)
{
	if(serverstats_ref)
	{
		GtkTreePath *path = gtk_tree_row_reference_get_path(serverstats_ref);
		if(path)
		{
			gint *indices = gtk_tree_path_get_indices(path);
			debug_printf(DEBUG_INFO,"Saving myself to position: %i\n", indices[0]);
			cfg_set_single_value_as_int(config, "serverstats","position",indices[0]);
			gtk_tree_path_free(path);
		}
	}
}
static void serverstats_clear()
{
	int i;
	for(i=0;i < SERVERSTATS_NUM_FIELDS;i++)
	{
		gtk_label_set_text(GTK_LABEL(serverstats_labels[i]), "");
	}

}
static void serverstats_update()
{
  gchar **handlers = NULL;
	gchar *value = NULL;

	serverstats_clear();
	if(!mpd_check_connected(connection))return;
	mpd_stats_update(connection);
	/** Version */
	value = mpd_server_get_version(connection); 
	gtk_label_set_text(GTK_LABEL(serverstats_labels[SERVERSTATS_MPD_VERSION]), value);
	free(value);
	/** Uptime  */
	value = serverstats_format_time(mpd_stats_get_uptime(connection));
	gtk_label_set_text(GTK_LABEL(serverstats_labels[SERVERSTATS_MPD_UPTIME]), value);
	g_free(value);
	/** Playtime*/
	value = serverstats_format_time(mpd_stats_get_playtime(connection));
	gtk_label_set_text(GTK_LABEL(serverstats_labels[SERVERSTATS_MPD_PLAYTIME]), value);
	g_free(value);
	/** DB Playtime*/
	value = serverstats_format_time(mpd_stats_get_db_playtime(connection));
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
  if(handlers)
  {
    value = g_strjoinv(",",handlers);
    g_strfreev(handlers);
    handlers = NULL;
  }
  else 
    value = g_strdup("N/A");
  gtk_label_set_text(GTK_LABEL(serverstats_labels[SERVERSTATS_MPD_URLHANDLERS]), value);
  g_free(value);
 
  if(mpd_server_check_version(connection, 0,13,0))
      handlers = mpd_server_get_tag_types(connection);
  if(handlers)
  {
    value = g_strjoinv(",",handlers);
    g_strfreev(handlers);
    handlers = NULL;
  }
  else 
    value = g_strdup("N/A");
  gtk_label_set_text(GTK_LABEL(serverstats_labels[SERVERSTATS_MPD_TAG_TYPES]), value);
  g_free(value);

}
typedef struct _ss_str{
    int total;
    int tag;
    int hits;
    MpdData *data;
    GtkTreeModel *model;
    GtkWidget *box,*pb;
    long unsigned max_i;
}ss_str;

static gboolean serverstats_idle_handler(ss_str *s)
{
    GtkTreeIter iter;
    MpdDBStats *stats = NULL;
    if(s->data == NULL || !mpd_check_connected(connection) || cancel_query)
    {

        if(gtk_tree_model_get_iter_first(s->model, &iter))
        {
            do{	gulong i;
                gchar *value = NULL;
                gtk_tree_model_get(s->model, &iter, 0, &i, -1);
                guint d = (guint)100*(i/(double)s->max_i);
                value = serverstats_format_time(i);
                gtk_list_store_set(GTK_LIST_STORE(s->model), &iter, 2, d, 3,value,-1);
                g_free(value);
            }while(gtk_tree_model_iter_next(s->model, &iter));
        }

        if(s->data)
            mpd_data_free(s->data);
        gtk_tree_view_set_model(GTK_TREE_VIEW(serverstats_tree), s->model);
        gtk_tree_view_set_search_column(GTK_TREE_VIEW(serverstats_tree), 1);
        gtk_widget_set_sensitive(GTK_WIDGET(s->box), TRUE);
        gtk_widget_hide(gtk_widget_get_parent(s->pb));

        if(cancel_query)
            gtk_list_store_clear(GTK_LIST_STORE(s->model));
        g_free(s);
        cancel_query = FALSE;
        return FALSE;
    }
    mpd_database_search_stats_start(connection);
    mpd_database_search_add_constraint(connection, s->tag,s->data->tag);

    stats = mpd_database_search_stats_commit(connection);
    if(stats)
    {

        gtk_list_store_prepend(GTK_LIST_STORE(s->model), &iter);
        gtk_list_store_set(GTK_LIST_STORE(s->model), &iter, 0, (unsigned long)(stats->playTime), 1,s->data->tag, -1);
        s->max_i = MAX(s->max_i, stats->playTime);

        mpd_database_search_free_stats(stats);


    }
    /* limit the amount of updating to 0.2 % */
    if((int)((1000*s->hits)/s->total)%5 == 0)
    {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(s->pb),s->hits/(double)s->total);
    }
    s->hits++;
    s->data = mpd_data_get_next(s->data);
    return TRUE;
}

static void serverstats_combo_changed(GtkComboBox *box, GtkWidget *pb)
{
     GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(serverstats_tree));
    int tag = gtk_combo_box_get_active(box);
    GtkTreeIter iter;
    
   if(!mpd_check_connected(connection)) 
        return;
    if(!mpd_server_check_version(connection,0,13,0))
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
    MpdData *node ,*data = mpd_database_search_commit(connection);
    gulong max_i = 0;

    int hits = 0;
    int total = 0;
    for(node = mpd_data_get_first(data);node != NULL; node = (MpdData *)mpd_data_get_next_real(node, FALSE)) total++;
    ss_str *s = g_malloc0(sizeof(*s));
    s->total = total;
    s->model = model;
    s->data = data;
    s->hits = 0;
    s->tag = tag;
    s->pb = pb;
    s->box = GTK_WIDGET(box);
    g_idle_add((GSourceFunc)serverstats_idle_handler,s);

}


static void serverstats_header_style_changed(GtkWidget *vbox, GtkStyle *style,  GtkWidget *vp)
{
    gtk_widget_modify_bg(vp,GTK_STATE_NORMAL, &(GTK_WIDGET(vbox)->style->white));
}

static void cancel_clicked(GtkWidget *cancel, gpointer data)
{
    cancel_query = TRUE;
}

static void serverstats_init()
{
    /** Get an allready exposed widgets to grab theme colors from. */
    GtkWidget *colw = (GtkWidget *)playlist3_get_category_tree_view();
    GtkWidget *label = NULL;
    GtkWidget *table = NULL;
    GtkWidget *hbox = NULL;
    GtkWidget *event = NULL;
    GtkWidget *serverstats_vbox = gtk_vbox_new(FALSE, 0);
    GtkWidget *serverstats_event;
    gchar *markup = NULL;

    serverstats_sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(serverstats_sw), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(serverstats_sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    serverstats_event = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(serverstats_event), TRUE);
    gtk_container_add(GTK_CONTAINER(serverstats_event),serverstats_vbox);

    /* wrap in event box to set bg color */
    event = gtk_event_box_new();
	gtk_widget_set_app_paintable(event, TRUE);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(event), TRUE);
    g_signal_connect(G_OBJECT(event), "expose-event", G_CALLBACK(serverstats_expose_event), NULL);

    gtk_widget_modify_bg(serverstats_event, GTK_STATE_NORMAL, &(colw->style->white));
    g_signal_connect(G_OBJECT(serverstats_vbox), "style-set", G_CALLBACK(serverstats_header_style_changed), serverstats_event);

    /* set label and padding */
    hbox = gtk_hbox_new(FALSE, 6);
    label = gtk_image_new_from_icon_name("mpd", GTK_ICON_SIZE_DND);
    gtk_box_pack_start(GTK_BOX(hbox),label, FALSE, TRUE, 0);
    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    markup = g_markup_printf_escaped("<span size='xx-large' weight='bold'>%s</span>", _("Server Statistics"));
    gtk_label_set_markup(GTK_LABEL(label),markup);
    g_free(markup);
    gtk_box_pack_start(GTK_BOX(hbox),label, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(event),hbox);

    gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
    gtk_box_pack_start(GTK_BOX(serverstats_vbox),event, FALSE, TRUE, 0);
    gtk_widget_show_all(event);

    label = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(serverstats_vbox),label, FALSE, TRUE, 0);
    gtk_widget_show(label);
    /**
     * Data list 
     */
//    gtk_container_set_border_width(GTK_CONTAINER(serverstats_vbox), 6);
    table = gtk_table_new(SERVERSTATS_NUM_FIELDS+2, 2,FALSE);
    gtk_table_set_col_spacings(GTK_TABLE(table), 6);
    gtk_table_set_row_spacings(GTK_TABLE(table), 6);
    gtk_container_set_border_width(GTK_CONTAINER(table), 12);

    /** Database */
    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    markup = g_markup_printf_escaped("<span size='x-large' weight='bold'>%s</span>", _("Server"));
    gtk_label_set_markup(GTK_LABEL(label),markup); 
    g_free(markup);
    gtk_table_attach(GTK_TABLE(table),label, 0,2,0,1,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);	

    /** Mpd version */
    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(label), 12,0);

    markup = g_markup_printf_escaped("<b>%s:</b>", _("Version"));
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    gtk_table_attach(GTK_TABLE(table),label, 0,1,1,2,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    label = serverstats_labels[SERVERSTATS_MPD_VERSION]= gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table),label, 1,2,1,2,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);

    /** Mpd Uptime */
    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(label), 12,0);
    markup = g_markup_printf_escaped("<b>%s:</b>", _("Uptime"));
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    gtk_table_attach(GTK_TABLE(table),label, 0,1,2,3,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    label = serverstats_labels[SERVERSTATS_MPD_UPTIME]= gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table),label, 1,2,2,3,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);	
    /** Mpd Playtime */
    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(label), 12,0);

    markup = g_markup_printf_escaped("<b>%s:</b>", _("Time Playing"));
    gtk_label_set_markup(GTK_LABEL(label),markup);
    g_free(markup);
    gtk_table_attach(GTK_TABLE(table),label, 0,1,3,4,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    label = serverstats_labels[SERVERSTATS_MPD_PLAYTIME]= gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table),label, 1,2,3,4,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);	
    /** Database */
    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    markup = g_markup_printf_escaped("<span size='x-large' weight='bold'>%s</span>", _("Database"));
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    gtk_table_attach(GTK_TABLE(table),label, 0,2,4,5,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);	
    /** Mpd Playtime */
    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(label), 12,0);

    markup = g_markup_printf_escaped("<b>%s:</b>", _("Total playtime"));
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    gtk_table_attach(GTK_TABLE(table),label, 0,1,5,6,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    label = serverstats_labels[SERVERSTATS_MPD_DB_PLAYTIME]= gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table),label, 1,2,5,6,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);	
    /** Mpd Artists*/
    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(label), 12,0);

    markup = g_markup_printf_escaped("<b>%s:</b>", _("Number of artists"));
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    gtk_table_attach(GTK_TABLE(table),label, 0,1,6,7,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    label = serverstats_labels[SERVERSTATS_MPD_DB_ARTISTS]= gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table),label, 1,2,6,7,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);	
    /** Mpd Albums */
    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(label), 12,0);

    markup = g_markup_printf_escaped("<b>%s:</b>", _("Number of albums"));
    gtk_label_set_markup(GTK_LABEL(label),markup);
    g_free(markup);
    gtk_table_attach(GTK_TABLE(table),label, 0,1,7,8,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    label = serverstats_labels[SERVERSTATS_MPD_DB_ALBUMS]= gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table),label, 1,2,7,8,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);	
    /** Mpd Songs */
    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(label), 12,0);

    markup = g_markup_printf_escaped("<b>%s:</b>", _("Number of songs"));
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    gtk_table_attach(GTK_TABLE(table),label, 0,1,8,9,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    label = serverstats_labels[SERVERSTATS_MPD_DB_SONGS]= gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table),label, 1,2,8,9,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    /** Mpd Songs */
    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(label), 12,0);

    markup = g_markup_printf_escaped("<b>%s:</b>", _("URL Handlers"));
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    gtk_table_attach(GTK_TABLE(table),label, 0,1,9,10,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    label = serverstats_labels[SERVERSTATS_MPD_URLHANDLERS]= gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table),label, 1,2,9,10,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    /** Mpd Songs */
    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(label), 12,0);

    markup = g_markup_printf_escaped("<b>%s:</b>", _("Tag Types"));
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    gtk_table_attach(GTK_TABLE(table),label, 0,1,10,11,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
    label = serverstats_labels[SERVERSTATS_MPD_TAG_TYPES]= gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table),label, 1,2,10,11,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);

    /** Stats */
    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    markup = g_markup_printf_escaped("<span size='x-large' weight='bold'>%s</span>", _("Tag statistics"));
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    gtk_table_attach(GTK_TABLE(table),label, 0,2,11,12,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);	


    gtk_widget_show_all(table);

    /**
     * Stats treeview
     */
    {
        GtkWidget *combo = NULL;
        GtkWidget *sw = NULL,*tree = NULL,*hbox=NULL,*cancel;
        GtkListStore *store;
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;
        int i;
        GtkWidget *pb = gtk_progress_bar_new();
        serverstats_combo = combo = gtk_combo_box_new_text();
        for(i=0;i<MPD_TAG_NUM_OF_ITEM_TYPES-2;i++)
        {
            gtk_combo_box_append_text(GTK_COMBO_BOX(combo), mpdTagItemKeys[i]);
        }


   //     gtk_box_pack_start(GTK_BOX(serverstats_vbox), combo,FALSE, TRUE,0);
     //   gtk_widget_show_all(combo);
        
        gtk_table_attach(GTK_TABLE(table),combo, 0,2,12,13,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
        gtk_widget_show(combo);

        hbox = gtk_hbox_new(FALSE,6);
        cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
        g_signal_connect(G_OBJECT(cancel), "clicked", G_CALLBACK(cancel_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(hbox), pb,TRUE, TRUE,0);
        gtk_box_pack_start(GTK_BOX(hbox), cancel,FALSE, TRUE,0);
//        gtk_box_pack_start(GTK_BOX(serverstats_vbox), hbox,FALSE, TRUE,0);

        gtk_table_attach(GTK_TABLE(table),hbox, 0,2,13,14,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0,0);
        g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(serverstats_combo_changed), pb);

        sw = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_container_set_border_width(GTK_CONTAINER(sw), 6);

        store = gtk_list_store_new(4, G_TYPE_ULONG, G_TYPE_STRING, G_TYPE_ULONG, G_TYPE_STRING);
        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), 0,GTK_SORT_DESCENDING);
        serverstats_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));	
        gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(serverstats_tree), FALSE);
        gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(serverstats_tree), TRUE);
        renderer = gtk_cell_renderer_text_new();
        g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END,"ellipsize-set", TRUE,"width-chars",30, NULL);
        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW(serverstats_tree),
                -1, "",
                renderer,
                "text", 1,NULL);
        renderer = gtk_cell_renderer_progress_new();
        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW(serverstats_tree),
                -1, "",
                renderer,
                "value", 2,
                "text", 3,NULL);

        gtk_tree_view_set_search_column(GTK_TREE_VIEW(serverstats_tree), 1);

        gtk_container_add(GTK_CONTAINER(sw), serverstats_tree);

        gtk_table_attach(GTK_TABLE(table),sw, 0,2,14,15,GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0,0);
        gtk_widget_show_all(sw);
    }
    /**
     * Add table
     */
    gtk_box_pack_start(GTK_BOX(serverstats_vbox), table, TRUE, TRUE,0);
    /* maintain my own reference to the widget, so it won't get destroyed removing 
     * from view
     */
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(serverstats_sw), serverstats_event);
    gtk_widget_show(serverstats_vbox);	
    gtk_widget_show(serverstats_event);	
    gtk_widget_show(serverstats_sw);	
    g_object_ref(serverstats_sw);
}

static guint timeout_source = 0;

static void serverstats_selected(GtkWidget *container)
{
    if(serverstats_sw== NULL) {
        serverstats_init();
    }
    serverstats_update();
    gtk_container_add(GTK_CONTAINER(container), serverstats_sw);
    gtk_widget_show(serverstats_sw);
    if(timeout_source)
        g_source_remove(timeout_source);
    timeout_source = g_timeout_add(30000, (GSourceFunc)serverstats_update, NULL);
}

static void serverstats_unselected(GtkWidget *container)
{
    if(timeout_source)
        g_source_remove(timeout_source);
    timeout_source = 0;
    gtk_container_remove(GTK_CONTAINER(container),serverstats_sw);
}



static gchar * serverstats_format_time(unsigned long seconds)
{
    GString *str = NULL;
    gulong days = seconds/86400;
    gulong houres = (seconds % 86400)/3600;
    gulong minutes = (seconds % 3600)/60;
    char *ret;
    if(seconds == 0)
    {
        return g_strdup("");
    }
    str = g_string_new("");
    if(days != 0)
    {

        g_string_append_printf(str, "%lu %s ", days, (days == 1)?("day"):("days"));
    }	
    if(houres != 0)
    {
        g_string_append_printf(str, "%lu %s ", houres, (houres == 1)?("hour"):("hours"));
    }
    if(minutes != 0)
    {
        g_string_append_printf(str, "%lu %s", minutes,(minutes==1)?("minute"):("minutes"));
    }
    ret = str->str;
    g_string_free(str, FALSE);
    return ret;
}


static void serverstats_plugin_init(void)
{
}

static void serverstats_connection_changed(MpdObj *mi, int connect,void *usedata)
{

    if(!connect && serverstats_tree)
    {
        GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(serverstats_tree));
        serverstats_clear();
        if(model)
            gtk_list_store_clear(GTK_LIST_STORE(model));
        gtk_combo_box_set_active(GTK_COMBO_BOX(serverstats_combo), -1);
    }
}

static void serverstats_status_changed(MpdObj *mi, ChangedStatusType what, void *pointer)
{


}

