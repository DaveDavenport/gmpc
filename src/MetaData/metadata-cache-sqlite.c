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

#include <stdio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <sqlite3.h>
#include "main.h"
#include "metadata.h"
#include "config1.h"
#include "metadata-cache-sqlite.h"

#define CACHE_NAME "Metadata cache sqlite"
#define MDC_LOG_DOMAIN "MetaDataCacheSqlite"

static const char meta_data_sqlite_sql_create[] =
"CREATE TABLE IF NOT EXISTS metadata("
"   type	INT NOT NULL,  "
"   key_a   VARCHAR,"
"   key_b   VARCHAR,"
"   contenttype INT NOT NULL,"
"   content VARCHAR "
");"
"CREATE INDEX IF NOT EXISTS"
" meta_data_sqlite_content ON metadata(type,key_a, key_b);"
"CREATE INDEX IF NOT EXISTS"
" meta_data_sqlite_content_type ON metadata(type);"
"";

enum meta_data_sqlite_sql {
	META_DATA_SQL_GET,
	META_DATA_SQL_GET_KEYA,
	META_DATA_SQL_SET,
	META_DATA_SQL_UPDATE,
	META_DATA_SQL_DELETE,
	META_DATA_SQL_CLEANUP,
	META_DATA_SQL_LIST_START,
	META_DATA_SQL_LIST_END,
	META_DATA_SQL_SET_SYNCHRONOUS,
	META_DATA_SQL_CHECK_INTEGRETY
};

static const char *const meta_data_sqlite_sql[] = {
	[META_DATA_SQL_GET] =
		"SELECT contenttype,content FROM metadata WHERE type=? AND key_a=? AND key_b=?",
	[META_DATA_SQL_GET_KEYA] =
		"SELECT contenttype,content FROM metadata INDEXED BY meta_data_sqlite_content_type WHERE type=? AND key_a=?",
	[META_DATA_SQL_SET] =
		"INSERT INTO metadata(contenttype,content,type,key_a, key_B) VALUES(?,?,?,?,?)",
	[META_DATA_SQL_UPDATE] =
		"UPDATE metadata SET contenttype=?,content=? WHERE type=? AND key_a=? AND key_b=?",
	[META_DATA_SQL_DELETE] =
		"DELETE FROM metadata WHERE type=? AND key_a=? AND key_b=?",
	/* Clean all entries that are set 'UNAVAILABLE' */
	[META_DATA_SQL_CLEANUP] =
		"DELETE FROM metadata WHERE contenttype=0",
	[META_DATA_SQL_LIST_START] =
		"BEGIN TRANSACTION",
	[META_DATA_SQL_LIST_END] =
		"COMMIT TRANSACTION",
	[META_DATA_SQL_SET_SYNCHRONOUS] = 
		"PRAGMA synchronous = 0",
	[META_DATA_SQL_CHECK_INTEGRETY] = 
		"PRAGMA quick_check;"
};

