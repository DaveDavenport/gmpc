/* gmpc-glyros (GMPC plugin)
 * Copyright (C) 2011 serztle <serztle@googlemail.com>
 *                    sahib <sahib@online.de>
 * Project homepage: https://github.com/sahib/gmpc-glyros

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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
#include <glib.h>
#include "config.h"

#ifdef HAVE_GLYR

#include <glyr/glyr.h>
#include <gtk/gtk.h>
#include <gmpc/plugin.h>
#include <gmpc/metadata.h>


#define GLYROS_DEBUG TRUE

#define LOG_DOMAIN "Gmpc.Provider.Glyros"

#define LOG_SUBCLASS        "glyros"
#define LOG_COVER_NAME      "fetch-art-album"
#define LOG_ARTIST_ART      "fetch-art-artist"
#define LOG_SIMILIAR_ARTIST "fetch-similiar-artist"
#define LOG_SIMILIAR_SONG   "fetch-similiar-song"
#define LOG_SIMILIAR_GENRE  "fetch-similiar-genre"
#define LOG_ARTIST_TXT      "fetch-biography-artist"
#define LOG_SONG_TXT        "fetch-lyrics"
#define LOG_ALBUM_TXT       "fetch-album-txt"

// other
#define LOG_FUZZYNESS      "fuzzyness"
#define LOG_CMINSIZE       "cminsize"
#define LOG_CMAXSIZE       "cmaxsize"
#define LOG_MSIMILIARTIST  "msimiliartist"
#define LOG_MSIMILISONG    "msimilisong"
#define LOG_QSRATIO        "qsratio"
#define LOG_PARALLEL       "parallel"
#define LOG_USERAGENT      "useragent"
#define LOG_FROM           "from"


/* plugi, getting catched via 'extern' */
gmpcPlugin glyros_plugin;

/* API Version. Needed. */
int plugin_api_version = PLUGIN_API_VERSION;

/**
 * @brief Both calls are strictly neccessary and not threadsafe
 */
static void glyros_init(void)
{
    glyr_init();
    atexit(glyr_cleanup);
}

/**
 * @brief All data that is passed to the main method, running in a seperate thread
 */
struct glyros_fetch_thread_data
{
    mpd_Song *song;
    MetaDataType type;
    void (*callback)(GList *list, gpointer data);
    gpointer user_data;
};

static int glyros_fetch_cover_priority(void)
{
    return cfg_get_single_value_as_int_with_default(config, LOG_SUBCLASS, "priority", 20);
}

static void glyros_fetch_cover_priority_set(int priority)
{
    cfg_set_single_value_as_int(config, LOG_SUBCLASS, "priority", priority);
}

static int glyros_get_enabled(void)
{
    return cfg_get_single_value_as_int_with_default(config, LOG_SUBCLASS, "enable", TRUE);
}

static void glyros_set_enabled(int enabled)
{
    cfg_set_single_value_as_int(config, LOG_SUBCLASS, "enable", enabled);
}

/**
 * @brief Parse gmpc's proxy settings to glyr's representation
 *
 * @param q the glyr-query used in this job
 */
static void glyros_set_proxy(GlyrQuery * q)
{
    if(q != NULL)
    {
        const char * sublcass = "Network Settings";
        if(cfg_get_single_value_as_int_with_default(config,sublcass,"Use Proxy",FALSE))
        {
            char * port = cfg_get_single_value_as_string_with_default(config,sublcass,"Proxy Port","8080");
            char * addr = cfg_get_single_value_as_string_with_default(config,sublcass,"Proxy Address","localhost");

            char * user = g_strdup("");
            char * passwd = g_strdup("");
            if(cfg_get_single_value_as_int_with_default(config,sublcass,"Use Proxy",FALSE))
            {
                user   = cfg_get_single_value_as_string_with_default(config,sublcass,"Proxy authentication username","");
                passwd = cfg_get_single_value_as_string_with_default(config,sublcass,"Proxy authentication password","");
            }

            if(port != NULL && addr != NULL)
            {
                /* remove protocol if present , it's not used. */
                char * proto;
                char * proxystring;
                if((proto = strstr(addr,"://")))
                {
                    proto += 3;
                    addr = proto;
                }

                proxystring = g_strdup_printf("%s:%s@%s:%s",user,passwd,addr,port);
                if(proxystring != NULL)
                {
                    glyr_opt_proxy(q,proxystring);
                    g_free(proxystring);
                }
            }
            if(port) g_free(port);
            if(addr) g_free(addr);
            if(user) g_free(user);
            if(passwd) g_free(passwd);
        }
    }
}

