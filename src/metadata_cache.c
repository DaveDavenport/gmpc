#include <stdio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <sqlite3.h>
#include "main.h"
#include "metadata.h"
#include "config1.h"
#include "metadata_cache.h"

#define CACHE_NAME "Metadata cache"

static const char metadata_sql_create[] = 
    "CREATE TABLE IF NOT EXISTS metadata("
    "   type    INT NOT NULL,  "
    "   key_a   VARCHAR,"
    "   key_b   VARCHAR,"
    "   contenttype INT NOT NULL,"
    "   content VARCHAR "
    ");"
    "CREATE INDEX IF NOT EXISTS"
    " metadata_content ON metadata(type, key_a, key_b)"
    "";

enum metadata_sql {
    META_DATA_SQL_GET,
    META_DATA_SQL_SET,
    META_DATA_SQL_UPDATE,
    META_DATA_SQL_DELETE,
    META_DATA_SQL_LIST_START,
    META_DATA_SQL_LIST_END
};

static const char *const metadata_sql[] = {
    [META_DATA_SQL_GET] = 
    "SELECT contenttype,content FROM metadata WHERE type=? AND key_a=? AND key_b=?",
    [META_DATA_SQL_SET] = 
    "INSERT INTO metadata(contenttype,content,type,key_a, key_B) VALUES(?,?,?,?,?)",
	[META_DATA_SQL_UPDATE] =
	"UPDATE metadata SET contenttype=?,content=? WHERE type=? AND key_a=? AND key_b=?",
    [META_DATA_SQL_DELETE] = 
    "DELETE FROM metadata WHERE type=? AND key_a=? AND key_b=?",
    [META_DATA_SQL_LIST_START] = 
    "BEGIN TRANSACTION",
    [META_DATA_SQL_LIST_END] = 
    "COMMIT TRANSACTION",
};

static sqlite3 *metadata_db;
static sqlite3_stmt *metadata_stmt[G_N_ELEMENTS(metadata_sql)];

static void sqlite_list_start(void)
{
	sqlite3_stmt *const stmt = metadata_stmt[META_DATA_SQL_LIST_START];
	int ret;
    sqlite3_reset(stmt);
    do{
        ret = sqlite3_step(stmt);
    }while(ret == SQLITE_BUSY);
	if (ret != SQLITE_DONE) {
		g_warning("%s: sqlite3_step() failed: %s",__FUNCTION__,
			  sqlite3_errmsg(metadata_db));
		return;
	}
	sqlite3_reset(stmt);
}

static void sqlite_list_end(void)
{
	sqlite3_stmt *const stmt = metadata_stmt[META_DATA_SQL_LIST_END];
	int ret;
    sqlite3_reset(stmt);
    do{
        ret = sqlite3_step(stmt);
    }while(ret == SQLITE_BUSY);
	if (ret != SQLITE_DONE) {
		g_warning("%s: sqlite3_step() failed: %s",__FUNCTION__,
			  sqlite3_errmsg(metadata_db));
		return ;
	}
	sqlite3_reset(stmt);
}
static gboolean sqlite_delete_value(MetaDataType type,const char *key_a,const char *key_b)
{
	sqlite3_stmt *const stmt = metadata_stmt[META_DATA_SQL_DELETE];
	int ret;
    sqlite3_reset(stmt);

    ret = sqlite3_bind_int(stmt, 1, type);
	if (ret != SQLITE_OK) {
		g_warning("sqlite3_bind_int() failed: %s",
			  sqlite3_errmsg(metadata_db));
		return FALSE;
	}

    ret = sqlite3_bind_text(stmt, 2, key_a,-1, NULL);
	if (ret != SQLITE_OK) {
		g_warning("sqlite3_bind_text() failed: %s",
			  sqlite3_errmsg(metadata_db));
		return FALSE;
	}

    ret = sqlite3_bind_text(stmt, 3, key_b,-1, NULL);
	if (ret != SQLITE_OK) {
		g_warning("sqlite3_bind_text() failed: %s",
			  sqlite3_errmsg(metadata_db));
		return FALSE;
	}
	do {
		ret = sqlite3_step(stmt);
	} while (ret == SQLITE_BUSY);

	if (ret != SQLITE_DONE) {
		g_warning("%s: sqlite3_step() failed: %s",__FUNCTION__,
			  sqlite3_errmsg(metadata_db));
		return FALSE;
	}


	ret = sqlite3_changes(metadata_db);

	sqlite3_reset(stmt);
	sqlite3_clear_bindings(stmt);
    return ret > 0;
}