static sqlite3 *meta_data_sqlite_db;
static sqlite3_stmt *meta_data_sqlite_stmt[G_N_ELEMENTS(meta_data_sqlite_sql)];
static void sqlite_set_synchronous(void)
{
	sqlite3_stmt *const stmt = meta_data_sqlite_stmt[META_DATA_SQL_SET_SYNCHRONOUS];
	int ret;
	sqlite3_reset(stmt);
	do{
		ret = sqlite3_step(stmt);
	}while(ret == SQLITE_BUSY);
	if (ret != SQLITE_DONE) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"%s: sqlite3_step() failed: %s",__FUNCTION__,
				sqlite3_errmsg(meta_data_sqlite_db));
		return;
	}
	sqlite3_reset(stmt);
}
static gboolean sqlite_check_integrity(void)
{
	int database_check = FALSE;
	sqlite3_stmt *const stmt = meta_data_sqlite_stmt[META_DATA_SQL_CHECK_INTEGRETY];
	int ret;
	do{
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW)
		{
			const gchar *value = (const gchar *)sqlite3_column_text(stmt, 0);
			if(strcmp(value, "ok") == 0){
				database_check = TRUE;
			}else{
				g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "%s: Sqlite database integrity check failed: %s\n",
						__FUNCTION__,
						value);
			}
		}
	}while(ret == SQLITE_BUSY || ret == SQLITE_ROW);
	if (ret != SQLITE_DONE) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"%s: sqlite3_step() failed: %s",__FUNCTION__,
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}
	sqlite3_reset(stmt);
	return database_check;
}
static void sqlite_cleanup(void)
{
	sqlite3_stmt *const stmt = meta_data_sqlite_stmt[META_DATA_SQL_CLEANUP];
	int ret;
	sqlite3_reset(stmt);
	do{
		ret = sqlite3_step(stmt);
	}while(ret == SQLITE_BUSY);
	if (ret != SQLITE_DONE) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"%s: sqlite3_step() failed: %s",__FUNCTION__,
				sqlite3_errmsg(meta_data_sqlite_db));
		return;
	}
	sqlite3_reset(stmt);
}
static void sqlite_list_start(void)
{
	sqlite3_stmt *const stmt = meta_data_sqlite_stmt[META_DATA_SQL_LIST_START];
	int ret;
	sqlite3_reset(stmt);
	do{
		ret = sqlite3_step(stmt);
	}while(ret == SQLITE_BUSY);
	if (ret != SQLITE_DONE) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"%s: sqlite3_step() failed: %s",__FUNCTION__,
				sqlite3_errmsg(meta_data_sqlite_db));
		return;
	}
	sqlite3_reset(stmt);
}

static void sqlite_list_end(void)
{
	sqlite3_stmt *const stmt = meta_data_sqlite_stmt[META_DATA_SQL_LIST_END];
	int ret;
	sqlite3_reset(stmt);
	do{
		ret = sqlite3_step(stmt);
	}while(ret == SQLITE_BUSY);
	if (ret != SQLITE_DONE) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"%s: sqlite3_step() failed: %s",__FUNCTION__,
				sqlite3_errmsg(meta_data_sqlite_db));
		return ;
	}
	sqlite3_reset(stmt);
}
static gboolean sqlite_delete_value(MetaDataType type,const char *key_a,const char *key_b)
{
	sqlite3_stmt *const stmt = meta_data_sqlite_stmt[META_DATA_SQL_DELETE];
	int ret;
	sqlite3_reset(stmt);

	ret = sqlite3_bind_int(stmt, 1, type);
	if (ret != SQLITE_OK) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"sqlite3_bind_int() failed: %s",
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}

	ret = sqlite3_bind_text(stmt, 2, key_a,-1, NULL);
	if (ret != SQLITE_OK) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"sqlite3_bind_text() failed: %s",
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}

	ret = sqlite3_bind_text(stmt, 3, key_b,-1, NULL);
	if (ret != SQLITE_OK) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"sqlite3_bind_text() failed: %s",
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}
	do {
		ret = sqlite3_step(stmt);
	} while (ret == SQLITE_BUSY);

	if (ret != SQLITE_DONE) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"%s: sqlite3_step() failed: %s",__FUNCTION__,
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}


	ret = sqlite3_changes(meta_data_sqlite_db);

	sqlite3_reset(stmt);
	sqlite3_clear_bindings(stmt);
	return ret > 0;
}

static gboolean sqlite_update_value(MetaDataType type,const char *key_a,const char *key_b, MetaDataContentType content_type, const char *content)
{
	sqlite3_stmt *const stmt = meta_data_sqlite_stmt[META_DATA_SQL_UPDATE];
	int ret;
	sqlite3_reset(stmt);

	ret = sqlite3_bind_int(stmt, 1, content_type);
	if (ret != SQLITE_OK) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"sqlite3_bind_int() failed: %s",
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}
	ret = sqlite3_bind_text(stmt, 2, content,-1, NULL);
	if (ret != SQLITE_OK) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"sqlite3_bind_text() failed: %s",
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}

	ret = sqlite3_bind_int(stmt, 3, type);
	if (ret != SQLITE_OK) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"sqlite3_bind_int() failed: %s",
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}

	ret = sqlite3_bind_text(stmt, 4, key_a,-1, NULL);
	if (ret != SQLITE_OK) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"sqlite3_bind_text() failed: %s",
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}

	ret = sqlite3_bind_text(stmt, 5, key_b,-1, NULL);
	if (ret != SQLITE_OK) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"sqlite3_bind_text() failed: %s",
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}
	do {
		ret = sqlite3_step(stmt);
	} while (ret == SQLITE_BUSY);

	if (ret != SQLITE_DONE) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"%s: sqlite3_step() failed: %s",__FUNCTION__,
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}

	ret = sqlite3_changes(meta_data_sqlite_db);

	sqlite3_reset(stmt);
	sqlite3_clear_bindings(stmt);
	g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Updated entry: %i-%s-%s with status: %i", type, key_a, key_b, ret>0);
	return ret > 0;
}