static MetaData * glyros_get_similiar_artist_names(GlyrMemCache * cache)
{
    MetaData * mtd = NULL;
    while(cache != NULL)
    {
        if(cache->data != NULL)
        {
            gchar ** split = g_strsplit(cache->data,"\n",0);
            if(split != NULL)
            {
                if(!mtd) {
                    mtd = meta_data_new();
                    mtd->type = META_ARTIST_SIMILAR;
                    mtd->plugin_name = glyros_plugin.name;
                    mtd->content_type = META_DATA_CONTENT_TEXT_LIST;
                    mtd->size = 0;
                }
                mtd->size++;
                mtd->content = g_list_append((GList*) mtd->content, g_strdup((char *)split[0]));
                g_strfreev(split);
            }
        }
        cache = cache->next;
    }
    return mtd;
}

static MetaData * glyros_get_similiar_song_names(GlyrMemCache * cache)
{
    MetaData * mtd = NULL;
    while(cache != NULL)
    {
        if(cache->data != NULL)
        {
            gchar ** split = g_strsplit(cache->data,"\n",0);
            if(split != NULL && split[0] != NULL)
            {
                gchar * buffer;
                if(!mtd) {
                    mtd = meta_data_new();
                    mtd->type = META_SONG_SIMILAR;
                    mtd->plugin_name = glyros_plugin.name;
                    mtd->content_type = META_DATA_CONTENT_TEXT_LIST;
                    mtd->size = 0;
                }

                buffer = g_strdup_printf("%s::%s",split[1],split[0]);
                g_log(LOG_DOMAIN,G_LOG_LEVEL_DEBUG, "%s\n", buffer);

                mtd->size++;
                mtd->content = g_list_append((GList*) mtd->content, buffer);
                g_strfreev(split);
            }
        }
        cache = cache->next;
    }
    return mtd;
}


/**
 * @brief The main() of this plugin
 *
 * @param data The thread-data passed to the function
 *
 * Note, that this always runs in an own thread.
 * Do not use globals vars.
 *
 * @return always NULL
 */
static gpointer glyros_fetch_thread(void * data)
{
    GLYR_ERROR err = GLYRE_OK;
    GList * retv = NULL;
    gchar *temp;
    /* arguments */
    struct glyros_fetch_thread_data * thread_data = data;

    /* cache */
    GlyrMemCache * cache = NULL;

    /* query */
    GlyrQuery q;

    /* data type */
    MetaDataContentType content_type = META_DATA_CONTENT_RAW;

    /* query init */
    glyr_init_query(&q);

    /* set metadata */
    glyr_opt_artist(&q,(char*)thread_data->song->artist);
    glyr_opt_album (&q,(char*)thread_data->song->album);
    glyr_opt_title (&q,(char*)thread_data->song->title);

    /* ask preferences */
    glyr_opt_fuzzyness(&q,cfg_get_single_value_as_int_with_default(config,LOG_SUBCLASS,LOG_FUZZYNESS,GLYR_DEFAULT_FUZZYNESS));
    glyr_opt_img_minsize(&q,cfg_get_single_value_as_int_with_default(config,LOG_SUBCLASS,LOG_CMINSIZE,GLYR_DEFAULT_CMINSIZE));
    glyr_opt_img_maxsize(&q,cfg_get_single_value_as_int_with_default(config,LOG_SUBCLASS,LOG_CMAXSIZE,GLYR_DEFAULT_CMAXSIZE));
    glyr_opt_qsratio(&q,cfg_get_single_value_as_float_with_default(config,LOG_SUBCLASS,LOG_QSRATIO,GLYR_DEFAULT_QSRATIO*100.0)/100.0);
    glyr_opt_parallel(&q,cfg_get_single_value_as_int_with_default(config,LOG_SUBCLASS,LOG_PARALLEL,GLYR_DEFAULT_PARALLEL));

    temp = cfg_get_single_value_as_string_with_default(config,LOG_SUBCLASS,LOG_USERAGENT,GLYR_DEFAULT_USERAGENT);
    glyr_opt_useragent(&q,temp);
    g_free(temp);

    temp = cfg_get_single_value_as_string_with_default(config,LOG_SUBCLASS,LOG_FROM,"all");
    glyr_opt_from(&q,temp);
    g_free(temp);

#ifdef GLYROS_DEBUG
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"fuzz: %d",(gint)q.fuzzyness);
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"cmin: %d",q.img_min_size);
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"cmax: %d",q.img_max_size);
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"para: %d",(gint)q.parallel);
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"qsra: %f",q.qsratio);
    glyr_opt_verbosity(&q, 2);
