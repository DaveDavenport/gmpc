/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@gmpclient.org>
 * Project homepage: http://gmpclient.org/

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

/**
 * This plugin consists of 3 parts
 * Metadata2 plugin: Implements metadata 2 browser.
 * Now Playing plugin: Reusing the metadata 2 browser it implements a now playing browser.
 * Custom widget, there are some custom widgets used by the metadata 2 browser
 *  * Similar songs.
 *  * Similar artist.
 *  * More. (expands, collapses a sub widget
 *
 */
using Config;
using Gtk;
using Gmpc;

private const bool use_transition_mdb = Gmpc.use_transition;
private const string some_unique_name_mdb = Config.VERSION;

public class  Gmpc.Browsers.Metadata : Gmpc.Plugin.Base, Gmpc.Plugin.BrowserIface
{
    private int block_update = 0;
    /* Stores the location in the cat_tree */
    private Gtk.TreeRowReference rref = null;

    private string title_color = config.get_string_with_default("Now Playing", "title-color", "#4d90dd");
    private string item_color = config.get_string_with_default("Now Playing", "item-color", "#304ab8");
    private bool theme_colors = (bool) config.get_int_with_default("Now Playing", "use-theme-color",1);
    private Gdk.Color background;
    private Gdk.Color foreground;


    private void show_metadata_search()
    {
        select_browser(null);
    }

    construct {
        /* Set the plugin as an internal one and of type pl_browser */
        this.plugin_type = 2|8;

        gmpcconn.connection_changed.connect(con_changed);
        gmpcconn.status_changed.connect(status_changed);


        var background = config.get_string_with_default("Now Playing", "background-color", "#000");
        var foreground = config.get_string_with_default("Now Playing", "foreground-color", "#FFF");
        Gdk.Color.parse(background,out this.background);
        Gdk.Color.parse(foreground,out this.foreground);

        easy_command.add_entry(
                _("switch metadata"),
                "",
                _("Switch to Metadata view"),
                (Gmpc.Easy.Command.Callback *)show_metadata_search,
                this);
    }

    private const int[] version =  {0,0,0};
    public override  unowned int[] get_version() {
        return version;
    }

