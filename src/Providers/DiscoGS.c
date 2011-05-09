/* gmpc-discogs (GMPC plugin)
 * Copyright (C) 2008-2009 Qball Cow <qball@sarine.nl>
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
/* gtk and lib stuff */
#include <gtk/gtk.h>
#include <config.h>
#include <glib.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "plugin.h"
#include "gmpc_easy_download.h"
#include "metadata.h"

#include <config.h>

#define DISCOGS_API_KEY "332020810c"
#define DISCOGS_API_ROOT "http://www.discogs.com/"

#define LOG_DOMAIN "Gmpc.Provider.DiscoGS"
gmpcPlugin discogs_plugin;

typedef struct ii
{
	MetaDataType type;
	mpd_Song *song;
	void (*callback)(GList *list, gpointer data);
	gpointer data;
	GList *releases;
	GList *uris;
}ii;

static ii *__create_ii(void)
{
	return g_slice_new0(ii);
}

static void __destroy_ii(ii *i)
{
	/* Free the song */
	if(i->song) mpd_freeSong(i->song);
	/* Free the uris */
	if(i->uris)
	{
		g_list_foreach(i->uris, (GFunc)meta_data_free, NULL);
		g_list_free(i->uris);
	}
	if(i->releases)
	{
		g_list_foreach(i->releases,(GFunc) g_free, NULL);
		g_list_free(i->releases);
	}
	g_slice_free(ii, i);
}

/* Get/Set enabled */
static int discogs_get_enabled(void)
{
	return cfg_get_single_value_as_int_with_default(config, "cover-discogs", "enable", TRUE);
}
static void discogs_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "cover-discogs", "enable", enabled);
}

/* Get/Set priority */
static int discogs_fetch_cover_priority(void)
{
	return cfg_get_single_value_as_int_with_default(config, "cover-discogs", "priority", 80);
}
static void discogs_fetch_cover_priority_set(int priority)
{
	cfg_set_single_value_as_int(config, "cover-discogs", "priority", priority);
}


static xmlNodePtr get_first_node_by_name(xmlNodePtr xml, const gchar *name)
{
	if(name == NULL) return NULL;
	if(xml)
	{
		xmlNodePtr c = xml->xmlChildrenNode;
		for(;c;c=c->next)
		{
			if(c->name && xmlStrEqual(c->name, BAD_CAST name))
				return c;
		}
	}
	return NULL;
}


static xmlNodePtr get_next_node_by_name(xmlNodePtr xml, const gchar *name)
{
	if(name == NULL) return NULL;
	if(xml)
	{
		xmlNodePtr c = xml->next;
		for(;c;c=c->next)
		{
			if(c->name && xmlStrEqual(c->name, BAD_CAST name))
				return c;
		}
	}
	return NULL;
}

static GList * __query_album_get_uri(mpd_Song *song, const gchar*data, gsize size)
{
	GList *retv = NULL;
	char *temp_b = NULL;
	xmlDocPtr doc;
	xmlNodePtr root;
	/**
	 * Get artist name
	 */
	if(size < 4 || strncmp(data, "<res",4))
	{
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Invalid XML");
		return NULL;
	}
	doc = xmlParseMemory(data,size);
	if(doc == NULL)
	{
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Failed to create xml document");
		return NULL;
	}
	temp_b = g_utf8_casefold(song->album,-1);
	root = xmlDocGetRootElement(doc);
	if(root)
	{
		/* loop through all albums */
		xmlNodePtr cur = get_first_node_by_name(root,"searchresults");
		if(cur)
		{
			xmlNodePtr cur2 = get_first_node_by_name(cur,"result");
			while(cur2)
			{
				xmlChar *temp  = xmlGetProp(cur2, (xmlChar *)"type");
				if(temp != NULL && xmlStrEqual(temp, (xmlChar *)"release"))
				{
					xmlNodePtr cur4 = get_first_node_by_name(cur2,"title");
					if(cur4)
					{
						xmlChar *title = xmlNodeGetContent(cur4);
						if(title)
						{
							char *temp_a = g_utf8_casefold((gchar *)title,-1);
								 /** Todo make this check fuzzy */
							if(strstr((char *)temp_a, temp_b))
							{
								xmlNodePtr cur3 = get_first_node_by_name(cur2,"uri");
								if(cur3)
								{
									xmlChar *xurl = xmlNodeGetContent(cur3);
									retv = g_list_prepend(retv,
									            g_strdup((char *)xurl));
									xmlFree(xurl);
								}
							}
							g_free(temp_a);
							xmlFree(title);
						}
					}
				}
				if(temp) xmlFree(temp);
				cur2 = get_next_node_by_name(cur2, "result");
			}
		}
	}
	xmlFreeDoc(doc);
    g_free(temp_b);
    return g_list_reverse(retv);
}