#endif

    /* Set proxy */
    glyros_set_proxy(&q);

    /* Set force utf8 */
    glyr_opt_force_utf8(&q, TRUE);

    /* set default type */
    glyr_opt_type(&q, GLYR_GET_UNSURE);

    /* set type */
    if (glyros_get_enabled() == TRUE)
    {
        if (thread_data->song->artist != NULL)
        {
            if (thread_data->type == META_ARTIST_ART &&
                    cfg_get_single_value_as_int_with_default(config,LOG_SUBCLASS, LOG_ARTIST_ART,TRUE))
            {
                glyr_opt_type(&q, GLYR_GET_ARTIST_PHOTOS);
                content_type = META_DATA_CONTENT_RAW;
            }
            else if (thread_data->type == META_ARTIST_TXT &&
                    cfg_get_single_value_as_int_with_default(config,LOG_SUBCLASS,LOG_ARTIST_TXT,TRUE))
            {
                glyr_opt_type(&q, GLYR_GET_ARTISTBIO);
                content_type = META_DATA_CONTENT_TEXT;
            }
            else if (thread_data->type == META_ARTIST_SIMILAR &&
                    cfg_get_single_value_as_int_with_default(config,LOG_SUBCLASS,LOG_SIMILIAR_ARTIST,TRUE))
            {
                glyr_opt_type(&q, GLYR_GET_SIMILIAR_ARTISTS);
                glyr_opt_number(&q, cfg_get_single_value_as_int_with_default(config,LOG_SUBCLASS,LOG_MSIMILIARTIST,20));
                content_type = META_DATA_CONTENT_TEXT;
            }
            else if (thread_data->type == META_ALBUM_ART &&
                    thread_data->song->album != NULL    &&
                    cfg_get_single_value_as_int_with_default(config,LOG_SUBCLASS,LOG_COVER_NAME,TRUE))
            {
                glyr_opt_type(&q, GLYR_GET_COVERART);
                content_type = META_DATA_CONTENT_RAW;
            }
            else if (thread_data->type == META_ALBUM_TXT &&
                    thread_data->song->album != NULL    &&
                    cfg_get_single_value_as_int_with_default(config,LOG_SUBCLASS,LOG_ALBUM_TXT,TRUE))
            {
                glyr_opt_type(&q, GLYR_GET_ALBUM_REVIEW);
                content_type = META_DATA_CONTENT_TEXT;
            }
            else if (thread_data->type == META_SONG_TXT &&
                    thread_data->song->title != NULL   &&
                    cfg_get_single_value_as_int_with_default(config,LOG_SUBCLASS,LOG_SONG_TXT,TRUE))
            {
                glyr_opt_type(&q, GLYR_GET_LYRICS);
                content_type = META_DATA_CONTENT_TEXT;
            }
            else if (thread_data->type == META_SONG_SIMILAR &&
                    thread_data->song->title != NULL       &&
                    cfg_get_single_value_as_int_with_default(config,LOG_SUBCLASS,LOG_SIMILIAR_SONG,TRUE))
            {
                glyr_opt_type(&q, GLYR_GET_SIMILIAR_SONGS);
                glyr_opt_number(&q, cfg_get_single_value_as_int_with_default(config,LOG_SUBCLASS,LOG_MSIMILISONG,20));
            }
            else if (thread_data->type == META_SONG_GUITAR_TAB && thread_data->song->title != NULL)
            {
                /* not supported yet. */
            }
        }
        else if (thread_data->song->genre != NULL)
        {
            if (thread_data->type == META_GENRE_SIMILAR)
            {
                /* not supported yet. */
            }
        }
    }

    /* get metadata */
    cache = glyr_get(&q,&err,NULL);

