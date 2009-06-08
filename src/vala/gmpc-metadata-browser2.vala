/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
using Config;
using Gtk;
using Gmpc;
using Gmpc.MpdData.Treeview.Tooltip;

private const bool use_transition_mb = Gmpc.use_transition;
private const string some_unique_name_mb = Config.VERSION;


public class Gmpc.Widget.SimilarSongs : Gtk.Expander {
    private MPD.Song song = null;
    private bool filled = false;
    private Gtk.Widget pchild = null;
    private uint idle_add = 0;
    ~SimilarSongs ()
    {
        if(this.idle_add > 0){
            GLib.Source.remove(this.idle_add);
            this.idle_add = 0;
        }
    }

    SimilarSongs (MPD.Song song) 
    {
        this.song = song;
        var label  = new Gtk.Label(_("Similar songs"));
        label.set_markup("<b>%s</b>".printf(_("Similar songs")));
        this.set_label_widget(label);
        label.show();
    }
    private void add_clicked(Gtk.ImageMenuItem item)
    {
        Gtk.TreeView tree = (Gtk.TreeView)this.pchild;

        var sel = tree.get_selection();
        Gtk.TreeModel model = null;
        Gtk.TreeIter iter;
        List<Gtk.TreePath> list = sel.get_selected_rows(out model);
        foreach(Gtk.TreePath path in list)
        {
            if(model.get_iter(out iter, path))
            {
                weak MPD.Song song = null;
                model.get(iter, 0, out song, -1);
                if(song != null)
                {
                   MPD.PlayQueue.queue_add_song(server, song.file); 
                }
            }
        }
        MPD.PlayQueue.queue_commit(server);

    }
    private void play_clicked(Gtk.ImageMenuItem item)
    {
        Gtk.TreeView tree = (Gtk.TreeView)this.pchild;

        var sel = tree.get_selection();
        Gtk.TreeModel model = null;
        Gtk.TreeIter iter;
        List<Gtk.TreePath> list = sel.get_selected_rows(out model);
        if(list != null)
        {
            Gtk.TreePath path = list.data;
            if(model.get_iter(out iter, path))
            {
                weak MPD.Song song = null;
                model.get(iter, 0, out song, -1);
                if(song != null)
                {
                    Gmpc.Misc.play_path(song.file);
                }
            }
        }
    }
    private void replace_clicked(Gtk.ImageMenuItem item)
    {
        bool found = false;
        Gtk.TreeView tree = (Gtk.TreeView)this.pchild;
        var sel = tree.get_selection();
        Gtk.TreeModel model = null;
        Gtk.TreeIter iter;
        List<Gtk.TreePath> list = sel.get_selected_rows(out model);
        foreach(Gtk.TreePath path in list)
        {
            if(model.get_iter(out iter, path))
            {
                weak MPD.Song song = null;
                model.get(iter, 0, out song, -1);
                if(song != null)
                {
                   MPD.PlayQueue.queue_add_song(server, song.file); 
                   found = true;
                }
            }
        }
        if(found)
        {
            MPD.PlayQueue.clear(server);
            MPD.PlayQueue.queue_commit(server);
            MPD.Player.play(server);
        }


        this.play_clicked(item);
    }
    private void tree_row_activated(Gmpc.MpdData.TreeView tree, Gtk.TreePath path , Gtk.TreeViewColumn column)
    {
        var model = tree.get_model();
        Gtk.TreeIter iter;
        if(model.get_iter(out iter, path))
        {
            weak MPD.Song song = null;
            model.get(iter, 0, out song, -1);
            if(song != null)
            {
                Gmpc.Misc.play_path(song.file);
            }
        }
    }
    private bool tree_right_menu(Gmpc.MpdData.TreeView tree, Gdk.EventButton event)
    {
        if(event.button == 3)
        {
            var menu = new Gtk.Menu();
            var item = new Gtk.ImageMenuItem.from_stock("gtk-media-play",null);
            item.activate += play_clicked;
            menu.append(item);

            item = new Gtk.ImageMenuItem.from_stock("gtk-add",null);
            item.activate += add_clicked;
            menu.append(item);

            item = new Gtk.ImageMenuItem.with_mnemonic("_Replace");
            item.set_image(new Gtk.Image.from_stock("gtk-redo", Gtk.IconSize.MENU));
            item.activate += replace_clicked;
            menu.append(item);

            menu.popup(null, null, null, event.button, event.time);
            menu.show_all();
            return true;
        }
        return false;
    }

    private Gmpc.MetaData.Item copy = null;
    MPD.Data.Item item = null;
    private weak List <weak string> current = null;
    private bool update_sim_song()
    {
        if(current == null){
           current = copy.get_text_list(); 
           pchild = new Gtk.ProgressBar();
           this.add(pchild);
           this.show_all();
        }
        ((Gtk.ProgressBar)pchild).pulse();
        if(current != null)
        {
            string entry = current.data;
            if(entry != null){
                var split = entry.split("::",2);
                MPD.Database.search_start(server, false);
                var art_split = split[0].split(" ");
                foreach(string artist in art_split)
                {
                    MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
                }

                MPD.Database.search_add_constraint(server, MPD.Tag.Type.TITLE, split[1]);
                var data = MPD.Database.search_commit(server);
                if(data != null)
                {
                    item.concatenate((owned)data); 
                }
            }
            current = current.next;
            if(current != null) return true;
        }
        this.pchild.destroy();
        if(item != null)
        {
            var model = new Gmpc.MpdData.Model();
            model.set_mpd_data((owned)item);
            Gmpc.MpdData.TreeView tree = new Gmpc.MpdData.TreeView("similar-song", true, model);
            tree.enable_click_fix();
            tree.button_release_event += tree_right_menu;
            tree.row_activated += tree_row_activated;
            this.add(tree);

            this.pchild = tree;
        }else {
            var label = new Gtk.Label(_("Unavailable"));
            label.set_alignment(0.0f, 0.0f);
            this.add(label);
            this.pchild = label;
        }

        copy = null;
        this.idle_add = 0;

        this.show_all();
        return false;
    }
    private void metadata_changed(MetaWatcher gmw, MPD.Song song, Gmpc.MetaData.Type type, Gmpc.MetaData.Result result, Gmpc.MetaData.Item? met)
    {
        if(this.song.artist.collate(song.artist)!=0) return;
        if(type != Gmpc.MetaData.Type.SONG_SIMILAR) return;
        
        if(this.pchild != null) this.pchild.destroy(); 

        if(result == Gmpc.MetaData.Result.FETCHING) {
            var label = new Gtk.Label(_("Fetching .. "));
            label.set_alignment(0.0f, 0.0f);
            this.add(label);
            this.pchild = label;
        }else if (result == Gmpc.MetaData.Result.UNAVAILABLE)
        {
            var label = new Gtk.Label(_("Unavailable"));
            label.set_alignment(0.0f, 0.0f);
            this.add(label);
            this.pchild = label;
        }else{
            if(met.is_text_list())
            {
                this.copy = met.dup_steal();
                this.idle_add =  GLib.Idle.add(this.update_sim_song);
                return;
            }else {
                var label = new Gtk.Label(_("Unavailable"));
                label.set_alignment(0.0f, 0.0f);
                this.add(label);
                this.pchild = label;
            }
        }
        this.show_all();

    }
    private void update()
    {
        MetaData.Item item = null;
        metawatcher.data_changed += metadata_changed;
        Gmpc.MetaData.Result gm_result = metawatcher.query(song, Gmpc.MetaData.Type.SONG_SIMILAR,out item);
        this.metadata_changed(metawatcher, this.song, Gmpc.MetaData.Type.SONG_SIMILAR, gm_result, item); 
    }

    override void activate()
    {
        if(!this.expanded) {
            this.set_expanded(true);
            if(!filled) {
                this.update();
                filled = true;
            }
        }
        else{
            this.set_expanded(false);
        }
    }

}

public class Gmpc.Widget.SimilarArtist : Gtk.Table {
    private MPD.Song song = null;
    private Gmpc.MetadataBrowser browser = null;