static GList *__query_album_get_uri_list(mpd_Song *song, const gchar*data, gsize size)
{
	GList *retv = NULL;
	xmlDocPtr doc;
	if(size < 4 || strncmp(data, "<res",4))
	{
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Invalid XML");
	}
	else
	{

		doc = xmlParseMemory(data,size);
		if(doc)
		{
			xmlNodePtr root = xmlDocGetRootElement(doc);
			if(root)
			{
				/* loop through all albums */
				xmlNodePtr cur = get_first_node_by_name(root,"release");
				if(cur)
				{
					xmlNodePtr cur2 = get_first_node_by_name(cur,"images");
					if(cur2)
					{
						xmlNodePtr cur3 = get_first_node_by_name(cur2,"image");
						while(cur3)
						{
							xmlChar *temp = xmlGetProp(cur3, BAD_CAST "type");

							if(temp)
							{
								if(xmlStrEqual(temp, BAD_CAST"primary"))
								{
									xmlChar *xurl = xmlGetProp(cur3, BAD_CAST"uri");
									MetaData *mtd = meta_data_new();
									mtd->type = META_ALBUM_ART; 
									mtd->plugin_name = discogs_plugin.name;
									mtd->content_type = META_DATA_CONTENT_URI;
									mtd->content = g_strdup((char *)xurl);
									mtd->size = -1;
									retv = g_list_prepend(retv,mtd);
									if(xurl) xmlFree(xurl);
								} else if(xmlStrEqual(temp, BAD_CAST"secondary"))
								{
									xmlChar *xurl = xmlGetProp(cur3, BAD_CAST"uri");
									MetaData *mtd = meta_data_new();
									mtd->type = META_ALBUM_ART; 
									mtd->plugin_name = discogs_plugin.name;
									mtd->content_type = META_DATA_CONTENT_URI;
									mtd->content = g_strdup((char *)xurl);
									mtd->size = -1;
									retv = g_list_append(retv,mtd);
									if(xurl) xmlFree(xurl);
								}

								xmlFree(temp);
							}
							cur3 = cur3->next;
						}
					}
				}
			}
			xmlFreeDoc(doc);
		}
	}
	return retv;
}


static void __query_get_album_art_uris(const GEADAsyncHandler *handle,
                            GEADStatus status,
                            gpointer user_data)
{
	ii *i = (ii *) user_data;
	if(status == GEAD_PROGRESS) return;
	if(status == GEAD_DONE)
	{
		goffset size;
		const gchar *data = gmpc_easy_handler_get_data(handle, &size);
		GList *list =  __query_album_get_uri_list(i->song, data, (gsize)size);
		i->uris = g_list_concat(i->uris, list);

		if(i->releases)
		{
			int j=0;
			char furl[1024];
			char *artist_uri = i->releases->data;
			/* Hack to fix bug in discogs api */
			for(j=strlen(artist_uri); artist_uri[j] != '/' && j > 0; j--);
			snprintf(furl,1024,DISCOGS_API_ROOT"release%s?f=xml&api_key=%s", 
			                &artist_uri[j],DISCOGS_API_KEY);
			i->releases = g_list_delete_link(i->releases, i->releases);
			g_free(artist_uri);
			gmpc_easy_async_downloader(furl, __query_get_album_art_uris, i);
			return;
		}

		i->callback(i->uris, i->data);
		/* callback takes over reference to list */
		i->uris = NULL;
		__destroy_ii(i);
		return;
	}
	i->callback(NULL, i->data);
	__destroy_ii(i);
}