    public override unowned string get_name() {
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
                unowned int[] indices  = path.get_indices();
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

    private Gtk.EventBox header = null;

    /**
     * Makes gmpc jump to the metadata browser
     */
    private void select_metadata_browser(Gtk.Widget widget)
    {
        this.select_browser(null);
    }
    /**
     * Makes gmpc jump to the metadata browser
     * and search
     */
    private void select_metadata_browser_search(Gtk.Widget widget)
    {
        select_browser(null);
        set_base();
    }
    /**
     * Gmpc.Plugin.BrowserIface.add_go_menu
     */
    private int browser_add_go_menu(Gtk.Menu menu)
    {
        if(this.get_enabled())
        {
            var item = new Gtk.ImageMenuItem.with_mnemonic(_(this.get_name()));
            item.set_image(new Gtk.Image.from_icon_name("gmpc-metabrowser", Gtk.IconSize.MENU));
            item.activate.connect(select_metadata_browser);
            item.add_accelerator("activate", menu.get_accel_group(),0xffc1,0, Gtk.AccelFlags.VISIBLE);
            menu.append(item);

            item = new Gtk.ImageMenuItem.with_mnemonic(_("Search metadata"));
            item.set_image(new Gtk.Image.from_stock("gtk-find", Gtk.IconSize.MENU));
            item.activate.connect(select_metadata_browser_search);
            item.add_accelerator("activate", menu.get_accel_group(),
                                    0x04d,Gdk.ModifierType.CONTROL_MASK,
                                    Gtk.AccelFlags.VISIBLE);
            menu.append(item);

            return 2;
        }
        return 0;
    }

    /**
     * This builds the browser
     */
     private void browser_bg_style_changed(Gtk.Widget bg,Gtk.Style? style)
     {
         this.metadata_box.modify_bg(Gtk.StateType.NORMAL,this.metadata_sw.style.base[Gtk.StateType.NORMAL]);

         debug("Change style signal");
         if(this.theme_colors) {
             this.title_color = this.paned.style.text[Gtk.StateType.PRELIGHT].to_string();
             this.item_color = this.paned.style.text[Gtk.StateType.PRELIGHT].to_string();
         }
         this.change_color_style(this.metadata_sw);
     }
    /* This hack makes clicking a selected row again, unselect it */
     private bool browser_button_press_event(Gtk.Widget treel, Gdk.EventButton event)
     {
        var tree = (treel as Gtk.TreeView);
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
     private void browser_artist_entry_changed(Gtk.Editable entry)
     {
        string text = (entry as Gtk.Entry).get_text();
        if(text.length > 0) {
            (entry as Gtk.Widget).show();
            (entry as Gtk.Widget).grab_focus();
        }else{
            (entry as Gtk.Widget).hide();
            this.tree_artist.grab_focus();
        }
        this.model_filter_artist.refilter();
     }
     /* Handle right mouse click */
    private bool artist_browser_button_release_event(Gtk.Widget treel, Gdk.EventButton event)
    {
        var tree = (treel as Gtk.TreeView);
        if(event.button == 3) {
            if(tree.get_selection().count_selected_rows()>0)
            {
                var menu = new Gtk.Menu();
                var item = new Gtk.ImageMenuItem.from_stock("gtk-add",null);
                item.activate.connect((source) => {
                    Gtk.TreeModel model;
                    Gtk.TreeIter iter;
                    if(tree.get_selection().get_selected(out model, out iter)) {
                        string artist;
                        model.get(iter,7, out artist);
                        if(artist != null) {
                            MPD.Database.search_start(server,true);
                            MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
                            MPD.Data.Item data = MPD.Database.search_commit(server);
                            data.sort_album_disc_track();
                            if(data != null)
                            {
                                    data.first();
                                    do{
                                        MPD.PlayQueue.queue_add_song(server, data.song.file);
                                        data.next_free();
                                    }while(data != null);
                                    MPD.PlayQueue.queue_commit(server);
                            }
                        }
                    }
                });
                menu.append(item);

                item = new Gtk.ImageMenuItem.from_stock("gtk-media-play",null);
                item.activate.connect((source) => {
                    Gtk.TreeModel model;
                    Gtk.TreeIter iter;
                    if(tree.get_selection().get_selected(out model, out iter)) {
                        string artist;
                        model.get(iter,7, out artist);
                        if(artist != null) {
                            MPD.PlayQueue.clear(server);
                            MPD.PlayQueue.add_artist(server, artist);
                            MPD.Player.play(server);
                        }
                    }
                });
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
         if (str != null && str.casefold().normalize().index_of(text.casefold().normalize()) >= 0)
             visible = true;

         return visible;
     }
     private bool browser_artist_key_press_event(Gtk.Widget widget, Gdk.EventKey event)
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
     private void album_add_clicked(Gtk.Widget item )
     {
        string artist = browser_get_selected_artist();
        if(artist != null)
        {
            string albumartist = null;
            string album = browser_get_selected_album();
            if(album != null && Gmpc.server.tag_supported(MPD.Tag.Type.ALBUM_ARTIST))
            {
                MPD.Database.search_field_start(server, MPD.Tag.Type.ALBUM_ARTIST);
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
                var ydata = MPD.Database.search_commit(server);
                if(ydata != null)
                {
                    if(ydata.tag.length > 0)
                        albumartist = ydata.tag;
                }
            }
            /* Fill in the first browser */
            MPD.Database.search_start(server,true);
            if(albumartist != null && albumartist.length > 0)
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM_ARTIST, albumartist);
            else
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);

            if(album != null)
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
            var data = MPD.Database.search_commit(server);

            data.sort_album_disc_track();
            if(data != null)
            {
                do{
                    MPD.PlayQueue.queue_add_song(server, data.song.file);
                    data.next_free();
                }while(data != null);
                MPD.PlayQueue.queue_commit(server);
            }
        }
     }
     private void album_replace_clicked(Gtk.Widget item)
     {
         MPD.PlayQueue.clear(server);
         album_add_clicked(item);
         MPD.Player.play(server);
     }
    /* Handle right mouse click */
    private bool album_browser_button_release_event(Gtk.Widget tree, Gdk.EventButton event)
    {
        if(event.button == 3) {
            if((tree as Gtk.TreeView).get_selection().count_selected_rows()>0)
            {
                var menu = new Gtk.Menu();
                var item = new Gtk.ImageMenuItem.from_stock("gtk-add",null);
                item.activate.connect(album_add_clicked);
                menu.append(item);

                item = new Gtk.ImageMenuItem.from_stock("gtk-media-play",null);
                item.activate.connect(album_replace_clicked);
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
         if (str != null && str.casefold().normalize().index_of(text.casefold().normalize()) >= 0)
             visible = true;

         return visible;
     }


     private bool browser_album_key_press_event(Gtk.Widget widget, Gdk.EventKey event)
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

     private void browser_album_entry_changed(Gtk.Editable pentry)
     {
        Gtk.Entry entry = (pentry as Gtk.Entry);
        string text = entry.get_text();
        if(text.length > 0) {
            entry.show();
            entry.grab_focus();
        }else{
            entry.hide();
            this.tree_album.grab_focus();
        }
        this.model_filter_album.refilter();
     }
     /**
      * Songs
      */
     private void song_add_clicked(Gtk.Widget item )
     {
        unowned MPD.Song? song = browser_get_selected_song();
        if(song != null)
        {
            MPD.PlayQueue.add_song(server,song.file);
        }
     }
     private void song_replace_clicked(Gtk.Widget item)
     {
         MPD.PlayQueue.clear(server);
         song_add_clicked(item);
         MPD.Player.play(server);
     }
     /* Handle right mouse click */
    private bool song_browser_button_release_event(Gtk.Widget ptree, Gdk.EventButton event)
    {
        Gtk.TreeView tree = (ptree as Gtk.TreeView);
        if(event.button == 3) {
            if(tree.get_selection().count_selected_rows()>0)
            {
                var menu = new Gtk.Menu();
                var item = new Gtk.ImageMenuItem.from_stock("gtk-add",null);
                item.activate.connect(song_add_clicked);
                menu.append(item);

                item = new Gtk.ImageMenuItem.from_stock("gtk-media-play",null);
                item.activate.connect(song_replace_clicked);
                menu.append(item);

                menu.popup(null, null, null, event.button, event.time);
                menu.show_all();
                return true;
            }
        }
        return false;
    }

    private bool browser_button_release_event(Gtk.Widget widget, Gdk.EventButton event)
    {
        if(event.button == 8) {
            history_previous();
            return true;
        }
        else if (event.button == 9) {
            history_next();
            return true;
        }
        return false;
    }
    private void browser_init()
    {
        if(this.paned == null)
        {
            this.paned = new Gtk.HPaned();
            paned_size_group.add_paned(this.paned);
            this.paned.style_set.connect(browser_bg_style_changed);
            /* Bow with browsers */
            this.browser_box = new Gtk.VBox(true, 6);
            this.paned.add1(this.browser_box);


            /* Artist list  */
            var box = new Gtk.VBox(false, 6);
            this.browser_box.pack_start(box, true, true, 0);

            this.artist_filter_entry = new Gtk.Entry();

            this.artist_filter_entry.set_icon_from_stock(Gtk.EntryIconPosition.SECONDARY, "gtk-clear");
            this.artist_filter_entry.icon_press.connect((source, pos, event)=>{
                if(pos == Gtk.EntryIconPosition.SECONDARY) source.set_text("");
            });

            this.artist_filter_entry.set_no_show_all(true);
            this.artist_filter_entry.changed.connect(browser_artist_entry_changed);

            box.pack_start(this.artist_filter_entry, false, false, 0);

            var sw = new Gtk.ScrolledWindow(null, null);
            sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
            sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN);
            box.pack_start(sw, true, true, 0);

            this.model_artist = new Gmpc.MpdData.Model();
            this.model_filter_artist = new Gtk.TreeModelFilter(this.model_artist, null);
            this.model_filter_artist.set_visible_func(visible_func_artist);
            this.tree_artist = new Gtk.TreeView.with_model(this.model_filter_artist);
            this.tree_artist.rules_hint = true;
            new Gmpc.MpdData.Treeview.Tooltip(this.tree_artist, Gmpc.MetaData.Type.ARTIST_ART);

            this.tree_artist.set_enable_search(false);
            this.tree_artist.button_press_event.connect(browser_button_press_event);
            this.tree_artist.button_release_event.connect(artist_browser_button_release_event);
            this.tree_artist.key_press_event.connect(browser_artist_key_press_event);
            sw.add(tree_artist);
            /* setup the columns */
            var column = new Gtk.TreeViewColumn();
            if(config.get_int_with_default("tag2-plugin", "show-image-column", 1) == 1)
            {
                var prenderer = new Gtk.CellRendererPixbuf();
                prenderer.set("height", this.model_artist.icon_size);
                column.pack_start(prenderer, false);
                column.add_attribute(prenderer, "pixbuf",27);
            }
            var trenderer = new Gtk.CellRendererText();
            column.pack_start(trenderer, true);
            column.add_attribute(trenderer, "text", 7);
            this.tree_artist.append_column(column);
            column.set_title(_("Artist"));
            this.tree_artist.get_selection().changed.connect(browser_artist_changed);

            /* set fixed height mode */
            column.sizing = Gtk.TreeViewColumnSizing.FIXED;
            this.tree_artist.set_fixed_height_mode(true);


            /* Album list */

            box = new Gtk.VBox(false, 6);
            this.browser_box.pack_start(box, true, true, 0);


            this.album_filter_entry = new Gtk.Entry();

            this.album_filter_entry.set_icon_from_stock(Gtk.EntryIconPosition.SECONDARY, "gtk-clear");
            this.album_filter_entry.icon_press.connect((source, pos, event)=>{
                if(pos == Gtk.EntryIconPosition.SECONDARY) source.set_text("");
            });
            this.album_filter_entry.set_no_show_all(true);
            this.album_filter_entry.changed.connect(browser_album_entry_changed);
            box.pack_start(this.album_filter_entry, false, false, 0);

            sw = new Gtk.ScrolledWindow(null, null);
            sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
            sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN);
            box.pack_start(sw, true, true, 0);
            this.model_albums = new Gmpc.MpdData.Model();
            this.model_filter_album = new Gtk.TreeModelFilter(this.model_albums, null);
            this.model_filter_album.set_visible_func(visible_func_album);
            this.tree_album = new Gtk.TreeView.with_model(this.model_filter_album);
            this.tree_album.rules_hint = true;
            this.tree_album.set_enable_search(false);
            new Gmpc.MpdData.Treeview.Tooltip(this.tree_album, Gmpc.MetaData.Type.ALBUM_ART);

            this.tree_album.button_press_event.connect(browser_button_press_event);
            this.tree_album.button_release_event.connect(album_browser_button_release_event);
            this.tree_album.key_press_event.connect(browser_album_key_press_event);
            sw.add(tree_album);
            /* setup the columns */
            column = new Gtk.TreeViewColumn();
            if(config.get_int_with_default("tag2-plugin", "show-image-column", 1) == 1)
            {
                var prenderer = new Gtk.CellRendererPixbuf();
                prenderer.set("height", this.model_albums.icon_size);
                column.pack_start(prenderer, false);
                column.add_attribute(prenderer, "pixbuf",27);
            }
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
            column.set_title(_("Album"));


            this.tree_album.get_selection().changed.connect(browser_album_changed);

            /* Song list */
            sw = new Gtk.ScrolledWindow(null, null);
            sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
            sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN);
            this.browser_box.pack_start(sw, true, true, 0);
            this.model_songs = new Gmpc.MpdData.Model();
            this.tree_songs = new Gtk.TreeView.with_model(this.model_songs);
            this.tree_songs.rules_hint = true;
            this.tree_songs.button_press_event.connect(browser_button_press_event);
            this.tree_songs.button_release_event.connect(song_browser_button_release_event);
            sw.add(tree_songs);
            /* setup the columns */
            column = new Gtk.TreeViewColumn();

            if(config.get_int_with_default("tag2-plugin", "show-image-column", 1) == 1)
            {
                var prenderer = new Gtk.CellRendererPixbuf();
                column.pack_start(prenderer, false);
                column.add_attribute(prenderer, "icon-name",23);
            }
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

            this.tree_songs.get_selection().changed.connect(browser_songs_changed);

            /* The right view */
            this.metadata_sw = new Gtk.ScrolledWindow(null, null);
            this.metadata_sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
            this.metadata_sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN);
            //            this.metadata_sw.style_set += browser_bg_style_changed;
            this.metadata_box = new Gtk.EventBox();
            this.metadata_box.set_visible_window(true);
            this.metadata_sw.add_with_viewport(this.metadata_box);
            (this.metadata_box as Gtk.Container).set_focus_hadjustment(
                this.metadata_sw.get_hadjustment());
            (this.metadata_box as Gtk.Container).set_focus_vadjustment(
                this.metadata_sw.get_vadjustment());

            /* header */
            var vb = new Gtk.VBox(false, 0);
            vb.pack_end(this.metadata_sw, true,true,0);

            this.header = new Gtk.EventBox();
            header.app_paintable = true;
            header.set_visible_window(true);
            header.expose_event.connect(Gmpc.Misc.misc_header_expose_event);

            vb.pack_start(header, false, false,0);
            header.show();

            this.paned.add2(vb);



            this.paned.button_release_event.connect(browser_button_release_event);

            this.reload_browsers();
        }
        this.paned.show_all();

        if(config.get_int_with_default("metadata browser", "show-browsers", 1) == 0)
        {
            this.browser_box.hide();
        }
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
        data.sort_album_disc_track();
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
    private unowned MPD.Song? browser_get_selected_song()
    {
        Gtk.TreeIter iter;
        var sel = this.tree_songs.get_selection();
        Gtk.TreeModel model;
        if(sel.get_selected(out model, out iter))
        {
            unowned MPD.Song? songs = null;
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
            data.sort_album_disc_track();

            this.model_albums.set_request_artist(artist);
            MPD.Data.Item list = null;
            unowned MPD.Data.Item iter = data.get_first();
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
                        unowned MPD.Data.Item yi = ydata.get_first();
                        while(list.song.date == null && yi != null)
                        {
                            if(yi.tag != null && yi.tag.length > 0) {
                                list.song.date = yi.tag;
                            }
                            yi.next(false);
                        }
                    }
                    iter.next(false);
                }while(iter!= null);
            }

            list.sort_album_disc_track();
            this.model_albums.set_mpd_data((owned)list);

            MPD.Database.search_start(server,true);
            MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
            data = MPD.Database.search_commit(server);
            data.sort_album_disc_track();
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

