/* gmpc-last.fm (GMPC plugin)
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

#include <stdio.h>
#include <string.h>
#include <config.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "main.h"
#include "plugin.h"
#include "gmpc_easy_download.h"
#include "metadata.h"

#define LASTFM_API_KEY "ec1cdd08d574e93fa6ef9ad861ae795a" 
#define LASTFM_API_ROOT "http://ws.audioscrobbler.com/2.0/"

#define LOG_DOMAIN "Gmpc.Provider.LastFM"

gmpcPlugin lastfm_plugin;

typedef struct Query {
	MetaDataType type;
	void (*callback)(GList *list, gpointer data);
	gpointer user_data;
}Query;

static int lastfm_get_enabled(void)
{
	return cfg_get_single_value_as_int_with_default(config, "cover-lastfm", "enable", TRUE);
}
static void lastfm_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "cover-lastfm", "enable", enabled);
}

static int lastfm_fetch_cover_priority(void){
	return cfg_get_single_value_as_int_with_default(config, "cover-lastfm", "priority", 80);
}
static void lastfm_fetch_cover_priority_set(int priority){
	cfg_set_single_value_as_int(config, "cover-lastfm", "priority", priority);
}

static xmlNodePtr get_first_node_by_name(xmlNodePtr xml, xmlChar *name) {
    if(name == NULL) return NULL;
	if(xml) {
		xmlNodePtr c = xml->xmlChildrenNode;
		for(;c;c=c->next) {
			if(c->name && xmlStrEqual(c->name, name))
				return c;
		}
	}
	return NULL;
}

static xmlNodePtr get_next_node_by_name(xmlNodePtr xml, xmlChar *name)
{
    xmlNodePtr iter;
	if(name == NULL || xml == NULL) return NULL;
	for(iter= xml->next;iter;iter=iter->next)
	{
		if(iter->name && xmlStrEqual(iter->name, name)){
			return iter;
        }
    }
	return NULL;
}

static GList *__lastfm_art_xml_get_artist_image(const char *data, gint size, MetaDataType mtype)
{
    GList *list = NULL;
    xmlDocPtr doc;
    if(size <= 0 || data == NULL || data[0] != '<')
        return NULL;

    doc = xmlParseMemory(data,size);
    if(doc)
    {
        xmlNodePtr root = xmlDocGetRootElement(doc);
        if(root)
        {
            /* loop through all albums */
            xmlNodePtr cur = get_first_node_by_name(root,BAD_CAST "images");
            if(cur)
            {
                xmlNodePtr cur2 = get_first_node_by_name(cur, BAD_CAST "image"); 
                for(;cur2;cur2 = get_next_node_by_name(cur2, BAD_CAST"image"))
				{
					xmlNodePtr cur3 = get_first_node_by_name(cur2, BAD_CAST"sizes"); 
					for(;cur3;cur3 = get_next_node_by_name(cur3, BAD_CAST"sizes"))
					{
						xmlNodePtr cur4 = get_first_node_by_name(cur3, BAD_CAST"size");
						for(;cur4;cur4 = get_next_node_by_name(cur4, BAD_CAST"size")) 
						{
							xmlChar *temp = xmlGetProp(cur4, BAD_CAST"name");
							if(temp)
							{
								if(xmlStrEqual(temp, BAD_CAST"original"))
								{
									xmlChar *xurl = xmlNodeGetContent(cur4);
									if(xurl){
										if(strstr((char *)xurl, "noartist") == NULL){
											MetaData *mtd = meta_data_new();
											mtd->type = mtype;
											mtd->plugin_name = lastfm_plugin.name;
											mtd->content_type = META_DATA_CONTENT_URI;
											mtd->content  = g_strdup((char *)xurl);
											mtd->size = 0;
											list =g_list_prepend(list, mtd);
										}
										xmlFree(xurl);
									}
								}
								xmlFree(temp);
							}
						}
					}
				}
            }
        }
        xmlFreeDoc(doc);
    }
	return g_list_reverse(list);
}
static GList* __lastfm_art_xml_get_image(const char* data, const gint size, const char* type, MetaDataType mtype)
{
	GList *list = NULL;
    xmlDocPtr doc;
	if(size <= 0 || data == NULL || data[0] != '<')
		return NULL;
	doc = xmlParseMemory(data,size);
	if(doc)
	{
		xmlNodePtr root = xmlDocGetRootElement(doc);
        if(root)
        {
            /* loop through all albums */
            xmlNodePtr cur = get_first_node_by_name(root,BAD_CAST type);
            if(cur)
            {
                xmlNodePtr cur2 = get_first_node_by_name(cur, BAD_CAST"image"); 
                for(;cur2!= NULL ;cur2 = get_next_node_by_name(cur2, BAD_CAST"image"))
				{
					xmlChar *temp = xmlGetProp(cur2, BAD_CAST"size");
					if(temp)
					{
						/**
						 * We want large image, but if that is not available, get the medium one 
						 */
						if(xmlStrEqual(temp, BAD_CAST"medium"))
						{
							xmlChar *xurl = xmlNodeGetContent(cur2);
							if(xurl){
								if(strstr((char *)xurl, "noartist") == NULL){
									MetaData *mtd = meta_data_new();
									mtd->type = mtype;
									mtd->plugin_name = lastfm_plugin.name;
									mtd->content_type = META_DATA_CONTENT_URI;
									mtd->content  = g_strdup((char *)xurl);
									mtd->size = 0;
									list =g_list_append(list, mtd);
								}
								xmlFree(xurl);
							}
						}else if(xmlStrEqual(temp, BAD_CAST"large") || 
								xmlStrEqual(temp, BAD_CAST"extralarge"))
						{
							xmlChar *xurl = xmlNodeGetContent(cur2);
							if(xurl)
							{
								if(strstr((char *)xurl, "noartist") == NULL){
									MetaData *mtd = meta_data_new();
									mtd->type = mtype;
									mtd->plugin_name = lastfm_plugin.name;
									mtd->content_type = META_DATA_CONTENT_URI;
									mtd->content  = g_strdup((char *)xurl);
									mtd->size = 0;
									list =g_list_prepend(list, mtd);
								}
								xmlFree(xurl);
							}
						}
						xmlFree(temp);
					}
				}
            }
        }
		xmlFreeDoc(doc);
	}
	return list;
}

