namespace Gmpc {
    [CCode (cname = "gmpcconn", cheader_filename="main.h")]
    public Connection gmpcconn; 

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
}
