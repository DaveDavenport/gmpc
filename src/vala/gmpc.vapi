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
     [CCode (cprefix = "GEAD_", cheader_filename = "gmpc_easy_download.h")]
        public enum Status {
            GEAD_DONE,
            GEAD_PROGRESS,
            GEAD_FAILED,
            GEAD_CANCELLED
        }

        [CCode (cname="GEADAsyncHandler", cheader_filename="gmpc_easy_download.h")]
        [CCode (free_function = "gmpc_easy_async_handle_free")]
        public struct Handle {
        }

        public static delegate void Callback (Gmpc.AsyncDownload.Handle handle, Gmpc.AsyncDownload.Status status, void *data);

        [CCode (cname="gmpc_easy_async_downloader", cheader_filename="gmpc_easy_dowload.h")]
        public Gmpc.AsyncDownload.Handle download(string uri, Gmpc.AsyncDownload.Callback callback, void *data);

   }
}