static gboolean sqlite_update_value(MetaDataType type,const char *key_a,const char *key_b, MetaDataContentType content_type, const char *content)
{
	sqlite3_stmt *const stmt = metadata_stmt[META_DATA_SQL_UPDATE];
	int ret;
    sqlite3_reset(stmt);

    ret = sqlite3_bind_int(stmt, 1, content_type);
	if (ret != SQLITE_OK) {
		g_warning("sqlite3_bind_int() failed: %s",
			  sqlite3_errmsg(metadata_db));
		return FALSE;
	}
    ret = sqlite3_bind_text(stmt, 2, content,-1, NULL);
	if (ret != SQLITE_OK) {
		g_warning("sqlite3_bind_text() failed: %s",
			  sqlite3_errmsg(metadata_db));
		return FALSE;
	}

    ret = sqlite3_bind_int(stmt, 3, type);
	if (ret != SQLITE_OK) {
		g_warning("sqlite3_bind_int() failed: %s",
			  sqlite3_errmsg(metadata_db));
		return FALSE;
	}

    ret = sqlite3_bind_text(stmt, 4, key_a,-1, NULL);
	if (ret != SQLITE_OK) {
		g_warning("sqlite3_bind_text() failed: %s",
			  sqlite3_errmsg(metadata_db));
		return FALSE;
	}

    ret = sqlite3_bind_text(stmt, 5, key_b,-1, NULL);
	if (ret != SQLITE_OK) {
		g_warning("sqlite3_bind_text() failed: %s",
			  sqlite3_errmsg(metadata_db));
		return FALSE;
	}
	do {
		ret = sqlite3_step(stmt);
	} while (ret == SQLITE_BUSY);

	if (ret != SQLITE_DONE) {
		g_warning("%s: sqlite3_step() failed: %s",__FUNCTION__,
			  sqlite3_errmsg(metadata_db));
		return FALSE;
	}

	ret = sqlite3_changes(metadata_db);

	sqlite3_reset(stmt);
	sqlite3_clear_bindings(stmt);
    return ret > 0;
}

static gboolean sqlite_set_value(MetaDataType type,const char *key_a,const char *key_b, MetaDataContentType content_type, const char *content)
{
	sqlite3_stmt *const stmt = metadata_stmt[META_DATA_SQL_SET];
	int ret;
    sqlite3_reset(stmt);

    ret = sqlite3_bind_int(stmt, 1, content_type);
	if (ret != SQLITE_OK) {
		g_warning("sqlite3_bind_int() failed: %s",
			  sqlite3_errmsg(metadata_db));
		return FALSE;
	}
    ret = sqlite3_bind_text(stmt, 2, content,-1, NULL);
	if (ret != SQLITE_OK) {
		g_warning("sqlite3_bind_text() failed: %s",
			  sqlite3_errmsg(metadata_db));
		return FALSE;
	}

    ret = sqlite3_bind_int(stmt, 3, type);
	if (ret != SQLITE_OK) {
		g_warning("sqlite3_bind_int() failed: %s",
			  sqlite3_errmsg(metadata_db));
		return FALSE;
	}

    ret = sqlite3_bind_text(stmt, 4, key_a,-1, NULL);
	if (ret != SQLITE_OK) {
		g_warning("sqlite3_bind_text() failed: %s",
			  sqlite3_errmsg(metadata_db));
		return FALSE;
	}

    ret = sqlite3_bind_text(stmt, 5, key_b,-1, NULL);
	if (ret != SQLITE_OK) {
		g_warning("sqlite3_bind_text() failed: %s",
			  sqlite3_errmsg(metadata_db));
		return FALSE;
	}
	do {
		ret = sqlite3_step(stmt);
	} while (ret == SQLITE_BUSY);

	if (ret != SQLITE_DONE) {
		g_warning("%s: sqlite3_step() failed %s::%s content:'%s' : %s",__FUNCTION__, key_a, key_b,content,
			  sqlite3_errmsg(metadata_db));
		return FALSE;
	}


	ret = sqlite3_changes(metadata_db);

	sqlite3_reset(stmt);
	sqlite3_clear_bindings(stmt);
    return ret > 0;
}