static void __query_get_album_art(const GEADAsyncHandler *handle,
                        GEADStatus status,
                        gpointer user_data)
{
	ii *i = (ii *) user_data;
	if(status == GEAD_PROGRESS) return;
	if(status == GEAD_DONE)
	{
		goffset size;
		char furl[1024];
		const gchar *data = gmpc_easy_handler_get_data(handle, &size);
		i->releases =g_list_first( __query_album_get_uri(i->song, data, (gsize)(size)));
		if(i->releases)
		{
			int j=0;
			char *artist_uri = i->releases->data;
			/* Hack to fix bug in discogs api */
			for(j=strlen(artist_uri); artist_uri[j] != '/' && j > 0; j--);
			snprintf(furl,1024,DISCOGS_API_ROOT"release%s?f=xml&api_key=%s",
			                &artist_uri[j],DISCOGS_API_KEY);
			i->releases = g_list_delete_link(i->releases, i->releases);
			g_free(artist_uri);
			gmpc_easy_async_downloader(furl, __query_get_album_art_uris, i);
			return;
		}
	}
	i->callback(NULL, i->data);
	__destroy_ii(i);
}


/** other */
static void discogs_fetch_cover_album_art(ii *i)
{
	char furl[1024];
	char *artist = gmpc_easy_download_uri_escape(i->song->artist);
	char *album = gmpc_easy_download_uri_escape(i->song->album);
	g_log(LOG_DOMAIN, G_LOG_LEVEL_INFO, "Trying to fetch: %s:%s\n", artist, album);

	snprintf(furl,1024,DISCOGS_API_ROOT"search?type=all&f=xml&q=%s%%20%s&api_key=%s",
	                artist,album,DISCOGS_API_KEY);
	gmpc_easy_async_downloader(furl, __query_get_album_art, i);

	g_free(artist);
	g_free(album);
}


/**
 * Artist
 */

static gchar * __query_artist_get_uri(mpd_Song *song, const gchar*data, gsize size)
{
	char *retv = NULL;
	xmlDocPtr doc;
	/**
	 * Get artist name
	 */
	if(size < 4 || strncmp(data, "<res",4))
	{
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Invalid XML");
	}
	else
	{
		doc = xmlParseMemory(data,size);
		if(doc)
		{
			xmlNodePtr root = xmlDocGetRootElement(doc);
			if(root)
			{
				/* loop through all albums */
				xmlNodePtr cur = get_first_node_by_name(root,"exactresults");
				if(cur)
				{
					xmlNodePtr cur2 = get_first_node_by_name(cur,"result");
					if(cur2)
					{
						xmlNodePtr cur3 = get_first_node_by_name(cur2,"uri");
						if(cur3)
						{
							xmlChar *xurl = xmlNodeGetContent(cur3);
							retv = g_strdup((char *)xurl);
							xmlFree(xurl);
						}
					}
				}
			}
			xmlFreeDoc(doc);
		}

	}
	return retv;
}


static GList *__query_artist_get_uri_list(mpd_Song *song, const gchar*data, gsize size)
{
	GList *retv = NULL;
	xmlDocPtr doc;
	if(size < 4 || strncmp(data, "<res",4))
	{
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Invalid XML");
	}
	else
	{
		doc = xmlParseMemory(data,size);
		if(doc)
		{
			xmlNodePtr root = xmlDocGetRootElement(doc);
			if(root)
			{
				/* loop through all albums */
				xmlNodePtr cur = get_first_node_by_name(root,"artist");
				if(cur)
				{
					xmlNodePtr cur2 = get_first_node_by_name(cur,"images");
					if(cur2)
					{
						xmlNodePtr cur3 = get_first_node_by_name(cur2,"image");
						while(cur3 )
						{
							xmlChar *temp = xmlGetProp(cur3, (xmlChar *)"type");
							if(temp)
							{
								if(xmlStrEqual(temp, (xmlChar *)"primary"))
								{
									xmlChar *xurl = xmlGetProp(cur3, (xmlChar *)"uri");
									MetaData *mtd = meta_data_new();
									mtd->type = META_ARTIST_ART; mtd->plugin_name = discogs_plugin.name;
									mtd->content_type = META_DATA_CONTENT_URI;
									mtd->content = g_strdup((char *)xurl);
									mtd->size = -1;
									retv = g_list_prepend(retv,mtd);
									if(xurl) xmlFree(xurl);
								} else if(xmlStrEqual(temp, (xmlChar *)"secondary"))
								{
									xmlChar *xurl = xmlGetProp(cur3, (xmlChar *)"uri");
									MetaData *mtd = meta_data_new();
									mtd->type = META_ARTIST_ART; mtd->plugin_name = discogs_plugin.name;
									mtd->content_type = META_DATA_CONTENT_URI;
									mtd->content = g_strdup((char *)xurl);
									mtd->size = -1;
									retv = g_list_append(retv,mtd);
									if(xurl) xmlFree(xurl);
								}

								xmlFree(temp);
							}
							cur3 = cur3->next;
						}
					}
				}
			}
			xmlFreeDoc(doc);
		}

	}
	return retv;
}