static gboolean sqlite_set_value(MetaDataType type,const char *key_a,const char *key_b, MetaDataContentType content_type, const char *content)
{
	sqlite3_stmt *const stmt = meta_data_sqlite_stmt[META_DATA_SQL_SET];
	int ret;
	sqlite3_reset(stmt);

	ret = sqlite3_bind_int(stmt, 1, content_type);
	if (ret != SQLITE_OK) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"sqlite3_bind_int() failed: %s",
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}
	ret = sqlite3_bind_text(stmt, 2, content,-1, NULL);
	if (ret != SQLITE_OK) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"sqlite3_bind_text() failed: %s",
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}

	ret = sqlite3_bind_int(stmt, 3, type);
	if (ret != SQLITE_OK) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"sqlite3_bind_int() failed: %s",
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}

	ret = sqlite3_bind_text(stmt, 4, key_a,-1, NULL);
	if (ret != SQLITE_OK) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"sqlite3_bind_text() failed: %s",
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}

	ret = sqlite3_bind_text(stmt, 5, key_b,-1, NULL);
	if (ret != SQLITE_OK) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"sqlite3_bind_text() failed: %s",
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}
	do {
		ret = sqlite3_step(stmt);
	} while (ret == SQLITE_BUSY);

	if (ret != SQLITE_DONE) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"%s: sqlite3_step() failed %s::%s content:'%s' : %s",__FUNCTION__, key_a, key_b,content,
				sqlite3_errmsg(meta_data_sqlite_db));
		return FALSE;
	}


	ret = sqlite3_changes(meta_data_sqlite_db);

	sqlite3_reset(stmt);
	sqlite3_clear_bindings(stmt);
	return ret > 0;
}

static MetaData *sqlite_get_value(MetaDataType type,const char *key_a,const char *key_b)
{
	sqlite3_stmt *stmt;
	
	int ret;
	MetaData *met = NULL;
	if(key_b == NULL) {
		stmt = meta_data_sqlite_stmt[META_DATA_SQL_GET_KEYA];
	}else {
		stmt = meta_data_sqlite_stmt[META_DATA_SQL_GET];
	}
	sqlite3_reset(stmt);

	ret = sqlite3_bind_int(stmt, 1, type);
	if (ret != SQLITE_OK) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"sqlite3_bind_int() failed: %s",
				sqlite3_errmsg(meta_data_sqlite_db));
		return NULL;
	}

	ret = sqlite3_bind_text(stmt, 2, key_a,-1, NULL);
	if (ret != SQLITE_OK) {
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"sqlite3_bind_text() failed: %s",
				sqlite3_errmsg(meta_data_sqlite_db));
		return NULL;
	}
	if(key_b != NULL)
	{

		ret = sqlite3_bind_text(stmt, 3, key_b,-1, NULL);
		if (ret != SQLITE_OK) {
			g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"sqlite3_bind_text() failed: %s",
					sqlite3_errmsg(meta_data_sqlite_db));
			return NULL;
		}
	}

	do {
		ret = sqlite3_step(stmt);
		switch(ret) {
			case SQLITE_ROW:
				if(!met) {
					met = meta_data_new();
					met->type = type;
					met->plugin_name = "Metadata Cache";
					met->content_type = sqlite3_column_int(stmt, 0);
				}
				if(meta_data_is_text_vector(met)) {
					gchar *value = (gchar *) sqlite3_column_text(stmt, 1);
					met->size += 1;
					met->content = g_realloc((gchar **)met->content, sizeof(gchar *)*(met->size+1));
					((gchar **)met->content)[met->size-1] = g_strdup(value);
					((gchar **)met->content)[met->size] = NULL;
				}else if (meta_data_is_text_list(met)) {
					gchar *value = (gchar *) sqlite3_column_text(stmt, 1);
					met->content = (void *)g_list_append((GList *)met->content, g_strdup(value));
					met->size = 0;
				}else{
					if(meta_data_is_uri(met) ||
							meta_data_is_text(met)||
							meta_data_is_html(met))
					{
						const guchar *value = sqlite3_column_text(stmt, 1);
						met->content = g_strdup((gchar *)value);
						met->size = -1;
					}
					else if (meta_data_is_raw(met))
					{
						const guchar *value = sqlite3_column_text(stmt, 1);
						gsize size;
						met->content = g_base64_decode((gchar *)value, &size);
						met->size = size;
					}
					/* indicate we don't query anymore */
					ret = SQLITE_DONE;
				}
			case SQLITE_DONE:
			case SQLITE_BUSY:
				break;
			default:

				g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"%s: sqlite3_step() failed: %s",__FUNCTION__,
						sqlite3_errmsg(meta_data_sqlite_db));
				if(met) meta_data_free(met);
				sqlite3_reset(stmt);
				sqlite3_clear_bindings(stmt);
				return NULL;
		}
	} while (ret != SQLITE_DONE);


	sqlite3_reset(stmt);
	sqlite3_clear_bindings(stmt);
	return met;
}


