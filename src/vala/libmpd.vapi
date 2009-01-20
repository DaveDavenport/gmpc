[CCode (cprefix = "mpd_", cheader_filename = "libmpd/libmpd.h")]

namespace MPD {

    [CCode (cname = "MpdOb",cprefix="mpd_", cheader_filename="libmpd/libmpd.h")]
    [Compact]
    [Immutable]
    public class Server {

        
        public MPD.Song playlist_get_song(int songid);
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

}

namespace Gmpc {
    [Immutable]
    [CCode (cname="gmpcPlugin" , cheader_filename="plugin.h",
        has_construct_function = false
    )]
    public struct Plugin { 
        public weak string name;
        public int type;

    }


}