static MetaData *sqlite_get_value(MetaDataType type,const char *key_a,const char *key_b)
{
	sqlite3_stmt *const stmt = metadata_stmt[META_DATA_SQL_GET];
	int ret;
    MetaData *met = NULL;
    sqlite3_reset(stmt);

    ret = sqlite3_bind_int(stmt, 1, type);
	if (ret != SQLITE_OK) {
		g_warning("sqlite3_bind_int() failed: %s",
			  sqlite3_errmsg(metadata_db));
		return NULL;
	}

    ret = sqlite3_bind_text(stmt, 2, key_a,-1, NULL);
	if (ret != SQLITE_OK) {
		g_warning("sqlite3_bind_text() failed: %s",
			  sqlite3_errmsg(metadata_db));
		return NULL;
	}

    ret = sqlite3_bind_text(stmt, 3, key_b,-1, NULL);
	if (ret != SQLITE_OK) {
		g_warning("sqlite3_bind_text() failed: %s",
			  sqlite3_errmsg(metadata_db));
		return NULL;
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
                if(met->content_type == META_DATA_CONTENT_TEXT_VECTOR) {
                    gchar *value = (gchar *) sqlite3_column_text(stmt, 1);
                    met->size += 1;
                    met->content = g_realloc((gchar **)met->content, sizeof(gchar *)*(met->size+1));
                    ((gchar **)met->content)[met->size-1] = g_strdup(value);
                    ((gchar **)met->content)[met->size] = NULL;
                }else if (met->content_type == META_DATA_CONTENT_TEXT_LIST) {
                    gchar *value = (gchar *) sqlite3_column_text(stmt, 1);
                    met->content = (void *)g_list_append((GList *)met->content, g_strdup(value));
                    met->size = 0;
                }else{
                    if(met->content_type == META_DATA_CONTENT_URI || 
                        met->content_type == META_DATA_CONTENT_TEXT  ||
                        met->content_type == META_DATA_CONTENT_HTML)
                    {
                        gchar *value = (unsigned char*)sqlite3_column_text(stmt, 1);
                        met->content = g_strdup(value);
                        met->size = -1;
                    }
                    else if (met->content_type == META_DATA_CONTENT_RAW)
                    {
                        gchar *value = (unsigned char*) sqlite3_column_text(stmt, 1);
                        gsize size;
                        met->content = g_base64_decode(value, &size);
                        met->size = size;
                    }
                    /* indicate we don't query anymore */
                    ret = SQLITE_DONE;
                }
            case SQLITE_DONE:
            case SQLITE_BUSY:
                break;
            default:

                g_warning("%s: sqlite3_step() failed: %s",__FUNCTION__,
                        sqlite3_errmsg(metadata_db));
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


/**
TODO: Find a  way to store the type of the data in the config file. (met->content_type)
*/

MetaDataResult meta_data_get_from_cache(mpd_Song *song, MetaDataType type, MetaData **met)
{
    char *key_a= "", *key_b = "";
    if(type == META_ALBUM_ART){
        key_a = song->artist;
        key_b = song->album;
    }else if(type == META_ALBUM_TXT){
        key_a = song->artist;
        key_b = song->album;
    }else if (type == META_ARTIST_ART || type == META_ARTIST_TXT || type == META_ARTIST_SIMILAR) {
        key_a = song->artist;
    }else if (type == META_SONG_TXT || type == META_SONG_SIMILAR) {
        key_a = song->artist;
        key_b = song->title;
    }else if (type == META_GENRE_SIMILAR) {
        key_a = song->genre;
    }

    *met = sqlite_get_value(type,key_a, key_b); 
    if((*met) == NULL)
    {
        *met = meta_data_new();
        (*met)->type = type;
        (*met)->plugin_name = "Metadata Cache";
        (*met)->content_type = META_DATA_CONTENT_EMPTY;
        printf("META_DATA_FETCHING: Got from %s-%s\n", key_a, key_b);
        return META_DATA_FETCHING;	
	}
    if((*met)->content_type == META_DATA_CONTENT_EMPTY)
    {
        printf("META_DATA_UNAVAILABLE: Got from %s-%s\n", key_a, key_b);
        return META_DATA_UNAVAILABLE;
    }
    if((*met)->content_type == META_DATA_CONTENT_URI)
    {
        const gchar *path = meta_data_get_uri(*met);
        if(!g_file_test(path, G_FILE_TEST_EXISTS))
        {
            sqlite_delete_value(type, key_a, key_b); 
            (*met)->content_type = META_DATA_CONTENT_EMPTY;
            g_free((gchar *)((*met)->content));
            (*met)->content = NULL;
            (*met)->size = 0;
            return META_DATA_FETCHING;
        }
    }
    printf("META_DATA_AVAILABLE: Got from %s-%s\n", key_a, key_b);
    return META_DATA_AVAILABLE;	
}
void meta_data_set_cache_real(mpd_Song *song, MetaDataResult result, MetaData *met)
{
    char *key_a= "", *key_b = "";
	if(!song) return;
    if((met)->type == META_ALBUM_ART){
        key_a = song->artist;
        key_b = song->album;
    }else if((met)->type == META_ALBUM_TXT){
        key_a = song->artist;
        key_b = song->album;
    }else if ((met)->type == META_ARTIST_ART || (met)->type == META_ARTIST_TXT || (met)->type == META_ARTIST_SIMILAR) {
        key_a = song->artist;
    }else if ((met)->type == META_SONG_TXT || (met)->type == META_SONG_SIMILAR) {
        key_a = song->artist;
        key_b = song->title;
    }else if ((met)->type == META_GENRE_SIMILAR) {
        key_a = song->genre;
    }


    if(met->content_type == META_DATA_CONTENT_URI ||
            met->content_type == META_DATA_CONTENT_TEXT ||
            met->content_type == META_DATA_CONTENT_HTML ||
            met->content_type == META_DATA_CONTENT_EMPTY)
    {
        sqlite_update_value(met->type, key_a, key_b, met->content_type, (const gchar *)met->content) || 
            sqlite_set_value(met->type, key_a, key_b, met->content_type, (const gchar *)met->content);
    }else if (met->content_type == META_DATA_CONTENT_RAW) {
        gsize size;
        const guchar *udata = meta_data_get_raw(met, &size);
        gchar *data = g_base64_encode(udata, size);
        sqlite_update_value(met->type, key_a, key_b, met->content_type, (const gchar *)data) ||
            sqlite_set_value(met->type, key_a, key_b, met->content_type, (const gchar *)data);
        g_free(data);
    }else if (met->content_type == META_DATA_CONTENT_TEXT_LIST) {
        GList *iter;
        sqlite_delete_value(met->type, key_a, key_b);
        iter = g_list_first((GList *)meta_data_get_text_list(met));
        sqlite_list_start();
        for(;iter; iter = g_list_next(iter)){
            sqlite_set_value(met->type, key_a, key_b, met->content_type, (const gchar *)iter->data);
        }
        sqlite_list_end();
    }else if (met->content_type == META_DATA_CONTENT_TEXT_VECTOR) {
        int i = 0;
        const gchar **text_vector = meta_data_get_text_vector(met);
        sqlite_delete_value(met->type, key_a, key_b);
        sqlite_list_start();
        for(i=0;text_vector && text_vector[i];i++){
            sqlite_set_value(met->type, key_a, key_b, met->content_type, (const gchar *)text_vector[i]);
        }
        sqlite_list_end();
    }
}
void meta_data_set_cache(mpd_Song *song, MetaDataResult result, MetaData *met)
{
    mpd_Song *edited = rewrite_mpd_song(song, met->type);
    meta_data_set_cache_real(edited, result, met);
    if(edited->artist)
    {
        if(strcmp(edited->artist, "Various Artists")!=0)
            meta_data_set_cache_real(song, result, met);
    }
    mpd_freeSong(edited);
}

static sqlite3_stmt *
metadata_prepare(const char *sql)
{
	int ret;
	sqlite3_stmt *stmt;

	ret = sqlite3_prepare_v2(metadata_db, sql, -1, &stmt, NULL);
	if (ret != SQLITE_OK)
		g_error("sqlite3_prepare_v2() failed: %s",
			sqlite3_errmsg(metadata_db));

	return stmt;
}

void metadata_cache_init(void)
{
    int ret;
    unsigned i;
    gchar *url = gmpc_get_covers_path(NULL);
    if(!g_file_test(url,G_FILE_TEST_IS_DIR)){
        if(g_mkdir(url, 0700)<0){
            g_error("Cannot make %s\n", url);
        }
    }
    q_free(url);

    url = gmpc_get_covers_path("covers.sql");
    ret = sqlite3_open(url, &metadata_db);
    if (ret != SQLITE_OK)
        g_error("Failed to open sqlite database '%s': %s",
                url, sqlite3_errmsg(metadata_db));

	ret = sqlite3_exec(metadata_db, metadata_sql_create, NULL, NULL, NULL);
	if (ret != SQLITE_OK)
		g_error("Failed to create metadata table: %s",
			sqlite3_errmsg(metadata_db));

	/* prepare the statements we're going to use */

	for (i = 0; i < G_N_ELEMENTS(metadata_sql); ++i) {
		g_assert(metadata_sql[i] != NULL);

		metadata_stmt[i] = metadata_prepare(metadata_sql[i]);
	}

    g_free(url);
}


void metadata_cache_cleanup(void)
{
}
void metadata_cache_destroy(void)
{
    unsigned i;
	for (i = 0; i < G_N_ELEMENTS(metadata_stmt); ++i) {
		g_assert(metadata_stmt[i] != NULL);

		sqlite3_finalize(metadata_stmt[i]);
	}
	sqlite3_close(metadata_db);
}