static GList* __lastfm_art_xml_get_album_info(const char* data, const gint size)
{
	GList *list = NULL;
    xmlDocPtr doc;
	if(size <= 0 || data == NULL || data[0] != '<')
		return NULL;
	doc = xmlParseMemory(data,size);

	if(doc)
	{
		xmlNodePtr root = xmlDocGetRootElement(doc);
        if(root)
        {
            /* loop through all albums */
            xmlNodePtr cur = get_first_node_by_name(root,BAD_CAST"album");
            if(cur)
            {
                xmlNodePtr cur2 = get_first_node_by_name(cur, BAD_CAST"wiki"); 
                if(cur2)
				{
					xmlNodePtr cur3 = get_first_node_by_name(cur2, BAD_CAST"content");
					if(cur3)
					{
						xmlChar *xurl = xmlNodeGetContent(cur3);
						if(xurl){
							MetaData *mtd = meta_data_new();
							mtd->type = META_ALBUM_TXT;
							mtd->plugin_name = lastfm_plugin.name;
							mtd->content_type = META_DATA_CONTENT_HTML;
							mtd->content  = g_strdup((char *)xurl);
							mtd->size = -1;
							list =g_list_append(list, mtd);
						}
						xmlFree(xurl);
					}
				}
            }
        }
		xmlFreeDoc(doc);
	}
	return list;
}