#if GLYROS_DEBUG
    if(err != GLYRE_OK)
    {
        g_log(LOG_DOMAIN, G_LOG_LEVEL_WARNING,"glyros-gmpc: %s",glyr_strerror(err));
    }
#endif

    /* something there? */
    if (cache != NULL)
    {
        if(thread_data->type == META_ARTIST_SIMILAR)
        {
            MetaData * cont = glyros_get_similiar_artist_names(cache);
            if(cont != NULL)
            {
                retv = g_list_prepend(retv,cont);
            }
        }
        else if (thread_data->type == META_SONG_SIMILAR)
        {
            MetaData * cont;
            glyr_opt_type(&q, GLYR_GET_SIMILIAR_SONGS);
            cont = glyros_get_similiar_song_names(cache);
            if (cont != NULL)
            {
                retv = g_list_prepend(retv,cont);
            }
        }
        else
        {
            MetaData *mtd;
#if GLYROS_DEBUG
            if(cache->data && (thread_data->type == META_SONG_TXT   ||
                        thread_data->type == META_ARTIST_TXT ||
                        thread_data->type == META_ALBUM_TXT
                        )
              )
            {
                gchar * debug_text = g_strdup_printf("%s\n\n[From: %s | %s]\n",
                        cache->data,
                        cache->prov,
                        cache->dsrc);
                g_free(cache->data);
                cache->data = debug_text;
                cache->size = strlen(debug_text);

            }
#endif
            mtd = meta_data_new();
            mtd->type = thread_data->type;
            mtd->plugin_name = glyros_plugin.name;
            mtd->content_type = content_type;

            mtd->content = g_malloc0(cache->size);
            memcpy(mtd->content, cache->data, cache->size);
            mtd->size = cache->size;

            retv = g_list_prepend(retv,mtd);
        }
        glyr_free_list(cache);
    }

    /* destroy */
    glyr_destroy_query(&q);
    g_free(data);

    thread_data->callback(retv, thread_data->user_data);
    return NULL;
}

/**
 * This is called by gmpc. Only starts the function above
 */
static void glyros_fetch(mpd_Song *song,MetaDataType type,
        void (*callback)(GList *list, gpointer data),
        gpointer user_data)
{
    struct glyros_fetch_thread_data * data = g_malloc(sizeof(struct glyros_fetch_thread_data));

    data->song = song;
    data->type = type;
    data->callback = callback;
    data->user_data = user_data;

    g_thread_create(glyros_fetch_thread, (gpointer)data, FALSE, NULL);
}

static void pref_enable_fetch(GtkWidget *con, gpointer data)
{
    MetaDataType type = GPOINTER_TO_INT(data);
    int state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(con));
    switch(type) {
        case META_ARTIST_ART:
            cfg_set_single_value_as_int(config, LOG_SUBCLASS, LOG_ARTIST_ART,state);
            break;
        case META_ALBUM_ART:
            cfg_set_single_value_as_int(config, LOG_SUBCLASS, LOG_COVER_NAME,state);
            break;
        case META_ARTIST_SIMILAR:
            cfg_set_single_value_as_int(config, LOG_SUBCLASS, LOG_SIMILIAR_ARTIST, state);
            break;
        case META_SONG_SIMILAR:
            cfg_set_single_value_as_int(config, LOG_SUBCLASS, LOG_SIMILIAR_SONG,state);
            break;
        case META_GENRE_SIMILAR:
            cfg_set_single_value_as_int(config, LOG_SUBCLASS, LOG_SIMILIAR_GENRE, state);
            break;
        case META_ARTIST_TXT:
            cfg_set_single_value_as_int(config, LOG_SUBCLASS, LOG_ARTIST_TXT,state);
            break;
        case META_SONG_TXT:
            cfg_set_single_value_as_int(config, LOG_SUBCLASS, LOG_SONG_TXT,state);
            break;
        case META_ALBUM_TXT:
            cfg_set_single_value_as_int(config, LOG_SUBCLASS, LOG_ALBUM_TXT,state);
            break;
        default:
            break;
    }
}