    private void metadata_changed(MetaWatcher gmw, MPD.Song song, Gmpc.MetaData.Type type, Gmpc.MetaData.Result result, Gmpc.MetaData.Item met)
    {
        if(this.song.artist.collate(song.artist)!=0) return;
        if(type != Gmpc.MetaData.Type.ARTIST_SIMILAR) return;

        /* clear widgets */
        var child_list = this.get_children();
        foreach(Gtk.Widget child in child_list)
        {
            child.destroy();
        }

        if(result == Gmpc.MetaData.Result.UNAVAILABLE || met.is_empty() || !met.is_text_list())
        {
            var label = new Gtk.Label(_("Unavailable"));
            this.attach(label, 0,1,0,1,Gtk.AttachOptions.SHRINK, Gtk.AttachOptions.SHRINK, 0,0);
        }
        else if(result == Gmpc.MetaData.Result.FETCHING){
            var label = new Gtk.Label(_("Fetching"));
            this.attach(label, 0,1,0,1,Gtk.AttachOptions.SHRINK, Gtk.AttachOptions.SHRINK, 0,0);
        }else
        {
            List<Gtk.Widget> in_db_list = null;
            GLib.List<weak string> list = met.get_text_list().copy();
            if(list != null)
            {
                MPD.Database.search_field_start(server, MPD.Tag.Type.ARTIST);
                var data = MPD.Database.search_commit(server);
                weak MPD.Data.Item iter = data.first();
                while(iter != null && list != null)
                {
                    if(iter.tag != null && iter.tag.length > 0)
                    {
                        weak List<weak string> liter= list.first();
                        var artist = GLib.Regex.escape_string(iter.tag);
                        try{
                            var reg = new GLib.Regex(artist, GLib.RegexCompileFlags.CASELESS);
                            do{
                                if(reg.match(liter.data))
                                {
                                    in_db_list.prepend(new_artist_button(iter.tag, true));
                                    list.remove(liter.data);
                                    liter = null;
                                }
                            }while(liter != null && (liter = liter.next) != null);
                        }catch (Error E)
                        {
                            GLib.assert_not_reached ();
                        }
                    }
                    iter = iter.next(false);
                }
            }
            foreach(string artist in list)
            {
                in_db_list.prepend(new_artist_button(artist, false));
            }
            in_db_list.reverse();
            int i=0;
            this.hide();
            foreach(Gtk.Widget item in in_db_list)
            {
                if(i<50){
                    this.attach(item, i%4,i%4+1,i/4,i/4+1,Gtk.AttachOptions.EXPAND|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK, 0,0);
                }else{
                    item.ref_sink();
                    item.destroy();
                }
                i++;
            }
        }

        this.show_all();
    }
    private
    void
    artist_button_clicked(Gtk.Button button)
    {
        weak string artist = (string)button.get_data("artist");
        this.browser.set_artist(artist);
    }
    public
    Gtk.Widget
    new_artist_button(string artist, bool in_db)
    {
        var hbox = new Gtk.HBox(false, 6);
        hbox.border_width = 6;

        var event = new Gtk.EventBox();
        event.app_paintable = true;
        event.expose_event += Gmpc.Misc.misc_header_expose_event;

        var image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ARTIST_ART, 48);
        var song = new MPD.Song();
        song.artist = artist;
        image.update_from_song_delayed(song);
        hbox.pack_start(image,false,false,0);

        var label = new Gtk.Label(artist);
        label.set_selectable(true);
        label.set_alignment(0.0f, 0.5f);
        label.ellipsize = Pango.EllipsizeMode.END; 
        hbox.pack_start(label,true,true,0);

        if(in_db)
        {
            var find = new Gtk.Button.from_stock("gtk-find");
            find.set_relief(Gtk.ReliefStyle.NONE);
            hbox.pack_start(find,false,false,0);

            find.set_data_full("artist",(void *)"%s".printf(artist), (GLib.DestroyNotify) g_free);
            find.clicked+= artist_button_clicked;
        }

        event.add(hbox);
        event.set_size_request(180,60);
        return event;
    }

    SimilarArtist(Gmpc.MetadataBrowser browser,MPD.Server server, MPD.Song song)
    {
        MetaData.Item item = null;
        this.browser = browser;
        this.song = song;

        this.set_homogeneous(true);

        this.set_row_spacings(6);
        this.set_col_spacings(6);

        metawatcher.data_changed += metadata_changed;

        Gmpc.MetaData.Result gm_result = metawatcher.query(song, Gmpc.MetaData.Type.ARTIST_SIMILAR,out item);
        if(gm_result == Gmpc.MetaData.Result.AVAILABLE)
        {
            this.metadata_changed(metawatcher, this.song, Gmpc.MetaData.Type.ARTIST_SIMILAR, gm_result, item); 
        }
    }
}

public class Gmpc.Widget.More : Gtk.Frame {
    private Gtk.Alignment ali = null;
    private int expand_state = 0;
    private Gtk.Button expand_button = null;
    private int max_height = 100;
    private Gtk.EventBox eventbox = null;
    private Gtk.Widget pchild = null;

    private void expand(Gtk.Button but)
    {
        if(this.expand_state == 0) {
            but.set_label(_("(less)"));
            this.ali.set_size_request(-1, -1);
            this.expand_state = 1;
        }else{
            but.set_label(_("(more)"));
            this.ali.set_size_request(-1, this.max_height);
            this.expand_state = 0;
        }

    }
    private void size_changed(Gtk.Widget child, Gdk.Rectangle alloc)
    {
        if(alloc.height < (this.max_height-12)){
            this.ali.set_size_request(-1,-1);
            this.expand_button.hide();
        }else{
            if(this.expand_state == 0)
                this.ali.set_size_request(-1, this.max_height);
            this.expand_button.show();
        }
    }

    private void bg_style_changed(Gtk.Widget frame,Gtk.Style? style)
    {
        this.pchild.modify_bg(Gtk.StateType.NORMAL,this.parent.style.mid[Gtk.StateType.NORMAL]);
        this.pchild.modify_base(Gtk.StateType.NORMAL,this.parent.style.mid[Gtk.StateType.NORMAL]);

        this.eventbox.modify_bg(Gtk.StateType.NORMAL,this.parent.style.dark[Gtk.StateType.NORMAL]);
        this.eventbox.modify_base(Gtk.StateType.NORMAL,this.parent.style.dark[Gtk.StateType.NORMAL]);
    }
    More(string markup,Gtk.Widget child)
    {
        this.set_shadow_type(Gtk.ShadowType.NONE);
        
        this.pchild = child;
        
        this.ali = new Gtk.Alignment(0f,0f,1f,0f); ali.set_padding(1,1,1,1);
        this.eventbox = new Gtk.EventBox();
        this.eventbox.set_visible_window(true);
        this.add(eventbox);
        this.eventbox.add(ali);
        this.ali.set_size_request(-1, this.max_height);
        this.ali.add(child);

        this.style_set += bg_style_changed;

        var hbox = new Gtk.HBox(false, 6);
        var label= new Gtk.Label("");
        label.set_selectable(true);
        label.set_markup(markup);
        hbox.pack_start(label, false, false,0);
        this.expand_button = new Gtk.Button.with_label(_("(more)"));
        this.expand_button.set_relief(Gtk.ReliefStyle.NONE);
        this.expand_button.clicked+=expand;
        hbox.pack_start(this.expand_button, false, false,0);
        
        this.set_label_widget(hbox);
        child.size_allocate += size_changed;
    }
}

/**
 * Now playing uses the MetaDataBrowser plugin to "plot" the view */
public class  Gmpc.NowPlaying : Gmpc.Plugin.Base, Gmpc.Plugin.BrowserIface {
    private Gtk.TreeRowReference np_ref = null;

    construct {
        /* Set the plugin as an internal one and of type pl_browser */
        this.plugin_type = 2|8; 
        /* Track changed status */
        gmpcconn.status_changed += status_changed;
        /* Create a metadata browser plugin, we abuse for the view */
        this.browser = new Gmpc.MetadataBrowser();
    }
    /* Version */
    public const int[] version =  {0,0,0};
    public override  weak int[3] get_version() {
        return version;
    }
    /* Name */
    public override weak string get_name() {
        return N_("Now Playing");
    }
    /* Save our position in the side-bar */
    public override void save_yourself() {
        if(this.paned != null) {
            this.paned.destroy();
            this.paned = null;
        }
        if(this.np_ref != null) {
            var path = np_ref.get_path();
            if(path != null) {
                weak int[] indices  = path.get_indices();
                config.set_int(this.get_name(), "position", indices[0]);
            }
        }
    }

    private 
    void
    status_changed(Gmpc.Connection conn, MPD.Server server, MPD.Status.Changed what)
    {
        if(this.paned == null) return;
        if((        (what&MPD.Status.Changed.SONGID) == MPD.Status.Changed.SONGID ||
                    (what&MPD.Status.Changed.PLAYLIST) == MPD.Status.Changed.PLAYLIST ||
                    (what&MPD.Status.Changed.STATE) == MPD.Status.Changed.STATE
                    ) && this.selected)
        {
            this.update();
        }
    }
    /* Browser */
    private Gmpc.MetadataBrowser browser = null;
    private Gtk.ScrolledWindow paned = null;
    private Gtk.EventBox container = null;
    /** 
     * Browser Interface bindings
     */
    public void browser_add (Gtk.Widget category_tree)
    {
        Gtk.TreeView tree = (Gtk.TreeView)category_tree;
        Gtk.ListStore store = (Gtk.ListStore)tree.get_model();
        Gtk.TreeModel model = tree.get_model();
        Gtk.TreeIter iter;
        Gmpc.Browser.insert(out iter, config.get_int_with_default(this.get_name(), "position", 0));
        store.set(iter, 0, this.id, 1, _(this.get_name()), 3, "media-audiofile"); 
        /* Create a row reference */
        this.np_ref = new Gtk.TreeRowReference(model,  model.get_path(iter));
    }
    public void browser_selected (Gtk.Container container)
    {
        this.selected = true;
        this.browser_init();
        container.add(this.paned);
        this.update();
    }

    private bool selected = false;
    public void browser_unselected(Gtk.Container container)
    {
        this.selected = false;
        container.remove(this.paned);
    }