static void __query_get_artist_art_uris(const GEADAsyncHandler *handle,
                        GEADStatus status,
                        gpointer user_data)
{
	ii *i = (ii *) user_data;
	if(status == GEAD_PROGRESS) return;
	if(status == GEAD_DONE)
	{
		goffset size;
		const gchar *data = gmpc_easy_handler_get_data(handle, &size);
		GList *list =  __query_artist_get_uri_list(i->song, data, (gsize)size);
		i->callback(list, i->data);
		__destroy_ii(i);
		return;
	}
	i->callback(NULL, i->data);
	__destroy_ii(i);
}


static void __query_get_artist_art(const GEADAsyncHandler *handle,
                        GEADStatus status,
                        gpointer user_data)
{
	ii *i = (ii *) user_data;
	if(status == GEAD_PROGRESS) return;
	if(status == GEAD_DONE)
	{
		gchar *artist_uri = NULL;
		goffset size;
		char furl[1024];
		const gchar *data = gmpc_easy_handler_get_data(handle, &size);
		artist_uri = __query_artist_get_uri(i->song, data, (gsize)(size));
		if(artist_uri)
		{
			/* Hack to fix bug in discogs api */
			if(strstr(artist_uri, "?") != NULL)
			{
				snprintf(furl,1024,"%s&f=xml&api_key=%s",
				            artist_uri,DISCOGS_API_KEY);
			}
			else
			{
				snprintf(furl,1024,"%s?f=xml&api_key=%s",
				        artist_uri,DISCOGS_API_KEY);
			}
			gmpc_easy_async_downloader(furl, __query_get_artist_art_uris, i);
			g_free(artist_uri);
			return;
		}
	}
	i->callback(NULL, i->data);
	__destroy_ii(i);
}


static void discogs_fetch_artist_art(ii *i)
{
	char *artist = gmpc_easy_download_uri_escape(i->song->artist);
	char furl[1024];
	snprintf(furl,1024,
	        DISCOGS_API_ROOT"search?type=all&f=xml&q=%s&api_key=%s", 
	        artist,DISCOGS_API_KEY);
	gmpc_easy_async_downloader(furl, __query_get_artist_art, i);
	g_free(artist);
	return ;
}


static void discogs_fetch_get_image(mpd_Song *song,MetaDataType type, 
                    void (*callback)(GList *list, gpointer data),
                    gpointer user_data)
{
    ii *i = NULL;
    /* check type, if enabled and if field is available */
	if(song->artist == NULL || discogs_get_enabled() == FALSE ||
	    (type != META_ARTIST_ART && type != META_ALBUM_ART))
	{
		callback(NULL, user_data);
		return ;
	}
	i = __create_ii();
	i->type = type;
	/* Make a copy */
	i->song = mpd_songDup(song);
	i->callback = callback;
	i->data = user_data;
	i->uris = NULL;
	if(type == META_ARTIST_ART)
	{
		discogs_fetch_artist_art(i);
		return ;
	}
	else if (type == META_ALBUM_ART && song->album)
	{
    	discogs_fetch_cover_album_art(i);
		return ;
	}
	/* cleanup */
    __destroy_ii(i);
	callback(NULL, user_data);
	return ;
}


static gmpcMetaDataPlugin discogs_metadata_object =
{
	.get_priority   = discogs_fetch_cover_priority,
	.set_priority   = discogs_fetch_cover_priority_set,
	.get_metadata   = discogs_fetch_get_image
};

gmpcPlugin discogs_plugin =
{
	.name           = ("DiscoGS Artist and Album Image Fetcher (internal)"),
	.version        = {0,21,0},
	.plugin_type    = GMPC_PLUGIN_META_DATA|GMPC_INTERNALL,
	.metadata       = &discogs_metadata_object,
	.get_enabled    = discogs_get_enabled,
	.set_enabled    = discogs_set_enabled,
};