/* get similar genres */
static MetaData* __lastfm_art_xml_get_genre_similar(const gchar* l_data, gint l_size)
{
	xmlDocPtr doc;
	MetaData* mtd = NULL;
	if(l_size <= 0 || l_data == NULL || l_data[0] != '<')
		return NULL;

	doc = xmlParseMemory(l_data, l_size);
	if(doc != NULL)
	{
		xmlNodePtr root = xmlDocGetRootElement(doc);
		xmlNodePtr cur = get_first_node_by_name(root, BAD_CAST "similartags");
		if(cur != NULL)
		{
			xmlNodePtr cur2 = get_first_node_by_name(cur, BAD_CAST"tag");
			for(; cur2 != NULL; cur2 = get_next_node_by_name(cur2,BAD_CAST"tag")) 
			{
				xmlNodePtr cur3 = get_first_node_by_name(cur2,BAD_CAST"name"); 
				for(; cur3 != NULL; cur3 = get_next_node_by_name(cur3, BAD_CAST "name"))
				{
					xmlChar* temp = xmlNodeGetContent(cur3);
					if(temp)
					{
						if(!mtd)
						{
							mtd = meta_data_new();
							mtd->type = META_GENRE_SIMILAR;
							mtd->plugin_name = lastfm_plugin.name;
							mtd->content_type = META_DATA_CONTENT_TEXT_LIST;
							mtd->size = 0;
						}
						mtd->size++;
						mtd->content = g_list_prepend((GList*) mtd->content, 
								g_strdup((char *)temp));
						xmlFree(temp);
						break;
					}
				}
			}
			if(mtd != NULL) {
				/* to have the match-order */
				mtd->content = g_list_reverse((GList*) mtd->content);
			}
		}
		xmlFreeDoc(doc);
	}

	return mtd;
}

/*
 * Get 20 artists
 */
static MetaData* __lastfm_art_xml_get_artist_similar(const gchar* data, gint size)
{
	MetaData *mtd = NULL;
	xmlDocPtr doc;
	xmlNodePtr root,cur;
	if(size <= 0 || data == NULL || data[0] != '<')
		return NULL;

	doc = xmlParseMemory(data,size);
	if(doc == NULL)
	{
		return NULL;	
	}
	root = xmlDocGetRootElement(doc);
	cur = get_first_node_by_name(root, BAD_CAST "similarartists");
	if(cur)
	{
		xmlNodePtr cur2 = get_first_node_by_name(cur, BAD_CAST "artist");
		for(;cur2;cur2=get_next_node_by_name(cur2, BAD_CAST "artist"))
		{
			xmlNodePtr cur3 = get_first_node_by_name(cur2, BAD_CAST "name");
			for(;cur3;cur3=get_next_node_by_name(cur3, BAD_CAST"name"))
			{
				xmlChar *temp = xmlNodeGetContent(cur3);
				if(temp)
				{
					if(!mtd) {
						mtd = meta_data_new();
						mtd->type = META_ARTIST_SIMILAR;
						mtd->plugin_name = lastfm_plugin.name;
						mtd->content_type = META_DATA_CONTENT_TEXT_LIST;
						mtd->size = 0;
					}
					mtd->size++;
					mtd->content = g_list_prepend((GList*) mtd->content,
							g_strdup((char *)temp));
					xmlFree(temp);
				}
			}
		}
		if(mtd != NULL) {
			mtd->content = g_list_reverse((GList*) mtd->content);
		}
	}
	xmlFreeDoc(doc);
	return mtd;
}

/*
 * Get 20Songs 
 */
