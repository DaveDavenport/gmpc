[CCode (cprefix = "mpd_", cheader_filename = "libmpd/libmpd.h")]

namespace MPD {

    [CCode (cname = "MpdObj",cprefix="mpd_", cheader_filename="libmpd/libmpd.h")]
    [Compact]
    public class Server {

        public MPD.Song playlist_get_song(MPD.Server server, int songid);
    }

    [CCode (cname = "mpd_Song",
    free_function = "mpd_freeSong", 
    copy_function = "mpd_songDup", 
    cheader_filename = "libmpd/libmpdclient.h")]
    [Compact]
    [Immutable]
    public class Song {
        [CCode (cname = "mpd_newSong")]
        public Song ();
        public string file;
        public string artist;
        public string title;
        public string album;
        public string track;
        public string name;
        public string date;
        public string genre;
        public string performer;
        public string disc;
        public string comment;
        public string albumartist;
        public int    time;
        public int    pos;
        public int    id;
        public unowned MPD.Song copy ();
        [CCode (instance_pos = -1)]
        public void markup (char[] buffer, int length, string markup);
    }
    namespace Sticker {
        namespace Song {
            public void     set(MPD.Server connection, string file, string tag, string value);
            public string   get(MPD.Server connection, string file, string tag);
        }

    }
    namespace Status {
        [CCode (cname="ChangedStatusType", cprefix = "MPD_CST_", cheader_filename = "metadata.h")]
            public enum Changed {
                /** The playlist has changed */
                PLAYLIST      = 0x0001,
                /** The song position of the playing song has changed*/
                SONGPOS       = 0x0002,
                /** The songid of the playing song has changed */
                SONGID        = 0x0004,
                /** The database has changed. */
                DATABASE      = 0x0008,
                /** the state of updating the database has changed.*/
                UPDATING      = 0x0010,
                /** the volume has changed */
                VOLUME        = 0x0020,
                /** The total time of the currently playing song has changed*/
                TOTAL_TIME    = 0x0040,
                /** The elapsed time of the current song has changed.*/
                ELAPSED_TIME  = 0x0080,
                /** The crossfade time has changed. */
                CROSSFADE     = 0x0100,
                /** The random state is changed.     */                 
                RANDOM        = 0x0200,
                /** repeat state is changed.     */                
                REPEAT        = 0x0400,
                /** Not implemented  */                                  
                AUDIO         = 0x0800,
                /** The state of the player has changed.*/               
                STATE         = 0x1000,
                /** The permissions the client has, has changed.*/  
                PERMISSION    = 0x2000,
                /** The bitrate of the playing song has changed.    */ 
                BITRATE       = 0x4000,
                /** the audio format of the playing song changed.*/
                AUDIOFORMAT   = 0x8000,
                /** the queue has changed */
                STORED_PLAYLIST		  = 0x20000,
                /** server error */
                SERVER_ERROR        = 0x40000,
                /** output changed */
                OUTPUT              = 0x80000

            }

    }

}

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
