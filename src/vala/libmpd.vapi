[CCode (cprefix = "mpd_", cheader_filename = "libmpd/libmpd.h")]

namespace MPD {

    [CCode (cname = "MpdObj",cprefix="mpd_", cheader_filename="libmpd/libmpd.h")]
    [Compact]
    public class Server {

        public MPD.Song playlist_get_song(int songid);
        public weak MPD.Song playlist_get_current_song();
        public int player_get_next_song_id();
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
        [CCode (cname="ChangedStatusType", cprefix = "MPD_CST_", cheader_filename = "libmpd/libmpd.h")]
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
                OUTPUT              = 0x80000,
                /** sticker changed */
                STICKER             = 0x100000,
                NEXTSONG            = 0x200000,
                SINGLE_MODE         = 0x400000,
                CONSUME_MODE        = 0x800000
            }
    }
    namespace Data{
        [CCode (cname="MpdDataType", cprefix = "MPD_DATA_TYPE_", cheader_filename = "libmpd/libmpd.h")]
            public enum Type {
                NONE,
                TAG,
                DIRECTORY,
                SONG,
                PLAYLIST,
                OUTPUT_DEV
            }

            [CCode (cname = "MpdData",
                free_function = "mpd_data_free", 
                cheader_filename = "libmpd/libmpd.h")]
            [Compact]
            [Immutable]
            public class Item {
                public Data.Type type;
                public MPD.Song  song;
               
                [CCode (cname="mpd_data_get_next_real")] 
                public weak Item? next(bool free);

                [CCode (cname="mpd_data_get_first")] 
                public weak Item? first();
            }
    }
    namespace Database {
        public MPD.Data.Item? get_playlist_content(MPD.Server server, string playlist_name); 
        public void playlist_list_add(MPD.Server server, string playlist_name, string path);
        public void playlist_list_delete(MPD.Server server, string playlist_name, int pos);
    }


}