static MetaData* __lastfm_art_xml_get_song_similar(const gchar* data, gint size)
{
	MetaData *mtd = NULL;
	xmlDocPtr doc;
	xmlNodePtr root, cur;
	if(size <= 0 || data == NULL || data[0] != '<')
		return NULL;

	doc = xmlParseMemory(data,size);
	if(doc == NULL)
	{
		return NULL;
	}
	root = xmlDocGetRootElement(doc);
	cur = get_first_node_by_name(root, BAD_CAST "similartracks");
	if(cur)
	{
		xmlNodePtr cur2 =  get_first_node_by_name(cur, BAD_CAST"track");
		for(;cur2;cur2= get_next_node_by_name(cur2, BAD_CAST"track"))
		{
			xmlNodePtr cur3 = cur2->xmlChildrenNode;
			xmlChar *artist = NULL;
			xmlChar *title = NULL;
			for(;cur3;cur3=cur3->next)
			{
				if(xmlStrEqual(cur3->name, BAD_CAST"name"))
				{
					xmlChar *temp = xmlNodeGetContent(cur3);
					title = temp; 
				}
				else if (xmlStrEqual(cur3->name, BAD_CAST"artist")) 
				{
					xmlNodePtr cur4 = get_first_node_by_name(cur3, BAD_CAST"name");
					if(cur4){
						xmlChar *temp = xmlNodeGetContent(cur4);
						artist = temp; 
					}
				}
			}
			if(artist && title) {
				if(!mtd) {
					mtd = meta_data_new();
					mtd->type = META_SONG_SIMILAR;
					mtd->plugin_name = lastfm_plugin.name;
					mtd->content_type = META_DATA_CONTENT_TEXT_LIST;
					mtd->size = 0;
				}
				mtd->size++;
				mtd->content = g_list_prepend((GList*) mtd->content,
						g_strdup_printf("%s::%s", artist, title));
			}
			if(artist) xmlFree(artist);
			if(title) xmlFree(title);
		}
		if(mtd != NULL)
			mtd->content = g_list_reverse((GList*) mtd->content);
	}
	xmlFreeDoc(doc);

	return mtd;
}


/** 
 * Get album image
 */
/**
 * Get artist info 
 */


static gchar* __lastfm_art_xml_get_artist_bio(const gchar* data , gint size)
{
	xmlDocPtr doc = xmlParseMemory(data,size);
	gchar* info=NULL;
	if(doc)
	{
		xmlNodePtr root = xmlDocGetRootElement(doc);
		/* Todo this is ugly */
		xmlNodePtr bio = get_first_node_by_name(
				get_first_node_by_name(get_first_node_by_name(root,
						BAD_CAST"artist"),
					BAD_CAST"bio"),
				BAD_CAST"content");
		if(bio)
		{
			xmlChar *temp = xmlNodeGetContent(bio);
			info = g_strdup((gchar*) temp);
			xmlFree(temp);
		}
	}
	xmlFreeDoc(doc);

	return info;
}


/**
 * Preferences 
 */
static void pref_destroy(GtkWidget *con)
{
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(con));
    if(child) {
        gtk_container_remove(GTK_CONTAINER(con), child);
    }
}

static void pref_enable_fetch(GtkWidget *con, gpointer data)
{
    MetaDataType type = GPOINTER_TO_INT(data);
    int state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(con));
    switch(type) {
        case META_ARTIST_ART:
            cfg_set_single_value_as_int(config, "cover-lastfm", "fetch-art-artist",state); 
            break;
        case META_ALBUM_ART:
            cfg_set_single_value_as_int(config, "cover-lastfm", "fetch-art-album",state); 
            break;
		case META_ALBUM_TXT:
			cfg_set_single_value_as_int(config, "cover-lastfm", "fetch-album-info",state); 
			break;                                                                                 
        case META_ARTIST_SIMILAR:
            cfg_set_single_value_as_int(config, "cover-lastfm", "fetch-similar-artist",state); 
            break;
        case META_SONG_SIMILAR:
            cfg_set_single_value_as_int(config, "cover-lastfm", "fetch-similar-song",state); 
            break;                                                                                 
		case META_GENRE_SIMILAR:
			cfg_set_single_value_as_int(config, "cover-lastfm", "fetch-similar-genre", state);
			break;
        case META_ARTIST_TXT:
            cfg_set_single_value_as_int(config, "cover-lastfm", "fetch-biography-artist",state); 
            break;                                                                                 
	
		// Stop compiler warnings.
		case META_SONG_TXT:
		case META_SONG_GUITAR_TAB:
		case META_QUERY_DATA_TYPES:
		case META_QUERY_NO_CACHE:
        default:
            break;
    }
}