MetaDataResult meta_data_sqlite_get_from_cache(mpd_Song *song, MetaDataType type, MetaData **met)
{
	GTimer *t = g_timer_new();
	const char *key_a= "", *key_b = NULL; 
	gboolean have_b = FALSE;
	if(type == META_ALBUM_ART){
		key_a = song->artist;
		key_b = song->album;
		have_b = TRUE;
	}else if(type == META_ALBUM_TXT){
		key_a = song->artist;
		key_b = song->album;
		have_b = TRUE;
	}else if (type == META_ARTIST_ART || type == META_ARTIST_TXT || type == META_ARTIST_SIMILAR) {
		key_a = song->artist;
	}else if (type == META_SONG_TXT || type == META_SONG_SIMILAR || type == META_SONG_GUITAR_TAB) {
		key_a = song->artist;
		key_b = song->title;
		have_b = TRUE;
	}else if (type == META_GENRE_SIMILAR) {
		key_a = song->genre;
	}
	if(key_a == NULL || (have_b && key_b == NULL)) {
		*met = meta_data_new(); (*met)->type = type;
		(*met)->plugin_name = CACHE_NAME; (*met)->content_type = META_DATA_CONTENT_EMPTY;
		g_timer_stop(t);
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_INFO,"get cache took: %.6f", g_timer_elapsed(t, NULL));
		g_timer_destroy(t);
		return META_DATA_UNAVAILABLE;
	}
	if(!g_utf8_validate(key_a, -1, NULL)){
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"Key_a is not valid utf-8");
		(*met) = meta_data_new(); (*met)->type = type;
		(*met)->plugin_name = CACHE_NAME; (*met)->content_type = META_DATA_CONTENT_EMPTY;

		g_timer_stop(t);
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_INFO,"get cache took: %.6f", g_timer_elapsed(t, NULL));
		g_timer_destroy(t);
		return META_DATA_UNAVAILABLE;
	}

	if(key_b != NULL && !g_utf8_validate(key_b, -1, NULL)){
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"Key_b is not valid utf-8");
		(*met) = meta_data_new(); (*met)->type = type;
		(*met)->plugin_name = CACHE_NAME; (*met)->content_type = META_DATA_CONTENT_EMPTY;

		g_timer_stop(t);
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_INFO,"get cache took: %.6f", g_timer_elapsed(t, NULL));
		g_timer_destroy(t);
		return META_DATA_UNAVAILABLE;
	}
	*met = sqlite_get_value(type,key_a, key_b);
	if((*met) == NULL)
	{
		*met = meta_data_new();
		(*met)->type = type;
		(*met)->plugin_name = "Metadata Cache";
		(*met)->content_type = META_DATA_CONTENT_EMPTY;

		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "No entry found for: %i-%s-%s", type, key_a, (key_b)?"(null)":key_b);

		g_timer_stop(t);
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_INFO,"get cache took: %.6f", g_timer_elapsed(t, NULL));
		g_timer_destroy(t);
		return META_DATA_FETCHING;
	}
	if(meta_data_is_empty(*met))
	{
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Empty entry found for: %i-%s-%s", type, key_a,  (key_b)?"(null)":key_b);

		g_timer_stop(t);
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_INFO,"get cache took: %.6f", g_timer_elapsed(t, NULL));
		g_timer_destroy(t);
		return META_DATA_UNAVAILABLE;
	}
	if(meta_data_is_uri(*met))
	{
		const gchar *path = meta_data_get_uri(*met);
		if(!g_file_test(path, G_FILE_TEST_EXISTS))
		{
			sqlite_delete_value(type, key_a, key_b);
			(*met)->content_type = META_DATA_CONTENT_EMPTY;
			g_free((gchar *)((*met)->content));
			(*met)->content = NULL;
			(*met)->size = 0;

			g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Uri entry found for but invalid: %i-%s-%s", type, key_a,(key_b)?"(null)":key_b);

			g_timer_stop(t);
			g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_INFO,"get cache took: %.6f", g_timer_elapsed(t, NULL));
			g_timer_destroy(t);
			return META_DATA_FETCHING;
		}
	}
	g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Entry found for: %i-%s-%s", type, key_a,(key_b)?"(null)":key_b);
	g_timer_stop(t);
	g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_INFO,"get cache took: %.6f", g_timer_elapsed(t, NULL));
	g_timer_destroy(t);
	return META_DATA_AVAILABLE;
}
void meta_data_sqlite_set_cache_real(mpd_Song *song, MetaDataResult result, MetaData *met)
{
	GTimer *t = g_timer_new();

	const char *key_a= "", *key_b = "";
	if(!song) return;
	if((met)->type == META_ALBUM_ART){
		key_a = song->artist;
		key_b = song->album;
	}else if((met)->type == META_ALBUM_TXT){
		key_a = song->artist;
		key_b = song->album;
	}else if ((met)->type == META_ARTIST_ART || (met)->type == META_ARTIST_TXT || (met)->type == META_ARTIST_SIMILAR) {
		key_a = song->artist;
	}else if ((met)->type == META_SONG_TXT || (met)->type == META_SONG_SIMILAR || (met)->type == META_SONG_GUITAR_TAB) {
		key_a = song->artist;
		key_b = song->title;
	}else if ((met)->type == META_GENRE_SIMILAR) {
		key_a = song->genre;
	}
	if(key_a == NULL || key_b == NULL) {
		return ;
	}
	if(!g_utf8_validate(key_a, -1, NULL)){
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"Key_a is not valid utf-8");
		return ;
	}

	if(!g_utf8_validate(key_b, -1, NULL)){
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING,"Key_b is not valid utf-8");
		return ;
	}


	if(meta_data_is_uri(met)||
			meta_data_is_text(met)||
			meta_data_is_html(met)||
			meta_data_is_empty(met))
	{
		if(!sqlite_update_value(met->type, key_a, key_b, met->content_type, (const gchar *)met->content))
			sqlite_set_value(met->type, key_a, key_b, met->content_type, (const gchar *)met->content);
	}else if (meta_data_is_raw(met)) {
		gsize size;
		const guchar *udata = meta_data_get_raw(met, &size);
		gchar *data = g_base64_encode(udata, size);
		if(!sqlite_update_value(met->type, key_a, key_b, met->content_type, (const gchar *)data))
			sqlite_set_value(met->type, key_a, key_b, met->content_type, (const gchar *)data);
		g_free(data);
	}else if (meta_data_is_text_list(met)) {
		GList *iter;
		sqlite_delete_value(met->type, key_a, key_b);
		iter = g_list_first((GList *)meta_data_get_text_list(met));
		sqlite_list_start();
		for(;iter; iter = g_list_next(iter)){
			sqlite_set_value(met->type, key_a, key_b, met->content_type, (const gchar *)iter->data);
		}
		sqlite_list_end();
	}else if (meta_data_is_text_vector(met)) {
		int i = 0;
		const gchar **text_vector = meta_data_get_text_vector(met);
		sqlite_delete_value(met->type, key_a, key_b);
		sqlite_list_start();
		for(i=0;text_vector && text_vector[i];i++){
			sqlite_set_value(met->type, key_a, key_b, met->content_type, (const gchar *)text_vector[i]);
		}
		sqlite_list_end();
	}

	g_timer_stop(t);
	g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_INFO,"set cache took: %.6f", g_timer_elapsed(t, NULL));
	g_timer_destroy(t);
}
void meta_data_sqlite_set_cache(mpd_Song *song, MetaDataResult result, MetaData *met)
{
	mpd_Song *edited = rewrite_mpd_song(song, met->type);
	meta_data_sqlite_set_cache_real(edited, result, met);
	if(edited->artist)
	{
		if(strcmp(edited->artist, song->artist)!=0)
			meta_data_sqlite_set_cache_real(song, result, met);
	}
	mpd_freeSong(edited);
}

	static sqlite3_stmt *