static void pref_add_checkbox(const char * text, MetaDataType type, const char * log_to, GtkWidget * vbox)
{
    GtkWidget * toggleb = gtk_check_button_new_with_label(text);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggleb),
            cfg_get_single_value_as_int_with_default(config, LOG_SUBCLASS, log_to, TRUE));
    gtk_box_pack_start(GTK_BOX(vbox), toggleb, FALSE, TRUE, 0);
    g_signal_connect(G_OBJECT(toggleb), "toggled", G_CALLBACK(pref_enable_fetch), GINT_TO_POINTER(type));
}

/* GTK* Stuff for bulding the preferences dialog */
enum SPINNER_CHOICES
{
    OPT_FUZZYNESS,
    OPT_CMINSIZE,
    OPT_CMAXSIZE,
    OPT_MSIMILIARTIST,
    OPT_MSIMILISONG,
    OPT_QSRATIO,
    OPT_PARALLEL
};

static void pref_spinner_callback(GtkSpinButton * spin, gpointer data)
{
    gdouble val = gtk_spin_button_get_value(spin);
    enum SPINNER_CHOICES ch = GPOINTER_TO_INT(data);
    switch(ch)
    {
        case OPT_FUZZYNESS:
            cfg_set_single_value_as_int(config, LOG_SUBCLASS, LOG_FUZZYNESS,val);
            break;
        case OPT_CMINSIZE:
            cfg_set_single_value_as_int(config, LOG_SUBCLASS, LOG_CMINSIZE,val);
            break;
        case OPT_CMAXSIZE:
            cfg_set_single_value_as_int(config, LOG_SUBCLASS, LOG_CMAXSIZE,val);
            break;
        case OPT_MSIMILIARTIST:
            cfg_set_single_value_as_int(config, LOG_SUBCLASS, LOG_MSIMILIARTIST,val);
            break;
        case OPT_MSIMILISONG:
            cfg_set_single_value_as_int(config, LOG_SUBCLASS, LOG_MSIMILISONG,val);
            break;
        case OPT_QSRATIO:
            cfg_set_single_value_as_int(config, LOG_SUBCLASS, LOG_QSRATIO, val);
            break;
        case OPT_PARALLEL:
            cfg_set_single_value_as_int(config, LOG_SUBCLASS, LOG_PARALLEL, val);
            break;
        default:
            break;
    }
}

static void restore_defaults(GtkButton * button, gpointer user_data)
{
    GtkWidget * spacer_label = user_data;
    cfg_set_single_value_as_int(config,LOG_SUBCLASS,LOG_FUZZYNESS,GLYR_DEFAULT_FUZZYNESS);
    cfg_set_single_value_as_int(config,LOG_SUBCLASS,LOG_CMINSIZE,GLYR_DEFAULT_CMINSIZE);
    cfg_set_single_value_as_int(config,LOG_SUBCLASS,LOG_CMAXSIZE,GLYR_DEFAULT_CMAXSIZE);
    cfg_set_single_value_as_int(config,LOG_SUBCLASS,LOG_MSIMILISONG,20);
    cfg_set_single_value_as_int(config,LOG_SUBCLASS,LOG_MSIMILIARTIST,20);
    cfg_set_single_value_as_int(config,LOG_SUBCLASS,LOG_QSRATIO,GLYR_DEFAULT_QSRATIO*100);
    cfg_set_single_value_as_int(config,LOG_SUBCLASS,LOG_PARALLEL,GLYR_DEFAULT_PARALLEL);
    cfg_set_single_value_as_string(config,LOG_SUBCLASS,LOG_USERAGENT,GLYR_DEFAULT_USERAGENT);
    cfg_set_single_value_as_string(config,LOG_SUBCLASS,LOG_FROM,"all");

    if(spacer_label != NULL)
    {
        gtk_label_set_text(GTK_LABEL(spacer_label),"Restored defaults. (Not yet displayed)");
    }
}