static void pref_construct(GtkWidget *con)
{
    GtkWidget *frame,*vbox;
    GtkWidget *a_a_ck, *a_b_ck, *a_s_ck, *c_a_ck, *a_i_ck, *s_s_ck, *s_g_ck;

    /**
     * Enable/Disable checkbox
     */
    frame = gtk_frame_new("");
    gtk_label_set_markup(GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(frame))), "<b>Fetch</b>");
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    vbox = gtk_vbox_new(FALSE,6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    /* Fetch artist art */
    a_a_ck = gtk_check_button_new_with_label(_("Artist images"));    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(a_a_ck),
        cfg_get_single_value_as_int_with_default(config, "cover-lastfm", "fetch-art-artist", TRUE)); 
    gtk_box_pack_start(GTK_BOX(vbox), a_a_ck, FALSE, TRUE, 0);
    g_signal_connect(G_OBJECT(a_a_ck), "toggled", G_CALLBACK(pref_enable_fetch), GINT_TO_POINTER(META_ARTIST_ART));

    /* Fetch artist text*/
    a_b_ck = gtk_check_button_new_with_label(_("Artist biography"));    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(a_b_ck),
        cfg_get_single_value_as_int_with_default(config, "cover-lastfm", "fetch-biography-artist", TRUE)); 
    gtk_box_pack_start(GTK_BOX(vbox), a_b_ck, FALSE, TRUE, 0);
    g_signal_connect(G_OBJECT(a_b_ck), "toggled", G_CALLBACK(pref_enable_fetch), GINT_TO_POINTER(META_ARTIST_TXT));

    /* Fetch similar artists */
    a_s_ck = gtk_check_button_new_with_label(_("Similar artists"));    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(a_s_ck),
        cfg_get_single_value_as_int_with_default(config, "cover-lastfm", "fetch-similar-artist", TRUE)); 
    gtk_box_pack_start(GTK_BOX(vbox), a_s_ck, FALSE, TRUE, 0);
    g_signal_connect(G_OBJECT(a_s_ck), "toggled", G_CALLBACK(pref_enable_fetch), GINT_TO_POINTER(META_ARTIST_SIMILAR));

    /* Fetch album art */
    c_a_ck = gtk_check_button_new_with_label(_("Album cover"));    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c_a_ck),
        cfg_get_single_value_as_int_with_default(config, "cover-lastfm", "fetch-art-album", TRUE)); 
    gtk_box_pack_start(GTK_BOX(vbox), c_a_ck, FALSE, TRUE, 0);
    g_signal_connect(G_OBJECT(c_a_ck), "toggled", G_CALLBACK(pref_enable_fetch), GINT_TO_POINTER(META_ALBUM_ART));

    /* Fetch album info */
    a_i_ck = gtk_check_button_new_with_label(_("Album information"));    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(a_i_ck),
        cfg_get_single_value_as_int_with_default(config, "cover-lastfm", "fetch-album-info", TRUE)); 
    gtk_box_pack_start(GTK_BOX(vbox), a_i_ck, FALSE, TRUE, 0);
    g_signal_connect(G_OBJECT(c_a_ck), "toggled", G_CALLBACK(pref_enable_fetch), GINT_TO_POINTER(META_ALBUM_TXT));

    /* Fetch similar songs */
    s_s_ck = gtk_check_button_new_with_label(_("Similar songs"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(s_s_ck),
        cfg_get_single_value_as_int_with_default(config, "cover-lastfm", "fetch-similar-song", TRUE)); 
    gtk_box_pack_start(GTK_BOX(vbox), s_s_ck, FALSE, TRUE, 0);
    g_signal_connect(G_OBJECT(s_s_ck), "toggled", G_CALLBACK(pref_enable_fetch), GINT_TO_POINTER(META_SONG_SIMILAR));

	/* Fetch similar genre */
	s_g_ck = gtk_check_button_new_with_label(_("Similar genres"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(s_g_ck),
		cfg_get_single_value_as_int_with_default(config, "cover-lastfm", "fetch-similar-genre", TRUE));
	gtk_box_pack_start(GTK_BOX(vbox), s_g_ck, FALSE, TRUE, 0);
	g_signal_connect(G_OBJECT(s_g_ck), "toggled", G_CALLBACK(pref_enable_fetch), GINT_TO_POINTER(META_GENRE_SIMILAR));


    if(!lastfm_get_enabled()) {
        gtk_widget_set_sensitive(GTK_WIDGET(vbox), FALSE);
    }

    gtk_widget_show_all(frame);
    gtk_container_add(GTK_CONTAINER(con), frame);
}
/**
 * Similarsong  
 */
static void similar_song_callback(const GEADAsyncHandler *handle, GEADStatus status, gpointer user_data)
{
	Query *q = (Query *)user_data;
	GList *list = NULL;
	if(status == GEAD_PROGRESS) return;
	if(status == GEAD_DONE)
	{
		goffset size=0;
		const char *data = gmpc_easy_handler_get_data(handle, &size);
		MetaData *mtd = __lastfm_art_xml_get_song_similar((char *)data,(gint)size);
        if(mtd) {
            list = g_list_append(list, mtd);
        }
    }
	q->callback(list, q->user_data);
	g_slice_free(Query, q);
}
/**
 * Similar artist
 */
static void similar_artist_callback(const GEADAsyncHandler *handle, GEADStatus status, gpointer user_data)
{
	Query *q = (Query *)user_data;
	GList *list = NULL;
	if(status == GEAD_PROGRESS) return;
	if(status == GEAD_DONE)
	{
		goffset size=0;
		const gchar* data = gmpc_easy_handler_get_data(handle, &size);
		MetaData *mtd = __lastfm_art_xml_get_artist_similar(data, size);
        if(mtd){
            list = g_list_append(list, mtd);
        }
    }
	q->callback(list, q->user_data);
	g_slice_free(Query, q);
}
/**
 * Similar genre
 */
static void similar_genre_callback(const GEADAsyncHandler *handle, GEADStatus status, gpointer user_data)
{
	Query* q = (Query*) user_data;
	GList* list = NULL;
	if(status == GEAD_PROGRESS)
		return;

	if(status == GEAD_DONE)
	{
		goffset size = 0;
		const gchar* data = gmpc_easy_handler_get_data(handle, &size);
		MetaData* mtd = __lastfm_art_xml_get_genre_similar(data, size);
		if(mtd) {
			list = g_list_append(list, mtd);
		}
	}
	q->callback(list, q->user_data);
	g_slice_free(Query, q);
}
/****
 * Get biograpy  new style
 */

static void biography_callback(const GEADAsyncHandler *handle, GEADStatus status, gpointer user_data)
{
	Query *q = (Query *)user_data;
	GList *list = NULL;
	if(status == GEAD_PROGRESS) return;
	if(status == GEAD_DONE)
	{
		goffset size=0;
		const gchar *data = gmpc_easy_handler_get_data(handle, &size);
		char* url = __lastfm_art_xml_get_artist_bio(data, size);
		if(url)
		{
			MetaData *mtd = meta_data_new();
			mtd->type = META_ARTIST_TXT;
			mtd->plugin_name = lastfm_plugin.name;
			mtd->content_type = META_DATA_CONTENT_HTML;
			mtd->content = url;
			mtd->size = -1;
			list = g_list_append(list, mtd);
		}

	}
	q->callback(list, q->user_data);
	g_slice_free(Query, q);
}
/****
 * Get album images new style
 */

static void album_image_callback(const GEADAsyncHandler *handle,
		GEADStatus status, gpointer user_data)
{
	Query *q = (Query *)user_data;
	GList *list = NULL;
	if(status == GEAD_PROGRESS) return;
	if(status == GEAD_DONE)
	{
		goffset size=0;
		const gchar* data = gmpc_easy_handler_get_data(handle, &size);
		list = __lastfm_art_xml_get_image(data, size, "album", META_ALBUM_ART);
	}
	q->callback(list, q->user_data);
	g_slice_free(Query, q);
}
/****
 * Get album info new style
 */

static void album_info_callback(const GEADAsyncHandler *handle,
		GEADStatus status, gpointer user_data)
{
	Query *q = (Query *)user_data;
	GList *list = NULL;
	if(status == GEAD_PROGRESS) return;
	if(status == GEAD_DONE)
	{
		goffset size=0;
		const gchar* data = gmpc_easy_handler_get_data(handle, &size);
		list = __lastfm_art_xml_get_album_info(data, size);
	}
	q->callback(list, q->user_data);
	g_slice_free(Query, q);
}
/**
 * Get artist image new style
 */
static void artist_image_callback(const GEADAsyncHandler *handle,
		GEADStatus status, gpointer user_data)
{
	Query *q = (Query *)user_data;
	GList *list = NULL;
	if(status == GEAD_PROGRESS) return;
	if(status == GEAD_DONE)
	{
		goffset size=0;
		const gchar* data = gmpc_easy_handler_get_data(handle, &size);
		list = __lastfm_art_xml_get_artist_image(data, size, META_ARTIST_ART);

	}
	q->callback(list, q->user_data);
	g_slice_free(Query, q);
}

static void lastfm_fetch_get_uris(mpd_Song *song,
		MetaDataType type,
		void (*callback)(GList *list, gpointer data),
		gpointer user_data)
{
	g_debug("Query last.fm api v2");
    if(song->artist != NULL && type == META_ARTIST_ART && 
			cfg_get_single_value_as_int_with_default(config, "cover-lastfm", "fetch-art-artist", TRUE))
	{
		char furl[1024];
		gchar *artist = gmpc_easy_download_uri_escape(song->artist);
		Query *q = g_slice_new0(Query);

		q->callback = callback;
		q->user_data = user_data;
		snprintf(furl,1024,LASTFM_API_ROOT"?method=artist.getImages&artist=%s&api_key=%s", artist,LASTFM_API_KEY);
		g_debug("url: '%s'", furl);
		gmpc_easy_async_downloader(furl, artist_image_callback, q); 
		g_free(artist);
		return;
	}
	else if (song->artist != NULL && song->album != NULL &&  type == META_ALBUM_ART && 
			cfg_get_single_value_as_int_with_default(config, "cover-lastfm", "fetch-art-album", TRUE))
	{
		char furl[1024];
		gchar *artist = gmpc_easy_download_uri_escape(song->artist);
		gchar *album = gmpc_easy_download_uri_escape(song->album);
		Query *q = g_slice_new0(Query);

		q->callback = callback;
		q->user_data = user_data;
		snprintf(furl,1024,LASTFM_API_ROOT"?method=album.getinfo&artist=%s&album=%s&api_key=%s", artist,album,LASTFM_API_KEY);
		g_debug("url: '%s'", furl);
		gmpc_easy_async_downloader(furl, album_image_callback, q); 
		g_free(artist);
		g_free(album);
		return;
	}

	else if (song->artist != NULL && song->album != NULL &&  type == META_ALBUM_TXT && 
			cfg_get_single_value_as_int_with_default(config, "cover-lastfm", "fetch-album-info", TRUE))
	{
		char furl[1024];
		gchar *artist = gmpc_easy_download_uri_escape(song->artist);
		gchar *album = gmpc_easy_download_uri_escape(song->album);
		Query *q = g_slice_new0(Query);

		q->callback = callback;
		q->user_data = user_data;
		snprintf(furl,1024,LASTFM_API_ROOT"?method=album.getinfo&artist=%s&album=%s&api_key=%s", artist,album,LASTFM_API_KEY);
		g_debug("url: '%s'", furl);
		gmpc_easy_async_downloader(furl, album_info_callback, q); 
		g_free(artist);
		g_free(album);
		return;
	}

	/* Fetch artist info */
	else if (song->artist != NULL && type == META_ARTIST_TXT && 
			cfg_get_single_value_as_int_with_default(config, "cover-lastfm", "fetch-biography-artist", TRUE))
	{

		char furl[1024];
		gchar *artist = gmpc_easy_download_uri_escape(song->artist);
		Query *q = g_slice_new0(Query);

		q->callback = callback;
		q->user_data = user_data;
		snprintf(furl,1024, LASTFM_API_ROOT"?method=artist.getinfo&artist=%s&api_key=%s", artist,LASTFM_API_KEY);
		g_debug("url: '%s'", furl);
		gmpc_easy_async_downloader(furl, biography_callback, q); 
		g_free(artist);

		return;
	}
    else if (song->artist != NULL && type == META_ARTIST_SIMILAR
            && cfg_get_single_value_as_int_with_default(config, "cover-lastfm", "fetch-similar-artist", TRUE))
    {
        char furl[1024];
        char *artist = gmpc_easy_download_uri_escape(song->artist);
		Query *q = g_slice_new0(Query);

		q->callback = callback;
		q->user_data = user_data;
        snprintf(furl,1024,LASTFM_API_ROOT"?method=artist.getsimilar&artist=%s&api_key=%s", artist,LASTFM_API_KEY);
		g_debug("url: '%s'", furl);
        g_free(artist);
		gmpc_easy_async_downloader(furl, similar_artist_callback, q); 
        return;
	}
	else if (song->genre != NULL && type == META_GENRE_SIMILAR
			&& cfg_get_single_value_as_int_with_default(config, "cover-lastfm", "fetch-similar-genre", TRUE))
	{
		gchar* genre = gmpc_easy_download_uri_escape(song->genre);
		gchar* furl = g_strdup_printf(LASTFM_API_ROOT"?method=tag.getsimilar&tag=%s&api_key=%s", genre, LASTFM_API_KEY);
		Query *q = g_slice_new0(Query);
		q->callback = callback;
		q->user_data = user_data;

		g_debug("url: '%s'", furl);
		gmpc_easy_async_downloader(furl, similar_genre_callback, q);
		g_free(genre);
		g_free(furl);

		return;
    }else if (song->title != NULL && song->artist != NULL && 
			type == META_SONG_SIMILAR &&
			cfg_get_single_value_as_int_with_default(config, "cover-lastfm", "fetch-similar-song", TRUE))
	{

        char furl[1024];
        char *artist = gmpc_easy_download_uri_escape(song->artist);
        char *title =  gmpc_easy_download_uri_escape(song->title);
		Query *q = g_slice_new0(Query);

		q->callback = callback;
		q->user_data = user_data;
        snprintf(furl,1024,LASTFM_API_ROOT"?method=track.getsimilar&artist=%s&track=%s&api_key=%s", artist,title,LASTFM_API_KEY);
		g_debug("url: '%s'", furl);
        g_free(artist);
		gmpc_easy_async_downloader(furl, similar_song_callback, q); 
        return;
    }

	callback(NULL, user_data);
}
static gmpcPrefPlugin lf_pref = {
    .construct      = pref_construct,
    .destroy        = pref_destroy
};

/**
 * Metadata Plugin
 */
static gmpcMetaDataPlugin lf_cover = {
	.get_priority   = lastfm_fetch_cover_priority,
	.set_priority   = lastfm_fetch_cover_priority_set,
	.get_metadata = lastfm_fetch_get_uris
};

gmpcPlugin lastfm_plugin = {
	.name           = N_("Last FM metadata fetcher (internal)"),
	.version        = {0,20,0},
	.plugin_type    = GMPC_PLUGIN_META_DATA|GMPC_INTERNALL,
	.metadata       = &lf_cover,
    .pref           = &lf_pref,
	.get_enabled    = lastfm_get_enabled,
	.set_enabled    = lastfm_set_enabled
};

/* vim:set ts=4 sw=4: */
