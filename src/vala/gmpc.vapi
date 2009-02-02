namespace Gmpc {
    [CCode (cname = "gmpcconn", cheader_filename="main.h")]
    static Connection gmpcconn; 

    [CCode (cheader_filename="gmpc-connection.h")]
    public class Connection {
        signal void connection_changed(MPD.Server server, int connect);
        signal void status_changed (MPD.Server server, MPD.Status.Changed what);
    }

    [Immutable]
    [CCode (cname="gmpcPlugin" , cheader_filename="plugin.h",
        has_construct_function = false
    )]
    public struct Plugin { 
        public weak string name;
        public int type;

    }
   
   namespace MetaData {
        [CCode (cprefix = "META_", cheader_filename = "metadata.h")]
        public enum Type {
            ALBUM_ART       = 1,
            ARTIST_ART      = 2,
            ALBUM_TXT       = 4,
            ARTIST_TXT      = 8,
            SONG_TXT        = 16,
            ARTIST_SIMILAR  = 32,
            SONG_SIMILAR    = 64,
            QUERY_DATA_TYPES = 127,
            QUERY_NO_CACHE   = 128
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
            [CCode (cname="gmpc_easy_async_free_handler", cheader_filename="gmpc_easy_download.h")]
            public void free ();
            [CCode (cname="gmpc_easy_async_cancel", cheader_filename="gmpc_easy_download.h")]
            public void cancel ();

            [CCode (cname="gmpc_easy_handler_get_data", cheader_filename="gmpc_easy_download.h")]
            public weak string get_data(out int64 length);
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
}
