#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "main.h"
#include "plugin.h"

GladeXML *cam_pref_xml = NULL;

void cover_art_pref_construct(GtkWidget *container);
void cover_art_pref_destroy(GtkWidget *container);
void cover_art_manager_load_tree(GtkTreeStore *cam_ts);

gmpcPrefPlugin cover_art_gpp = {
	cover_art_pref_construct,
	cover_art_pref_destroy
};

gmpcPlugin cover_art_plug = {
	"Cover Art",
	{1,1,1},
	GMPC_INTERNALL,
	0,
	NULL,
	NULL,
	NULL,
	NULL,	
	NULL,	
	&cover_art_gpp
};

typedef struct _ca_dl {
	struct _ca_dl *next;
	struct _ca_dl *previous;
	gmpcPlugin *plug;
	qthread *qt;
	GQueue *function;
	mpd_Song *song;
}ca_dl;

typedef struct _ca_callback{
	CoverArtCallback function;
	gpointer userdata;
} ca_callback;
GList *fetch_que_list = NULL;

config_obj *cover_index= NULL;


CoverArtResult cover_art_fetch_image_path(mpd_Song *song, gchar **path)
{
	int i=0;
	int priority = 1000;
	int can_try = 0;
	if(!cfg_get_single_value_as_int_with_default(config, "cover-art", "enable",TRUE)) {
		return COVER_ART_NO_IMAGE;
	}
	if(song == NULL) {
		return COVER_ART_NO_IMAGE;
	}

	if(song->artist && song->album){
		gchar *cipath = cfg_get_single_value_as_string(cover_index, song->artist, song->album);
		if(cipath)
		{
			*path = g_strdup(cipath);
			return COVER_ART_OK_LOCAL;
		}
	}
	for(i =  0; plugins[i] != NULL; i++)
	{
		if(plugins[i]->plugin_type == GMPC_PLUGIN_COVER_ART)
		{
			if(plugins[i]->coverart->fetch_image_path != NULL)
			{
				char *temp_path= NULL;
				int retv = plugins[i]->coverart->fetch_image_path(song, &temp_path);
				if(retv == COVER_ART_OK_LOCAL || retv == COVER_ART_OK_REMOTE)
				{
					if(priority > plugins[i]->coverart->get_priority())
					{
						if(*path){
							g_free(*path);
							*path = NULL;
						}
						*path = temp_path;
						priority = plugins[i]->coverart->get_priority();
					}
					debug_printf(DEBUG_INFO,"%s has image \n", plugins[i]->name);
				}
				else if(retv == COVER_ART_NOT_FETCHED)
				{
					debug_printf(DEBUG_INFO,"%s can try \n", plugins[i]->name);
					can_try = 1;
				}
				else if (retv == COVER_ART_NO_IMAGE)
				{
					debug_printf(DEBUG_INFO,"%s has no image\n", plugins[i]->name);
				}
			}
		}	
	}
	if(*path)
	{
		if(song->artist && song->album){
			cfg_set_single_value_as_string(cover_index, song->artist, song->album,*path);
		}
		debug_printf(DEBUG_INFO,"returned image: %s", *path);
		return 	COVER_ART_OK_LOCAL;

	}
	if(can_try)
	{
		return COVER_ART_NOT_FETCHED;
	}
	return COVER_ART_NO_IMAGE;
}


void __internall_fetch_cover_art(ca_dl *cd)
{
	debug_printf(DEBUG_INFO,"Starting cover art fetch with plugin: %s\n", cd->plug->name);
	cd->plug->coverart->fetch_image(cd->song,NULL);
}
void cover_art_execute_signal(ca_callback *function, mpd_Song *song)
{
	debug_printf(DEBUG_INFO,"Executing callback: %p %s-%s\n", function, song->artist, song->album);
	function->function(song,function->userdata);
	g_free(function);
}

