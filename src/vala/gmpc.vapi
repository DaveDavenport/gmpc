namespace Gmpc {

    [CCode (cname="TRUE",cheader_filename="gtktransition.h")]
    static bool use_transition;

    [CCode (cname = "gmpcconn", cheader_filename="main.h")]
    static Connection gmpcconn; 

    [CCode (cname = "connection", cheader_filename="main.h")]
    static MPD.Server server;

    [CCode (cname = "gmw", cheader_filename="main.h")]
    static MetaWatcher metawatcher;

    [CCode (cheader_filename="gmpc-meta-watcher.h")]
    public class MetaWatcher {
        signal void data_changed(MPD.Song song,  Gmpc.MetaData.Type type, Gmpc.MetaData.Result result,MetaData.Item met);


        [CCode ( cname="gmpc_meta_watcher_get_meta_path", cheader_filename="gmpc-meta-watcher.h" )]
        public MetaData.Result query(MPD.Song song, Gmpc.MetaData.Type type, out MetaData.Item met);

    }

    [CCode (cheader_filename="gmpc-connection.h")]
    public class Connection {
        signal void connection_changed(MPD.Server server, int connect);
        signal void status_changed (MPD.Server server, MPD.Status.Changed what);
    }

   namespace MetaData {
    
        [CCode (cname="MetaDataContentType", cprefix = "META_DATA_CONTENT_", cheader_filename = "metadata.h")]
        public enum ContentType {
            EMPTY,
            URI,
            TEXT,
            RAW,
            HTML,
            STRV,
            TEXT_LIST
        }
        [CCode (cname="MetaData", cheader_filename="metadata.h")]
        [Compact]
        [Immutable]
        [CCode (free_function="meta_data_free")]
        public class Item {
           public Gmpc.MetaData.Type type; 
           public weak string plugin_name;
           public int size;
           public Gmpc.MetaData.ContentType content_type;

           [CCode (cname="meta_data_is_empty")]
           public bool is_empty();

           [CCode (cname="meta_data_set_uri")]
           public void set_uri(string uri);
           /* add accessors? */
           [CCode (cname="meta_data_get_image")]
           public weak uchar[] get_raw();

           [CCode (cname="meta_data_get_text")]
           public weak string  get_text();
           /* same as get_text */

           [CCode (cname="meta_data_get_uri")]
           public weak string get_uri();

           [CCode (cname="meta_data_get_html")]
           public weak string get_html();

           [CCode (cname="meta_data_get_text_vector")]
           public weak string[] get_text_vector();

           [CCode (cname="meta_data_get_text_list")]
           public weak GLib.List<weak string> get_text_list();

           [CCode (cname="meta_data_is_text_list")]
           public bool  is_text_list();
           [CCode (cname="meta_data_dup_steal")]
           public MetaData.Item dup_steal();
        }
        [CCode (cname="MetaDataType", cprefix = "META_", cheader_filename = "metadata.h")]
        public enum Type {
            ALBUM_ART       = 1,
            ARTIST_ART      = 2,
            ALBUM_TXT       = 4,
            ARTIST_TXT      = 8,
            SONG_TXT        = 16,
            ARTIST_SIMILAR  = 32,
            SONG_SIMILAR    = 64,
            GENRE_SIMILAR   = 128,
            SONG_GUITAR_TAB = 256,
            QUERY_DATA_TYPES = 65535,
            QUERY_NO_CACHE   = 65536
        }

        [CCode (cname="MetaDataResult", cprefix = "META_DATA_", cheader_filename = "metadata.h")]
        public enum Result {
            AVAILABLE,
            UNAVAILABLE,
            FETCHING
        }
       

        public delegate void Callback (void *handle,string? plugin_name, GLib.List<MetaData.Item>? list);
        [CCode ( cname="metadata_get_list", cheader_filename="metadata.h" )]
        public void* get_list(MPD.Song song, Type type, Callback callback);

        [CCode ( cname="metadata_get_list_cancel", cheader_filename="metadata.h" )]
        public void* get_list_cancel(void *handle);


        [CCode ( cname="meta_data_set_cache", cheader_filename="metadata_cache.h")]
        public void set_metadata(MPD.Song song, Result result, Gmpc.MetaData.Item met); 

        [CCode ( cname="gmpc_get_metadata_filename", cheader_filename="metadata.h")]
        public string get_metadata_filename(Type type, MPD.Song song, string? extention);

        [CCode ( cname="GmpcMetaImage", cheader_filename="gmpc-metaimage.h")]
        public class Image: Gtk.Widget {
            [CCode (cname="gmpc_metaimage_new_size")]
            public Image(Type type, int size);
            [CCode (cname="gmpc_metaimage_update_cover_from_song")]
            public void update_from_song(MPD.Song song);

            [CCode (cname="gmpc_metaimage_update_cover_from_song_delayed")]
            public void update_from_song_delayed(MPD.Song song);
            [CCode (cname="gmpc_metaimage_set_squared")]
            public void set_squared(bool squared);

        }
        [CCode ( cname="GmpcStatsLabel", cheader_filename="gmpc-stats-label.h")]
        public class StatsLabel : Gtk.Label {
            [CCode (cprefix="")]
            public enum Type {
                ARTIST_NUM_SONGS,
                ARTIST_PLAYTIME_SONGS,
                ARTIST_GENRES_SONGS,
                ARTIST_DATES_SONGS,
                ALBUM_NUM_SONGS,
                ALBUM_PLAYTIME_SONGS,
                ALBUM_GENRES_SONGS,
                ALBUM_DATES_SONGS
            }
            [CCode (cname="gmpc_stats_label_new")]
            public StatsLabel(Type type, MPD.Song song);
        }
        [CCode ( cname="GmpcMetaTextView", cheader_filename="gmpc-meta-text-view.h")]
        public class TextView: Gtk.TextView {
            [CCode (cname="gmpc_meta_text_view_new")]
                public TextView(Type type);
            [CCode (cname="gmpc_meta_text_view_query_text_from_song")]
                public void query_from_song(MPD.Song song);
        }

   }
   namespace Messages {
       [CCode (cprefix = "ERROR_", cheader_filename = "playlist3-messages.h")]
       public enum Level{
        INFO,
        WARNING,
        CRITICAL        
    }

    [CCode (cname = "playlist3_show_error_message", cheader_filename="playlist3-messages.h")]
    void show(string message, Gmpc.Messages.Level level);
   }

   namespace AsyncDownload {
     [CCode (cname="GEADStatus",cprefix = "GEAD_", cheader_filename = "gmpc_easy_download.h")]
        public enum Status {
            DONE,
            PROGRESS,
            FAILED,
            CANCELLED
        }

        [CCode (cname="GEADAsyncHandler", cheader_filename="gmpc_easy_download.h")]
        [Compact]
        [Immutable]
        [CCode (ref_function="", unref_function ="")]
        public class Handle {
            [CCode (cname="gmpc_easy_async_cancel", cheader_filename="gmpc_easy_download.h")]
            public void cancel ();

            [CCode (cname="gmpc_easy_handler_get_data_vala_wrap", cheader_filename="gmpc_easy_download.h")]
            public weak uchar[] get_data();
            [CCode (cname="gmpc_easy_handler_get_uri", cheader_filename="gmpc_easy_download.h")]
            public weak string get_uri();

            [CCode (cname="gmpc_easy_handler_get_user_data", cheader_filename="gmpc_easy_download.h")]
            public void * get_user_data();

            [CCode (cname="gmpc_easy_handler_set_user_data", cheader_filename="gmpc_easy_download.h")]
            public void set_user_data(void *user_data);

            [CCode (cname="gmpc_easy_handler_get_content_size", cheader_filename="gmpc_easy_download.h")]
            public int64 get_content_size();
        }



        public delegate void Callback (Gmpc.AsyncDownload.Handle handle, Gmpc.AsyncDownload.Status status);

        [CCode (cname="gmpc_easy_async_downloader", cheader_filename="gmpc_easy_download.h")]
        public weak Gmpc.AsyncDownload.Handle download(string uri, Gmpc.AsyncDownload.Callback callback);
        
        [CCode (cname="gmpc_easy_download_uri_escape", cheader_filename="gmpc_easy_download.h")]
        public string escape_uri(string part);
   }

    [CCode (cname="gmpc_get_full_glade_path", cheader_filename="plugin.h")]
    public string data_path(string file);
    [CCode (cname="gmpc_get_user_path", cheader_filename="plugin.h")]
    public string user_path(string file);
    [CCode (cname="open_uri", cheader_filename="misc.h")]
    public void open_uri(string uri);

    namespace Playlist {
        [CCode (cname="(GtkWindow *)playlist3_get_window", cheader_filename="plugin.h")]
        public weak Gtk.Window get_window();
[CCode (cname="playlist3_window_is_hidden", cheader_filename="plugin.h")]
        public bool is_hidden();

    }



    [CCode (cname = "config", cheader_filename="plugin.h")]
    static Settings config; 
    [CCode (cheader_filename="config1.h")]
        [Compact]
        [Immutable]
    public class Settings {
        [CCode (cname="cfg_get_single_value_as_int_with_default", cheader_filename="config1.h")]
        public int get_int_with_default(string class, string key, int value); 
        [CCode (cname="cfg_set_single_value_as_int", cheader_filename="config1.h")]
        public int set_int(string class, string key, int value); 
    }

    namespace Misc{
        [CCode (cname="colorshift_pixbuf",cheader_filename="misc.h")]
        public void colorshift_pixbuf(Gdk.Pixbuf dest, Gdk.Pixbuf src, int shift);
        [CCode (cname="misc_header_expose_event",cheader_filename="misc.h")]
        public bool misc_header_expose_event(Gtk.Widget widget, Gdk.EventExpose event);
    }

    /* Browser */
    namespace Browser{
        [CCode (cname="playlist3_insert_browser")]
        public void insert(out Gtk.TreeIter iter, int position);
    }
    namespace Playlist3 {
        [CCode (cname="playlist3_get_category_tree_view")]
        public unowned Gtk.TreeView get_category_tree_view();

    }

    /* objects */
    namespace MpdData {
        [CCode (chader_filename="gmpc-mpddata-treeview.h",cname="GmpcMpdDataTreeview")]
        public class TreeView : Gtk.TreeView {
            [CCode (cname="gmpc_mpddata_treeview_new")]
            TreeView(string name, bool sort, Gtk.TreeModel model);
            [CCode (cname="gmpc_mpddata_treeview_enable_click_fix")]
            public void enable_click_fix();
        }
        [CCode (cheader_filename="misc.h", cname="misc_sort_mpddata_by_album_disc_track")]
        public weak MPD.Data.Item? sort_album_disc_track(owned MPD.Data.Item ?data);
        [CCode (cheader_filename="gmpc-mpddata-model.h")]
        public class Model : GLib.Object, Gtk.TreeModel{
            [CCode (has_construct_function = true,cname="gmpc_mpddata_model_new")]
            public Model();

            [CCode (cname="gmpc_mpddata_model_set_mpd_data")]
            public void set_mpd_data(owned MPD.Data.Item? list);

            [CCode (cname="gmpc_mpddata_model_set_request_artist")]
            public void set_request_artist(string? list);

            public int icon_size;
        }
    }


    namespace Misc {
        [CCode (cname="play_path")]
        public void play_path(string path);
        [CCode (cname="mpd_song_checksum",cheader_filename="misc.h")]
        public string? song_checksum(MPD.Song? song);
    }
}
