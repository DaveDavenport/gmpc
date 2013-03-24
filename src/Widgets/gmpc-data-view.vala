using Gmpc;

using Gtk;

const string log_domain = "Gmpc.DataView";

/** The Default column width. */
const int default_column_width = 200;

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
        log(log_domain, LogLevelFlags.LEVEL_INFO, "Constructing dataview: "+name);

        this.uid = name;
        // Connect row activated signal.
        this.row_activated.connect(__row_activated);
        this.key_release_event.connect(__key_release_event_callback);
        // When it getst he destroy signal.
        this.destroy.connect(column_store_state);

        this.set_rules_hint(true);

        this.get_selection().set_mode(Gtk.SelectionMode.MULTIPLE);
        this.set_fixed_height_mode(true);
        // Create the view.
        column_populate();
    }


    /**
     * Deconstructor.
     */
    ~DataView()
    {
    }

    /**
     * Internal functions.
     */

    /**
     * Store the position, visibility and width of the columns
     */
    private void column_store_state()
    {
        // Save the position of the columns
        var columns = get_columns();
        int index = 0;
        foreach(var column in columns)
        {
            int col_index = column.get_data("index");
            int width = column.get_width();
            config.set_int(uid+"-colpos", gmpc_data_view_col_names[col_index], index);
            config.set_bool(uid+"-colshow", gmpc_data_view_col_names[col_index], column.visible);
            // Only store width if bigger then 0.
            if(width > 0 ) {
                config.set_int(uid+"-colsize",gmpc_data_view_col_names[col_index], width); 
            }
            index++;
        }
    }
    // Hack to make vala not destroy the menu directly.
    private Gtk.Menu column_selection_menu = null;
    private void column_show_selection_menu()
    {
        column_selection_menu = new Gtk.Menu();
        foreach(var col in tree_columns)
        {
            int index = col.get_data("index");
            // Do not show the icon id in the selection list.
            if(gmpc_data_view_col_ids[index] == MpdData.ColumnTypes.ICON_ID) continue;
            var item = new Gtk.CheckMenuItem.with_label(gmpc_data_view_col_names[index]);
            if(col.visible) {
                item.set_active(true);
            }
            // On activation toggle the state.
            item.activate.connect((source) => {
                col.visible = (source as Gtk.CheckMenuItem).get_active();
            });
            column_selection_menu.append(item);
        }
        column_selection_menu.show_all();
        column_selection_menu.popup(null, null, null, 0, Gtk.get_current_event_time());
    }
    /**
     * Populate the treeview with the right columns.
     * The treeview should have a name now.
     */
    private void column_populate()
    {
        for(int i = 0; i < NUM_COLS; i++)
        {
            Gtk.TreeViewColumn col = new Gtk.TreeViewColumn();
            col.set_data("index", i);
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
                col.clickable = true;
                // If the user clicks on the column, show dropdown allowing to enable/disable columns.
                col.clicked.connect((source) => {
                        column_show_selection_menu();
                        });
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
                
                int width = config.get_int_with_default(uid+"-colsize",gmpc_data_view_col_names[i], default_column_width);
                // Do not set to size 0, then revert back to 200.
                col.set_fixed_width(width>0?width:default_column_width);
            }
            col.set_sizing(Gtk.TreeViewColumnSizing.FIXED);
            col.set_reorderable(true);

            // Fixed width.
            int pos = config.get_int_with_default(uid+"-colpos", gmpc_data_view_col_names[i], gmpc_data_view_col_position[i]);
            this.tree_columns[pos] = col;
        }
        // Add the columns (in right order)
        for(int i = 0; i < NUM_COLS; i++) {
            int index = this.tree_columns[i].get_data("index");
            this.insert_column(this.tree_columns[i], i); 
            this.tree_columns[i].set_visible(config.get_bool_with_default(uid+"-colshow", gmpc_data_view_col_names[index], gmpc_data_view_col_enabled[index]));
        }
    }



    

    /**
     * Function handles the row-activate signal.
     */
    private void __row_activated (Gtk.TreePath path, Gtk.TreeViewColumn col)
    {
        Gtk.TreeModel? model = this.get_model();
        if(model != null)
        {
            Gtk.TreeIter iter;
            if(!model.get_iter(out iter, path)) return;
            if(is_play_queue) {
                /* If we are play-queue, play the selected song. */
                int song_id;
                model.get(iter, Gmpc.MpdData.ColumnTypes.SONG_ID, out song_id);
                MPD.Player.play_id(Gmpc.server, song_id); 
            } else {
                /* If we are a song browser, add the path and play it. */
                string song_path;
                model.get(iter, Gmpc.MpdData.ColumnTypes.PATH, out song_path);
                MpdInteraction.play_path(song_path);
            }
        }
    }
    /**
     * Check if current row is playing.
     * TODO
     */
    private void highlight_row_current_song_playing(Gtk.TreeViewColumn col, Gtk.CellRenderer renderer, Gtk.TreeModel model, Gtk.TreeIter iter)
    {
       (renderer as Gtk.CellRendererText).weight = Pango.Weight.BOLD;
    }


    /**
     * Handle keyboard input.
     */
    private bool __key_release_event_callback_play_queue(Gdk.EventKey event)
    {
        if (event.keyval == Gdk.Key_Q) 
        {
            // remove priority.
            return selected_songs_remove_priority();
        }
        else if (event.keyval == Gdk.Key_q)
        {
            // Raise priority.
            return selected_songs_raise_priority();
        }
        else if (event.keyval == Gdk.Key_d)
        {
            if(!selected_songs_remove())
            {
                // Detach model (for some reason keeping it attached
                // Makes thing break, work-around for now)
                // TODO: fixme
                var model = get_model();
                this.model = null; 
                // Clear
                MPD.PlayQueue.clear(server);
                // Re-add model
                this.model = model;
                return true;
            }
        }
        return false;
    }
    private bool __key_release_event_callback(Gdk.EventKey event)
    {
        if(event.keyval == Gdk.Key_y)
        {
            // Copy data to clipboard

        }
        else if (event.keyval == Gdk.Key_c)
        {
            // Cut (if available) into clipboard
        }
        else if (event.keyval == Gdk.Key_P)
        {
            // Paste before  
        }
        else if (event.keyval == Gdk.Key_p)
        {
            // Paste after
        }
        else if (event.keyval == Gdk.Key_Escape)
        {

        }
        else if (event.keyval == Gdk.Key_m)
        {
            // Configure columns
            column_show_selection_menu();
            return true;
        }

        // Commands specific to play_queue
        if(is_play_queue)
        {
            if(__key_release_event_callback_play_queue(event)) return true;
        }
        else
        {
            if(event.keyval == Gdk.Key_i)
            {
                // Insert
            }
        }
        return false;
    }

    /**
     * Interaction on selected songs.
     */
    // Set priority on the selected songs.
    private bool selected_songs_raise_priority()
    {
        if(server.check_command_allowed("prioid") != MPD.Server.Command.ALLOWED) return false;
        var selection = this.get_selection();

        if(selection.count_selected_rows() > 254) {
            Gmpc.Messages.show(_("You can only queue 254 songs at the time."), Gmpc.Messages.Level.WARNING);
            return false;
        }

        int priority = 255;
        Gtk.TreeModel model;
        foreach(var path in selection.get_selected_rows(out model))
        {
            Gtk.TreeIter iter;
            if(model.get_iter(out iter, path))
            {
                int song_id;
                model.get(iter,Gmpc.MpdData.ColumnTypes.SONG_ID, out song_id);
                MPD.PlayQueue.set_priority(server, song_id, priority--);
            }            
        }
        return true;
    }
    // Remove the set priority from the selected songs.
    private bool selected_songs_remove_priority()
    {
        if(server.check_command_allowed("prioid") != MPD.Server.Command.ALLOWED) return false;
        var selection = this.get_selection();

        Gtk.TreeModel model;
        foreach(var path in selection.get_selected_rows(out model))
        {
            Gtk.TreeIter iter;
            if(model.get_iter(out iter, path))
            {
                int song_id;
                model.get(iter,Gmpc.MpdData.ColumnTypes.SONG_ID, out song_id);
                MPD.PlayQueue.set_priority(server, song_id, 0);
            }            
        }
        return true;
    }
    // Remove the selected songs from the play queue.
    private bool selected_songs_remove()
    {
        int deleted_rows = 0;
        var selection = this.get_selection();

        Gtk.TreeModel model;
        foreach(var path in selection.get_selected_rows(out model))
        {
            Gtk.TreeIter iter;
            if(model.get_iter(out iter, path))
            {
                int song_id;
                model.get(iter,Gmpc.MpdData.ColumnTypes.SONG_ID, out song_id);
                MPD.PlayQueue.queue_delete_id(server, song_id);
                deleted_rows++;
            }            
        }
        MPD.PlayQueue.queue_commit(server);
        return (deleted_rows > 0);

    }
}