static void pref_add_spinbutton(const gchar * descr, const gchar * log_to, gdouble default_to, gdouble low, gdouble high, GtkWidget * vbox, enum SPINNER_CHOICES choice, gdouble step)
{
    GtkWidget * hbox_cont  = gtk_hbox_new(FALSE,2);
    GtkWidget * descr_label = gtk_label_new(descr);

    gdouble get_value = cfg_get_single_value_as_float_with_default(config, LOG_SUBCLASS, log_to, default_to);
    GtkWidget * spinner = GTK_WIDGET(gtk_spin_button_new_with_range(low,high,step));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinner), get_value);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_cont, FALSE, TRUE, 0);

    /* Description  */
    gtk_box_pack_start(GTK_BOX(hbox_cont), descr_label, FALSE, TRUE, 0);

    /* Spacer */
    gtk_box_pack_start(GTK_BOX(hbox_cont), gtk_label_new(""), TRUE,TRUE,2);

    /* Spinbutton */
    gtk_box_pack_start(GTK_BOX(hbox_cont), spinner, FALSE,TRUE,0);

    /* Callback */
    g_signal_connect(G_OBJECT(spinner), "value-changed", G_CALLBACK(pref_spinner_callback), GINT_TO_POINTER(choice));
}

static void text_value_changed_callback(GtkTextBuffer *textbuffer, gpointer user_data)
{
    GtkTextIter start;
    GtkTextIter end;
    gchar * text_value;
    gtk_text_buffer_get_bounds(textbuffer, &start, &end);
    text_value = gtk_text_buffer_get_text(textbuffer,&start,&end,false);
    cfg_set_single_value_as_string(config, LOG_SUBCLASS,LOG_USERAGENT,text_value);
    g_free(text_value);
}

static void setup_text_view(const gchar * descr,
        const gchar * default_value,
        GtkBox * container,
        const gchar * log_to)
{
    GtkBox * hbox = GTK_BOX(gtk_hbox_new(FALSE,2));

    GtkTextView * text_box = GTK_TEXT_VIEW(gtk_text_view_new());
    GtkTextBuffer * buffer = gtk_text_view_get_buffer(text_box);
    gtk_text_buffer_set_text (buffer, cfg_get_single_value_as_string_with_default(config, LOG_SUBCLASS, log_to,default_value), -1);

    gtk_box_pack_start(hbox,gtk_label_new(descr),FALSE,TRUE,2);
    gtk_box_pack_start(hbox,gtk_label_new(""),TRUE,TRUE,2);
    gtk_box_pack_start(hbox,GTK_WIDGET(text_box),FALSE,TRUE,2);

    gtk_box_pack_start(container,GTK_WIDGET(hbox),FALSE,TRUE,2);

    g_signal_connect(G_OBJECT(gtk_text_view_get_buffer(text_box)),
            "changed",
            G_CALLBACK(text_value_changed_callback),
            (void *)log_to);
}