meta_data_sqlite_prepare(const char *sql)
{
	int ret;
	sqlite3_stmt *stmt;

	ret = sqlite3_prepare_v2(meta_data_sqlite_db, sql, -1, &stmt, NULL);
	if (ret != SQLITE_OK)
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_ERROR,"sqlite3_prepare_v2() failed: %s",
				sqlite3_errmsg(meta_data_sqlite_db));

	return stmt;
}

void meta_data_sqlite_cache_init(void)
{
	int ret;
	unsigned i;
	gboolean database_valid = FALSE;
	gchar *url = gmpc_get_covers_path("covers.sql");
	INIT_TIC_TAC();
	do{
		
		ret = sqlite3_open(url, &meta_data_sqlite_db);
		if (ret != SQLITE_OK)
			g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_ERROR, "Failed to open sqlite database '%s': %s",
					url, sqlite3_errmsg(meta_data_sqlite_db));
		TEC("Sqlite3 open database");
		ret = sqlite3_exec(meta_data_sqlite_db, meta_data_sqlite_sql_create, NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_ERROR, "Failed to create metadata table: %s",
					sqlite3_errmsg(meta_data_sqlite_db));

		TEC("Sqlite3 create database");
		/* prepare the statements we're going to use */

		for (i = 0; i < G_N_ELEMENTS(meta_data_sqlite_sql); ++i) {
			g_assert(meta_data_sqlite_sql[i] != NULL);

			meta_data_sqlite_stmt[i] = meta_data_sqlite_prepare(meta_data_sqlite_sql[i]);
		}
		TEC("Prepare metadata statements");
		database_valid = sqlite_check_integrity();
		TEC("Checked integrity1");
		if(!database_valid){
			gchar buffer[128];
			gchar *new_uri = NULL;
			const time_t tt= time(NULL);
			struct tm *tm = localtime(&tt);
			strftime(buffer, 128, "%H%M-%d-%m-%Y", tm);
			new_uri = g_strdup_printf("%s-%s", url,buffer); 

			meta_data_sqlite_cache_destroy();
			g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "Database was invalid. Renaming db to %s and trying again.",
					new_uri);
			if(g_rename(url, new_uri) <0){
				g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_ERROR, "Failed to rename corrupt database: %s",
						url);
			}
			g_free(new_uri);
		}
		TEC("Checked integrity");
		g_log(MDC_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Database integrity check: valid");
	}while(!database_valid);
	g_free(url);

	sqlite_set_synchronous();
	TEC("Set synchronous()");
}


void meta_data_sqlite_cache_cleanup(void)
{
	sqlite_cleanup();
}
void meta_data_sqlite_cache_destroy(void)
{
	unsigned i;
	for (i = 0; i < G_N_ELEMENTS(meta_data_sqlite_stmt); ++i) {
		g_assert(meta_data_sqlite_stmt[i] != NULL);

		sqlite3_finalize(meta_data_sqlite_stmt[i]);
	}
	sqlite3_close(meta_data_sqlite_db);
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
