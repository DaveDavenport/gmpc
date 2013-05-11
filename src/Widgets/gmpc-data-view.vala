using Gmpc;

using Gtk;

public List<string> paste_queue;
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
    Gmpc.MpdData.ColumnTypes.COL_MARKUP,
    Gmpc.MpdData.ColumnTypes.COL_SONG_ARTIST,			      /* album name */
    Gmpc.MpdData.ColumnTypes.COL_SONG_ALBUM,			      /* album name */
    Gmpc.MpdData.ColumnTypes.COL_SONG_TITLE,			      /* song title */
    Gmpc.MpdData.ColumnTypes.COL_SONG_TITLEFILE,		    /* song title */
    Gmpc.MpdData.ColumnTypes.COL_SONG_GENRE,			      /* song genre */
    Gmpc.MpdData.ColumnTypes.COL_SONG_TRACK,			      /* song track */
    Gmpc.MpdData.ColumnTypes.COL_SONG_NAME,			      /* stream name */
    Gmpc.MpdData.ColumnTypes.COL_SONG_COMPOSER,		    /* composer name */
    Gmpc.MpdData.ColumnTypes.COL_SONG_PERFORMER,		    /* performer */
    Gmpc.MpdData.ColumnTypes.COL_SONG_DATE,			      /* date */
    Gmpc.MpdData.ColumnTypes.COL_SONG_LENGTH_FORMAT,	  /* length formatted */
    Gmpc.MpdData.ColumnTypes.COL_SONG_DISC,			      /* disc */
    Gmpc.MpdData.ColumnTypes.COL_SONG_COMMENT,			    /* comment */
    Gmpc.MpdData.ColumnTypes.COL_ICON_ID,				      /* icon id */
    Gmpc.MpdData.ColumnTypes.COL_SONG_POS,
    Gmpc.MpdData.ColumnTypes.COL_SONG_ALBUMARTIST,
    Gmpc.MpdData.ColumnTypes.COL_PATH_EXTENSION,				/* Extension */
    Gmpc.MpdData.ColumnTypes.COL_PATH_DIRECTORY,				/* Directory */
    Gmpc.MpdData.ColumnTypes.COL_SONG_PRIORITY
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
    public enum  ViewType {
        SONG_LIST,
        PLAY_QUEUE,
        PLAYLIST
    }

    private Gtk.TreeViewColumn[] tree_columns = new Gtk.TreeViewColumn[NUM_COLS];
    /**
     * If we are a play-queue we should treat the content.
     * slightly different.
     * e.g. add-replace will be play-crop
     */
    public ViewType view_mode {get; set;default=ViewType.SONG_LIST;}
    /**
     * If we 'view' a playlist, we need to know the name of the play queue.
     * This can be set here.
     */
    private string __playlist_name = null;
    public string playlist_name {
        get{
            return __playlist_name;
        }
        set{
            if(view_mode == ViewType.PLAYLIST) {
                stdout.printf("Set playlist name: %s\n", value);
                __playlist_name = value;
            }else {
                error("Cannot set playlist name on non-playlist view.");
            }
        }
    }

    /**
     * The name of the treeview. 
     * This is used to store the column layout.
     */
    public string uid {get; set; default="default";}


    /**
     * Construction function.
     */
    public DataView(string name, ViewType mode = ViewType.SONG_LIST) 
    {
        this.can_focus = true;
        log(log_domain, LogLevelFlags.LEVEL_INFO, "Constructing dataview: "+name);
        this.view_mode = mode;
        this.uid = name;
        // Connect row activated signal.
        this.row_activated.connect(__row_activated);
        this.key_press_event.connect(__key_press_event_callback);
        this.button_press_event.connect(__button_press_event_callback);
        this.button_release_event.connect(__button_release_event_callback);
        // When it getst he destroy signal.
        this.destroy.connect(column_store_state);

        this.set_rules_hint(true);
        this.set_enable_search(false);

        this.get_selection().set_mode(Gtk.SelectionMode.MULTIPLE);
        this.set_fixed_height_mode(true);

        // Create the view.
        column_populate();

        // Update the sorting when it changed.
        this.notify["model"].connect((source) => {
            // Only the SONG_LIST is sortable.
            if(this.model is Gtk.TreeSortable && view_mode == ViewType.SONG_LIST) {
                int sort_column = config.get_int_with_default(uid,
                    "sort-column", Gmpc.MpdData.ColumnTypes.COL_ICON_ID);
                Gtk.SortType sort_order = (Gtk.SortType)config.get_int_with_default(uid,
                    "sort-order",(int)Gtk.SortType.ASCENDING);

                (this.model as Gtk.TreeSortable).set_sort_column_id(
                    sort_column, sort_order
                    );
            }
         });
    }


    /**
     * Deconstructor.
     */
    ~DataView()
    {
    }
    /** small wrapper.. */
    private void playlist_editor_add_new(Gtk.Widget menu)
    {
        string playlist = menu.get_data("playlist");
        selected_songs_add_to_playlist(playlist);
    }

    public void right_mouse_menu(Gtk.Menu menu)
    {
        int selected_rows = this.get_selection().count_selected_rows();
        if(selected_rows == 1) {
            Gtk.TreeModel model;
            var path = this.get_selection().get_selected_rows(out model).first().data; 
            Gtk.TreeIter iter;
            if(model.get_iter(out iter, path))
            {
                MPD.Data.Type row_type; 
                model.get(iter, Gmpc.MpdData.ColumnTypes.ROW_TYPE, out row_type);
                if(row_type == MPD.Data.Type.SONG) {
                    var item = new Gtk.ImageMenuItem.from_stock(Gtk.Stock.MEDIA_PLAY,null);
                    item.activate.connect((source)=>{
                            selected_songs_play();
                            });
                    menu.append(item);

                    item = new Gtk.ImageMenuItem.from_stock(Gtk.Stock.INFO, null);
                    item.activate.connect((source)=>{
                            selected_songs_info();
                            });
                    menu.append(item);
                }
            }
        }

        if(view_mode == ViewType.PLAY_QUEUE || view_mode == ViewType.PLAYLIST)
        {
            if(selected_rows > 0) {
                var item = new Gtk.ImageMenuItem.from_stock(Gtk.Stock.REMOVE,null);
                item.activate.connect((source)=>{
                    selected_songs_remove();
                });
                menu.append(item);

                item = new Gtk.ImageMenuItem.with_label(_("Crop"));
                item.set_image(new Gtk.Image.from_stock(Gtk.Stock.CUT, Gtk.IconSize.MENU));
                item.activate.connect((source)=>{ selected_songs_crop(); });
                menu.append(item);
            }
        }else{
            var item = new Gtk.ImageMenuItem.from_stock(Gtk.Stock.ADD,null);
            item.activate.connect((source)=>{
                    selected_songs_add();
                    });
            menu.append(item);
            item = new Gtk.ImageMenuItem.with_label(_("Replace"));
            item.activate.connect((source)=>{
                    MPD.PlayQueue.clear(server);
                    selected_songs_add();
                    });
            menu.append(item);
        }
        if(server.check_command_allowed("prioid") == MPD.Server.Command.ALLOWED)
        {
            menu.append( new Gtk.SeparatorMenuItem());
            var item = new Gtk.MenuItem.with_label(_("Queue"));
            item.activate.connect((source)=>{ selected_songs_raise_priority();});
            menu.append(item);

            if(view_mode == ViewType.PLAY_QUEUE) {
                item = new Gtk.MenuItem.with_label(_("Dequeue"));
                item.activate.connect((source)=>{ selected_songs_remove_priority();});
                menu.append(item);
            }
        }
        // Copy paste.
        if(selected_rows > 0)
        {
            Gtk.MenuItem item;
            item = new Gtk.SeparatorMenuItem();
            menu.append(item);
            if(view_mode == ViewType.PLAY_QUEUE || view_mode == ViewType.PLAYLIST) {
                item = new Gtk.ImageMenuItem.from_stock(Gtk.Stock.CUT, null);
                item.activate.connect((source)=>{ selected_songs_paste_queue_cut();});
                menu.append(item);
            }

            item = new Gtk.ImageMenuItem.from_stock(Gtk.Stock.COPY, null);
            item.activate.connect((source)=>{ selected_songs_paste_queue_copy();});
            menu.append(item);

            if( (view_mode == ViewType.PLAY_QUEUE ||
                 view_mode == ViewType.PLAYLIST )
                && paste_queue != null) {
                item = new Gtk.ImageMenuItem.with_label(_("Paste before"));
                (item as Gtk.ImageMenuItem).set_image(new Image.from_stock(Gtk.Stock.PASTE, Gtk.IconSize.MENU));
                item.activate.connect((source)=>{ selected_songs_paste_before();});
                menu.append(item);

                item = new Gtk.ImageMenuItem.with_label(_("Paste after"));
                (item as Gtk.ImageMenuItem).set_image(new Image.from_stock(Gtk.Stock.PASTE, Gtk.IconSize.MENU));
                item.activate.connect((source)=>{ selected_songs_paste_after();});
                menu.append(item);
            }

            // Add to playlist integration.
            
        }
        if(selected_rows == 1)
        {
            Gtk.TreeModel model;
            var list = get_selection().get_selected_rows(out model);
            var path = list.first().data;
            Gtk.TreeIter iter;
            if(model.get_iter(out iter, path))
            { 
                unowned MPD.Song? song;
                model.get(iter, MpdData.ColumnTypes.COL_MPDSONG, out song);
                if(song != null){
                    MpdInteraction.submenu_for_song(menu, song);
                }
            }
        }

        // Tool menu integration.
        bool initial = false;
        var item = new Gtk.MenuItem.with_label(_("Tools"));
        var smenu = new Gtk.Menu();
        item.set_submenu(smenu);
        for(int i = 0; i < Gmpc.num_plugins; i++){
            if(plugins[i].get_enabled() &&
                    plugins[i].browser_song_list_option_menu(this,smenu) > 0){
                stdout.printf("adding entry\n");
                initial = true;
            }
        } 
        if(initial) {
            menu.append(new Gtk.SeparatorMenuItem());
            menu.append(item);
        }

        Gmpc.Browser.PlaylistEditor.right_mouse(menu, playlist_editor_add_new);



        item = new Gtk.MenuItem.with_label(_("Edit Columns"));
        smenu = new Gtk.Menu();
        item.set_submenu(smenu);
        generate_column_selection_menu(smenu);
        
        menu.append(item);
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

        if(this.model is Gtk.TreeSortable) {
            int sort_column;
            int sort_order;
            if((this.model as Gtk.TreeSortable).get_sort_column_id(out sort_column, out sort_order)) {
                config.set_int(uid, "sort-column", sort_column);
                config.set_int(uid, "sort-order", sort_order);
            }
        }
    }
    // Hack to make vala not destroy the menu directly.

    private void generate_column_selection_menu(Gtk.Menu menu)
    {
        foreach(var col in tree_columns)
        {
            int index = col.get_data("index");
            // Do not show the icon id in the selection list.
            if(gmpc_data_view_col_ids[index] == MpdData.ColumnTypes.COL_ICON_ID) continue;
            var item = new Gtk.CheckMenuItem.with_label(FixGtk.gettext(gmpc_data_view_col_names[index]));
            if(col.visible) {
                item.set_active(true);
            }
            // On activation toggle the state.
            item.activate.connect((source) => {
                col.visible = (source as Gtk.CheckMenuItem).get_active();
            });
            menu.append(item);
        }

    }

    private Gtk.Menu column_selection_menu = null;
    private void column_show_selection_menu()
    {
        column_selection_menu = new Gtk.Menu();
        generate_column_selection_menu(column_selection_menu);
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
            if(gmpc_data_view_col_ids[i] == Gmpc.MpdData.ColumnTypes.COL_ICON_ID)
            {
                /**
                 * Picture.
                 */
                var renderer = new Gtk.CellRendererPixbuf(); 
                renderer.xalign = 0.0f;
                // Set up column
                col.pack_start(renderer, true);
                col.set_attributes(renderer, "icon-name", Gmpc.MpdData.ColumnTypes.COL_ICON_ID);
                col.set_resizable(false);
                col.set_fixed_width(20);
                col.clickable = true;
            } else {
                /**
                 * Text column
                 */
                col.set_title(FixGtk.gettext(gmpc_data_view_col_names[i]));
                var renderer = new Gtk.CellRendererText(); 
                renderer.ellipsize = Pango.EllipsizeMode.END;
                // Set up column
                if(view_mode == ViewType.PLAY_QUEUE) {
                    renderer.weight_set = true;
                    renderer.style_set = true;
                    col.set_cell_data_func(renderer, highlight_row_current_song_playing);
                }
                col.pack_start(renderer, true);
                col.set_attributes(renderer, "text", gmpc_data_view_col_ids[i]); 
                col.set_resizable(true);

                int width = config.get_int_with_default(uid+"-colsize",
                        gmpc_data_view_col_names[i], default_column_width);
                // Do not set to size 0, then revert back to 200.
                col.set_fixed_width(width>0?width:default_column_width);
            }
            col.set_sizing(Gtk.TreeViewColumnSizing.FIXED);
            col.set_reorderable(true);

            // Sorting.
            if(view_mode == ViewType.SONG_LIST) {
                col.set_sort_indicator(true);
                col.set_sort_column_id(gmpc_data_view_col_ids[i]); 
            }

            // Fixed width.
            int pos = config.get_int_with_default(uid+"-colpos",
                    gmpc_data_view_col_names[i], gmpc_data_view_col_position[i]);
            this.tree_columns[pos] = col;

        }
        // Add the columns (in right order)
        for(int i = 0; i < NUM_COLS; i++) {
            int index = this.tree_columns[i].get_data("index");
            this.insert_column(this.tree_columns[i], i); 
            this.tree_columns[i].set_visible(config.get_bool_with_default(uid+"-colshow",
                        gmpc_data_view_col_names[index], gmpc_data_view_col_enabled[index]));
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
            MPD.Data.Type row_type; 
            Gtk.TreeIter iter;
            if(!model.get_iter(out iter, path)) return;

            model.get(iter, Gmpc.MpdData.ColumnTypes.ROW_TYPE, out row_type);
            if(view_mode == ViewType.PLAY_QUEUE) {
                /* If we are play-queue, play the selected song. */
                int song_id;
                model.get(iter, Gmpc.MpdData.ColumnTypes.COL_SONG_ID, out song_id);
                MPD.Player.play_id(Gmpc.server, song_id); 
            } else {
                string song_path;
                model.get(iter, Gmpc.MpdData.ColumnTypes.COL_PATH, out song_path);
                if(row_type == MPD.Data.Type.SONG) {                
                    /* If we are a song browser, add the path and play it. */
                    MpdInteraction.play_path(song_path);
                } else if (row_type == MPD.Data.Type.PLAYLIST) {
                    MPD.PlayQueue.queue_load_playlist(server, song_path);
                    MPD.PlayQueue.queue_commit(server);
                }
            }
        }
    }

    /**
     * Check if current row is playing.
     */
    private void highlight_row_current_song_playing(
            Gtk.CellLayout col,
            Gtk.CellRenderer renderer, 
            Gtk.TreeModel model,
            Gtk.TreeIter iter)
    {
        // The current song we make bold.
        if(model is Gmpc.MpdData.ModelPlaylist && 
                (model as Gmpc.MpdData.ModelPlaylist).is_current_song(iter)){
            (renderer as Gtk.CellRendererText).weight = Pango.Weight.BOLD;
        }else{
            (renderer as Gtk.CellRendererText).weight = Pango.Weight.NORMAL;
        }

        // A prioritized song we make italic.
        int prio = 0;
        model.get(iter, Gmpc.MpdData.ColumnTypes.COL_SONG_PRIORITY, out prio);
        if(prio > 0) {
            (renderer as Gtk.CellRendererText).style = Pango.Style.ITALIC;
        }else {
            (renderer as Gtk.CellRendererText).style = Pango.Style.NORMAL;
        }
    }


    /**
     * Check if modifier is pressed.
     */
    private bool __key_mod_pressed(Gdk.EventKey event)
    {
        return (event.state&Gdk.ModifierType.CONTROL_MASK) == Gdk.ModifierType.CONTROL_MASK;
    }

    /**
     * Handle keyboard input.
     */
    private bool __key_press_event_callback_song_list(Gdk.EventKey event)
    {
        if(event.keyval == Gdk.Key_i && !__key_mod_pressed(event))
        {
            return selected_songs_add();
        }
        else if (event.keyval == Gdk.Key_r && !__key_mod_pressed(event))
        {
            // If there are songs selected, clear the play queue,
            // and add selection.
            if(this.get_selection().count_selected_rows() > 0) {
                MPD.PlayQueue.clear(server);
                return selected_songs_add();
            }
        }
        return false;
    }
    /**
     * Keybinding for playlists.
     */
    private bool __key_press_event_callback_playlist(Gdk.EventKey event)
    {
        if (event.keyval == Gdk.Key_x && !__key_mod_pressed(event))
        {
            // Cut (if available) into clipboard
            selected_songs_paste_queue_cut();
        }
        else if (event.keyval == Gdk.Key_X && !__key_mod_pressed(event))
        {
            // Crop selected songs.
            selected_songs_crop();
        }
        else if (event.keyval == Gdk.Key_P && !__key_mod_pressed(event))
        {
            // Paste before  
            return selected_songs_paste_before();
        }
        else if (event.keyval == Gdk.Key_p && !__key_mod_pressed(event))
        {
            // Paste after
            return selected_songs_paste_after();
        }
        else if (event.keyval == Gdk.Key_d && !__key_mod_pressed(event))
        {
            if(!selected_songs_remove())
            {
                // Clear the playlist
                MPD.Database.playlist_clear(server, playlist_name);
                return true;
            }
        }
        else if (event.keyval == Gdk.Key_D && !__key_mod_pressed(event))
        {
            MPD.Database.playlist_clear(server, playlist_name);
            return true;
        }
        return false;
    }
    private bool __key_press_event_callback_play_queue(Gdk.EventKey event)
    {
        if (event.keyval == Gdk.Key_Q && !__key_mod_pressed(event)) 
        {
            // remove priority.
            return selected_songs_remove_priority();
        }
        else if (__key_mod_pressed(event) && 
                event.keyval == Gdk.Key_X)
        {
           stdout.printf("Clear everything but playing song\n"); 
            // Select the playing song:
            unowned MPD.Song? song = server.playlist_get_current_song();
            if(song != null) {
                var path = new Gtk.TreePath.from_indices(song.pos);
                this.get_selection().unselect_all();
                this.get_selection().select_path(path);
                selected_songs_crop();
            } 
            return false;
        }
        else if (event.keyval == Gdk.Key_x && !__key_mod_pressed(event))
        {
            // Cut (if available) into clipboard
            selected_songs_paste_queue_cut();
        }
        else if (event.keyval == Gdk.Key_X && !__key_mod_pressed(event))
        {
            // Crop selected songs.
            selected_songs_crop();
        }
        else if (event.keyval == Gdk.Key_P && !__key_mod_pressed(event))
        {
            // Paste before  
            return selected_songs_paste_before();
        }
        else if (event.keyval == Gdk.Key_p && !__key_mod_pressed(event))
        {
            // Paste after
            return selected_songs_paste_after();
        }
        else if (event.keyval == Gdk.Key_d && !__key_mod_pressed(event))
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
        else if (event.keyval == Gdk.Key_D && !__key_mod_pressed(event))
        {
            var model = get_model();
            this.model = null; 
            // Clear
            MPD.PlayQueue.clear(server);
            // Re-add model
            this.model = model;
            return true;
        }
        return false;
    }
    private bool __key_press_event_callback(Gdk.EventKey event)
    {
        if(event.keyval == Gdk.Key_h && !__key_mod_pressed(event))
        {
            move_cursor_left();
        }
        else if (event.keyval == Gdk.Key_l && !__key_mod_pressed(event))
        {
            move_cursor_right();
        }
        else if(event.keyval == Gdk.Key_j && !__key_mod_pressed(event))
        {
            // Move cursor down.
            move_cursor_down();
        }
        else if (event.keyval == Gdk.Key_k && !__key_mod_pressed(event))
        {
            // Move cursor up
            move_cursor_up();
        }
        else if (event.keyval == Gdk.Key_g && !__key_mod_pressed(event))
        {
            move_cursor_top();
        }
        else if (event.keyval == Gdk.Key_G && !__key_mod_pressed(event))
        {
            move_cursor_bottom();
        }
        else if(event.keyval == Gdk.Key_y && !__key_mod_pressed(event))
        {
            // Copy data to clipboard
            return selected_songs_paste_queue_copy();
        }
        else if (event.keyval == Gdk.Key_o && !__key_mod_pressed(event))
        {
            return selected_songs_info();
        }
        else if (event.keyval == Gdk.Key_m && !__key_mod_pressed(event))
        {
            // Configure columns
            column_show_selection_menu();
            return true;
        }
        else if (event.keyval == Gdk.Key_q && !__key_mod_pressed(event))
        {
            // Raise priority.
            return selected_songs_raise_priority();
        }
        else if (event.keyval == Gdk.Key_Menu && !__key_mod_pressed(event))
        {
            __button_press_menu = new Gtk.Menu();

            right_mouse_menu(__button_press_menu);




            __button_press_menu.show_all();
            __button_press_menu.popup(null, null,null,0, Gtk.get_current_event_time()); 
            return true;
        }

        // Commands specific to play_queue
        if(view_mode == ViewType.PLAY_QUEUE)
        {
            if(__key_press_event_callback_play_queue(event)) return true;
        }
        else if (view_mode == ViewType.PLAYLIST)
        {
            if(__key_press_event_callback_playlist(event)) return true;
        }
        else if (view_mode == ViewType.SONG_LIST)
        {
            if(__key_press_event_callback_song_list(event)) return true;
        }
        return false;
    }

    /**
     * Right mouse popup.
     */
    // Hack to stop vala from destroying my menu.
    private Gtk.Menu __button_press_menu = null;
    private bool __button_press_event_callback(Gdk.EventButton event)
    {
        if(event.button == 3)
        {
            __button_press_menu = new Gtk.Menu();
            right_mouse_menu(__button_press_menu);

            __button_press_menu.show_all();
            __button_press_menu.popup(null, null,null, event.button, event.time);
            return true;
        }
        return false;
    }
    private bool __button_release_event_callback(Gdk.EventButton event)
    {
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
                MPD.Data.Type row_type;
                int song_id;
                model.get(iter,
                        Gmpc.MpdData.ColumnTypes.COL_SONG_ID, out song_id,
                        Gmpc.MpdData.ColumnTypes.ROW_TYPE, out row_type);
                // Only act on songs.
                if(row_type != MPD.Data.Type.SONG) continue;

                if(song_id >= 0) {
                    MPD.PlayQueue.set_priority(server, song_id, priority--);
                } else {
                    string song_path;
                    model.get(iter, Gmpc.MpdData.ColumnTypes.COL_PATH, out song_path);
                    song_id = MPD.PlayQueue.add_song_get_id(server,song_path);
                    MPD.PlayQueue.set_priority(server, song_id, priority--);
                }
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
                model.get(iter,Gmpc.MpdData.ColumnTypes.COL_SONG_ID, out song_id);
                if ( song_id >= 0 ) {
                    MPD.PlayQueue.set_priority(server, song_id, 0);
                }
            }            
        }
        return true;
    }
    // Add the selected song
    private bool selected_songs_add()
    {
        int added_rows = 0;
        var selection = this.get_selection();

        Gtk.TreeModel model;
        foreach(var path in selection.get_selected_rows(out model))
        {
            Gtk.TreeIter iter;
            if(model.get_iter(out iter, path))
            {
                string song_path;
                MPD.Data.Type row_type; 
                model.get(iter,
                        Gmpc.MpdData.ColumnTypes.ROW_TYPE, out row_type,
                        Gmpc.MpdData.ColumnTypes.COL_PATH, out song_path);
                if ( row_type == MPD.Data.Type.SONG || row_type == MPD.Data.Type.DIRECTORY) { 
                    MPD.PlayQueue.queue_add_song(server, song_path);
                    added_rows++;
                }else if (row_type == MPD.Data.Type.PLAYLIST) {
                    MPD.PlayQueue.queue_load_playlist(server, song_path);
                    added_rows++;
                }
            }            
        }
        MPD.PlayQueue.queue_commit(server);
        if(added_rows > 0) {
            return true;
        }
        return false;
    }
    // Play the selected song
    private bool selected_songs_play()
    {
        var selection = this.get_selection();
        Gtk.TreeModel model;
        foreach(var path in selection.get_selected_rows(out model))
        {
            Gtk.TreeIter iter;
            if(model.get_iter(out iter, path))
            {
                if(view_mode == ViewType.PLAY_QUEUE) {
                int song_id;
                model.get(iter, Gmpc.MpdData.ColumnTypes.COL_SONG_ID, out song_id);
                if(song_id >= 0){
                    MPD.Player.play_id(server, song_id);
                    return true; 
                }
                }else{
                    string song_path;
                    model.get(iter, Gmpc.MpdData.ColumnTypes.COL_PATH, out song_path);
                    MpdInteraction.play_path(song_path);
                    return true;
                }
            }
        }
        return selection.count_selected_rows() > 0;
    }
    // Crop the selected songs.
    private void selected_songs_crop()
    {
        int deleted_rows = 0;
        var selection = this.get_selection();
        
        model = this.get_model();
        List<int> items_to_delete = new List<int>();

        TreeIter iter;
        if(model.get_iter_first(out iter))
        {
            do{
                if(!this.get_selection().iter_is_selected(iter))
                {
                    if(view_mode == ViewType.PLAY_QUEUE) {
                        int song_id;
                        model.get(iter,Gmpc.MpdData.ColumnTypes.COL_SONG_ID, out song_id);
                        items_to_delete.prepend(song_id);
                    }else if ( view_mode == ViewType.PLAYLIST) {
                        var path = model.get_path(iter);
                        var indc = path.get_indices(); 
                        int song_pos = indc[0];
                        items_to_delete.prepend(song_pos);
                    }
                }
            }while(model.iter_next(ref iter));
        }
        foreach(int id in items_to_delete)
        {
            if(view_mode == ViewType.PLAY_QUEUE)
            {
                MPD.PlayQueue.queue_delete_id(server, id);
            } else if (view_mode == ViewType.PLAYLIST)
            {
                MPD.Database.playlist_list_delete(server, playlist_name, id);
            }
        }
        MPD.PlayQueue.queue_commit(server);
    }
    // Remove the selected songs from the play queue.
    private bool selected_songs_remove()
    {
        int deleted_rows = 0;
        var selection = this.get_selection();

        Gtk.TreeModel model;
        var sel_rows = selection.get_selected_rows(out model);
        sel_rows.reverse();
        foreach(var path in sel_rows)
        {
            Gtk.TreeIter iter;
            if(model.get_iter(out iter, path))
            {
                if(view_mode == ViewType.PLAY_QUEUE) {
                    int song_id;
                    model.get(iter,Gmpc.MpdData.ColumnTypes.COL_SONG_ID, out song_id);
                    MPD.PlayQueue.queue_delete_id(server, song_id);
                }else if ( view_mode == ViewType.PLAYLIST) {
                    var indc = path.get_indices(); 
                    int song_pos = indc[0];
                    MPD.Database.playlist_list_delete(server, playlist_name, song_pos);
                }else {
                    error("Invalid state, cannot call this here.");
                }
                deleted_rows++;
            }            
        }
        MPD.PlayQueue.queue_commit(server);
        if(deleted_rows > 0) {
            selection.unselect_all();
            return true;
        }
        return false;
    }
    // Show the Information of the first selected song.
    private bool selected_songs_info()
    {
        var selection = this.get_selection();
        Gtk.TreeModel model;
        foreach(var path in selection.get_selected_rows(out model))
        {
            Gtk.TreeIter iter;
            if(model.get_iter(out iter, path))
            {
                unowned MPD.Song? song = null; 
                model.get(iter, Gmpc.MpdData.ColumnTypes.COL_MPDSONG, out song);
                if(song != null) {
                    Browser.Metadata.show();
                    Browser.Metadata.show_song(song);
                    return true;
                }
            }
        }
        return selection.count_selected_rows() > 0;
    }

    // Cut the selected songs in the play queue 
    private bool selected_songs_paste_queue_cut()
    {
        if(selected_songs_paste_queue_copy()){
            selected_songs_remove();
        }
        return false;
    }
    // Copy the selected songs in the play queue 
    private bool selected_songs_paste_queue_copy()
    {
        var selection = this.get_selection();
        // If nothing to copy , do nothing.
        if(selection.count_selected_rows() == 0) return false;
        // Clear paste_queue
        paste_queue = null;
        Gtk.TreeModel model;
        foreach(var path in selection.get_selected_rows(out model))
        {
            Gtk.TreeIter iter;
            if(model.get_iter(out iter, path))
            {
                string insert_path;
                model.get(iter,Gmpc.MpdData.ColumnTypes.COL_PATH, out insert_path);
                paste_queue.prepend(insert_path);
            }            
        }
        paste_queue.reverse();
        return true;

    }
    
    private bool selected_songs_paste_before()
    {
        var selection = this.get_selection();
        // If nothing selected. 
        if(selection.count_selected_rows() == 0|| paste_queue == null)
        {
            return false;
        }
        Gtk.TreeModel model;
        Gtk.TreeIter iter;
        Gtk.TreePath path = selection.get_selected_rows(out model).last().data;
        if(model.get_iter(out iter, path))
        {
            int songpos;
            if( view_mode == ViewType.PLAY_QUEUE) {
                model.get(iter, Gmpc.MpdData.ColumnTypes.COL_SONG_POS, out songpos);
                if(songpos> 0) {
                    songpos--;
                    paste_queue.reverse();
                    foreach(var fpath in paste_queue)
                    {
                        int nsongid = MPD.PlayQueue.add_song_get_id(server,fpath);
                        MPD.PlayQueue.song_move_id(server, nsongid, songpos);
                    }
                    paste_queue.reverse();
                }
            }else if (view_mode == ViewType.PLAYLIST) {
                    int length = this.model.iter_n_children(null);
                    var indc = path.get_indices(); 
                    songpos = indc[0];
                    paste_queue.reverse();
                    foreach(var fpath in paste_queue)
                    {
                        MPD.Database.playlist_list_add(server, playlist_name, fpath);
                        MPD.Database.playlist_move(server, playlist_name, length,songpos);
                        length++;
                    }
                    paste_queue.reverse();

            }else{
                error("We should be able to be here.");
            }
        }
        return true; 
    }
    private bool selected_songs_paste_after()
    {
        var selection = this.get_selection();
        // If nothing selected. 
        if(selection.count_selected_rows() == 0|| paste_queue == null)
        {
            return false;
        }
        Gtk.TreeModel model;
        Gtk.TreeIter iter;
        Gtk.TreePath path = selection.get_selected_rows(out model).last().data;
        if(model.get_iter(out iter, path))
        {
            int songpos;
            if(view_mode == ViewType.PLAY_QUEUE) {
                model.get(iter, Gmpc.MpdData.ColumnTypes.COL_SONG_POS, out songpos);
                if(songpos> 0) {
                    paste_queue.reverse();
                    foreach(var fpath in paste_queue)
                    {
                        int nsongid = MPD.PlayQueue.add_song_get_id(server,fpath);
                        MPD.PlayQueue.song_move_id(server, nsongid, songpos);
                    }
                    paste_queue.reverse();
                }
            }
            else if (view_mode == ViewType.PLAYLIST) {
                    int length = this.model.iter_n_children(null);
                    var indc = path.get_indices(); 
                    songpos = indc[0]+1;
                    paste_queue.reverse();
                    foreach(var fpath in paste_queue)
                    {
                        MPD.Database.playlist_list_add(server, playlist_name, fpath);
                        MPD.Database.playlist_move(server, playlist_name, length,songpos);
                        length++;
                    }
                    paste_queue.reverse();
            }else{
                error("We should be able to be here.");
            }

        }
        return true; 
    }

    private bool selected_songs_add_to_playlist(string playlist)
    {
        int songs_added = 0;
        var selection = this.get_selection();
        // If nothing selected. 
        if(selection.count_selected_rows() == 0)
        {
            return false;
        }
        Gtk.TreeModel model;
        foreach (var path in selection.get_selected_rows(out model))
        {
            Gtk.TreeIter iter;
            if(model.get_iter(out iter, path))
            {
                string? song_path = null;
                model.get(iter, Gmpc.MpdData.ColumnTypes.COL_PATH, out song_path);
                if(song_path != null) {
                    MPD.Database.playlist_list_add(server, playlist, song_path); 
                    songs_added++;
                }
            }
        }
        return (songs_added > 0);
    }

    /**
     * Move
     */
    private void move_cursor_down()
    {
        Gtk.TreePath? path;
        Gtk.TreeViewColumn? col;
        this.get_cursor(out path, out col);
        if(path != null)
        {
            Gtk.TreeIter iter;
            path.next();
            if(this.model.get_iter(out iter, path))
            {
                this.set_cursor(path, col, false);
            }
        }
    }
    private void move_cursor_top()
    {
        Gtk.TreeViewColumn? col;
        var rows = this.model.iter_n_children(null);
        if(rows > 0){
            Gtk.TreePath? path;
            this.get_cursor(null, out col);
            path = new Gtk.TreePath.from_indices(0);
            this.set_cursor(path, col, false);
        }
    }
    private void move_cursor_bottom()
    {
        Gtk.TreeViewColumn? col;
        var rows = this.model.iter_n_children(null);
        if(rows > 0){
            Gtk.TreePath? path;
            this.get_cursor(null, out col);
            path = new Gtk.TreePath.from_indices(rows-1);
            this.set_cursor(path, col, false);
        }
    }
    private void move_cursor_up()
    {
        Gtk.TreePath? path;
        Gtk.TreeViewColumn? col;
        this.get_cursor(out path, out col);
        if(path != null)
        {
            if(path.prev())
            {
                this.set_cursor(path, col, false);
            }
        }
    }
    private void move_cursor_left()
    {
        if(this is Gtk.Scrollable)
        {
            var hadj = this.get_hadjustment();
            hadj.set_value(hadj.value-hadj.step_increment);
        }
    }
    private void move_cursor_right()
    {
        if(this is Gtk.Scrollable)
        {
            var hadj = this.get_hadjustment();
            hadj.set_value(hadj.value+hadj.step_increment);
        }
    }
}
