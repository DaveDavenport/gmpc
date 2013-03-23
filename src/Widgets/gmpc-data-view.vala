using Gmpc;
using Gtk;


/**
 * List of columns.
 * * List of column ids to show.
 * * Name of each column
 * * Default set of enabled columns.
 */
const int NUM_COLS = 20;
const int[] gmpc_data_view_col_ids = {
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
    Gmpc.MpdData.ColumnTypes.SONG_PRIORITY
};
const string[] gmpc_data_view_col_names = {
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

const bool[] gmpc_data_view_col_enabled = {
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

const int[]  gmpc_data_view_col_position = {
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
    private Gtk.TreeViewColumn[] tree_columns = new Gtk.TreeViewColumn[NUM_COLS];
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
    public DataView(string name)
    {
        this.uid = name;
        // Connect row activated signal.
        this.row_activated.connect(__row_activated);

        this.set_rules_hint(true);
    }


    /**
     * Deconstructor.
     */
    ~DataView()
    {

    }


    /**
     * Populate the treeview with the right columns.
     * The treeview should have a name now.
     */
    public void populate()
    {
        for(int i = 0; i < NUM_COLS; i++) {
            Gtk.TreeViewColumn col = new Gtk.TreeViewColumn();
            col.set_data("id", i);
            if(gmpc_data_view_col_ids[i] == Gmpc.MpdData.ColumnTypes.ICON_ID)
            {
                /**
                 * Picture.
                 */
                var renderer = new Gtk.CellRendererPixbuf(); 
                renderer.xalign = 0.0f;
                // Set up column
                col.pack_start(renderer, true);
                col.set_attributes(renderer, "icon-name", Gmpc.MpdData.ColumnTypes.ICON_ID);
                col.set_resizable(false);
                col.set_fixed_width(20);
            } else {
                /**
                 * Text column
                 */
                col.set_title(gmpc_data_view_col_names[i]);
                var renderer = new Gtk.CellRendererText(); 
                renderer.ellipsize = Pango.EllipsizeMode.END;
                renderer.weight_set = true;
                // Set up column
                col.pack_start(renderer, true);
                col.set_attributes(renderer, "text", gmpc_data_view_col_ids[i]); 
                col.set_resizable(true);
                if(is_play_queue) {
                // TODO fix this.
                //    col.set_cell_data_func(renderer, (Gtk.CellLayoutDataFunc)highlight_row_current_song_playing);
                }
                col.set_fixed_width(config.get_int_with_default(uid+"-colsize",gmpc_data_view_col_names[i], 200));
            }
            col.set_sizing(Gtk.TreeViewColumnSizing.FIXED);
            col.set_reorderable(true);

            // Fixed width.
            int pos = config.get_int_with_default(uid+"-colpos", gmpc_data_view_col_names[i], gmpc_data_view_col_position[i]);
            this.tree_columns[pos] = col;
        }
        // Add the columns (in right order)
        for(int i = 0; i < NUM_COLS; i++) {
            int id = this.tree_columns[i].get_data("id");
            if(config.get_bool_with_default(uid+"-colshow", gmpc_data_view_col_names[id], gmpc_data_view_col_enabled[id]))
            {
                this.insert_column(this.tree_columns[i], i); 
            }
        }
    }



    /**
     * Internal functions.
     */
    private void __row_activated (Gtk.TreePath path, Gtk.TreeViewColumn col)
    {
        Gtk.TreeIter iter;
        Gtk.TreeModel? model = this.get_model();
        if(model != null) {
            if(model.get_iter(out iter, path))
            {
                if(is_play_queue) {
                    int song_id;
                    model.get(iter, Gmpc.MpdData.ColumnTypes.SONG_ID, out song_id);
                    MPD.Player.play_id(Gmpc.server, song_id); 
                } else {
                    string song_path;
                    model.get(iter, Gmpc.MpdData.ColumnTypes.PATH, out song_path);
                    MpdInteraction.play_path(song_path);
                }
            }
        }
    }
    /**
     * Check if current row is playing.
     */
    private void highlight_row_current_song_playing(Gtk.TreeViewColumn col, Gtk.CellRenderer renderer, Gtk.TreeModel model, Gtk.TreeIter iter)
    {
       (renderer as Gtk.CellRendererText).weight = Pango.Weight.BOLD;
    }
}