    private void browser_bg_style_changed(Gtk.ScrolledWindow bg,Gtk.Style? style)
    {
        this.container.modify_bg(Gtk.StateType.NORMAL,this.paned.style.base[Gtk.StateType.NORMAL]);
    }
    /* Handle buttons presses, f.e. for scrolling */
    private bool browser_key_release_event(Gdk.EventKey event)
    {
        var adj = this.paned.get_vadjustment();
        double incr = 20;
        adj.get("step-increment", out incr);
        if(event.keyval == 0xff55 )// GDK_Page_Up
        {
            adj.set_value(adj.get_value()-incr);
            return true;
        }
        else if (event.keyval == 0xff56) // GDK_Page_Down
        {
            adj.set_value(adj.get_value()+incr);
            return true;
        }
        return false;
    }
    private void browser_init() {
        if(this.paned == null)
        {
            this.paned = new Gtk.ScrolledWindow(null,null);
            this.paned.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
            this.paned.set_shadow_type(Gtk.ShadowType.NONE);
            this.container = new Gtk.EventBox();
            this.container.set_visible_window(true);
            this.paned.style_set += browser_bg_style_changed;
            this.paned.add_with_viewport(this.container);
            this.paned.get_vadjustment().set("step-increment", 20.0);
            this.container.set_focus_vadjustment(this.paned.get_vadjustment());
            /* Bind keys */
            this.paned.key_release_event += browser_key_release_event;
        }
    }

    private string song_checksum = null;
    private void update()
    {
        if(this.paned == null) return;
  

        MPD.Song song = server.playlist_get_current_song(); 
        if(song != null && MPD.Player.get_state(server) != MPD.Player.State.STOP) {
            var checksum = Gmpc.Misc.song_checksum(song);
            if(checksum != this.song_checksum)
            {
                /* Clear */
                var list = this.container.get_children();
                foreach(Gtk.Widget child in list){
                    child.destroy();
                }
                var view = this.browser.metadata_box_show_song(song);
                this.container.add(view);
                this.song_checksum = checksum;
            }
        } else{
            this.song_checksum = null;
            /* Clear */
            var list = this.container.get_children();
            foreach(Gtk.Widget child in list){
                child.destroy();
            }
            var it = Gtk.IconTheme.get_default();
            weak Gtk.IconInfo info = it.lookup_icon("gmpc", 150, 0);
            var path = info.get_filename();
            Gtk.Image image = null;
            if(path != null)
            {
                try {
                var pb = new Gdk.Pixbuf.from_file_at_scale(path, 150, 150, true);
                image = new Gtk.Image.from_pixbuf(pb);
                } catch (Error e)
                {

                }
            }
            if(image == null){
                image = new Gtk.Image.from_icon_name("gmpc", Gtk.IconSize.DIALOG);
            }
            
            var hbox = new Gtk.HBox(false, 6);
            var label = new Gtk.Label(_("Gnome Music Player Client"));
            label.set_selectable(true);
            label.set_markup("<span size='%i' weight='bold'>%s</span>".printf(28*Pango.SCALE,_("Gnome Music Player Client")));
            hbox.pack_start(image, false, false, 0);
            hbox.pack_start(label, false, false, 0);

            var ali = new Gtk.Alignment(0.5f,0.5f,0.0f, 0.0f);
            ali.add(hbox);
            this.container.add(ali);
        }
        this.paned.show_all();
    }
    private void select_now_playing_browser(Gtk.ImageMenuItem item)
    {
        weak Gtk.TreeView tree = Gmpc.Playlist3.get_category_tree_view();
        var sel = tree.get_selection();
        var path = np_ref.get_path();
        sel.select_path(path);
    }
    private int browser_add_go_menu(Gtk.Menu menu)
    {
        var item = new Gtk.ImageMenuItem.with_mnemonic(_("Now Playing"));
        item.set_image(new Gtk.Image.from_stock("gtk-info", Gtk.IconSize.MENU));
        item.activate += select_now_playing_browser;
        item.add_accelerator("activate", menu.get_accel_group(),0x069, Gdk.ModifierType.CONTROL_MASK, Gtk.AccelFlags.VISIBLE);
        menu.append(item);

        return 1;
    }
}

public class  Gmpc.MetadataBrowser : Gmpc.Plugin.Base, Gmpc.Plugin.BrowserIface, Gmpc.Plugin.PreferencesIface {
    private int block_update = 0;
    /* Stores the location in the cat_tree */
    private Gtk.TreeRowReference rref = null;

    construct {
        /* Set the plugin as an internal one and of type pl_browser */
        this.plugin_type = 2|8; 

        gmpcconn.connection_changed += con_changed;
        gmpcconn.status_changed += status_changed;
    }

    public const int[] version =  {0,0,0};
    public override  weak int[3] get_version() {
        return version;
    }

    public override weak string get_name() {
        return N_("Metadata Browser");
    }

    public override void save_yourself() {
        if(this.paned != null) {
            int pos = this.paned.get_position();
            config.set_int(this.get_name(), "pane-pos", pos);
        }
        
        if(this.model_artist != null) this.model_artist.set_mpd_data(null);
        if(this.model_albums != null)this.model_albums.set_mpd_data(null);

        if(this.rref != null) {
            var path = rref.get_path();
            if(path != null) {
                weak int[] indices  = path.get_indices();
                config.set_int(this.get_name(), "position", indices[0]);
            }
        }
    }
    /* Now playing browser */
    

    /**
     * Browser part
     */
     /* 'base' widget */
    private Gtk.Paned paned = null;
    /* holding the 3 browsers */
    private Gtk.Box browser_box = null;
    /* The 3 browsers */
    /* artist */
    private Gtk.TreeView tree_artist = null;
    private Gmpc.MpdData.Model model_artist = null;
    private Gtk.TreeModelFilter model_filter_artist = null; 
    private Gtk.Entry artist_filter_entry = null;

    /* album */
    private Gtk.TreeView tree_album  = null;
    private Gmpc.MpdData.Model model_albums = null;
    private Gtk.TreeModelFilter model_filter_album = null; 
    private Gtk.Entry album_filter_entry = null;
    
    /* song */
    private Gtk.TreeView tree_songs  = null;
    private Gmpc.MpdData.Model model_songs = null;

    /* The right hand "browser" box */
    private Gtk.ScrolledWindow metadata_sw = null;
    private Gtk.EventBox metadata_box = null;

    /**
     * This builds the browser
     */
     private void browser_bg_style_changed(Gtk.ScrolledWindow bg,Gtk.Style? style)
     {
        this.metadata_box.modify_bg(Gtk.StateType.NORMAL,this.metadata_sw.style.base[Gtk.StateType.NORMAL]);
     }
     /* This hack makes clicking a selected row again, unselect it */
     private bool browser_button_press_event(Gtk.TreeView tree, Gdk.EventButton event)
     {
        Gtk.TreePath path= null;
        if(event.button != 1) return false;
        if(tree.get_path_at_pos((int)event.x,(int)event.y,out path, null, null, null))
        {
            if(tree.get_selection().path_is_selected(path)){
                tree.get_selection().unselect_path(path);
                return true;
            }
        }
        return false;
     }
    /**
     * Artist tree view functions */
     private void browser_artist_entry_changed(Gtk.Entry entry)
     {
        string text = entry.get_text();
        if(text.size() > 0) {
            entry.show();
        }else{
            entry.hide();
            this.tree_artist.grab_focus();
        }
        this.model_filter_artist.refilter();
     }
     private void artist_add_clicked(Gtk.ImageMenuItem item )
     {
        string artist = browser_get_selected_artist(); 
        if(artist != null)
        {
            MPD.Database.search_field_start(server, MPD.Tag.Type.FILENAME);
            MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
            MPD.Data.Item data = MPD.Database.search_commit(server); 
            if(data != null)
            {
                do{
                    MPD.PlayQueue.queue_add_song(server, data.tag);
                    data.next_free();
                }while(data != null);
                MPD.PlayQueue.queue_commit(server);
            }
        }
     }
     private void artist_replace_clicked(Gtk.ImageMenuItem item)
     {
         MPD.PlayQueue.clear(server);
         artist_add_clicked(item);
         MPD.Player.play(server);
     }
     /* Handle right mouse click */
    private bool artist_browser_button_release_event(Gtk.TreeView tree, Gdk.EventButton event)
    {
        if(event.button == 3) {
            if(tree.get_selection().count_selected_rows()>0)
            {
                var menu = new Gtk.Menu();
                var item = new Gtk.ImageMenuItem.from_stock("gtk-add",null);
                item.activate += artist_add_clicked;
                menu.append(item);

                item = new Gtk.ImageMenuItem.with_mnemonic("_Replace");
                item.set_image(new Gtk.Image.from_stock("gtk-redo", Gtk.IconSize.MENU));
                item.activate += artist_replace_clicked;
                menu.append(item);

                menu.popup(null, null, null, event.button, event.time);
                menu.show_all();
                return true;
            }
        }

        return false;
    }