int cover_art_check_fetch_done(ca_dl *cd)
{
	if(qthread_is_done(cd->qt))
	{
		fetch_que_list = g_list_remove(fetch_que_list,cd);
		/* execute signals */
		g_queue_foreach(cd->function,(GFunc)cover_art_execute_signal, cd->song); 
		g_queue_free(cd->function);
		/* free song */
		mpd_freeSong(cd->song);
		/* cleanup thread */
		qthread_free(cd->qt);
		/* free cd */
		g_free(cd);
		return FALSE;
	}
	return TRUE;	
}
void cover_art_thread_fetch_image(gmpcPlugin *plug, mpd_Song *song, CoverArtCallback function, gpointer userdata)
{
	ca_dl *list = NULL;
	ca_callback *cc = g_malloc0(sizeof(ca_callback));
	cc->function = function;
	cc->userdata = userdata;
	if(fetch_que_list)
	{
		GList *first = g_list_first(fetch_que_list);
		do {
			ca_dl *list = first->data;
			if(!strcmp(list->song->artist, song->artist) &&
					!strcmp(list->song->album, song->album))
			{
				debug_printf(DEBUG_INFO,"Fetch allready in progress\n");
				g_queue_push_tail(list->function, cc);
				return;
			}
		}while((first = g_list_next(first))!= NULL);
	}
	list =  g_malloc(sizeof(ca_dl));
	list->plug = plug;
	list->song = mpd_songDup(song);
	list->qt = qthread_new((GSourceFunc)__internall_fetch_cover_art, list);
	list->function = g_queue_new();
	g_queue_push_tail(list->function, cc);
	fetch_que_list = g_list_append(fetch_que_list, list);
	qthread_run(list->qt);
	g_timeout_add(500, (GSourceFunc)cover_art_check_fetch_done,list);
}

void cover_art_fetch_image(mpd_Song *song, CoverArtCallback function,gpointer userdata){
	int i=0;
	int priority = 1000;
	gmpcPlugin *plugin = NULL;
	for(i =  0; plugins[i] != NULL; i++)
	{
		if(plugins[i]->plugin_type == GMPC_PLUGIN_COVER_ART)
		{
			if(plugins[i]->coverart->fetch_image_path != NULL)
			{
				char *temp_path= NULL;
				int retv = plugins[i]->coverart->fetch_image_path(song, &temp_path);
				if(retv == COVER_ART_OK_LOCAL || retv == COVER_ART_OK_REMOTE)
				{
					debug_printf(DEBUG_WARNING,"Not fetching image, allready availible %s\n",temp_path);
					if(temp_path)g_free(temp_path);

					return;
				}
				else if(retv == COVER_ART_NOT_FETCHED)
				{
					if(plugins[i]->coverart->fetch_image)
					{
						if(priority > plugins[i]->coverart->get_priority())
						{
							plugin = plugins[i];
							priority = plugins[i]->coverart->get_priority();
						}
					}
				}
			}
		}

	}
	if(plugin != NULL)
	{
		debug_printf(DEBUG_INFO,"Trying to fetch image from: %s\n", plugin->name);
		cover_art_thread_fetch_image(plugin,song,function,userdata);
	}
}

CoverArtResult cover_art_fetch_image_path_aa(gchar *artist,gchar *album, gchar **path)
{
	if(!mpd_server_check_version(connection,0,12,0))return COVER_ART_NO_IMAGE;
	MpdData *data = mpd_playlist_find_adv(connection, FALSE, MPD_TAG_ITEM_ARTIST, artist,
			MPD_TAG_ITEM_ALBUM,album,-1);
	if(data){
		CoverArtResult ret =COVER_ART_NO_IMAGE;
		if(data->type == MPD_DATA_TYPE_SONG)
		{
			ret = cover_art_fetch_image_path(data->song,path);
		}
		mpd_data_free(data);
		return ret;
	}

	return COVER_ART_NO_IMAGE;
}


void cover_art_fetch_image_aa(gchar *artist, gchar *album, CoverArtCallback function,gpointer userdata)
{
	if(!mpd_server_check_version(connection,0,12,0))return;
	MpdData *data = mpd_playlist_find_adv(connection, FALSE, MPD_TAG_ITEM_ARTIST, artist,
			MPD_TAG_ITEM_ALBUM,album,-1);
	if(data){
		if(data->type == MPD_DATA_TYPE_SONG)
		{
			cover_art_fetch_image(data->song,function, userdata);
		}
		mpd_data_free(data);
	}

	return ;
}


void cover_art_init()
{
	gchar *url = g_strdup_printf("%s/.covers/", g_get_home_dir());
	if(!g_file_test(url,G_FILE_TEST_IS_DIR)){
		if(mkdir(url, 0755)<0){
			g_error("Cannot make %s\n", url);
		}
	}
	g_free(url);
	url = g_strdup_printf("%s/.covers/covers.db", g_get_home_dir());
	cover_index = cfg_open(url);
	g_free(url);

	/* test code 
	conf_mult_obj *mult = cfg_get_class_list(cover_index);
	for(;mult->next != NULL; mult = mult->next){
		printf("%s\n", mult->key);
	}
	cfg_free_multiple(mult);
	*/
}
void cover_art_manager_close(GtkWidget *widget)
{
	GladeXML *camxml = glade_get_widget_tree(widget);
	if(camxml){
		gtk_widget_destroy(glade_xml_get_widget(camxml, "cam-win"));
		g_object_unref(camxml);
	}

}

