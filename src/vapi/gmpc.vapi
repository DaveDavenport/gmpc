using Gmpc.PanedSizeGroup;

namespace Gmpc {
    [CCode (cname="TRUE",cheader_filename="gtk/gtk.h,gtktransition.h")]
    static bool use_transition;

    [CCode (cname="paned_size_group", cheader_filename="plugin.h")]
    static PanedSizeGroup paned_size_group;

    [CCode (cname = "gmpcconn", cheader_filename="main.h")]
    static Connection gmpcconn; 

    [CCode (cname = "connection", cheader_filename="plugin.h")]
    static MPD.Server server;

    [CCode (cname = "gmw", cheader_filename="main.h")]
    static MetaWatcher metawatcher;

    [CCode (cname = "gmpc_profiles", cheader_filename="plugin.h")]
    static Profiles profiles;

    [CCode (cheader_filename="gmpc-meta-watcher.h")]
    public class MetaWatcher {
        public signal void data_changed(MPD.Song song,  Gmpc.MetaData.Type type, Gmpc.MetaData.Result result,MetaData.Item met);


        [CCode ( cname="gmpc_meta_watcher_get_meta_path", cheader_filename="gmpc-meta-watcher.h" )]
        public Gmpc.MetaData.Result query(MPD.Song song, Gmpc.MetaData.Type type, out MetaData.Item met);

    }


   namespace MetaData {
    
        [CCode (cname="MetaDataContentType", cprefix = "META_DATA_CONTENT_", cheader_filename = "libmpd/libmpd.h,metadata.h")]
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
            [CCode (cname="meta_data_new")]
            public Item ();
            public Gmpc.MetaData.Type type; 
           public weak string plugin_name;
           public int size;
           public Gmpc.MetaData.ContentType content_type;

           [CCode (cname="meta_data_is_empty")]
           public bool is_empty();

           [CCode (cname="meta_data_set_uri")]
           public void set_uri(string uri);
           /* add accessors? */
           [CCode (cname="meta_data_get_raw")]
           public weak uchar[] get_raw();

           [CCode (cname="meta_data_get_text")]
           public weak string  get_text();
           [CCode (cname="meta_data_get_text_from_html")]
           public string get_text_from_html();
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
        [CCode ( cname="metadata_get_list", cheader_filename="libmpd/libmpd.h,metadata.h" )]
        public void* get_list(MPD.Song song, Type type, Callback callback);

        [CCode ( cname="metadata_get_list_cancel", cheader_filename="metadata.h" )]
        public void* get_list_cancel(void *handle);


        [CCode ( cname="meta_data_set_cache", cheader_filename="metadata-cache.h")]
        public void set_metadata(MPD.Song song, Result result, Gmpc.MetaData.Item met); 

        [CCode ( cname="gmpc_get_metadata_filename", cheader_filename="libmpd/libmpd.h,metadata.h")]
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
            [CCode (cname="gmpc_metaimage_set_hide_on_na")]
            public void set_hide_on_na(bool hide);

            [CCode (cname="gmpc_metaimage_set_scale_up")]
            public void set_scale_up(bool scale);

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
                public bool use_monospace;
            [CCode (cname="gmpc_meta_text_view_new")]
                public TextView(Type type);
            [CCode (cname="gmpc_meta_text_view_query_text_from_song")]
                public void query_from_song(MPD.Song song);
                public bool force_ro;
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
        [CCode (cname="cfg_get_single_value_as_string_with_default", cheader_filename="config1.h")]
        public string get_string_with_default(string class, string key, string value); 
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


        [CCode (cname="format_time_real", cheader_filename="misc.h")]
        public string format_time(ulong seconds, string pre);
    }

    /* Browser */
    namespace Browser{
        [CCode (cname="playlist3_insert_browser")]
        public void insert(out Gtk.TreeIter iter, int position);
        namespace File {
            [CCode (cname="pl3_file_browser_open_path")]
            public void open_path(string path);
        }
        namespace Find {
            [CCode (cname="pl3_find2_ec_database")]
            public void query_database(void *user_data, string query);
        }
        namespace Metadata {
            [CCode (cname="info2_fill_artist_view")]
            public void show_artist(string artist);
            
            [CCode (cname="info2_fill_album_view")]
            public void show_album(string artist,string album);
        }
    }
    namespace Playlist3 {
        [CCode (cname="playlist3_get_category_tree_view")]
        public unowned Gtk.TreeView get_category_tree_view();

        [CCode (cname="main_window_add_status_icon")]
        public void add_status_icon(Gtk.Widget widget);
    }

    /* objects */
    namespace MpdData {
        [CCode (chader_filename="gmpc-mpddata-treeview.h",cname="GmpcMpdDataTreeview")]
        public class TreeView : Gtk.TreeView {
            [CCode (cname="gmpc_mpddata_treeview_new")]
            public TreeView(string name, bool sort, Gtk.TreeModel model);
            [CCode (cname="gmpc_mpddata_treeview_enable_click_fix")]
            public void enable_click_fix();
            [CCode (cname="gmpc_mpddata_treeview_right_mouse_intergration")]
            public int right_mouse_integration(Gtk.Menu menu);
        }

        [CCode (cheader_filename="gmpc-mpddata-model.h")]
        public class Model : GLib.Object, Gtk.TreeModel{
            [CCode (has_construct_function = true,cname="gmpc_mpddata_model_new")]
            public Model();

            [CCode (cname="gmpc_mpddata_model_set_mpd_data")]
            public void set_mpd_data(owned MPD.Data.Item? list);

            [CCode (cname="gmpc_mpddata_model_set_request_artist")]
            public void set_request_artist(string? list);

            [CCode (cname="gmpc_mpddata_model_get_request_artist")]
            public weak string get_request_artist();
            public int icon_size;
        }
    }


    namespace Misc {
        [CCode (cname="mpd_song_checksum",cheader_filename="misc.h")]
        public string? song_checksum(MPD.Song? song);
    }
    namespace MpdInteraction {
        [CCode (cname="play_path",cheader_filename="mpdinteraction.h")]
        public void play_path(string path);
        [CCode (cname="submenu_for_song")]
        public void submenu_for_song(Gtk.Widget menu, MPD.Song song);
    }

        [CCode (cheader_filename="gmpc-profiles.h")]
        [Compact]
        [Immutable]
        class Profiles {
            [CCode (cname="gmpc_profiles_get_current",cheader_filename="gmpc-profiles.h")]
            public string? get_current_id();
            public void set_db_update_time(string id, int value);
            public int get_db_update_time(string id);
            public weak string? get_music_directory(string id); 

    }

    namespace Fix{
        [CCode (cname="gdk_pixbuf_loader_write", cheader_filename="gdk-pixbuf/gdk-pixdata.h")]
            public void write_loader(Gdk.PixbufLoader loader, string data, size_t size) throws GLib.Error;

        [CCode (cname="screenshot_add_border", cheader_filename="misc.h")]
            public void add_border(Gdk.Pixbuf image);
    }
    [CCode (cheader_filename="Widgets/pixbuf-cache.h")]
    namespace PixbufCache {
        [CCode (cname="pixbuf_cache_lookup_icon")]
            public Gdk.Pixbuf? lookup_icon(int size, string url);
        [CCode (cname="pixbuf_cache_add_icon")]
            public void add_icon(int size, string url, Gdk.Pixbuf pb);

    }
}