static void pref_construct(GtkWidget * con)
{
    GtkWidget *top_box, *checkbox_frame, *vbox, *spinner_frame;
    GtkWidget *restore_hbox, *restore_button, *space_label;
    GtkWidget *spin_vbox, *text_box_vbox, *text_box_frame, *scrolled_window;
    scrolled_window = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    top_box = gtk_vbox_new(FALSE,2);

    checkbox_frame = gtk_frame_new("");
    gtk_label_set_markup(GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(checkbox_frame))), "<b>Fetch</b>");
    vbox = gtk_vbox_new(FALSE,6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    gtk_container_add(GTK_CONTAINER(checkbox_frame), vbox);

    pref_add_checkbox("Artist Images",META_ARTIST_ART,LOG_ARTIST_ART,vbox);
    pref_add_checkbox("Artist Biography",META_ARTIST_TXT,LOG_ARTIST_TXT,vbox);
    pref_add_checkbox("Similar artist",META_ARTIST_SIMILAR,LOG_SIMILIAR_ARTIST,vbox);
    pref_add_checkbox("Similar songs",META_SONG_SIMILAR,LOG_SIMILIAR_SONG,vbox); // -> seb
    pref_add_checkbox("Album cover",META_ALBUM_ART,LOG_COVER_NAME,vbox);
    pref_add_checkbox("Songlyrics",META_SONG_TXT,LOG_SONG_TXT,vbox);
    pref_add_checkbox("Album information",META_ALBUM_TXT,LOG_ALBUM_TXT,vbox);

    spinner_frame = gtk_frame_new("");
    gtk_label_set_markup(GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(spinner_frame))), "<b>Miscellaneous Settings</b>");

    spin_vbox     = gtk_vbox_new(FALSE,6);
    pref_add_spinbutton("Fuzzyness factor:      ",LOG_FUZZYNESS,GLYR_DEFAULT_FUZZYNESS,0.0,42.0,spin_vbox,OPT_FUZZYNESS,1);
    pref_add_spinbutton("Minimal imagesize:     ",LOG_CMINSIZE,GLYR_DEFAULT_CMINSIZE,-1.0,5000.0,spin_vbox,OPT_CMINSIZE,1);
    pref_add_spinbutton("Maxmimal imagesize:    ",LOG_CMAXSIZE,GLYR_DEFAULT_CMAXSIZE,-1.0,5001.0,spin_vbox,OPT_CMAXSIZE,1);
    pref_add_spinbutton("Max. similiar artists: ",LOG_MSIMILIARTIST,20,0.0,1000.0,spin_vbox,OPT_MSIMILIARTIST,1);
    pref_add_spinbutton("Max. similiar songs:   ",LOG_MSIMILISONG,20,0.0,1000.0,spin_vbox,OPT_MSIMILISONG,1);
    pref_add_spinbutton("Max parallel plugins:  ",LOG_PARALLEL,GLYR_DEFAULT_PARALLEL,0.0,42.0,spin_vbox,OPT_PARALLEL,1);
    pref_add_spinbutton("Quality/Speed ratio:   ",LOG_QSRATIO,GLYR_DEFAULT_QSRATIO*100,0.0,100.0,spin_vbox,OPT_QSRATIO,1);
    gtk_container_add(GTK_CONTAINER(spinner_frame), spin_vbox);

    if(!glyros_get_enabled()) {
        gtk_widget_set_sensitive(GTK_WIDGET(vbox), FALSE);
    }

    text_box_vbox  = gtk_vbox_new(FALSE,2);
    text_box_frame = gtk_frame_new("");
    gtk_container_add(GTK_CONTAINER(text_box_frame),text_box_vbox);
    gtk_label_set_markup(GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(text_box_frame))), "<b>Less important</b>");
    setup_text_view("Useragent: ","gmpc-glyros",GTK_BOX(text_box_vbox),LOG_USERAGENT);
    setup_text_view("Allowed providers:","all;",GTK_BOX(text_box_vbox),LOG_FROM);

    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), top_box);
    gtk_container_add(GTK_CONTAINER(con),scrolled_window);
    gtk_box_pack_start(GTK_BOX(top_box),checkbox_frame, FALSE, TRUE,  2);
    gtk_box_pack_start(GTK_BOX(top_box),spinner_frame,  FALSE, TRUE,  2);
    gtk_box_pack_start(GTK_BOX(top_box),text_box_frame, FALSE, TRUE,  2);

    restore_hbox = gtk_hbox_new(FALSE,2);
    restore_button = gtk_button_new_with_label("Restore defaults");
    space_label = gtk_label_new("");
    g_signal_connect(restore_button,"clicked",G_CALLBACK(restore_defaults),space_label);

    gtk_box_pack_start(GTK_BOX(restore_hbox),space_label, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(restore_hbox),restore_button, FALSE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(top_box), restore_hbox, FALSE, TRUE,  2);

    gtk_widget_show_all(con);
}


static void pref_destroy(GtkWidget *con)
{
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(con));
    if(child != NULL)
    {
        gtk_container_remove(GTK_CONTAINER(con), child);
    }
}

static gmpcMetaDataPlugin glyros_metadata_object =
{
    .get_priority   = glyros_fetch_cover_priority,
    .set_priority   = glyros_fetch_cover_priority_set,
    .get_metadata   = glyros_fetch
};

static gmpcPrefPlugin glyros_pref_object =
{
    .construct = pref_construct,
    .destroy   = pref_destroy,
};


gmpcPlugin glyros_plugin =
{
    .name           = ("Glyros fetcher plugin"),
    .version        = {0,0,1},
    .plugin_type    = GMPC_PLUGIN_META_DATA,
    .init           = glyros_init,
    .pref 		= &glyros_pref_object,
    .metadata       = &glyros_metadata_object,
    .get_enabled    = glyros_get_enabled,
    .set_enabled    = glyros_set_enabled,
};
#endif