void cover_art_manager_create()
{
	GtkTreeStore *cam_ts = NULL;
	GtkTreeViewColumn *column = NULL;
	GtkCellRenderer *renderer = NULL;
	GtkWidget *label = NULL;
	GladeXML *camxml = NULL;
	gchar *camp = gmpc_get_full_glade_path("gmpc.glade");
	camxml = glade_xml_new(camp, "cam-win",NULL);
	g_free(camp);
	if(!camp) return;
	/* create storage */
	cam_ts = gtk_tree_store_new(5,
			G_TYPE_STRING, /* name */
			GDK_TYPE_PIXBUF, /*cover art */
			G_TYPE_STRING, /*artist */
			G_TYPE_STRING, /*album */
			G_TYPE_INT /* type of row */
			);
	/* set the model */
	gtk_tree_view_set_model(
			GTK_TREE_VIEW(glade_xml_get_widget(camxml, "camtree")),
			GTK_TREE_MODEL(cam_ts));
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,renderer,"pixbuf",1, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column,renderer,"markup", 0, NULL);
	gtk_tree_view_append_column (
			GTK_TREE_VIEW (glade_xml_get_widget(camxml, "camtree")), column);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_spacing(column,6);

	gtk_tree_view_column_set_sort_column_id (column, 0);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(cam_ts), 0, GTK_SORT_ASCENDING);


	label = glade_xml_get_widget(camxml, "title_label_box");
	gtk_widget_modify_bg(label, GTK_STATE_NORMAL, &label->style->bg[GTK_STATE_SELECTED]);
	label = glade_xml_get_widget(camxml, "title_label");
	gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &label->style->fg[GTK_STATE_SELECTED]);

	cover_art_manager_load_tree(cam_ts);
	glade_xml_signal_autoconnect(camxml);

}
void cover_art_manager_load_tree(GtkTreeStore *cam_ts) 
{
	if(cover_index){
		conf_mult_obj *mult = cfg_get_class_list(cover_index);
		while(mult){
			GtkTreeIter iter;
			gchar *string = g_markup_printf_escaped("<b>%s</b>",mult->key);
			conf_mult_obj *mult2 = cfg_get_key_list(cover_index, mult->key);
			gtk_tree_store_append(cam_ts, &iter, NULL);
			gtk_tree_store_set(cam_ts, &iter,
					0, string,/*name*/
					1, NULL, /* No Image */
					2, mult->key, /* artist*/
					3, NULL, /*album */
					4, 0,
					-1);
			g_free(string);
			if(mult2)
			{
				while(mult2 != NULL){
					GtkTreeIter child;
					gchar *string = g_markup_printf_escaped("<i>%s</i>",mult2->key);
					int size = cfg_get_single_value_as_int_with_default(config,
							"cover-art", "browser-size",80);

					GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_size(mult2->value,size,size,NULL);


					gtk_tree_store_append(cam_ts, &child,&iter);
					gtk_tree_store_set(cam_ts, &child,
							0, string,/*name*/
							1, pb, /* No Image */
							2, mult->key, /* artist*/
							3, mult2->key, /*album */
							4, 1,
							-1);
					g_object_unref(pb);
					g_free(string);
					if(mult2->next == NULL){
						cfg_free_multiple(mult2);
						mult2 = NULL;
					}
					else{
						mult2 = mult2->next;
					}
				}

			}
			while(gtk_events_pending()) gtk_main_iteration();
			if(mult->next == NULL)
			{
				cfg_free_multiple(mult);
				mult = NULL;
			}
			else{
				mult = mult->next;
			}
		}
	}


}


void cover_art_pref_destroy(GtkWidget *container)
{
	if(cam_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(cam_pref_xml, "cam-vbox");
		gtk_container_remove(GTK_CONTAINER(container),vbox);
		g_object_unref(cam_pref_xml);
		cam_pref_xml = NULL;
	}
}
void cover_art_pref_toggle_enable(GtkToggleButton *tog)
{
	cfg_set_single_value_as_int(config, "cover-art", "enable", gtk_toggle_button_get_active(tog));
}

void cover_art_cover_manager(GtkButton *but)
{
	cover_art_manager_create();
}



void cover_art_pref_construct(GtkWidget *container)
{
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
	cam_pref_xml = glade_xml_new(path, "cam-vbox",NULL);
	g_free(path);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(cam_pref_xml, "tb-enable-cam")), 
			cfg_get_single_value_as_int_with_default(config,"cover-art", "enable", TRUE));

	gtk_container_add(GTK_CONTAINER(container),glade_xml_get_widget(cam_pref_xml, "cam-vbox"));
	glade_xml_signal_autoconnect(cam_pref_xml);
}