     private bool visible_func_artist (Gtk.TreeModel model, Gtk.TreeIter  iter)
     {
         string text = this.artist_filter_entry.get_text();
         /* Visible if row is non-empty and first column is "HI" */
         string str = null;
         bool visible = false;

         if(text[0] == '\0') return true;

         model.get (iter, 7, out str, -1);
         if (str != null && str.casefold().normalize().str(text.casefold().normalize()) != null)
             visible = true;

         return visible;
     }
     private bool browser_artist_key_press_event(Gtk.TreeView widget, Gdk.EventKey event)
     {
        unichar uc = Gdk.keyval_to_unicode(event.keyval);
        if(uc > 0)
        {
            string outbuf = "       ";
            int i = uc.to_utf8(outbuf);
            ((char[])outbuf)[i] = '\0';
            this.artist_filter_entry.set_text(outbuf);
            this.artist_filter_entry.grab_focus();
            this.artist_filter_entry.set_position(1);

           return true; 
        }
        return false;
     }
     /** 
      * Album tree view
      */
     private void album_add_clicked(Gtk.ImageMenuItem item )
     {
        string artist = browser_get_selected_artist(); 
        if(artist != null)
        {
            string albumartist = null;
            string album = browser_get_selected_album();
            if(album != null)
            {
                MPD.Database.search_field_start(server, MPD.Tag.Type.ALBUM_ARTIST);
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
                var ydata = MPD.Database.search_commit(server);
                if(ydata != null)
                {
                    if(ydata.tag.length > 0)
                        albumartist = ydata.tag;
                }
            } 
            /* Fill in the first browser */ 
            MPD.Database.search_field_start(server,MPD.Tag.Type.FILENAME);
            if(albumartist != null)
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM_ARTIST, albumartist);
            else
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);