            if(album != null && Gmpc.server.tag_supported(MPD.Tag.Type.ALBUM_ARTIST))
            {
                MPD.Database.search_field_start(server, MPD.Tag.Type.ALBUM_ARTIST);
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
                var ydata = MPD.Database.search_commit(server);
                if(ydata != null)
                {
                    if(ydata.tag.length > 0)
                        albumartist = ydata.tag;
                }
            }
            /* Fill in the first browser */
            MPD.Database.search_start(server,true);
            if(albumartist != null&& albumartist.length > 0)
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM_ARTIST, albumartist);
            else
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);

            if(album != null)
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
            var data = MPD.Database.search_commit(server);
            data.sort_album_disc_track();
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

     private void add_selected_song(Gtk.Button button)
     {
        string artist = browser_get_selected_artist();
        string album = browser_get_selected_album();
        unowned MPD.Song? song = browser_get_selected_song();
        if(song != null){
            MPD.PlayQueue.add_song(server,song.file);
            return;
        }
        if(artist != null ) {
            /* Hunt albumartist */
            string albumartist = null;
            if(album != null&& Gmpc.server.tag_supported(MPD.Tag.Type.ALBUM_ARTIST))
            {
                MPD.Database.search_field_start(server, MPD.Tag.Type.ALBUM_ARTIST);
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
                var ydata = MPD.Database.search_commit(server);
                if(ydata != null)
                {
                    if(ydata.tag.length > 0)
                        albumartist = ydata.tag;
                }
            }

            MPD.Database.search_start(server,true);//server,MPD.Tag.Type.FILENAME);
            if(albumartist != null&& albumartist.length > 0)
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM_ARTIST, albumartist);
            else
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
            if(album != null)
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
            var data = MPD.Database.search_commit(server);
            if(data != null) {
                data.sort_album_disc_track();
                while(data != null){
                    MPD.PlayQueue.queue_add_song(server, data.song.file);
                    data.next_free();
                }
                MPD.PlayQueue.queue_commit(server);
            }
        }
     }

     private void play_selected_song(Gtk.Button button)
     {
        unowned MPD.Song? song = browser_get_selected_song();
        if(song != null) {
            Gmpc.MpdInteraction.play_path(song.file);
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

     private void add_entry(Gtk.Table table, string entry_label, string? value,Gtk.Widget? extra,  ref int i, string ? image = null)
     {
         int j=0;
         if(value == null && extra == null) return;
         var box = new Gtk.HBox(false, 6);
         var label = new Gtk.Label("");
         label.set_alignment(0.0f, 0.0f);
         label.set_markup(Markup.printf_escaped("<b>%s:</b>",entry_label));
         if(image != null)
         {
            /* Make sure the icon always lines out to the top */
             var ali = new Gtk.Alignment (0f, 0f, 0f, 0f);
             var wimage = new Gtk.Image.from_icon_name(image, Gtk.IconSize.MENU);
             ali.add(wimage);
             box.pack_start(ali, false, false, 0);
         }
         box.pack_start(label, true, true, 0);
         table.attach(box, j,(j+1),i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
         j++;

         if(value != null)
         {
             var pt_label = new Gtk.Label(value);
             pt_label.set_selectable(true);
             pt_label.set_alignment(0.0f, 0.0f);
             pt_label.set_line_wrap(true);
             table.attach(pt_label, j,(j+1),i,i+1,Gtk.AttachOptions.EXPAND|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
             j++;
         }
         if(extra != null)
         {
             table.attach(extra, j,(j+1),i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
             j++;
         }
         i++;
     }

    public Gtk.Widget metadata_box_show_song(MPD.Song tmp_song, bool show_controls)
    {
        var master_box = new Gtk.VBox (false, 6);
        var vbox = new Gtk.VBox (false,6);
		var song = tmp_song.copy();
        vbox.border_width = 8;

        history_bc_header(HitemType.SONG, song.artist, song.album, song.title);


        /* Start building the gui */
        /* Artist image */
        var hbox = new Gtk.HBox(false, 6);
        /* Album image */
        var ali = new Gtk.Alignment(0f,0f,0f,0f);
        int meta_size =  (int)(this.metadata_box.allocation.width*0.20);
        meta_size = int.min(meta_size, (int)(this.metadata_sw.allocation.height*0.30));
        meta_size = int.min(int.max(100, meta_size), 250);
        var album_image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ALBUM_ART, meta_size);
        album_image.set_scale_up(true);
        album_image.set_squared(false);
        ali.add(album_image);
        album_image.update_from_song(song);
        hbox.pack_start(ali, false, false, 0);

        /* Artist image */
        ali = new Gtk.Alignment(1f,0f,0f,0f);
        var artist_image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ARTIST_ART, meta_size);

        artist_image.set_no_cover_icon("no-artist");
        artist_image.set_loading_cover_icon("fetching-artist");
        artist_image.set_scale_up(true);
        artist_image.set_hide_on_na(true);
        artist_image.set_squared(false);
        artist_image.update_from_song(song);
        ali.add(artist_image);
        hbox.pack_end(ali, false, false, 0);


        /* Information box */
        var info_box = new Gtk.Table (4,2,false);
        info_box.set_row_spacings(3);
        info_box.set_col_spacings(8);
        int i=0;
        /* Title */
        if(song.title != null) {

            var box = new Gtk.HBox(false, 6);

	    if(config.get_int_with_default("Interface", "hide-favorites-icon",0) == 0){
		    var fav_button = new Gmpc.Favorites.Button();
		    fav_button.set_song(song);
		    ali = new Gtk.Alignment(0.0f, 0.5f,0f,0f);
		    ali.add(fav_button);
		    box.pack_start(ali, false, false, 0);
	    }
	    var label = new Gtk.Label(song.title);
            label.selectable = true;
            label.set_markup(GLib.Markup.printf_escaped("<span color='%s' size='%i' weight='bold'>%s</span>",
                        this.title_color,Pango.SCALE*20 ,song.title));
            label.set_ellipsize(Pango.EllipsizeMode.END);
            label.set_alignment(0.0f, 0.5f);
            box.pack_start(label, true, true, 0);
            //hist_box.pack_start(box, true, true, 0);
            //vbox.pack_start(box, true, true, 0);
            info_box.attach(box, 0,2, i, i+1,Gtk.AttachOptions.FILL,Gtk.AttachOptions.FILL,0,0);
            i++;
        }else if (song.name!= null) {
            var label = new Gtk.Label(song.name);
            label.selectable = true;
            label.set_markup(GLib.Markup.printf_escaped("<span color='%s' size='%i' weight='bold'>%s</span>",this.
                        title_color, Pango.SCALE*20, song.name));
            label.set_ellipsize(Pango.EllipsizeMode.END);
            label.set_alignment(0.0f, 0.5f);
            info_box.attach(label, 0,2, i, i+1,Gtk.AttachOptions.FILL,Gtk.AttachOptions.FILL,0,0);
            i++;
        }
        else if (song.file != null)
        {
            var filename = GLib.Path.get_basename(song.file);
            var label = new Gtk.Label(song.name);
            label.selectable = true;
            try {
                var regex_a = new GLib.Regex ("\\.[0-9a-zA-Z]*$");
                filename = regex_a.replace_literal (filename, -1, 0, "");
            } catch (GLib.RegexError e) {
                GLib.error("%s", e.message);
            }
            try {
                var regex_b = new GLib.Regex ("_");
                filename = regex_b.replace_literal (filename, -1, 0, " ");
            } catch (GLib.RegexError e) {
                GLib.error("%s", e.message);
            }
            label.set_markup(GLib.Markup.printf_escaped("<span color='%s' size='%i' weight='bold'>%s</span>",this.
                        title_color, Pango.SCALE*20, filename));
            label.set_ellipsize(Pango.EllipsizeMode.END);
            label.set_alignment(0.0f, 0.5f);
            info_box.attach(label, 0,2, i, i+1,Gtk.AttachOptions.FILL,Gtk.AttachOptions.FILL,0,0);
            i++;
        }
        /* Artist */
        if(song.artist != null) {
            this.add_entry(info_box, _("Artist"), song.artist, null,ref i, "media-artist");
        }
        /* Album */
        if(song.album != null) {
            this.add_entry(info_box, _("Album"), song.album,null, ref i, "media-album");
        }
        /* Genre */
        if(song.genre != null) {
            this.add_entry(info_box, _("Genre"), song.genre,null, ref i, "media-genre");
        }
        /* Genre */
        if(song.date != null) {
            this.add_entry(info_box, _("Date"), song.date,null, ref i, "media-date");
        }
        if(song.file != null)
        {

            string extension = null;
            extension = get_extension(song.file);
            if(extension != null)
            {
                this.add_entry(info_box, _("Codec"), extension,null, ref i, "media-codec");
            }

            string directory = GLib.Path.get_dirname(song.file);
            if(directory != null)
            {
                this.add_entry(info_box, _("Directory"), directory, null, ref i, "gtk-open");
            }

        }
        /* Time*/
        if(song.time > 0) {
            this.add_entry(info_box, _("Length"),Gmpc.Misc.format_time((ulong) song.time, ""),null, ref i, "media-track-length");
        }

        if(song.track != null) {
            var label = new Gtk.Label("");
            label.selectable = true;
            label.set_ellipsize(Pango.EllipsizeMode.END);
            label.set_markup(GLib.Markup.printf_escaped("%s %s",
                        song.track,
                        (song.disc != null)? "[%s]".printf(song.disc):""
                        ));
            label.set_alignment(0.0f, 0.5f);
            this.add_entry(info_box, _("Track"),null,label, ref i, "media-num-tracks");
        }


       /* Player controls */
        var control_hbox = new Gtk.VBox (false, 6);

        var abutton = new Gtk.Button.from_stock("gtk-add");
        abutton.set_tooltip_text(_("Add the item to the play queue"));
        abutton.set_property("xalign", 0.0f);
        abutton.set_relief(Gtk.ReliefStyle.NONE);
        abutton.clicked.connect(add_selected_song);
        control_hbox.pack_start(abutton, false, false,0);

        abutton = new Gtk.Button.with_mnemonic(_("_Clear and play"));
        abutton.set_tooltip_text(_("Replace the current play queue with this item"));
        abutton.set_property("xalign", 0.0f);
        abutton.set_image(new Gtk.Image.from_stock("gtk-redo", Gtk.IconSize.BUTTON));
        abutton.set_relief(Gtk.ReliefStyle.NONE);
        abutton.clicked.connect(replace_selected_song);
        control_hbox.pack_start(abutton, false, false,0);

        abutton = new Gtk.Button.from_stock("gtk-media-play");
        abutton.set_tooltip_text(_("Play this song"));
        abutton.set_property("xalign", 0.0f);
        abutton.set_relief(Gtk.ReliefStyle.NONE);
        abutton.clicked.connect(play_selected_song);
        control_hbox.pack_start(abutton, false, false,0);

//        info_box.attach(control_hbox, 0,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        i++;


        hbox.pack_start(info_box, true, true, 0);
        hbox.pack_start(control_hbox, false, false, 0);
        vbox.pack_start(hbox, false, false, 0);


        var hboxje = new Gtk.HBox(false, 6);

        /* Notebook where all the metadata items are kept, Override the tabs by a radiobutton list. */
        var notebook = new Gtk.Notebook();
        notebook.set_show_border(false);
        notebook.set_show_tabs(false);

        /* Lyrics */
        i = 0;
        unowned SList<unowned Gtk.RadioButton> group  = null;
        if(config.get_int_with_default("MetaData", "show-lyrics",1) == 1)
        {
            var alib = new Gtk.Alignment(0f,0f,1f,0f);
            var text_view = new Gmpc.MetaData.TextView(Gmpc.MetaData.Type.SONG_TXT);
            text_view.set_left_margin(8);
            text_view.query_from_song(song);

            alib.add(text_view);
            notebook.append_page(alib, new Gtk.Label("Lyrics"));
            var button = new Gtk.RadioButton.with_label(group,("Lyrics"));
            group = button.get_group();
            hboxje.pack_start(button, false, false, 0);
            var j = i;
            button.clicked.connect((source) => {
                    debug("notebook page %i clicked", j);
                    notebook.set_current_page(j);
                    });
            i++;

            alib.show();
        }

        /* Guitar Tabs */
        if(config.get_int_with_default("MetaData", "show-guitar-tabs",1) == 1)
        {
            var alib = new Gtk.Alignment(0f,0f,1f,0f);
            var text_view = new Gmpc.MetaData.TextView(Gmpc.MetaData.Type.SONG_GUITAR_TAB);
            text_view.use_monospace = true;
            text_view.set_left_margin(8);
            var text_view_queried = false;

            alib.add(text_view);
            notebook.append_page(alib, new Gtk.Label(_("Guitar Tabs")));
            var button = new Gtk.RadioButton.with_label(group,_("Guitar Tabs"));
            group = button.get_group();
            hboxje.pack_start(button, false, false, 0);
            var j = i;
            /* Only query the guitar-tab when opened or first notebook page*/
            button.clicked.connect((source) => {
                    debug("notebook page %i clicked", j);
                    notebook.set_current_page(j);
                    if(!text_view_queried){
                    text_view.query_from_song(song);
                    text_view_queried = true;
                    this.change_color_style(text_view);
                    }
                    });
            if(i == 0){
                text_view.query_from_song(song);
                text_view_queried = true;
            }
            alib.show();
            i++;
        }
        /* Similar songs */

        if(config.get_int_with_default("MetaData", "show-similar-songs",1) == 1)
        {
            var similar_songs_queried = false;
            var similar_songs_box = new Gtk.Alignment(0f,0f,0f,0f);

            notebook.append_page(similar_songs_box, new Gtk.Label(_("Similar Songs")));

            var button = new Gtk.RadioButton.with_label(group,_("Similar Songs"));
            group = button.get_group();
            hboxje.pack_start(button, false, false, 0);

            var j = i;
            /* Only query when opened or first notebook page*/
            button.clicked.connect((source) => {
                    debug("notebook page %i clicked", j);
                    notebook.set_current_page(j);
                    if(!similar_songs_queried){
                    var similar_songs = new Gmpc.MetaData.Widgets.SimilarSongs(song);
                    similar_songs.update();
                    similar_songs_queried = true;
                    similar_songs_box.add(similar_songs);
                    this.change_color_style(similar_songs_box);
                    similar_songs_box.show_all();
                    }
                    });
            if(i == 0){
                var similar_songs = new Gmpc.MetaData.Widgets.SimilarSongs(song);
                similar_songs.update();
                similar_songs_queried = true;
                similar_songs_box.add(similar_songs);
                similar_songs_box.show_all();
            }
            similar_songs_box.show();
            i++;
        }

        if(config.get_int_with_default("MetaData", "show-similar-artist",1) == 1 && song.artist != null)
        {
            var similar_artist = new Gmpc.MetaData.Widgets.SimilarArtists(Gmpc.server,song);

            notebook.append_page(similar_artist, new Gtk.Label(_("Similar Artist")));

            var button = new Gtk.RadioButton.with_label(group,_("Similar Artist"));
            group = button.get_group();
            hboxje.pack_start(button, false, false, 0);

            var j = i;
            button.clicked.connect((source) => {
                    debug("notebook page %i clicked", j);
                    notebook.set_current_page(j);
                    similar_artist.first_show();
            });
            similar_artist.show();
            i++;
        }
        if(config.get_int_with_default("MetaData", "show-web-links",1) == 1)
        {

            var song_links = new Gmpc.MetaData.Widgets.SongLinks(Gmpc.MetaData.Widgets.SongLinks.Type.SONG,song);
            notebook.append_page(song_links, new Gtk.Label(_("Web Links")));
            var button = new Gtk.RadioButton.with_label(group,_("Web Links"));
            group = button.get_group();
            hboxje.pack_start(button, false, false, 0);
            var j = i;
            button.clicked.connect((source) => {
                    debug("notebook page %i clicked", j);
                    notebook.set_current_page(j);
                    });
            song_links.show();
            i++;
        }
        /* Track changed pages */
        notebook.notify["page"].connect((source,spec) => {
                var page = notebook.get_current_page();
                config.set_int("MetaData", "song-last-page", (int)page);

                });
        /* Restore right page */
        if(i > 0){
            var page = config.get_int_with_default("MetaData", "song-last-page", 0);
            if(page > i)
            {
                notebook.set_current_page(0);
            }else{
                /* The list is in reversed order, compensate for that. */
                var w = group.nth_data(i-page-1);
                w.set_active(true);
                notebook.set_current_page(page);
            }
        }

        ali = new Gtk.Alignment(0.0f, 0.5f,0f,0f);
        ali.add(hboxje);

        /* Create pane in 2. */
        var bottom_hbox = new Gtk.HBox(false, 6);
        /* left pane */

        var metadata_vbox = new Gtk.VBox(false, 6);

        var sep = new Gtk.HSeparator();
        sep.set_size_request(-1, 1);
        metadata_vbox.pack_start(sep, false, false, 0);

        metadata_vbox.pack_start(ali, false, false, 0);
        sep = new Gtk.HSeparator();
        sep.set_size_request(-1, 1);
        metadata_vbox.pack_start(sep, false, false, 0);
        metadata_vbox.pack_start(notebook, false, false, 0);

        bottom_hbox.pack_start(metadata_vbox, true, true, 0);

        vbox.pack_start(bottom_hbox, true, true, 0);

        /* show it */
        master_box.pack_start(vbox, false, false, 0);
        return master_box;
    }

    private void metadata_box_show_base()
    {
        Gtk.Entry MetadataBoxShowBaseEntry = null;
        var master_box = new Gtk.VBox (false, 6);
        var vbox = new Gtk.VBox (false,6);
        vbox.border_width = 8;


        history_bc_header(HitemType.BASE, null, null, null);

        var hbox = new Gtk.HBox(false, 6);
        var label = new Gtk.Label("");
        label.set_markup(_("<b>Search:</b>"));
        hbox.pack_start(label, false, false, 0);
        MetadataBoxShowBaseEntry = new Gtk.Entry();

        MetadataBoxShowBaseEntry.add_accelerator("grab-focus",
                            Gmpc.Playlist.get_accel_group(),
                            0x046,
                            Gdk.ModifierType.CONTROL_MASK,
                            0);

        /* Add clear icon */
        MetadataBoxShowBaseEntry.set_icon_from_stock(Gtk.EntryIconPosition.SECONDARY, "gtk-clear");
        MetadataBoxShowBaseEntry.icon_press.connect((source, pos, event) => {
            source.set_text("");
        });


        hbox.pack_start(MetadataBoxShowBaseEntry, true, true, 0);

        vbox.pack_start(hbox, false, false, 0);


        var result_hbox = new Gmpc.Widgets.Songlist();
        result_hbox.artist_song_clicked.connect((source, song) => {
                if(song.artist != null) {
                this.set_artist(song.artist);
                }
                });
        result_hbox.album_song_clicked.connect((source, song) => {
                if(song.artist != null && song.album != null) {
                this.set_album(song.artist, song.album);
                }
                });
        result_hbox.song_clicked.connect((source, song) => {
                this.set_song(song);

                });
        result_hbox.play_song_clicked.connect((source, song) => {
                if(song.file != null) {
                    Gmpc.MpdInteraction.play_path(song.file);
                }
            });

        MetadataBoxShowBaseEntry.activate.connect((source) => {
            string value = source.get_text();
            current.data.search_string = value;
            MPD.Data.Item? list = Gmpc.Query.search(value, false);
            result_hbox.set_from_data((owned)list,true,true);
            result_hbox.show_all();
        });
        result_hbox.show();
        vbox.pack_start(result_hbox, false, false);

        if(current != null) {
            if(current.data.type == HitemType.BASE  && current.data.search_string != null)
            {
                MetadataBoxShowBaseEntry.set_text(current.data.search_string);
                MetadataBoxShowBaseEntry.activate();
            }

        }
        /**
         * Button that allows you to enable/disable the side bars
         */
        var show_button = new Gtk.ToggleButton.with_label(_("Hide sidebar"));

        if(config.get_int_with_default("metadata browser", "show-browsers", 1) == 0)
        {
            show_button.label = _("Show sidebar");
            show_button.set_active(true);
        }
        show_button.toggled.connect((source) => {
            var state = show_button.active;
            if(state == true) {
                this.browser_box.hide();
                show_button.label = _("Show sidebar");
            }else {
                this.browser_box.show();
                show_button.label = _("Hide sidebar");
            }
            config.set_int("metadata browser", "show-browsers", (int)!state);

        });

        var ali = new Gtk.Alignment(0.0f, 0.5f, 0f,0f);
        ali.add(show_button);
        vbox.pack_start(ali, false, false);
        /**
         * Add it to the view
         */
        master_box.pack_start(vbox, false, false, 0);
        this.metadata_box.add(master_box);
        this.change_color_style(this.metadata_sw);
        this.metadata_sw.show_all();
    //    MetadataBoxShowBaseEntry.grab_focus();
    }



    private void metadata_box_show_album(string artist, string album)
    {
        var master_box = new Gtk.VBox (false, 6);
        var vbox = new Gtk.VBox (false,6);
        vbox.border_width = 8;


        history_bc_header(HitemType.ALBUM, artist, album, null);

        /* Artist image */
        var hbox = new Gtk.HBox(false, 6);
        var ali = new Gtk.Alignment(0f,0f,0f,0f);

        int meta_size =  (int)(this.metadata_box.allocation.width*0.20);
        meta_size = int.min(meta_size, (int)(this.metadata_sw.allocation.height*0.30));
        meta_size = int.min(int.max(100, meta_size), 250);
        var artist_image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ALBUM_ART, meta_size);
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
        hbox.pack_start(info_box, true, true, 0);
        int i=0;


        var label = new Gtk.Label(artist != null?artist:_("Unknown"));
        label.set_alignment(0.0f, 0.5f);
        label.set_line_wrap(true);
        this.add_entry(info_box, _("Artist"), null, label, ref i, "media-artist");

        label = new Gtk.Label(album != null?album:_("Unknown"));
        label.set_alignment(0.0f, 0.5f);
        label.set_line_wrap(true);
        this.add_entry(info_box, _("Album"), null, label, ref i, "media-album");

        /* Genres of songs */
        var pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ALBUM_GENRES_SONGS, song);
        pt_label.set_selectable(true);
        pt_label.set_alignment(0.0f, 0.5f);
        pt_label.set_line_wrap(true);
        this.add_entry(info_box, _("Genres"), null, pt_label, ref i, "media-genre");

        /* Dates of songs */
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ALBUM_DATES_SONGS, song);
        pt_label.set_selectable(true);
        pt_label.set_line_wrap(true);
        pt_label.set_alignment(0.0f, 0.5f);
        this.add_entry(info_box, _("Dates"), null, pt_label, ref i,"media-date");
        /* Total number of songs */
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ALBUM_NUM_SONGS, song);
        pt_label.set_selectable(true);
        pt_label.set_line_wrap(true);
        pt_label.set_alignment(0.0f, 0.5f);
        this.add_entry(info_box, _("Songs"), null, pt_label, ref i, "media-num-tracks");
        /* Total playtime */
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ALBUM_PLAYTIME_SONGS, song);
        pt_label.set_selectable(true);
        pt_label.set_line_wrap(true);
        pt_label.set_alignment(0.0f, 0.5f);
        this.add_entry(info_box, _("Playtime"), null, pt_label, ref i, "media-track-length");

        vbox.pack_start(hbox , false, false, 0);

        /* Player controls */
        var control_hbox = new Gtk.VBox (false, 6);

        var button = new Gtk.Button.from_stock("gtk-add");
        button.set_tooltip_text(_("Add the item to the play queue"));
        button.set_property("xalign", 0.0f);
        button.set_relief(Gtk.ReliefStyle.NONE);
        button.clicked.connect(add_selected_song);
        control_hbox.pack_start(button, false, false,0);

        button = new Gtk.Button.from_stock("gtk-media-play");
        button.set_tooltip_text(_("Replace the current play queue with this item and play"));
        button.set_property("xalign", 0.0f);
        button.set_relief(Gtk.ReliefStyle.NONE);
        button.clicked.connect(replace_selected_song);
        control_hbox.pack_start(button, false, false,0);

        hbox.pack_start(control_hbox, false, false, 0);

        /* Separator */
        var sep = new Gtk.HSeparator();
        sep.set_size_request(-1, 4);
        vbox.pack_start(sep, false, false, 0);

        var hboxje = new Gtk.HBox(false, 6);

        /* Notebook where all the metadata items are kept, Override the tabs by a radiobutton list. */
        var notebook = new Gtk.Notebook();
        notebook.set_show_border(false);
        notebook.set_show_tabs(false);

        /* Lyrics */
        i = 0;
        unowned SList<unowned Gtk.RadioButton> group  = null;
        /* Album information */
        if(config.get_int_with_default("MetaData", "show-album-information",1) == 1)
        {
            var alib = new Gtk.Alignment(0f,0f,1f,0f);
            var text_view = new Gmpc.MetaData.TextView(Gmpc.MetaData.Type.ALBUM_TXT);
            text_view.set_left_margin(8);
            text_view.query_from_song(song);

            alib.add(text_view);
            notebook.append_page(alib, new Gtk.Label(_("Album information")));
            var rbutton = new Gtk.RadioButton.with_label(group,_("Album information"));
            group = rbutton.get_group();
            hboxje.pack_start(rbutton, false, false, 0);
            var j = i;
            rbutton.clicked.connect((source) => {
                    debug("notebook page %i clicked", j);
                    notebook.set_current_page(j);
                    });
            i++;

            alib.show();
        }
        {
            var slhbox = new Gtk.VBox(false, 6);

            var sl = new Gmpc.Widgets.Songlist();
            slhbox.pack_start(sl, false, false, 0);

            sl.album_song_clicked.connect((source, song) => {
                if(song.artist != null && song.album != null) {
                    this.set_album(song.artist, song.album);
                }
            });
            sl.song_clicked.connect((source, song) => {
                this.set_song(song);

            });
            sl.play_song_clicked.connect((source, song) => {
                if(song.file != null) {
                    Gmpc.MpdInteraction.play_path(song.file);
                }
            });


            notebook.append_page(slhbox, new Gtk.Label(_("Song list")));
            var rbutton = new Gtk.RadioButton.with_label(group,_("Song list"));
            group = rbutton.get_group();
            hboxje.pack_start(rbutton, false, false, 0);
            var j = i;
            bool seen = false;
            rbutton.clicked.connect((source) => {
					notebook.set_current_page(j);
					});
			notebook.notify["page"].connect((source)=>
				{
					if(notebook.page != j) return;
                    debug("notebook page %i clicked", j);
                    if(!seen)
                    {
                        string albumartist = null;
                        if(Gmpc.server.tag_supported(MPD.Tag.Type.ALBUM_ARTIST))
                        {
                            MPD.Database.search_field_start(server, MPD.Tag.Type.ALBUM_ARTIST);
                            MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
                            MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
                            var ydata = MPD.Database.search_commit(server);
                            if(ydata != null)
                            {
                                if(ydata.tag.length > 0)
                                albumartist = ydata.tag;
                            }
                        }
                        MPD.Database.search_start(server,true);
                        if(albumartist != null&& albumartist.length > 0)
                            MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM_ARTIST, albumartist);
                        else
                            MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);

                        if(album != null)
                            MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
                        var data = MPD.Database.search_commit(server);
                        data.sort_album_disc_track();

                        sl.set_from_data((owned)data,false);

                        seen = true;
                    }
                    });
            slhbox.show();
            i++;
        }
        /* Show web links */
        if(config.get_int_with_default("MetaData", "show-web-links",1) == 1)
        {
            var song_links = new Gmpc.MetaData.Widgets.SongLinks(Gmpc.MetaData.Widgets.SongLinks.Type.ALBUM,song);
            notebook.append_page(song_links, new Gtk.Label(_("Web Links")));
            var rbutton = new Gtk.RadioButton.with_label(group,_("Web Links"));
            group = rbutton.get_group();
            hboxje.pack_start(rbutton, false, false, 0);
            var j = i;
            rbutton.clicked.connect((source) => {
                    debug("notebook page %i clicked", j);
                    notebook.set_current_page(j);
                    });
            song_links.show();
            i++;
        }
        /* Track changed pages */
        notebook.notify["page"].connect((source,spec) => {
                var page = notebook.get_current_page();
                config.set_int("MetaData", "album-last-page", (int)page);

                });
        /* Restore right page */
        if(i > 0){
            var page = config.get_int_with_default("MetaData", "album-last-page", 0);
            if(page > i)
            {
                notebook.set_current_page(0);
            }else{
                /* The list is in reversed order, compensate for that. */
                var w = group.nth_data(i-page-1);
                w.set_active(true);
                notebook.set_current_page(page);
            }
        }

        ali = new Gtk.Alignment(0.0f, 0.5f,0f,0f);
        ali.add(hboxje);

        /* Create pane in 2. */
        var bottom_hbox = new Gtk.HBox(false, 6);
        /* left pane */
        var metadata_vbox = new Gtk.VBox(false, 6);
        metadata_vbox.pack_start(ali, false, false, 0);
        sep = new Gtk.HSeparator();
        sep.set_size_request(-1, 1);
        metadata_vbox.pack_start(sep, false, false, 0);
        metadata_vbox.pack_start(notebook, false, false, 0);
        bottom_hbox.pack_start(metadata_vbox, true, true, 0);


        vbox.pack_start(bottom_hbox, true, true, 0);
        /**
         * Add it to the view
         */
        master_box.pack_start(vbox, false, false, 0);
        this.metadata_box.add(master_box);
        this.change_color_style(this.metadata_sw);
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
        var master_box = new Gtk.VBox (false, 6);
        var vbox = new Gtk.VBox (false,6);
        var i = 0;
        vbox.border_width = 8;

        /* Set up header */
        history_bc_header(HitemType.ARTIST, artist, null, null);

        /* Create an MPD.Song with the info for this type set */
        MPD.Song song = new MPD.Song();
        song.artist = artist;

        /* Artist image */
        var hbox = new Gtk.HBox(false, 6);
        var ali = new Gtk.Alignment(0f,0f,0f,0f);
        int meta_size =  (int)(this.metadata_box.allocation.width*0.20);
        meta_size = int.min(meta_size, (int)(this.metadata_sw.allocation.height*0.30));
        meta_size = int.min(int.max(100, meta_size), 250);
        var artist_image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ARTIST_ART, meta_size);

        artist_image.set_no_cover_icon("no-artist");
        artist_image.set_loading_cover_icon("fetching-artist");
        artist_image.set_squared(false);
        artist_image.update_from_song(song);
        ali.add(artist_image);
        hbox.pack_start(ali, false, false, 0);
        /* Artist information */
        var info_box = new Gtk.Table (4,2,false);
        info_box.set_row_spacings(3);
        info_box.set_col_spacings(8);
        hbox.pack_start(info_box, true, true, 0);

        /* Artist */
        var label = new Gtk.Label((artist != null)?artist:_("Unknown"));
        label.set_selectable(true);
        label.set_alignment(0.0f, 0.5f);
        this.add_entry(info_box, _("Artist"), null, label, ref i, "media-artist");

        /* Genres of songs */
        var pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ARTIST_GENRES_SONGS, song);
        pt_label.set_selectable(true);
        pt_label.set_alignment(0.0f, 0.5f);
        pt_label.set_line_wrap(true);
        this.add_entry(info_box, _("Genres"), null, pt_label, ref i, "media-genre");
        /* Dates of songs */
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ARTIST_DATES_SONGS, song);
        pt_label.set_selectable(true);
        pt_label.set_line_wrap(true);
        pt_label.set_alignment(0.0f, 0.5f);
        this.add_entry(info_box, _("Dates"), null, pt_label, ref i,"media-date");
        /* Total number of songs */
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ARTIST_NUM_SONGS, song);
        pt_label.set_selectable(true);
        pt_label.set_line_wrap(true);
        pt_label.set_alignment(0.0f, 0.5f);
        this.add_entry(info_box, _("Songs"), null, pt_label, ref i, "media-num-tracks");
        /* Total playtime */
        pt_label = new Gmpc.MetaData.StatsLabel(Gmpc.MetaData.StatsLabel.Type.ARTIST_PLAYTIME_SONGS, song);
        pt_label.set_selectable(true);
        pt_label.set_line_wrap(true);
        pt_label.set_alignment(0.0f, 0.5f);
        this.add_entry(info_box, _("Playtime"), null, pt_label, ref i, "media-track-length");

        vbox.pack_start(hbox , true, true, 0);

        /* Player controls */
        var hbox_2 = new Gtk.VBox (false, 6);

        var button = new Gtk.Button.from_stock("gtk-add");
        button.set_tooltip_text(_("Add the item to the play queue"));
        button.set_property("xalign", 0.0f);
        button.set_relief(Gtk.ReliefStyle.NONE);
        button.clicked.connect(add_selected_song);
        hbox_2.pack_start(button, false, false,0);

        button = new Gtk.Button.from_stock("gtk-media-play");
        button.set_tooltip_text(_("Replace the current play queue with this item and play"));
        button.set_property("xalign", 0.0f);
        button.set_relief(Gtk.ReliefStyle.NONE);
        button.clicked.connect(replace_selected_song);
        hbox_2.pack_start(button, false, false,0);

        hbox.pack_start(hbox_2, false, false, 0);


        var hboxje = new Gtk.HBox(false, 6);
        /* Separator */
        var sep = new Gtk.HSeparator();
        sep.set_size_request(-1, 4);
        vbox.pack_start(sep, false, false, 0);


        /* Notebook where all the metadata items are kept, Override the tabs by a radiobutton list. */
        var notebook = new Gtk.Notebook();
        notebook.set_show_border(false);
        notebook.set_show_tabs(false);

        i = 0;
        unowned SList<unowned Gtk.RadioButton> group  = null;
        /* Artist information */
        if(config.get_int_with_default("MetaData", "show-artist-information",1) == 1)
        {
            var alib = new Gtk.Alignment(0f,0f,1f,0f);
            var text_view = new Gmpc.MetaData.TextView(Gmpc.MetaData.Type.ARTIST_TXT);
            text_view.set_left_margin(8);
            text_view.query_from_song(song);

            alib.add(text_view);
            notebook.append_page(alib, new Gtk.Label(_("Artist information")));
            var button_sai = new Gtk.RadioButton.with_label(group,_("Artist information"));
            group = button_sai.get_group();
            hboxje.pack_start(button_sai, false, false, 0);

            var j = i;
            button_sai.clicked.connect((source) => {
                    if((source as Gtk.CheckButton).get_active()) {
                        debug("notebook page %i clicked", j);
                        notebook.set_current_page(j);
                    }
                    });
            i++;

            alib.show();
        }

        /* Show similar artist */
        if(config.get_int_with_default("MetaData", "show-similar-artist",1) == 1)
        {
            var similar_artist = new Gmpc.MetaData.Widgets.SimilarArtists(Gmpc.server,song);

            notebook.append_page(similar_artist, new Gtk.Label(_("Similar Artist")));

            var button_sa = new Gtk.RadioButton.with_label(group,_("Similar Artist"));
            group = button_sa.get_group();
            hboxje.pack_start(button_sa, false, false, 0);

            var j = i;
            button_sa.clicked.connect((source) => {
                    if((source as Gtk.CheckButton).get_active()) {
                        debug("notebook page %i clicked", j);
                        similar_artist.first_show();
                        notebook.set_current_page(j);
                    }
                    });
            similar_artist.show();
            i++;
        }

        {
            var slhbox = new Gtk.VBox(false, 6);

            var sl = new Gmpc.Widgets.Songlist();
            slhbox.pack_start(sl, false, false, 0);

            sl.album_song_clicked.connect((source, song) => {
                if(song.artist != null && song.album != null) {
                    this.set_album(song.artist, song.album);
                }
            });
            sl.song_clicked.connect((source, song) => {
                this.set_song(song);

            });
            sl.play_song_clicked.connect((source, song) => {
                if(song.file != null) {
                    Gmpc.MpdInteraction.play_path(song.file);
                }
            });


            notebook.append_page(slhbox, new Gtk.Label(_("Song list")));
            var rbutton = new Gtk.RadioButton.with_label(group,_("Song list"));
            group = rbutton.get_group();
            hboxje.pack_start(rbutton, false, false, 0);
            var j = i;
            bool seen = false;
			rbutton.clicked.connect((source) => {
					notebook.set_current_page(j);
					});
			notebook.notify["page"].connect((source)=>
					{
					if(notebook.page != j) return;
                    debug("notebook page %i clicked", j);
                    if(!seen)
                    {
                        MPD.Database.search_start(server,true);
                        MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
                        var data = MPD.Database.search_commit(server);
                        data.sort_album_disc_track();

                        sl.set_from_data((owned)data, true);

                        seen  = true;
                    }
                    });
            slhbox.show();
            i++;
        }
        /* Show web links */
        if(config.get_int_with_default("MetaData", "show-web-links",1) == 1)
        {
            var song_links = new Gmpc.MetaData.Widgets.SongLinks(Gmpc.MetaData.Widgets.SongLinks.Type.ARTIST,song);
            notebook.append_page(song_links, new Gtk.Label(_("Web Links")));
            var button_sl = new Gtk.RadioButton.with_label(group,_("Web Links"));
            group = button_sl.get_group();
            hboxje.pack_start(button_sl, false, false, 0);
            var j = i;
            button_sl.clicked.connect((source) => {
                    if((source as Gtk.CheckButton).get_active()) {
                        debug("notebook page %i clicked", j);
                        notebook.set_current_page(j);
                    }
            });
            song_links.show();
            i++;
        }

        if( song.artist != null)
        {
            int albums =0;
            /* Create album list */
            var album_hbox = new Gtk.VBox(false, 6);

            var album_view = new Gmpc.Widgets.Qtable();
            album_hbox.pack_start(album_view, false, true, 0);
            /**
             * Reuse the album browser view.
             */
            Gtk.TreeIter titer;
            if(this.model_albums.get_iter_first(out titer))
            {
                do{
                    unowned MPD.Song? asong  = null;
                    this.model_albums.get(titer,0, &asong);
                    if(asong != null)
                    {
                        /* Make a copy of the song. Otherwise only a reference is added to the
                         * button clicked handler.
                         * This reference can be invalid before click completed.
                         */
                        MPD.Song but_song = asong.copy();

                        /* Create button */
                        var but_hbox = new Gtk.VBox(false, 6);
                        var image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ALBUM_ART, 175);
                        image.set_squared(true);
						image.set_scale_up(true);
						image.set_has_tooltip(false);
                        image.update_from_song_delayed(but_song);

                        but_hbox.pack_start(image, false, false, 0);

                        var but_label = new Gtk.Label(but_song.album);
                        but_label.selectable = true;
                        but_label.set_alignment(0.5f, 0.0f);
                        /* Create label */
                        var strlabel = "<b>";
                        if(but_song.date != null && but_song.date.length > 0) strlabel += GLib.Markup.printf_escaped("%s - ",but_song.date);
                        if(but_song.album != null) strlabel+=GLib.Markup.escape_text(but_song.album);
                        else strlabel += _("No Album");
						strlabel += "</b>";
						if(but_song.artist != null) strlabel += GLib.Markup.printf_escaped("\n%s",but_song.artist);
                        but_label.set_markup(strlabel);
						(but_label as Gtk.Misc).set_alignment(0.0f, 0.0f);
                        but_label.set_ellipsize(Pango.EllipsizeMode.END);
                        /* add label */
                        but_hbox.pack_start(but_label, true, true, 0);

                        /* If clicked switch to browser */
                        image.button_release_event.connect((source, event) => {
							if(event.button == 2) {
								set_album(but_song.artist, but_song.album);
								return true;
							}else if (event.button == 1) {
                                var p = new PopupMenu();

                                hbox = new Gtk.HBox(false, 0);
                                var open = new Gtk.Button();
                                open.set_tooltip_text(_("Open"));
                                open.add(new Image.from_stock("gtk-open", Gtk.IconSize.DND));
                                hbox.pack_start(open, false, false, 0);
                                open.clicked.connect( (source) => {
                                    set_album(but_song.artist, but_song.album);
                                    p.destroy();
                                });

                                /**
                                 * Play machine
                                 */
                                var play = new Button();
                                play.add(new Image.from_stock("gtk-media-play", Gtk.IconSize.DND));
                                play.set_tooltip_text(_("Play album"));
                                hbox.pack_start(play, false, false, 0);
                                play.clicked.connect( (source) => {
                                    MPD.PlayQueue.clear(server);
                                    MPD.PlayQueue.add_album(server,
                                                            but_song.artist,
                                                            but_song.album);
                                    MPD.Player.play(server);
                                    p.destroy();
                                });

                                /**
                                 * Add to play queue
                                 */
                                var add = new Button();
                                add.add(new Image.from_stock("gtk-add", Gtk.IconSize.DND));
                                add.set_tooltip_text(_("Add album to play-queue"));
                                hbox.pack_start(add, false, false, 0);
                                add.clicked.connect( (source) => {
                                    MPD.PlayQueue.add_album(server,
                                                            but_song.artist,
                                                            but_song.album);
                                    p.destroy();
                                });

                                p.add(hbox);
                                hbox.show_all();
                                p.popup(event);
                                return true;
                            }
							return false;
						});
						albums++;


                        album_view.add(but_hbox);
                    }
                }while(this.model_albums.iter_next(ref titer));
            }

            notebook.append_page(album_hbox, new Gtk.Label(_("Albums")));
            var button_sl = new Gtk.RadioButton.with_label(group,_("Albums"));
            group = button_sl.get_group();
            hboxje.pack_start(button_sl, false, false, 0);
            var j = i;
            button_sl.clicked.connect((source) => {
                    if((source as Gtk.CheckButton).get_active()) {
                        debug("notebook page %i clicked", j);
                        notebook.set_current_page(j);
                    }
            });
            album_hbox.show();
            i++;
        }

        /* Track changed pages */
        notebook.notify["page"].connect((source,spec) => {
                var page = notebook.get_current_page();
                config.set_int("MetaData", "artist-last-page", (int)page);

                });
        /* Restore right page */
        if(i > 0){
            var page = config.get_int_with_default("MetaData", "artist-last-page", 0);
            if(page > i)
            {
                notebook.set_current_page(0);
            }else{
                /* The list is in reversed order, compensate for that. */
                var w = group.nth_data(i-page-1);
                w.set_active(true);
                notebook.set_current_page(page);
            }
        }


        ali = new Gtk.Alignment(0.0f, 0.5f,0f,0f);
        ali.add(hboxje);

        /* left pane */
        var metadata_vbox = new Gtk.VBox(false, 6);
        metadata_vbox.pack_start(ali, false, false, 0);
        sep = new Gtk.HSeparator();
        sep.set_size_request(-1, 1);
        metadata_vbox.pack_start(sep, false, false, 0);
        metadata_vbox.pack_start(notebook, false, false, 0);


        vbox.pack_start(metadata_vbox, true, true, 0);


        /**
         * Add it to the view
         */

        master_box.pack_start(vbox, false, false, 0);
        this.metadata_box.add(master_box);
        this.change_color_style(this.metadata_sw);
        this.metadata_box.show_all();
    }


    private uint update_timeout = 0;
    /* If we get an update, wait for idle time, then do it.*/
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
        unowned MPD.Song? song = browser_get_selected_song();

        if(song != null) {
            /** Add item to history */
            var item = new Hitem();
            item.song = song.copy();
            item.type = HitemType.SONG;
            history_add(item);

            var view = metadata_box_show_song(song,true);
            this.metadata_box.add(view);
            this.change_color_style(this.metadata_sw);
            this.metadata_box.show_all();
        }else if(album != null && artist != null) {
            /** Add item to history */
            var item = new Hitem();
            item.song = new MPD.Song();
            item.song.artist =artist;
            item.song.album = album;
            item.type = HitemType.ALBUM;
            history_add(item);

            metadata_box_show_album(artist,album);
        }else if (artist != null) {
            /** Add item to history */
            var item = new Hitem();
            item.song = new MPD.Song();
            item.song.artist =artist;
            item.type = HitemType.ARTIST;
            history_add(item);

            metadata_box_show_artist(artist);
        }else {
            var item = new Hitem();
            item.search_string = null;
            item.type = HitemType.BASE;
            history_add(item);
            metadata_box_show_base();
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
        Gmpc.Browser.insert(out iter, 
Playlist.BrowserType.LIBRARY+config.get_int_with_default(this.get_name(), "position", 100)%Playlist.BrowserType.LIBRARY);
        store.set(iter, 0, this.id, 1, _(this.get_name()), 3, "gmpc-metabrowser");
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
        this.history_clear();

        this.reload_browsers();
        metadata_box_clear();
        metadata_box_update();
    }
    private
    void
    status_changed(Gmpc.Connection conn, MPD.Server server, MPD.Status.Changed what)
    {
        if(this.paned == null) return;
        if((what&MPD.Status.Changed.DATABASE) != 0)
        {
            this.reload_browsers();
            /* Restore old selection */
            if(this.current != null) {
                this.show_hitem(this.current.data);
            }
        }

    }
    /**
     * History
     */
     private enum HitemType {
        BASE,
        ARTIST,
        ALBUM,
        SONG
     }
    private class Hitem {
        public HitemType type;
        public MPD.Song? song;
        public string search_string ;
    }
    private List<Hitem?> history = null;
    private unowned List<Hitem?> current = null;
    private void show_hitem(Hitem hi)
    {
        switch(hi.type)
        {
            case HitemType.BASE:
                this.set_base();
                break;
            case HitemType.ARTIST:
                this.set_artist(hi.song.artist);
                break;
            case HitemType.ALBUM:
                this.set_album(hi.song.artist, hi.song.album);
                break;
            case HitemType.SONG:
                this.set_song(hi.song);
                break;
            default:
                metadata_box_clear();
                break;
        }
    }
    private void history_previous()
    {
        if(history == null || current == null){
         return;
        }
        if(current.next == null) {
            return;
        }
        current = current.next;
        if(current != null) show_hitem(current.data);
        else metadata_box_clear();

    }
    private void history_next()
    {
        if(history == null || current == null){
         return;
        }
        if(current.prev == null) {
            return;
        }
        current = current.prev;
        if(current != null) show_hitem(current.data);
        else metadata_box_clear();

    }
    private void history_show_list_clicked(Gtk.MenuItem item)
    {
        unowned List<Hitem?> a = (List<Hitem?>) item.get_data<List<Hitem?>>("current");
        if(a != null)
        {
            current = a;
            show_hitem(current.data);
        }
    }
    private void history_show_list()
    {
        var menu = new Gtk.Menu();
        unowned List<Hitem?> iter = history.last();
        while(iter!= null){
            unowned Hitem i = iter.data;
            string label = "";
            if(i.type == HitemType.ARTIST)
                label = i.song.artist;
            else if (i.type == HitemType.ALBUM)
                label = "%s - %s".printf(i.song.artist, i.song.album);
            else if (i.type == HitemType.SONG)
            {
                if(i.song.title != null)
                    label = i.song.title;
                else
                    label = _("Unknown");
            }else if (i.type == HitemType.BASE)
            {
                label = _("Metadata Browser");
                if(i.search_string != null) {
                    label += " ("+i.search_string+")";
                }
            }

            var item = new Gtk.CheckMenuItem.with_label(label);
            item.draw_as_radio = true;
            if(current != null && current == iter){
                item.set_active(true);
            }
            item.activate.connect(history_show_list_clicked);
            item.set_data("current", (void*)iter);
            menu.append(item);
            iter = iter.prev;
        }
        menu.show_all();
        menu.popup(null, null, null, 0, Gtk.get_current_event_time());

    }

    private void history_bc_header(HitemType type, string? artist, string? album, string? title)
    {
        foreach(var child in this.header.get_children()) {
            child.destroy();
        }
        /* Add the history buttons.
         * we use the history buttons hbox for adding our buttons */
        var hist_box = history_buttons();
        this.header.add(hist_box);

        /* The buttons are in a schrinking hbox.
         * The final label will be placed next to it, with ellipsizing on.
         */
        var bc_ali = new Gtk.Alignment(0f, 0.5f, 0f, 0f);
        var bc_box = new Gtk.HBox(false, 6);
        bc_ali.add(bc_box);
        hist_box.pack_start(bc_ali, false, false, 0);

        /* The 'BASE' button */
        if(artist == null && album == null && title==null)
        {
            var image = new Gtk.Image.from_icon_name("gmpc-metabrowser", Gtk.IconSize.MENU);
            var ali = new Gtk.Alignment(0.5f,0.5f, 0f, 0f);
            ali.set_padding(6,6,6,6);
            ali.add(image);
            bc_box.pack_start(ali, true, true, 0);
        }
        else
        {
            var image = new Gtk.Image.from_icon_name("gmpc-metabrowser", Gtk.IconSize.MENU);
            var button = new Gtk.Button();
            button.set_relief(Gtk.ReliefStyle.NONE);
            button.set_image(image);
            button.clicked.connect((source)=> { set_base();});
            bc_box.pack_start(button, true, true, 0);
        }
        /* If there is an artist, we can show more */
        if(artist != null) {
            if(type != HitemType.ARTIST)
            {
                var button = new Gtk.Button.with_label(artist);
                button.set_relief(Gtk.ReliefStyle.NONE);
                button.set_image(new Gtk.Image.from_icon_name("media-artist", Gtk.IconSize.MENU));
                button.clicked.connect((source)=>{
                        set_artist(artist);
                        });
                bc_box.pack_start(button, true, true, 0);
            }
            else
            {
                var label = new Gtk.Label(artist);
                var image = new Gtk.Image.from_icon_name("media-artist", Gtk.IconSize.MENU);
                label.set_ellipsize(Pango.EllipsizeMode.END);
                label.set_alignment(0.0f, 0.5f);
                hist_box.pack_start(image, false, false, 0);
                hist_box.pack_start(label, true, true, 0);
            }
            /* Album */
            if(album != null) {
                if(type == HitemType.ALBUM)
                {
                    var label = new Gtk.Label(album);
                    var image = new Gtk.Image.from_icon_name("media-album", Gtk.IconSize.MENU);
                    label.set_ellipsize(Pango.EllipsizeMode.END);
                    label.set_alignment(0.0f, 0.5f);
                    hist_box.pack_start(image, false, false, 0);
                    hist_box.pack_start(label, true, true, 0);
                }
                else
                {
                    var button = new Gtk.Button.with_label(album);
                    button.set_relief(Gtk.ReliefStyle.NONE);
                    button.set_image(new Gtk.Image.from_icon_name("media-album", Gtk.IconSize.MENU));
                    button.clicked.connect((source)=>{
                            set_album(artist, album);
                            });
                    bc_box.pack_start(button, true, true, 0);
                }
            }
        }
        if(title != null)
        {
            var label = new Gtk.Label(title);
            var image = new Gtk.Image.from_icon_name("media-audiofile", Gtk.IconSize.MENU);
            label.set_ellipsize(Pango.EllipsizeMode.END);
            label.set_alignment(0.0f, 0.5f);
            hist_box.pack_start(image, false, false, 0);
            hist_box.pack_start(label, true, true, 0);

        }
        this.header.show_all();
    }

    private Gtk.HBox history_buttons()
    {
        var box = new HBox(false, 6);
        if(history == null && current == null) return box;

        var next_but = new Gtk.Button.from_stock("gtk-go-forward");
        next_but.set_relief(Gtk.ReliefStyle.NONE);
        if(current == null || current.prev == null) next_but.sensitive = false;
        next_but.clicked.connect(history_next);
        box.pack_end(next_but, false, false, 0);
        if(current != null && (current.next != null || current.prev != null))
        {
            var dd_but = new Gtk.Button();
            var arrow = new Gtk.Arrow(Gtk.ArrowType.DOWN, Gtk.ShadowType.ETCHED_IN);
            dd_but.add(arrow);
            dd_but.set_relief(Gtk.ReliefStyle.NONE);
            dd_but.clicked.connect(history_show_list);
            box.pack_end(dd_but, false, false, 0);
        }
        var back_but = new Gtk.Button.from_stock("gtk-go-back");
        back_but.set_relief(Gtk.ReliefStyle.NONE);
        if(current == null || current.next == null) back_but.sensitive = false;
        back_but.clicked.connect(history_previous);
        box.pack_end(back_but, false, false, 0);


        return box;
    }
    private void history_add(Hitem hi)
    {
        if(history != null)
        {
            unowned Hitem a = current.data;
            if(a.type == hi.type) {
                if(Gmpc.Misc.song_checksum(a.song) == Gmpc.Misc.song_checksum(hi.song)){
                    return;
                }
            }
        }
        /* clean items in the future */
        if(history != null) {
            unowned List<Hitem?>? iter = current.prev;
            while(iter != null && iter != current) {
                history.remove(iter.data);
                iter = history.first();
            }
        }

        history.prepend(hi);
        if(history.length() > 25){
            unowned List<Hitem?> a = history.last();
            history.remove(a.data);
        }
        current = history;
    }
    private void history_clear()
    {
        this.current = null;
        this.history = null;

    }

    /**
     * Public api
     */
     public
     void
     set_base()

     {
        this.tree_artist.get_selection().unselect_all();
        this.artist_filter_entry.set_text("");

        this.metadata_box_clear();
        this.metadata_box_update();
     }
    public
    void
    set_artist(string artist)
    {
        if(!this.get_enabled()) return;
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
        if(!this.get_enabled()) return;
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
        if(!this.get_enabled()) return;
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

        /** Add item to history */
        var item = new Hitem();
        item.song = song.copy();
        item.type = HitemType.SONG;
        history_add(item);

        var view = metadata_box_show_song(song,true);
        this.metadata_box.add(view);
        this.change_color_style(this.metadata_sw);
        this.metadata_box.show_all();
    }
    public
    void
    select_browser(Gtk.TreeView? tree)
    {
        if(rref != null)
        {
            unowned Gtk.TreeView category_tree = Gmpc.Playlist3.get_category_tree_view();
            var sel = category_tree.get_selection();
            var path = rref.get_path();
            if(path != null){
                sel.select_path(path);
            }
        }
    }

     private void change_color_style(Gtk.Widget bg)
     {
         debug("change style");
         if(bg is Gtk.Separator || bg is Gtk.Notebook || bg is Gtk.CheckButton){
             /* Do nothing */
         }else{
             if(theme_colors)
             {
                 bg.modify_bg(Gtk.StateType.NORMAL,this.paned.style.base[Gtk.StateType.NORMAL]);
             }else{
                 bg.modify_bg(Gtk.StateType.NORMAL,this.background);
                 bg.modify_base(Gtk.StateType.NORMAL,this.background);
                 bg.modify_text(Gtk.StateType.NORMAL,this.foreground);
                 bg.modify_fg(Gtk.StateType.NORMAL,this.foreground);
                 bg.modify_text(Gtk.StateType.ACTIVE,this.foreground);
                 bg.modify_fg(Gtk.StateType.ACTIVE,this.foreground);
                 bg.modify_bg(Gtk.StateType.INSENSITIVE,this.background);
                 bg.modify_base(Gtk.StateType.INSENSITIVE,this.background);
             }
         }
         /* Recurse into children, if the widget can hold children (so is a container */
         if(bg is Gtk.Container){
             foreach(Gtk.Widget child in ((Gtk.Container)bg).get_children())
             {
                 change_color_style(child);
             }
         }
     }
     private string get_extension(string path)
     {
         long length = path.length;
         long i=length;
         string retv = null;
         for(;i>0 && (length-i) <8;i--){
             if(path[i] == '.') {
                 retv = path.substring(i+1);
                 return retv;
             }
         }
         return retv;
     }

     public override void set_enabled(bool state) {
	     if(state) {
		     if(rref == null) {
			     browser_add( Gmpc.Playlist3.get_category_tree_view());
			     browser_init();
		     }
	     }else {
		     if(this.rref != null) {
			     var path = rref.get_path();
			     if(path != null) {
				     unowned int[] indices  = path.get_indices();
				     config.set_int(this.get_name(), "position", indices[0]);
				     Gtk.ListStore model = (Gtk.ListStore)rref.get_model();
				     Gtk.TreeIter iter;
				     if(model.get_iter(out iter, path))
				     {
					     model.remove(iter);
				     }
			     }
			     rref = null;
		     }
	     }

	     if(this.get_name() != null)
		     Gmpc.config.set_int(this.get_name(), "enabled", (int)state);
     }
}
