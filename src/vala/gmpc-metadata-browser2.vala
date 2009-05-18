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

private const bool use_transition_mb = Gmpc.use_transition;
private const string some_unique_name = Config.VERSION;


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
        stdout.printf("height: %i\n",alloc.height);
        if(alloc.height < (this.max_height-12)){
            this.expand_button.hide();
        }else{
            this.expand_button.show();
        }
    }

    private void bg_style_changed(Gtk.Widget frame,Gtk.Style? style)
    {
        this.modify_bg(Gtk.StateType.NORMAL,this.parent.style.mid[Gtk.StateType.NORMAL]);
        this.modify_base(Gtk.StateType.NORMAL,this.parent.style.mid[Gtk.StateType.NORMAL]);
        this.eventbox.modify_bg(Gtk.StateType.NORMAL,this.parent.style.mid[Gtk.StateType.NORMAL]);
        this.eventbox.modify_base(Gtk.StateType.NORMAL,this.parent.style.mid[Gtk.StateType.NORMAL]);
        this.pchild.modify_bg(Gtk.StateType.NORMAL,this.parent.style.mid[Gtk.StateType.NORMAL]);
        this.pchild.modify_base(Gtk.StateType.NORMAL,this.parent.style.mid[Gtk.StateType.NORMAL]);
    }
    More(string markup,Gtk.Widget child)
    {
        this.pchild = child;

        this.ali = new Gtk.Alignment(0f,0f,1f,0f); ali.set_padding(6,6,12,12);
        this.eventbox = new Gtk.EventBox();
        this.eventbox.set_visible_window(true);
        this.add(eventbox);
        this.eventbox.add(ali);
        this.ali.set_size_request(-1, this.max_height);
        this.ali.add(child);

        this.ali.style_set += bg_style_changed;

        var hbox = new Gtk.HBox(false, 6);
        var label= new Gtk.Label("");
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

public class  Gmpc.MetadataBrowser : Gmpc.Plugin.Base, Gmpc.Plugin.BrowserIface {

    construct {
        /* Set the plugin as an internal one and of type pl_browser */
        this.plugin_type = 2|8; 

        gmpcconn.connection_changed += con_changed;
        gmpcconn.status_changed += status_changed;
    }

    public const int[3] version =  {0,0,0};
    public override  weak int[3] get_version() {
        return version;
    }

    public override weak string get_name() {
        return "MetaData Browser 2";
    }

    public override void save_yourself() {
        if(this.paned != null) {
            int pos = this.paned.get_position();
            config.set_int("Metadata Browser 2", "pane-pos", pos);
        }
        
        if(this.model_artist != null) this.model_artist.set_mpd_data(null);
        if(this.model_albums != null)this.model_albums.set_mpd_data(null);
    }

    /**
     * Browser part
     */
     /* 'base' widget */
    private Gtk.Paned paned = null;
    /* holding the 3 browsers */
    private Gtk.Box browser_box = null;
    /* The 3 browsers */
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

     private bool visible_func_album (Gtk.TreeModel model, Gtk.TreeIter  iter)
     {
         string text = this.album_filter_entry.get_text();
         /* Visible if row is non-empty and first column is "HI" */
         string str = null;
         bool visible = false;

         if(text[0] == '\0') return true;

         model.get (iter, 7, out str, -1);
         if (str != null && str.casefold().normalize().str(text.casefold().normalize()) != null)
             visible = true;

         return visible;
     }
     private void browser_artist_entry_changed(Gtk.Entry entry)
     {
        this.model_filter_artist.refilter();
     }
     private void browser_album_entry_changed(Gtk.Entry entry)
     {
        this.model_filter_album.refilter();
     }

    private void browser_init()
    {
        if(this.paned == null)
        {
            this.paned = new Gtk.HPaned();
            this.paned.set_position(config.get_int_with_default("Metadata Browser 2", "pane-pos", 150));
            /* Bow with browsers */
            this.browser_box = new Gtk.VBox(true, 6);
            this.paned.add1(this.browser_box);

            /* Artist list  */
            var box = new Gtk.VBox(false, 6);
            this.browser_box.pack_start(box, true, true, 0);

            this.artist_filter_entry = new Gtk.Entry();
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
            this.tree_artist.button_press_event+=browser_button_press_event;
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
            this.tree_album.button_press_event+=browser_button_press_event;
            sw.add(tree_album);
            /* setup the columns */ 
            column = new Gtk.TreeViewColumn();
            prenderer = new Gtk.CellRendererPixbuf();
            prenderer.set("height", this.model_albums.icon_size);
            column.pack_start(prenderer, false);
            column.add_attribute(prenderer, "pixbuf",27); 
            trenderer = new Gtk.CellRendererText();
            column.pack_start(trenderer, true);
            column.add_attribute(trenderer, "text", 7);
            this.tree_album.append_column(column);
            this.tree_album.set_search_column(7);
            column.set_title(_("Album"));


            this.tree_album.get_selection().changed += browser_album_changed;
            /* set fixed height mode */
            column.sizing = Gtk.TreeViewColumnSizing.FIXED;
            this.tree_album.set_fixed_height_mode(true);


            /* Song list */
            sw = new Gtk.ScrolledWindow(null, null);
            sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
            sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN);
            this.browser_box.pack_start(sw, true, true, 0);
            this.model_songs = new Gmpc.MpdData.Model();
            this.tree_songs = new Gtk.TreeView.with_model(this.model_songs);
            this.tree_songs.button_press_event+=browser_button_press_event;
            sw.add(tree_songs);
            /* setup the columns */ 
            column = new Gtk.TreeViewColumn();
            prenderer = new Gtk.CellRendererPixbuf();
            column.pack_start(prenderer, false);
            column.add_attribute(prenderer, "icon-name",23); 
            trenderer = new Gtk.CellRendererText();
            column.pack_start(trenderer, false);
            column.add_attribute(trenderer, "text", 10);

            trenderer = new Gtk.CellRendererText();
            column.pack_start(trenderer, true);
            column.add_attribute(trenderer, "text", 7);


            this.tree_songs.append_column(column);
            this.tree_songs.set_search_column(7);
            column.set_title(_("Songs"));

            /* set fixed height mode */
            column.sizing = Gtk.TreeViewColumnSizing.FIXED;
            this.tree_songs.set_fixed_height_mode(true);

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
            this.metadata_box_update();
        }
        this.paned.show_all();
    }

    private void reload_browsers()
    {
        if(this.paned == null) return;

        this.model_songs.set_mpd_data(null);
        this.model_albums.set_mpd_data(null);
        this.model_artist.set_mpd_data(null);

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
            model.get(iter, 7,out album, -1);
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
            this.model_albums.set_mpd_data((owned)data);

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
            /* Fill in the first browser */
            MPD.Database.search_start(server,true);

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
     private void play_selected_song(Gtk.Button button)
     {
        MPD.Song? song = browser_get_selected_song(); 
        if(song != null){
            Gmpc.Misc.play_path(song.file);
        }
     }

     private void add_selected_song(Gtk.Button button)
     {
        string artist = browser_get_selected_artist(); 
        string album = browser_get_selected_album(); 
        MPD.Song? song = browser_get_selected_song(); 
        if(song != null){
            MPD.PlayQueue.add_song(server,song.file);
        }
        if(artist != null ) {
            MPD.Database.search_field_start(server,MPD.Tag.Type.FILENAME);
            MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
            if(album != null)
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
            var data = MPD.Database.search_commit(server);
            if(data != null) {
                weak MPD.Data.Item iter = data.first();
                do{
                    MPD.PlayQueue.queue_add_song(server, iter.tag);
                }while((iter = iter.next(false)) != null);
                MPD.PlayQueue.queue_commit(server);
            }   
        }
     }

     private void replace_selected_song(Gtk.Button button)
     {
        string artist = browser_get_selected_artist(); 
        string album = browser_get_selected_album(); 
        MPD.Song? song = browser_get_selected_song(); 
        if(song != null){
            MPD.PlayQueue.clear(server);
            MPD.PlayQueue.add_song(server,song.file);
        }
        if(artist != null) {
            MPD.Database.search_field_start(server,MPD.Tag.Type.FILENAME);
            MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
            if(album != null)
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
            var data = MPD.Database.search_commit(server);
            if(data != null) {
                weak MPD.Data.Item iter = data.first();
                MPD.PlayQueue.clear(server);
                do{
                    MPD.PlayQueue.queue_add_song(server, iter.tag);
                }while((iter = iter.next(false)) != null);
                MPD.PlayQueue.queue_commit(server);
                MPD.Player.play(server);
            }   
        }
     }
    private void metadata_box_clear()
    {
        var list = this.metadata_box.get_children();
        foreach(Gtk.Widget child in list){
            child.destroy();
        }
    }
    private void metadata_box_show_song(MPD.Song song)
    {
        var vbox = new Gtk.VBox (false,6);
        vbox.border_width = 8;
        var label = new Gtk.Label("");
        label.set_markup(Markup.printf_escaped("<span size='xx-large' weight='bold'>%s</span>",song.title));
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

        /* Artist label */
        var pt_label = new Gtk.Label(song.artist); 
        label = new Gtk.Label("");
        label.set_alignment(0.0f, 0.5f);
        label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Artist")));
        info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        pt_label.set_alignment(0.0f, 0.5f);
        pt_label.set_line_wrap(true);
        info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        i++;
        /* AlbumArtist label */
        if(song.albumartist != null)
        {
            pt_label = new Gtk.Label(song.albumartist); 
            label = new Gtk.Label("");
            label.set_alignment(0.0f, 0.5f);
            label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Album artist")));
            info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
            pt_label.set_alignment(0.0f, 0.5f);
            pt_label.set_line_wrap(true);
            info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
            i++;
        }

        /* Album */
        pt_label = new Gtk.Label(song.album); 
        label = new Gtk.Label("");
        label.set_alignment(0.0f, 0.5f);
        label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Album")));
        info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        pt_label.set_alignment(0.0f, 0.5f);
        pt_label.set_line_wrap(true);
        info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        i++;

        /* track */
        if(song.track != null) {
            pt_label = new Gtk.Label(song.track); 
            label = new Gtk.Label("");
            label.set_alignment(0.0f, 0.5f);
            label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Track")));
            info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
            pt_label.set_alignment(0.0f, 0.5f);
            pt_label.set_line_wrap(true);
            info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
            i++;
        }

        /* date */
        if(song.date != null) {
            pt_label = new Gtk.Label(song.date); 
            label = new Gtk.Label("");
            label.set_alignment(0.0f, 0.5f);
            label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Date")));
            info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
            pt_label.set_alignment(0.0f, 0.5f);
            pt_label.set_line_wrap(true);
            info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
            i++;
        }
        /* performer */
        if(song.performer != null) {
            pt_label = new Gtk.Label(song.performer); 
            label = new Gtk.Label("");
            label.set_alignment(0.0f, 0.5f);
            label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Performer")));
            info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
            pt_label.set_alignment(0.0f, 0.5f);
            pt_label.set_line_wrap(true);
            info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
            i++;
        }
        /* disc */
        if(song.disc != null) {
            pt_label = new Gtk.Label(song.disc); 
            label = new Gtk.Label("");
            label.set_alignment(0.0f, 0.5f);
            label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Disc")));
            info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
            pt_label.set_alignment(0.0f, 0.5f);
            pt_label.set_line_wrap(true);
            info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
            i++;
        }

        /* Genre */
        if(song.genre != null) {
            pt_label = new Gtk.Label(song.genre); 
            label = new Gtk.Label("");
            label.set_alignment(0.0f, 0.5f);
            label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Genre")));
            info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
            pt_label.set_alignment(0.0f, 0.5f);
            pt_label.set_line_wrap(true);
            info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
            i++;
        }
        /* Genre */
        if(song.file != null) {
            pt_label = new Gtk.Label(song.file); 
            label = new Gtk.Label("");
            label.set_alignment(0.0f, 0.0f);
            label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Path")));
            info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
            pt_label.set_alignment(0.0f, 0.5f);
            pt_label.set_line_wrap(true);
            info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
            i++;
        }
        /* Comment */
        if(song.comment != null) {
            pt_label = new Gtk.Label(song.comment); 
            label = new Gtk.Label("");
            label.set_alignment(0.0f, 0.0f);
            label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Comment")));
            info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
            pt_label.set_alignment(0.0f, 0.5f);
            pt_label.set_line_wrap(true);
            info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
            i++;
        }

        vbox.pack_start(hbox , false, false, 0);

        /* Player controls */
        var button = new Gtk.Button.from_stock("gtk-media-play");
        button.set_relief(Gtk.ReliefStyle.NONE);
        button.clicked += play_selected_song;
        hbox = new Gtk.HBox (false, 6);
        hbox.pack_start(button, false, false,0);

        button = new Gtk.Button.from_stock("gtk-add");
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



        /* Lyrics */

        var text_view = new Gmpc.MetaData.TextView(Gmpc.MetaData.Type.SONG_TXT);
        var frame = new Gmpc.Widget.More(Markup.printf_escaped("<b>%s:</b>", _("Lyrics")),text_view);
        text_view.query_from_song(song);

        vbox.pack_start(frame, false, false, 0);


        var song_links = new Gmpc.Song.Links(Gmpc.Song.Links.Type.SONG,song);
        vbox.pack_start(song_links,false, false, 0);
        /**
         * Add it to the view
         */
        this.metadata_box.add(vbox);
        this.metadata_sw.show_all();
    }
    private void metadata_box_show_album(string artist, string album)
    {
        var vbox = new Gtk.VBox (false,6);
        vbox.border_width = 8;
        var label = new Gtk.Label("");
        label.set_markup(Markup.printf_escaped("<span size='xx-large' weight='bold'>%s - %s</span>",artist, album));
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
        label = new Gtk.Label("");
        label.set_alignment(0.0f, 0.5f);
        label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Genres")));
        info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        pt_label.set_alignment(0.0f, 0.5f);
        pt_label.set_line_wrap(true);
        info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        i++;

        /* Dates of songs */ 
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ALBUM_DATES_SONGS, song);
        pt_label.set_line_wrap(true);
        label = new Gtk.Label("");
        label.set_alignment(0.0f, 0.0f);
        label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Dates")));
        info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        pt_label.set_alignment(0.0f, 0.5f);
        info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        i++;
        /* Total number of songs */ 
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ALBUM_NUM_SONGS, song);
        pt_label.set_line_wrap(true);
        label = new Gtk.Label("");
        label.set_alignment(0.0f, 0.0f);
        label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Songs")));
        info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        pt_label.set_alignment(0.0f, 0.5f);
        info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        i++;
        /* Total playtime */
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ALBUM_PLAYTIME_SONGS, song);
        pt_label.set_line_wrap(true);
        label = new Gtk.Label("");
        label.set_alignment(0.0f, 0.0f);
        label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Playtime")));
        info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        pt_label.set_alignment(0.0f, 0.5f);
        info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        i++;

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
        var text_view = new Gmpc.MetaData.TextView(Gmpc.MetaData.Type.ALBUM_TXT);
        var frame = new Gmpc.Widget.More(Markup.printf_escaped("<b>%s:</b>", _("Album information")),text_view);
        text_view.query_from_song(song);

        vbox.pack_start(frame, false, false, 0);

        var song_links = new Gmpc.Song.Links(Gmpc.Song.Links.Type.ALBUM,song);
        vbox.pack_start(song_links,false, false, 0);
        /**
         * Add it to the view
         */
        this.metadata_box.add(vbox);
        this.metadata_sw.show_all();
    }

    private void metadata_box_show_artist(string artist)
    {
        var vbox = new Gtk.VBox (false,6);
        vbox.border_width = 8;
        var label = new Gtk.Label("");
        label.set_markup(Markup.printf_escaped("<span size='xx-large' weight='bold'>%s</span>",artist));
        label.set_alignment(0.0f, 0.5f);
        vbox.pack_start(label, false, false, 0);

        /* Artist image */
        var hbox = new Gtk.HBox(false, 6);
        var ali = new Gtk.Alignment(0f,0f,0f,0f);
        var artist_image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ARTIST_ART, 250);
        artist_image.set_squared(false);
        MPD.Song song = new MPD.Song();
        song.artist = artist;
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
        label = new Gtk.Label("");
        label.set_alignment(0.0f, 0.5f);
        label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Genres")));
        info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        pt_label.set_alignment(0.0f, 0.5f);
        pt_label.set_line_wrap(true);
        info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        i++;

        /* Dates of songs */ 
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ARTIST_DATES_SONGS, song);
        pt_label.set_line_wrap(true);
        label = new Gtk.Label("");
        label.set_alignment(0.0f, 0.0f);
        label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Dates")));
        info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        pt_label.set_alignment(0.0f, 0.5f);
        info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        i++;
        /* Total number of songs */ 
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ARTIST_NUM_SONGS, song);
        pt_label.set_line_wrap(true);
        label = new Gtk.Label("");
        label.set_alignment(0.0f, 0.0f);
        label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Songs")));
        info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        pt_label.set_alignment(0.0f, 0.5f);
        info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        i++;
        /* Total playtime */
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ARTIST_PLAYTIME_SONGS, song);
        pt_label.set_line_wrap(true);
        label = new Gtk.Label("");
        label.set_alignment(0.0f, 0.0f);
        label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Playtime")));
        info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        pt_label.set_alignment(0.0f, 0.5f);
        info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        i++;

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

        var text_view = new Gmpc.MetaData.TextView(Gmpc.MetaData.Type.ARTIST_TXT);
        var frame = new Gmpc.Widget.More(Markup.printf_escaped("<b>%s:</b>", _("Artist information")),text_view);
        text_view.query_from_song(song);

        vbox.pack_start(frame, false, false, 0);


        var song_links = new Gmpc.Song.Links(Gmpc.Song.Links.Type.ARTIST,song);
        vbox.pack_start(song_links,false, false, 0);
        /**
         * Add it to the view
         */
        this.metadata_box.add(vbox);
        this.metadata_sw.show_all();
    }
    private void metadata_box_update()
    {
        string artist = browser_get_selected_artist();
        string album = browser_get_selected_album();
        MPD.Song? song = browser_get_selected_song();

        if(song != null) {
            metadata_box_show_song(song);
        }else if(album != null && artist != null) {
            metadata_box_show_album(artist,album);
        }else if (artist != null) {
            metadata_box_show_artist(artist);
        }
        else
        {
            song = server.playlist_get_current_song(); 
            if(song != null)
                metadata_box_show_song(song);
        }
    }
    /** 
     * Browser Interface bindings
     */
    public void browser_add (Gtk.Widget category_tree)
    {
        Gtk.TreeView tree = (Gtk.TreeView)category_tree;
        Gtk.ListStore store = (Gtk.ListStore)tree.get_model();
        Gtk.TreeIter iter;
        Gmpc.Browser.insert(out iter, 100);

        store.set(iter, 0, this.id, 1, _("Metadata Browser 2"), 3, "gtk-info"); 
    }
    public void browser_selected (Gtk.Container container)
    {
        this.browser_init();
        stdout.printf("blub\n");
        container.add(this.paned);
    }

    public void browser_unselected(Gtk.Container container)
    {
        stdout.printf("blob\n");
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
        if((what&MPD.Status.Changed.SONGID) == MPD.Status.Changed.SONGID)
        {
            string artist = browser_get_selected_artist();
            if(artist == null) {
                metadata_box_clear();
                metadata_box_update();
            }
        }
    }
}
