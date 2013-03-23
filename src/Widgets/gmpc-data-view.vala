using Gmpc;
using Gtk;
const int NUM_COLS = 20;
const int[] col_ids = {
    Gmpc.MpdData.ColumnTypes.MARKUP,
    Gmpc.MpdData.ColumnTypes.SONG_ARTIST,			      /* album name */
    Gmpc.MpdData.ColumnTypes.SONG_ALBUM,			      /* album name */
    Gmpc.MpdData.ColumnTypes.SONG_TITLE,			      /* song title */
    Gmpc.MpdData.ColumnTypes.SONG_TITLEFILE,		    /* song title */
    Gmpc.MpdData.ColumnTypes.SONG_GENRE,			      /* song genre */
    Gmpc.MpdData.ColumnTypes.SONG_TRACK,			      /* song track */
    Gmpc.MpdData.ColumnTypes.SONG_NAME,			      /* stream name */
    Gmpc.MpdData.ColumnTypes.SONG_COMPOSER,		    /* composer name */
    Gmpc.MpdData.ColumnTypes.SONG_PERFORMER,		    /* performer */
    Gmpc.MpdData.ColumnTypes.SONG_DATE,			      /* date */
    Gmpc.MpdData.ColumnTypes.SONG_LENGTH_FORMAT,	  /* length formatted */
    Gmpc.MpdData.ColumnTypes.SONG_DISC,			      /* disc */
    Gmpc.MpdData.ColumnTypes.SONG_COMMENT,			    /* comment */
    Gmpc.MpdData.ColumnTypes.ICON_ID,				      /* icon id */
    Gmpc.MpdData.ColumnTypes.SONG_POS,
    Gmpc.MpdData.ColumnTypes.SONG_ALBUMARTIST,
    Gmpc.MpdData.ColumnTypes.PATH_EXTENSION,				/* Extension */
    Gmpc.MpdData.ColumnTypes.PATH_DIRECTORY,				/* Directory */
    Gmpc.MpdData.ColumnTypes.SONG_PRIORITY,
};
const string[] col_names = {
    N_("Markup"),
    N_("Artist"),
    N_("Album"),
    N_("Title"),
    N_("File"),
    N_("Genre"),
    N_("Track"),
    N_("Name"),
    N_("Composer"),
    N_("Performer"),
    N_("Date"),
    N_("Duration"),
    N_("Disc"),
    N_("Comment"),
    N_("Icon Id"),
    N_("Position"),
    N_("AlbumArtist"),
    N_("Extension"),
    N_("Directory"),
    N_("Priority")
};

const bool[] col_enabled = {
    false,//"Markup",
    true, //"Artist",
    true,//"Album",
    true,//"Title",
    false,//"File",
    false,//"Genre",
    false,//"Track",
    false,//"Name",
    false,//"Composer",
    false,//"Performer",
    false,//"Date",
    false,//"Duration",
    false,//"Disc",
    false,//"Comment",
    true,//"Icon Id"
    false,//"Position"
    false,//"AlbumArtist"
    false,//Extension
    false,//Directory
    false//Priority

};

const int[]  col_position = {
    14,//"Markup",
    3, //"Artist",
    2,//"Album",
    1,//"Title",
    4,//"File",
    5,//"Genre",
    6,//"Track",
    7,//"Name",
    8,//"Composer",
    9,//"Performer",
    10,//"Date",
    11,//"Duration",
    12,//"Disc",
    13,//"Comment",
    0,//"Icon Id"
    15, // "Position"
    18, // "AlbumArtist"
    16,// Extension
    17, // Directory
    19
};

public class Gmpc.DataView : Gtk.TreeView
{
    /**
     * If we are a play-queue we should treat the content.
     * slightly different.
     * e.g. add-replace will be play-crop
     */
    public bool is_play_queue {get; set; default=false;}

    /**
     * The name of the treeview. 
     * This is used to store the column layout.
     */
    public string uid {get; set; default="default";}


    /**
     * Construction function.
     */
    public DataView()
    {
        // Connect row activated signal.
        this.row_activated.connect(__row_activated);
    }


    /**
     * Populate the treeview with the right columns.
     * The treeview should have a name now.
     */
    public void populate()
    {
        for(int i = 0; i < NUM_COLS; i++) {
            Gtk.TreeViewColumn col = new Gtk.TreeViewColumn();
            col.set_title(col_names[i]);

            this.insert_column(col, col_position[i]); 
        }
    }



    /**
     * Internal functions.
     */
    private void __row_activated(Gtk.TreePath path, Gtk.TreeViewColumn col)
    {

    }
}
