[CCode (cprefix = "mpd_", cheader_filename = "libmpd/libmpd.h")]

namespace MPD {

    [CCode (cname = "MpdObj",cprefix="mpd_", cheader_filename="libmpd/libmpd.h")]
    [Compact]
    public class Server {

        public MPD.Song playlist_get_song(int songid);
        public weak MPD.Song playlist_get_current_song();
        public int player_get_next_song_id();
        [CCode (cname="mpd_server_get_database_update_time")]
        public int get_database_update_time();
        [CCode (cname="mpd_status_db_is_updating")]
        public bool is_updating_database();
    }


    [CCode (cname = "mpd_Song",
    free_function = "mpd_freeSong", 
    copy_function = "mpd_songDup", 
    cheader_filename = "libmpd/libmpdclient.h,libmpd/libmpd.h")]
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
        [CCode (cname="mpd_songDup")]
        public unowned MPD.Song copy ();
        [CCode (instance_pos = -1)]
        public void markup (char[] buffer, int length, string markup);
        [CCode (cname = "mpd_freeSong")]
        public static void free;
    }
    namespace Sticker {
        namespace Song {
            public void     set(MPD.Server connection, string file, string tag, string value);
            public string   get(MPD.Server connection, string file, string tag);
        }
        public bool supported(MPD.Server connection);
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
                public string tag;
              
                [CCode (cname="mpd_data_get_next")]
                [ReturnsModifiedPointer ()]
                public void next_free();


                [CCode (cheader_filename="misc.h", cname="misc_sort_mpddata_by_album_disc_track")]
                [ReturnsModifiedPointer ()]
                public void sort_album_disc_track();

                [CCode (cname="mpd_data_get_next_real")] 
                public weak Item? next(bool free);

                [CCode (cname="mpd_data_get_first")] 
                public weak Item? get_first();

                [CCode (cname="mpd_data_get_first")] 
                [ReturnsModifiedPointer ()]
                public void first();

                [CCode (cname="mpd_data_concatenate")]
                [ReturnsModifiedPointer ()]
                public void concatenate(owned MPD.Data.Item b); 

                [CCode (cname="mpd_new_data_struct_append")]
                [ReturnsModifiedPointer ()]
                public void append_new();
            }
    }
    namespace PlayQueue {
        [CCode (cname="mpd_playlist_add")]
        public void add_song(MPD.Server server, string path);

        [CCode (cname="mpd_playlist_queue_add")]
        public void queue_add_song(MPD.Server server, string path);
        [CCode (cname="mpd_playlist_queue_commit")]
        public void queue_commit(MPD.Server server);
        [CCode (cname="mpd_playlist_clear")]
        public void clear(MPD.Server server);
    }

    namespace Player {
        [CCode (cname="mpd_player_play")]
        public void play(MPD.Server server);
        public void next(MPD.Server server);
        public void prev(MPD.Server server);
        public void pause(MPD.Server server);
        public void stop(MPD.Server server);
        public MPD.Player.State get_state(MPD.Server server);
        [CCode (cprefix="MPD_STATUS_STATE_")]
        public enum State{
            UNKNOWN = 0,
            STOP    = 1,
            PLAY    = 2,
            PAUSE   = 3
        }
    }

    namespace Database {
        public MPD.Data.Item? get_playlist_content(MPD.Server server, string playlist_name); 
        public void playlist_list_add(MPD.Server server, string playlist_name, string path);
        public void playlist_list_delete(MPD.Server server, string playlist_name, int pos);

        [CCode (cname="mpd_database_search_field_start")]
        public void search_field_start(MPD.Server server,MPD.Tag.Type type);

        [CCode (cname="mpd_database_search_start")]
        public void search_start(MPD.Server server,bool exact);
        public MPD.Data.Item? search_commit(MPD.Server server);

        [CCode (cname="mpd_database_search_add_constraint")]
        public void search_add_constraint(MPD.Server server, MPD.Tag.Type type, string value);

    }

    namespace Tag {
        [CCode (cname="mpd_TagItems", cprefix = "MPD_TAG_ITEM_", cheader_filename = "libmpd/libmpdclient.h")]
            public enum Type{
                ARTIST,
                    ALBUM,
                    TITLE,
                    TRACK,
                    NAME,
                    GENRE,
                    DATE,
                    COMPOSER,
                    PERFORMER,
                    COMMENT,
                    DISC,
                    FILENAME,
                    ALBUM_ARTIST,
                    ANY
            }
    }

}