            if(album != null)
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
            var data = MPD.Database.search_commit(server);
            if(data != null)
            {
                do{
                    MPD.PlayQueue.queue_add_song(server, data.tag);
                    data.next_free();
                }while(data != null);
                MPD.PlayQueue.queue_commit(server);
            }
        }
     }
     private void album_replace_clicked(Gtk.ImageMenuItem item)
     {
         MPD.PlayQueue.clear(server);
         album_add_clicked(item);
         MPD.Player.play(server);
     }
    /* Handle right mouse click */
    private bool album_browser_button_release_event(Gtk.TreeView tree, Gdk.EventButton event)
    {
        if(event.button == 3) {
            if(tree.get_selection().count_selected_rows()>0)
            {
                var menu = new Gtk.Menu();
                var item = new Gtk.ImageMenuItem.from_stock("gtk-add",null);
                item.activate += album_add_clicked;
                menu.append(item);

                item = new Gtk.ImageMenuItem.with_mnemonic("_Replace");
                item.set_image(new Gtk.Image.from_stock("gtk-redo", Gtk.IconSize.MENU));
                item.activate += album_replace_clicked;
                menu.append(item);

                menu.popup(null, null, null, event.button, event.time);
                menu.show_all();
                return true;
            }
        }

        return false;
    }
     private bool visible_func_album (Gtk.TreeModel model, Gtk.TreeIter  iter)
     {
         string text = this.album_filter_entry.get_text();
         /* Visible if row is non-empty and first column is "HI" */
         string str = null;
         bool visible = false;

         if(text[0] == '\0') return true;

         model.get (iter, 6, out str, -1);
         if (str != null && str.casefold().normalize().str(text.casefold().normalize()) != null)
             visible = true;

         return visible;
     }


     private bool browser_album_key_press_event(Gtk.TreeView widget, Gdk.EventKey event)
     {
        unichar uc = Gdk.keyval_to_unicode(event.keyval);
        if(uc > 0)
        {
            string outbuf = "       ";
            int i = uc.to_utf8(outbuf);
            ((char[])outbuf)[i] = '\0';
            this.album_filter_entry.set_text(outbuf);
            this.album_filter_entry.grab_focus();
            this.album_filter_entry.set_position(1);

           return true; 
        }
        return false;
     }

     private void browser_album_entry_changed(Gtk.Entry entry)
     {
        string text = entry.get_text();
        if(text.size() > 0) {
            entry.show();
        }else{
            entry.hide();
            this.tree_album.grab_focus();
        }
        this.model_filter_album.refilter();
     }
     /**
      * Songs 
      */
     private void song_add_clicked(Gtk.ImageMenuItem item )
     {
        MPD.Song? song = browser_get_selected_song(); 
        if(song != null)
        {
            MPD.PlayQueue.add_song(server,song.file); 
        }
     }
     private void song_replace_clicked(Gtk.ImageMenuItem item)
     {
         MPD.PlayQueue.clear(server);
         song_add_clicked(item);
         MPD.Player.play(server);
     }
     /* Handle right mouse click */
    private bool song_browser_button_release_event(Gtk.TreeView tree, Gdk.EventButton event)
    {
        if(event.button == 3) {
            if(tree.get_selection().count_selected_rows()>0)
            {
                var menu = new Gtk.Menu();
                var item = new Gtk.ImageMenuItem.from_stock("gtk-add",null);
                item.activate += song_add_clicked;
                menu.append(item);

                item = new Gtk.ImageMenuItem.with_mnemonic("_Replace");
                item.set_image(new Gtk.Image.from_stock("gtk-redo", Gtk.IconSize.MENU));
                item.activate += song_replace_clicked;
                menu.append(item);

                menu.popup(null, null, null, event.button, event.time);
                menu.show_all();
                return true;
            }
        }
        return false;
    }

    private void browser_init()
    {
        if(this.paned == null)
        {
            this.paned = new Gtk.HPaned();
            this.paned.set_position(config.get_int_with_default(this.get_name(), "pane-pos", 150));
            /* Bow with browsers */
            this.browser_box = new Gtk.VBox(true, 6);
            this.paned.add1(this.browser_box);

            /* Artist list  */
            var box = new Gtk.VBox(false, 6);
            this.browser_box.pack_start(box, true, true, 0);

            this.artist_filter_entry = new Gtk.Entry();
            this.artist_filter_entry.set_no_show_all(true);
            this.artist_filter_entry.changed += browser_artist_entry_changed;

            box.pack_start(this.artist_filter_entry, false, false, 0);

            var sw = new Gtk.ScrolledWindow(null, null);
            sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
            sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN);
            box.pack_start(sw, true, true, 0);

            this.model_artist = new Gmpc.MpdData.Model();
            this.model_filter_artist = new Gtk.TreeModelFilter(this.model_artist, null);
            this.model_filter_artist.set_visible_func(visible_func_artist);
            this.tree_artist = new Gtk.TreeView.with_model(this.model_filter_artist);
            new Gmpc.MpdData.Treeview.Tooltip(this.tree_artist, Gmpc.MetaData.Type.ARTIST_ART);

            this.tree_artist.button_press_event+=browser_button_press_event;
            this.tree_artist.button_release_event+=artist_browser_button_release_event;
            this.tree_artist.key_press_event += browser_artist_key_press_event;
            sw.add(tree_artist);
            /* setup the columns */ 
            var column = new Gtk.TreeViewColumn();
            var prenderer = new Gtk.CellRendererPixbuf();
            prenderer.set("height", this.model_artist.icon_size);
            column.pack_start(prenderer, false);
            column.add_attribute(prenderer, "pixbuf",27); 
            var trenderer = new Gtk.CellRendererText();
            column.pack_start(trenderer, true);
            column.add_attribute(trenderer, "text", 7);
            this.tree_artist.append_column(column);
            column.set_title(_("Artist"));
            this.tree_artist.get_selection().changed += browser_artist_changed;
            this.tree_artist.set_search_column(7);

            /* set fixed height mode */
            column.sizing = Gtk.TreeViewColumnSizing.FIXED;
            this.tree_artist.set_fixed_height_mode(true);


            /* Album list */

            box = new Gtk.VBox(false, 6);
            this.browser_box.pack_start(box, true, true, 0);

            this.album_filter_entry = new Gtk.Entry();
            this.album_filter_entry.set_no_show_all(true);
            this.album_filter_entry.changed += browser_album_entry_changed;
            box.pack_start(this.album_filter_entry, false, false, 0);

            sw = new Gtk.ScrolledWindow(null, null);
            sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
            sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN);
            box.pack_start(sw, true, true, 0);
            this.model_albums = new Gmpc.MpdData.Model();
            this.model_filter_album = new Gtk.TreeModelFilter(this.model_albums, null);
            this.model_filter_album.set_visible_func(visible_func_album);
            this.tree_album = new Gtk.TreeView.with_model(this.model_filter_album);
            new Gmpc.MpdData.Treeview.Tooltip(this.tree_album, Gmpc.MetaData.Type.ALBUM_ART);

            this.tree_album.button_press_event+=browser_button_press_event;
            this.tree_album.button_release_event+=album_browser_button_release_event;
            this.tree_album.key_press_event += browser_album_key_press_event;
            sw.add(tree_album);
            /* setup the columns */ 
            column = new Gtk.TreeViewColumn();
            prenderer = new Gtk.CellRendererPixbuf();
            prenderer.set("height", this.model_albums.icon_size);
            column.pack_start(prenderer, false);
            column.add_attribute(prenderer, "pixbuf",27); 
            this.tree_album.append_column(column);

            column = new Gtk.TreeViewColumn();
            trenderer = new Gtk.CellRendererText();
            column.pack_start(trenderer, true);
            column.add_attribute(trenderer, "text", 14);
            this.tree_album.append_column(column);
            column.set_title(_("Year"));

            column = new Gtk.TreeViewColumn();
            trenderer = new Gtk.CellRendererText();
            column.pack_start(trenderer, true);
            column.add_attribute(trenderer, "text", 6);
            this.tree_album.append_column(column);
            this.tree_album.set_search_column(6);
            column.set_title(_("Album"));


            this.tree_album.get_selection().changed += browser_album_changed;

            /* Song list */
            sw = new Gtk.ScrolledWindow(null, null);
            sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
            sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN);
            this.browser_box.pack_start(sw, true, true, 0);
            this.model_songs = new Gmpc.MpdData.Model();
            this.tree_songs = new Gtk.TreeView.with_model(this.model_songs);
            this.tree_songs.button_press_event+=browser_button_press_event;
            this.tree_songs.button_release_event+=song_browser_button_release_event;
            sw.add(tree_songs);
            /* setup the columns */ 
            column = new Gtk.TreeViewColumn();
            prenderer = new Gtk.CellRendererPixbuf();
            column.pack_start(prenderer, false);
            column.add_attribute(prenderer, "icon-name",23); 
            trenderer = new Gtk.CellRendererText();
            column.pack_start(trenderer, false);
            column.add_attribute(trenderer, "text", 10);

            column.set_title(_("Track"));
            this.tree_songs.append_column(column);

            column = new Gtk.TreeViewColumn();
            trenderer = new Gtk.CellRendererText();
            column.pack_start(trenderer, true);
            column.add_attribute(trenderer, "text", 7);


            this.tree_songs.append_column(column);
            this.tree_songs.set_search_column(7);
            column.set_title(_("Songs"));

            this.tree_songs.get_selection().changed += browser_songs_changed;

            /* The right view */
            this.metadata_sw = new Gtk.ScrolledWindow(null, null);
            this.metadata_sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
            this.metadata_sw.style_set += browser_bg_style_changed;
            this.metadata_box = new Gtk.EventBox();
            this.metadata_box.set_visible_window(true);
            this.metadata_sw.add_with_viewport(this.metadata_box);
            
            this.paned.add2(this.metadata_sw);

            this.reload_browsers();
        }
        this.paned.show_all();
    }

    private void reload_browsers()
    {
        if(this.paned == null) return;

        this.model_songs.set_mpd_data(null);
        this.model_albums.set_mpd_data(null);
        this.model_artist.set_mpd_data(null);

        this.artist_filter_entry.set_text("");
        this.album_filter_entry.set_text("");

        /* Fill in the first browser */
        MPD.Database.search_field_start(server,MPD.Tag.Type.ARTIST);
        var data = MPD.Database.search_commit(server);
        data = Gmpc.MpdData.sort_album_disc_track((owned)data);
        this.model_artist.set_mpd_data((owned)data);
    }

    private string? browser_get_selected_artist()
    {
        Gtk.TreeIter iter;
        var sel = this.tree_artist.get_selection();
        Gtk.TreeModel model = null;//this.model_artist;
        if(sel.get_selected(out model, out iter))
        {
            string artist = null;
            model.get(iter, 7,out artist, -1);
            return artist;
        }
        return null;
    }

    private string? browser_get_selected_album()
    {
        Gtk.TreeIter iter;
        var sel = this.tree_album.get_selection();
        Gtk.TreeModel model = null;//this.model_albums;
        if(sel.get_selected(out model, out iter))
        {
            string album = null;
            model.get(iter, 6,out album, -1);
            return album;
        }
        return null;
    }
    private MPD.Song? browser_get_selected_song()
    {
        Gtk.TreeIter iter;
        var sel = this.tree_songs.get_selection();
        if(sel.get_selected(out this.model_songs, out iter))
        {
            weak MPD.Song songs = null;
            this.model_songs .get(iter, 0,out songs, -1);
            return songs;
        }
        return null;
    }
    private void browser_artist_changed(Gtk.TreeSelection sel)
    {
        this.model_albums.set_mpd_data(null);
        this.model_songs.set_mpd_data(null);
        this.metadata_box_clear();

        string artist = browser_get_selected_artist();
        if(artist != null)
        {
            /* Fill in the first browser */
            MPD.Database.search_field_start(server,MPD.Tag.Type.ALBUM);
            MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
            var data = MPD.Database.search_commit(server);
            data = Gmpc.MpdData.sort_album_disc_track((owned)data);

            this.model_albums.set_request_artist(artist);
            MPD.Data.Item list = null;
            weak MPD.Data.Item iter = data.first();
            if(iter!= null)
            {
                do{
                    list.append_new();
                    list.type = MPD.Data.Type.SONG;
                    list.song = new MPD.Song();
                    list.song.artist = artist;
                    list.song.album  = iter.tag;
                    MPD.Database.search_field_start(server,MPD.Tag.Type.DATE);
                    MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
                    MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, iter.tag);
                    var ydata = MPD.Database.search_commit(server);
                    if(ydata != null) {
                        list.song.date = ydata.tag;
                    }
                    iter = iter.next(false);
                }while(iter!= null);
            }

            list = Gmpc.MpdData.sort_album_disc_track((owned)list);
            this.model_albums.set_mpd_data((owned)list);

            MPD.Database.search_start(server,true);
            MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
            data = MPD.Database.search_commit(server);
            data = Gmpc.MpdData.sort_album_disc_track((owned)data);
            this.model_songs.set_mpd_data((owned)data);

        }
        this.metadata_box_update();
    }
    private void browser_album_changed(Gtk.TreeSelection album_sel)
    {
        this.model_songs.set_mpd_data(null);
        this.metadata_box_clear();

        string artist = browser_get_selected_artist();
        if(artist != null)
        {
            string album = browser_get_selected_album();
            string albumartist = null;

            if(album != null)
            {
                MPD.Database.search_field_start(server, MPD.Tag.Type.ALBUM_ARTIST);
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
                var ydata = MPD.Database.search_commit(server);
                if(ydata != null)
                {
                    if(ydata.tag.length > 0)
                        albumartist = ydata.tag;
                }
            } 
            /* Fill in the first browser */ 
            MPD.Database.search_start(server,true);
            if(albumartist != null)
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM_ARTIST, albumartist);
            else
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);

            if(album != null)
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
            var data = MPD.Database.search_commit(server);
            data = Gmpc.MpdData.sort_album_disc_track((owned)data);
            this.model_songs.set_mpd_data((owned)data);
        }
        this.metadata_box_update();
    }
    private void browser_songs_changed (Gtk.TreeSelection song_sel)
    {
        this.metadata_box_clear();
        this.metadata_box_update();
    }
    /** 
     * Metadata box
     */

     private void play_song(Gtk.Button button)
     {
        MPD.Song? song = (MPD.Song )button.get_data("song"); 
        if(song != null){
            Gmpc.Misc.play_path(song.file);
        }
     }

     private void add_song(Gtk.Button button)
     {
        MPD.Song? song = (MPD.Song )button.get_data("song"); 
        if(song != null){
            MPD.PlayQueue.add_song(server,song.file);
            return;
        }
     }
     private void replace_song(Gtk.Button button)
     {
         MPD.Song? song = (MPD.Song )button.get_data("song"); 
         if(song != null){
             MPD.PlayQueue.clear(server);
             MPD.PlayQueue.add_song(server,song.file);
             MPD.Player.play(server);
             return;
         }
     }
     private void add_selected_song(Gtk.Button button)
     {
        string artist = browser_get_selected_artist(); 
        string album = browser_get_selected_album(); 
        MPD.Song? song = browser_get_selected_song(); 
        if(song != null){
            MPD.PlayQueue.add_song(server,song.file);
            return;
        }
        if(artist != null ) {
            /* Hunt albumartist */
            string albumartist = null;
            if(album != null)
            {
                MPD.Database.search_field_start(server, MPD.Tag.Type.ALBUM_ARTIST);
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
                var ydata = MPD.Database.search_commit(server);
                if(ydata != null)
                {
                    if(ydata.tag.length > 0)
                        albumartist = ydata.tag;
                }
            }

            MPD.Database.search_field_start(server,MPD.Tag.Type.FILENAME);
            if(albumartist != null)
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM_ARTIST, albumartist);
            else
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
            if(album != null)
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
            var data = MPD.Database.search_commit(server);
            if(data != null) {
                weak MPD.Data.Item iter = Gmpc.MpdData.sort_album_disc_track(data);
                do{
                    MPD.PlayQueue.queue_add_song(server, iter.tag);
                }while((iter = iter.next(false)) != null);
                MPD.PlayQueue.queue_commit(server);
            }   
        }
     }

     private void replace_selected_song(Gtk.Button button)
     {
            MPD.PlayQueue.clear(server);
            this.add_selected_song(button);
            MPD.Player.play(server);
     }
     private void metadata_box_clear()
     {
         var list = this.metadata_box.get_children();
         foreach(Gtk.Widget child in list){
             child.destroy();
         }
     }
     /**
      * Add a row to a gtk table
      * <b>$label:</b> $value
      * then increments i 
      */

     private void add_entry(Gtk.Table table, string entry_label, string? value,Gtk.Widget? extra,  out int i)
     {
         if(value == null && extra == null) return;
         var label = new Gtk.Label("");
         label.set_selectable(true);
         label.set_alignment(0.0f, 0.5f);
         label.set_markup(Markup.printf_escaped("<b>%s:</b>",entry_label));
         table.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);

         var dhbox = new Gtk.HBox(false, 6);
         if(value != null)
         {
             var pt_label = new Gtk.Label(value);
             pt_label.set_selectable(true);
             pt_label.set_alignment(0.0f, 0.5f);
             pt_label.set_line_wrap(true);
             dhbox.pack_start(pt_label, false, false, 0);
         }
         if(extra != null)
         {
             dhbox.pack_start(extra, false, false, 0);
         }
         table.attach(dhbox, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
         i++;
     }

    public Gtk.Widget metadata_box_show_song(MPD.Song song)
    {
        var vbox = new Gtk.VBox (false,6);
        vbox.border_width = 8;
        var label = new Gtk.Label("");
        label.set_selectable(true);
        if(song.title != null) {
            label.set_markup(Markup.printf_escaped("<span size='xx-large' weight='bold'>%s</span>",song.title));
        }else {
            label.set_markup(Markup.printf_escaped("<span size='xx-large' weight='bold'>%s</span>",_("Unknown")));
        }
        label.set_alignment(0.0f, 0.5f);
        vbox.pack_start(label, false, false, 0);

        /* Artist image */
        var hbox = new Gtk.HBox(false, 6);
        var ali = new Gtk.Alignment(0f,0f,0f,0f);
        var artist_image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ALBUM_ART, 250);
        artist_image.set_squared(false);
        artist_image.update_from_song(song);
        ali.add(artist_image);
        hbox.pack_start(ali, false, false, 0);
        /* Artist information */
        var info_box = new Gtk.Table (4,2,false);
        info_box.set_row_spacings(3);
        info_box.set_col_spacings(8);
        hbox.pack_start(info_box, false, false, 0);
        int i=0;

        if(song.title != null)
        {
            /* Button to search for song with same title */
            var button = new Gtk.Button();
            button.add(new Gtk.Image.from_stock("gtk-find", Gtk.IconSize.MENU));
            button.set_relief(Gtk.ReliefStyle.NONE);
            button.set_data_full("query", (void *)"title=(%s)".printf(song.title), (GLib.DestroyNotify) g_free);
            button.clicked += metadata_find_query;
            button.set_tooltip_text(_("Search songs with similar title"));

            this.add_entry(info_box, _("Title"), song.title, button, out i);
        }
        /* Artist label */
        this.add_entry(info_box, _("Artist"), song.artist, null, out i);
        /* AlbumArtist label */
        this.add_entry(info_box, _("Album artist"), song.albumartist, null, out i);

        /* Album */
        this.add_entry(info_box, _("Album"), song.album, null, out i);

        /* track */
        this.add_entry(info_box, _("Track"), song.track, null, out i);

        /* date */
        this.add_entry(info_box, _("Date"), song.date, null, out i);
        /* performer */
        this.add_entry(info_box, _("Performer"), song.performer, null, out i);
        /* disc */
        this.add_entry(info_box, _("Disc"), song.disc, null, out i);

        /* Genre */
        this.add_entry(info_box, _("Disc"), song.genre, null, out i);

        /* Path */
        if(song.file != null) {
            var dbutton = new Gtk.Button();
            dbutton.set_relief(Gtk.ReliefStyle.NONE);
            dbutton.add(new Gtk.Image.from_stock("gtk-open", Gtk.IconSize.MENU));
            dbutton.set_data_full("path", (void *)GLib.Path.get_dirname(song.file), (GLib.DestroyNotify) g_free);
            dbutton.clicked += metadata_button_open_file_browser_path;
            dbutton.set_tooltip_text(_("Open path to song in file browser"));

            this.add_entry(info_box, _("Path"), song.file, dbutton, out i);
        }

        /* Favored button */
        var fav_button = new Gmpc.Favorites.Button();
        fav_button.set_song(song);
        ali = new Gtk.Alignment(0.0f, 0.5f,0f,0f);
        ali.add(fav_button);
        this.add_entry(info_box, _("Favored"), null, ali, out i);

        if(MPD.Sticker.supported(server))
        {
            /* Favored button */
            var rating_button = new Gmpc.Rating(server, song);
            ali = new Gtk.Alignment(0.0f, 0.5f,0f,0f);
            ali.add(rating_button);
            this.add_entry(info_box, _("Rating"), null, ali, out i);
        }


        /* Comment */
        this.add_entry(info_box, _("Comment"), song.comment, null, out i);

        vbox.pack_start(hbox , false, false, 0);
        /* Player controls */
        var button = new Gtk.Button.from_stock("gtk-media-play");
        button.set_relief(Gtk.ReliefStyle.NONE);
        button.set_data_full("song", song.copy(), (GLib.DestroyNotify) MPD.Song.free);
        button.clicked += play_song;
        hbox = new Gtk.HBox (false, 6);
        hbox.pack_start(button, false, false,0);

        button = new Gtk.Button.from_stock("gtk-add");
        button.set_relief(Gtk.ReliefStyle.NONE);
        button.set_data_full("song", song.copy(), (GLib.DestroyNotify) MPD.Song.free);
        button.clicked += add_song;
        hbox.pack_start(button, false, false,0);

        button = new Gtk.Button.with_mnemonic("_Replace");
        button.set_image(new Gtk.Image.from_stock("gtk-redo", Gtk.IconSize.BUTTON));
        button.set_relief(Gtk.ReliefStyle.NONE);
        button.set_data_full("song", song.copy(), (GLib.DestroyNotify) MPD.Song.free);
        button.clicked += replace_song;
        hbox.pack_start(button, false, false,0);

        info_box.attach(hbox, 0,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        i++;



        /* Lyrics */
        if(config.get_int_with_default("MetaData", "show-lyrics",1) == 1)
        {
            var text_view = new Gmpc.MetaData.TextView(Gmpc.MetaData.Type.SONG_TXT);
            text_view.set_left_margin(8);
            var frame = new Gmpc.Widget.More(Markup.printf_escaped("<b>%s:</b>", _("Lyrics")),text_view);
            text_view.query_from_song(song);

            vbox.pack_start(frame, false, false, 0);
        }

        /* Guitar Tab */

        if(config.get_int_with_default("MetaData", "show-guitar-tabs",1) == 1)
        {
            var text_view = new Gmpc.MetaData.TextView(Gmpc.MetaData.Type.SONG_GUITAR_TAB);
            text_view.use_monospace = true;
            text_view.set_left_margin(8);
            var frame = new Gmpc.Widget.More(Markup.printf_escaped("<b>%s:</b>", _("Guitar Tabs")),text_view);
            text_view.query_from_song(song);

            vbox.pack_start(frame, false, false, 0);
        }

        if(config.get_int_with_default("MetaData", "show-similar-songs",1) == 1)
        {
            var similar_songs = new Gmpc.Widget.SimilarSongs(song);
            vbox.pack_start(similar_songs, false, false, 0);
        }


        /* Show web links */
        if(config.get_int_with_default("MetaData", "show-web-links",1) == 1)
        {
            var song_links = new Gmpc.Song.Links(Gmpc.Song.Links.Type.SONG,song);
            vbox.pack_start(song_links,false, false, 0);
        }


        /**
         * Add it to the view
         */
         /*
        this.metadata_box.add(vbox);
        this.metadata_sw.show_all();
        */
        return vbox;
    }
    private void metadata_button_open_file_browser_path(Gtk.Button button)
    {
        string path = (string?)button.get_data("path");
        if(path != null)
        {
            Gmpc.Browser.File.open_path(path);
        }
    }
    private void metadata_find_query(Gtk.Button button)
    {
        string path = (string?)button.get_data("query");
        if(path != null)
        {
            Gmpc.Browser.Find.query_database(null, path);
        }

    }
    private void album_song_tree_row_activated(Gtk.TreeView tree, Gtk.TreePath path, Gtk.TreeViewColumn column)
    {
        Gtk.TreeIter iter;
        var model = tree.get_model();
        if(model.get_iter(out iter, path))
        {
            weak MPD.Song song = null;
            model.get(iter, 0, out song, -1);
            if(song != null)
            {
                MPD.PlayQueue.add_song(server, song.file); 
            }
        }

    }
    private void album_song_browser_add_clicked(Gtk.ImageMenuItem item)
    {
        Gtk.TreeView tree = (Gtk.TreeView)item.get_data("tree");
        if(tree != null)
        {
            Gtk.TreeIter iter;
            var model = tree.get_model();
            var sel = tree.get_selection();
            GLib.List<Gtk.TreePath> list = sel.get_selected_rows(out model);
            foreach(Gtk.TreePath path in list)
            {
                if(model.get_iter(out iter, path))
                {
                    weak MPD.Song song = null;
                    model.get(iter, 0, out song, -1);
                    if(song != null)
                    {
                        MPD.PlayQueue.queue_add_song(server, song.file); 
                    }
                }
            }
            MPD.PlayQueue.queue_commit(server);
        }
    }
    private void album_song_browser_replace_clicked(Gtk.ImageMenuItem item)
    {
         MPD.PlayQueue.clear(server);
         album_song_browser_add_clicked(item);
         MPD.Player.play(server);
    }


    private bool album_song_tree_button_press_event(Gtk.TreeView tree, Gdk.EventButton event)
    {
        if(event.button == 3) {
            var menu = new Gtk.Menu();
            var item = new Gtk.ImageMenuItem.from_stock("gtk-add",null);
            item.activate += album_song_browser_add_clicked;
            item.set_data("tree", (void *)tree);
            menu.append(item);

            item = new Gtk.ImageMenuItem.with_mnemonic("_Replace");
            item.set_image(new Gtk.Image.from_stock("gtk-redo", Gtk.IconSize.MENU));
            item.set_data("tree", (void *)tree);
            item.activate += album_song_browser_replace_clicked;
            menu.append(item);

            menu.popup(null, null, null, event.button, event.time);
            menu.show_all();
            return true;
        }
        return false;
    }
    private void metadata_box_show_album(string artist, string album)
    {
        var vbox = new Gtk.VBox (false,6);
        vbox.border_width = 8;
        var label = new Gtk.Label("");
        label.set_selectable(true);
        label.set_markup(Markup.printf_escaped("<span size='xx-large' weight='bold'>%s - %s</span>",(artist != null)?artist:_("Unknown"), (album!= null)?album:_("Unknown")));
        label.set_alignment(0.0f, 0.5f);
        vbox.pack_start(label, false, false, 0);

        /* Artist image */
        var hbox = new Gtk.HBox(false, 6);
        var ali = new Gtk.Alignment(0f,0f,0f,0f);
        var artist_image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ALBUM_ART, 250);
        artist_image.set_squared(false);
        MPD.Song song = new MPD.Song();
        song.artist = artist;
        song.album = album;
        artist_image.update_from_song(song);
        ali.add(artist_image);
        hbox.pack_start(ali, false, false, 0);
        /* Artist information */
        var info_box = new Gtk.Table (4,2,false);
        info_box.set_row_spacings(3);
        info_box.set_col_spacings(8);
        hbox.pack_start(info_box, false, false, 0);
        int i=0;

        /* Genres of songs */ 
        var pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ALBUM_GENRES_SONGS, song);
        pt_label.set_alignment(0.0f, 0.5f);
        pt_label.set_line_wrap(true);
        this.add_entry(info_box, _("Genres"), null, pt_label, out i);

        /* Dates of songs */ 
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ALBUM_DATES_SONGS, song);
        pt_label.set_line_wrap(true);
        pt_label.set_alignment(0.0f, 0.5f);
        this.add_entry(info_box, _("Dates"), null, pt_label, out i);
        /* Total number of songs */ 
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ALBUM_NUM_SONGS, song);
        pt_label.set_line_wrap(true);
        pt_label.set_alignment(0.0f, 0.5f);
        this.add_entry(info_box, _("Songs"), null, pt_label, out i);
        /* Total playtime */
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ALBUM_PLAYTIME_SONGS, song);
        pt_label.set_line_wrap(true);
        pt_label.set_alignment(0.0f, 0.5f);
        this.add_entry(info_box, _("Playtime"), null, pt_label, out i);

        vbox.pack_start(hbox , false, false, 0);

        /* Player controls */
        hbox = new Gtk.HBox (false, 6);

        var button = new Gtk.Button.from_stock("gtk-add");
        button.set_relief(Gtk.ReliefStyle.NONE);
        button.clicked += add_selected_song;
        hbox.pack_start(button, false, false,0);

        button = new Gtk.Button.with_mnemonic("_Replace");
        button.set_image(new Gtk.Image.from_stock("gtk-redo", Gtk.IconSize.BUTTON));
        button.set_relief(Gtk.ReliefStyle.NONE);
        button.clicked += replace_selected_song;
        hbox.pack_start(button, false, false,0);


        info_box.attach(hbox, 0,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        i++;

        /* Album information */
        if(config.get_int_with_default("MetaData", "show-album-information",1) == 1)
        {
            var text_view = new Gmpc.MetaData.TextView(Gmpc.MetaData.Type.ALBUM_TXT);
            text_view.set_left_margin(8);
            var frame = new Gmpc.Widget.More(Markup.printf_escaped("<b>%s:</b>", _("Album information")),text_view);
            text_view.query_from_song(song);

            vbox.pack_start(frame, false, false, 0);
        }

        /* Song list. Show songs in album  */
        label = new Gtk.Label("");
        label.set_selectable(true);
        label.set_markup("<b>%s</b>".printf(_("Songs")));
        label.set_alignment(0.0f, 0.5f);
        vbox.pack_start(label, false, false, 0);

        var sw = new Gtk.ScrolledWindow(null, null);
        sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.NEVER);
        sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN);
        var song_tree = new Gmpc.MpdData.TreeView("album-songs", true, this.model_songs);
        song_tree.button_press_event += album_song_tree_button_press_event;
        song_tree.row_activated += album_song_tree_row_activated;
        sw.add(song_tree);
        vbox.pack_start(sw, false, false, 0);

        /* Show web links */
        if(config.get_int_with_default("MetaData", "show-web-links",1) == 1)
        {
            var song_links = new Gmpc.Song.Links(Gmpc.Song.Links.Type.ALBUM,song);
            vbox.pack_start(song_links,false, false, 0);
        }
        /**
         * Add it to the view
         */
        this.metadata_box.add(vbox);
        this.metadata_sw.show_all();
    }
    /**
     * This fills the view for artist 
     * <artist name>
     * <image> | <array with info>
     *           < buttonss>
     *
     * <artist info text>
     *
     * <similar artists>
     * <links>
     */
    private void metadata_box_show_artist(string artist)
    {
        var vbox = new Gtk.VBox (false,6);
        vbox.border_width = 8;
        var label = new Gtk.Label("");
        label.set_selectable(true);
        label.set_markup(Markup.printf_escaped("<span size='xx-large' weight='bold'>%s</span>",(artist != null)?artist:_("Unknown")));
        label.set_alignment(0.0f, 0.5f);
        vbox.pack_start(label, false, false, 0);

        /* Create an MPD.Song with the info for this type set */
        MPD.Song song = new MPD.Song();
        song.artist = artist;
        /* Artist image */
        var hbox = new Gtk.HBox(false, 6);
        var ali = new Gtk.Alignment(0f,0f,0f,0f);
        var artist_image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ARTIST_ART, 250);
        artist_image.set_squared(false);
        artist_image.update_from_song(song);
        ali.add(artist_image);
        hbox.pack_start(ali, false, false, 0);
        /* Artist information */
        var info_box = new Gtk.Table (4,2,false);
        info_box.set_row_spacings(3);
        info_box.set_col_spacings(8);
        hbox.pack_start(info_box, false, false, 0);
        int i=0;

        /* Genres of songs */ 
        var pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ARTIST_GENRES_SONGS, song);
        pt_label.set_alignment(0.0f, 0.5f);
        pt_label.set_line_wrap(true);
        this.add_entry(info_box, _("Genres"), null, pt_label, out i);
        /* Dates of songs */ 
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ARTIST_DATES_SONGS, song);
        pt_label.set_line_wrap(true);
        pt_label.set_alignment(0.0f, 0.5f);
        this.add_entry(info_box, _("Dates"), null, pt_label, out i);
        /* Total number of songs */ 
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ARTIST_NUM_SONGS, song);
        pt_label.set_line_wrap(true);
        pt_label.set_alignment(0.0f, 0.5f);
        this.add_entry(info_box, _("Songs"), null, pt_label, out i);
        /* Total playtime */
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ARTIST_PLAYTIME_SONGS, song);
        pt_label.set_line_wrap(true);
        pt_label.set_alignment(0.0f, 0.5f);
        this.add_entry(info_box, _("Playtime"), null, pt_label, out i);

        vbox.pack_start(hbox , false, false, 0);
        /* Player controls */
        hbox = new Gtk.HBox (false, 6);

        var button = new Gtk.Button.from_stock("gtk-add");
        button.set_relief(Gtk.ReliefStyle.NONE);
        button.clicked += add_selected_song;
        hbox.pack_start(button, false, false,0);

        button = new Gtk.Button.with_mnemonic("_Replace");
        button.set_image(new Gtk.Image.from_stock("gtk-redo", Gtk.IconSize.BUTTON));
        button.set_relief(Gtk.ReliefStyle.NONE);
        button.clicked += replace_selected_song;
        hbox.pack_start(button, false, false,0);


        info_box.attach(hbox, 0,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        i++;

        /* Artist information */
        if(config.get_int_with_default("MetaData", "show-artist-information",1) == 1)
        {
            var text_view = new Gmpc.MetaData.TextView(Gmpc.MetaData.Type.ARTIST_TXT);
            text_view.set_left_margin(8);
            var frame = new Gmpc.Widget.More(Markup.printf_escaped("<b>%s:</b>", _("Artist information")),text_view);
            text_view.query_from_song(song);
            vbox.pack_start(frame, false, false, 0);
        }
        
        /* Show similar artist */
        if(config.get_int_with_default("MetaData", "show-similar-artist",1) == 1)
        {
            label = new Gtk.Label(_("Similar artist"));
            label.set_selectable(true);
            label.set_markup("<span weight='bold'>%s</span>".printf(_("Similar artist")));
            label.set_alignment(0.0f, 0.0f);
            vbox.pack_start(label, false, false, 0);
            var similar_artist = new Gmpc.Widget.SimilarArtist(this,server, song); 
            vbox.pack_start(similar_artist, false, false, 0);
        }

        /* Show web links */
        if(config.get_int_with_default("MetaData", "show-web-links",1) == 1)
        {
            var song_links = new Gmpc.Song.Links(Gmpc.Song.Links.Type.ARTIST,song);
            vbox.pack_start(song_links,false, false, 0);
        }
        /**
         * Add it to the view
         */
        this.metadata_box.add(vbox);
        this.metadata_sw.show_all();
    }


    private uint update_timeout = 0;
    private void metadata_box_update()
    {
        if(this.update_timeout > 0) {
           GLib.Source.remove(this.update_timeout);
        }
        update_timeout = GLib.Idle.add(this.metadata_box_update_real); 
    }
    private bool metadata_box_update_real()
    {
        if(this.block_update > 0){
            this.update_timeout = 0;
            return false;
        }
        string artist = browser_get_selected_artist();
        string album = browser_get_selected_album();
        MPD.Song? song = browser_get_selected_song();

        if(song != null) {
            var view = metadata_box_show_song(song);
            this.metadata_box.add(view);
            this.metadata_sw.show_all();
        }else if(album != null && artist != null) {
            metadata_box_show_album(artist,album);
        }else if (artist != null) {
            metadata_box_show_artist(artist);
        }

        this.update_timeout = 0;
        return false;
    }
    /** 
     * Browser Interface bindings
     */
    public void browser_add (Gtk.Widget category_tree)
    {
        Gtk.TreeView tree = (Gtk.TreeView)category_tree;
        Gtk.ListStore store = (Gtk.ListStore)tree.get_model();
        Gtk.TreeModel model = tree.get_model();
        Gtk.TreeIter iter;
        Gmpc.Browser.insert(out iter, config.get_int_with_default(this.get_name(), "position", 100));
        store.set(iter, 0, this.id, 1, _(this.get_name()), 3, "gtk-info"); 
        /* Create a row reference */
        this.rref = new Gtk.TreeRowReference(model,  model.get_path(iter));
    }
    public void browser_selected (Gtk.Container container)
    {
        string artist;
        this.selected = true;
        this.browser_init();
        container.add(this.paned);

        /* update if non selected */
        artist = browser_get_selected_artist();
        if(artist == null) {
            metadata_box_clear();
            metadata_box_update();
        }
    }

    private bool selected = false;
    public void browser_unselected(Gtk.Container container)
    {
        this.selected = false;
        container.remove(this.paned);
    }
    private
    void 
    con_changed(Gmpc.Connection conn, MPD.Server server, int connect)
    {
        if(this.paned == null) return;
        this.reload_browsers();
        metadata_box_clear();
        metadata_box_update();
    }
    private 
    void
    status_changed(Gmpc.Connection conn, MPD.Server server, MPD.Status.Changed what)
    {
        if(this.paned == null) return;

    }

    public
    void
    set_artist(string artist)
    {
        this.block_update++;


        this.tree_artist.get_selection().unselect_all();
        this.tree_album.get_selection().unselect_all();
        /* clear */
        this.artist_filter_entry.set_text("");
        Gtk.TreeIter iter;
        if(this.model_filter_artist.get_iter_first(out iter))
        {
            do{
                string lartist= null;
                this.model_filter_artist.get(iter, 7, out lartist, -1);
                if( lartist != null && lartist.collate(artist) == 0){
                    this.tree_artist.get_selection().select_iter(iter);
                    this.tree_artist.scroll_to_cell(this.model_filter_artist.get_path(iter), null, true, 0.5f,0f);
                    this.block_update--;

                    this.metadata_box_clear();
                    this.metadata_box_update();
                    return;
                }
            }while((this.model_filter_artist).iter_next(ref iter));
        }
        this.block_update--;

        this.metadata_box_clear();
        this.metadata_box_update();
    }

    public
    void
    set_album(string artist, string album)
    {
        this.block_update++;
        this.set_artist(artist); 
        /* clear */
        this.album_filter_entry.set_text("");
        Gtk.TreeIter iter;
        if(this.model_filter_album.get_iter_first(out iter))
        {
            do{
                string lalbum= null;
                this.model_filter_album.get(iter, 6, out lalbum, -1);
                if( lalbum != null && lalbum.collate(album) == 0){
                    this.tree_album.get_selection().select_iter(iter);
                    this.tree_album.scroll_to_cell(this.model_filter_album.get_path(iter), null, true, 0.5f,0f);
                    this.tree_songs.get_selection().unselect_all();
                    this.block_update--;
                    this.metadata_box_update();
                    return;
                }
            }while((this.model_filter_album).iter_next(ref iter));
        }

        this.tree_songs.get_selection().unselect_all();

        this.block_update--;
        this.metadata_box_clear();
        this.metadata_box_update();
    }

    public 
    void
    set_song(MPD.Song song)
    {
        this.block_update++;
        if(song.artist != null)
        {
            this.set_artist(song.artist);
            if(song.album != null)
            {
                this.set_album(song.artist,song.album);
            }
        }

        Gtk.TreeIter iter;
        if(this.model_songs.get_iter_first(out iter))
        {
            do{
                string ltitle = null;
                this.model_songs.get(iter, 7, out ltitle, -1);
                if( ltitle != null && ltitle.collate(song.title) == 0){
                    this.tree_songs.get_selection().select_iter(iter);
                    this.tree_songs.scroll_to_cell(this.model_songs.get_path(iter), null, true, 0.5f,0f);
                    this.block_update--;
                    this.metadata_box_update();
                    return;
                }
            }while((this.model_songs).iter_next(ref iter));
        }


        this.block_update--;
        this.metadata_box_clear();
        if(this.update_timeout > 0) {
           GLib.Source.remove(this.update_timeout);
           this.update_timeout = 0;
        }
        var view = metadata_box_show_song(song);
        this.metadata_box.add(view);
        this.metadata_box.show_all();
    }
    public 
    void
    select_browser(Gtk.TreeView tree)
    {
        var path = rref.get_path();
        var model = rref.get_model();
        if(path != null){
            Gtk.TreeIter iter;
            if(model.get_iter(out iter, path))
            {
                tree.get_selection().select_iter(iter);
            }
        }

    }
    /** 
     * Preferences
     */
     
     public void
     preferences_pane_construct(Gtk.Container container)
     {
        var box = new Gtk.VBox(false, 6);
        /* Title */
        var label = new Gtk.Label(_("Enable/disable metadata options"));
        label.set_alignment(0.0f, 0.5f);
        box.pack_start(label, false, false,0);

        /* Artist information */
        var chk = new Gtk.CheckButton.with_label(_("Artist information"));
        chk.set_active((config.get_int_with_default("MetaData", "show-artist-information",1) == 1));
        box.pack_start(chk, false, false,0);
        chk.toggled.connect ((source)=> {
           config.set_int("MetaData", "show-artist-information", (int)source.get_active());  
        });
        /* Album information */
        chk = new Gtk.CheckButton.with_label(_("Album information"));
        chk.set_active((config.get_int_with_default("MetaData", "show-album-information",1) == 1));
        box.pack_start(chk, false, false,0);
        chk.toggled.connect ((source)=> {
           config.set_int("MetaData", "show-album-information", (int)source.get_active());  
        });

        /* Artist similar */
        chk = new Gtk.CheckButton.with_label(_("Similar Artist"));
        chk.set_active((config.get_int_with_default("MetaData", "show-similar-artist",1) == 1));
        box.pack_start(chk, false, false,0);
        chk.toggled.connect ((source)=> {
           config.set_int("MetaData", "show-similar-artist", (int)source.get_active());  
        });

        /* Lyrics */ 
        chk = new Gtk.CheckButton.with_label(_("Lyrics"));
        chk.set_active((config.get_int_with_default("MetaData", "show-lyrics",1) == 1));
        box.pack_start(chk, false, false,0);
        chk.toggled.connect ((source)=> {
           config.set_int("MetaData", "show-lyrics", (int)source.get_active());  
        });

        /* Guitar Tabs*/ 
        chk = new Gtk.CheckButton.with_label(_("Guitar Tabs"));
        chk.set_active((config.get_int_with_default("MetaData", "show-guitar-tabs",1) == 1));
        box.pack_start(chk, false, false,0);
        chk.toggled.connect ((source)=> {
           config.set_int("MetaData", "show-guitar-tabs", (int)source.get_active());  
        });
        /* Similar songs*/ 
        chk = new Gtk.CheckButton.with_label(_("Similar Songs"));
        chk.set_active((config.get_int_with_default("MetaData", "show-similar-songs",1) == 1));
        box.pack_start(chk, false, false,0);
        chk.toggled.connect ((source)=> {
           config.set_int("MetaData", "show-similar-songs", (int)source.get_active());  
        });
        /* Web links*/ 
        chk = new Gtk.CheckButton.with_label(_("Web links"));
        chk.set_active((config.get_int_with_default("MetaData", "show-web-links",1) == 1));
        box.pack_start(chk, false, false,0);
        chk.toggled.connect ((source)=> {
           config.set_int("MetaData", "show-web-links", (int)source.get_active());  
        });

        container.add(box);
        box.show_all();
     }

     public void
     preferences_pane_destroy(Gtk.Container container)
     {
        foreach(Gtk.Widget child in container.get_children())
        {
            container.remove(child);
        }
     }
}
