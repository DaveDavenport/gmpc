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

public class  Gmpc.MetadataBrowser : Gmpc.Plugin.Base, Gmpc.Plugin.BrowserIface {

    construct {
        /* Set the plugin as an internal one and of type pl_browser */
        this.plugin_type = 2|8; 
    }

    public const int[3] version =  {0,0,0};
    public override  weak int[3] get_version()
    {
        return version;
    }

    public override weak string get_name()
    {
        return "MetaData Browser 2";
    }

    public override void save_yourself()
    {
        if(this.paned != null) {
            int pos = this.paned.get_position();
            config.set_int("Metadata Browser 2", "pane-pos", pos);
        }
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
    private Gtk.TreeView tree_album  = null;
    private Gmpc.MpdData.Model model_albums = null;
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
    private void browser_init()
    {
        if(this.paned == null)
        {
            this.paned = new Gtk.HPaned();
            this.paned.set_position(config.get_int_with_default("Metadata Browser 2", "pane-pos", 150));
            /* Bow with browsers */
            this.browser_box =  new Gtk.VBox(true, 6);
            this.paned.add1(this.browser_box);

            /* Artist list  */
            var sw = new Gtk.ScrolledWindow(null, null);
            sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
            this.browser_box.pack_start(sw, true, true, 0);
            this.model_artist = new Gmpc.MpdData.Model();
            this.tree_artist = new Gtk.TreeView.with_model(this.model_artist);
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
            sw = new Gtk.ScrolledWindow(null, null);
            sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
            this.browser_box.pack_start(sw, true, true, 0);
            this.model_albums = new Gmpc.MpdData.Model();
            this.tree_album = new Gtk.TreeView.with_model(this.model_albums);
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
            this.browser_box.pack_start(sw, true, true, 0);
            this.model_songs = new Gmpc.MpdData.Model();
            this.tree_songs = new Gtk.TreeView.with_model(this.model_songs);
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
        if(sel.get_selected(out this.model_artist, out iter))
        {
            string artist = null;
            this.model_artist.get(iter, 7,out artist, -1);
            return artist;
        }
        return null;
    }

    private string? browser_get_selected_album()
    {
        Gtk.TreeIter iter;
        var sel = this.tree_album.get_selection();
        if(sel.get_selected(out this.model_albums, out iter))
        {
            string album = null;
            this.model_albums.get(iter, 7,out album, -1);
            return album;
        }
        return null;
    }
    private string? browser_get_selected_song()
    {
        Gtk.TreeIter iter;
        var sel = this.tree_songs.get_selection();
        if(sel.get_selected(out this.model_songs, out iter))
        {
            string songs = null;
            this.model_songs .get(iter, 7,out songs, -1);
            return songs;
        }
        return null;
    }
    private void browser_artist_changed(Gtk.TreeSelection sel)
    {
        this.model_albums.set_mpd_data(null);
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

        }
        this.metadata_box_update();
    }
    private void browser_album_changed(Gtk.TreeSelection album_sel)
    {
        this.model_songs.set_mpd_data(null);
        this.metadata_box_clear();

        string album = browser_get_selected_album();
        if(album != null)
        {
            string artist = browser_get_selected_artist();
            if(artist != null){
                /* Fill in the first browser */
                MPD.Database.search_start(server,true);
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
                MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, album);
                var data = MPD.Database.search_commit(server);
                data = Gmpc.MpdData.sort_album_disc_track((owned)data);
                this.model_songs.set_mpd_data((owned)data);
            }
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
    private void metadata_box_clear()
    {
        var list = this.metadata_box.get_children();
        foreach(Gtk.Widget child in list){
            child.destroy();
        }
    }
    private void metadata_box_show_song(string artist, string album, string song_title)
    {
        var vbox = new Gtk.VBox (false,6);
        vbox.border_width = 8;
        var label = new Gtk.Label("");
        label.set_markup(Markup.printf_escaped("<span size='xx-large' weight='bold'>%s</span>",song_title));
        label.set_alignment(0.0f, 0.5f);
        vbox.pack_start(label, false, false, 0);

        /* Artist image */
        var hbox = new Gtk.HBox(false, 6);
        var ali = new Gtk.Alignment(0f,0f,0f,0f);
        var artist_image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ALBUM_ART, 250);
        artist_image.set_squared(false);
        MPD.Song song = new MPD.Song();
        song.title = song_title;
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

        var pt_label = new Gtk.Label(artist); 
        label = new Gtk.Label("");
        label.set_alignment(0.0f, 0.5f);
        label.set_markup(Markup.printf_escaped("<b>%s:</b>",_("Artist")));
        info_box.attach(label, 0,1,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        pt_label.set_alignment(0.0f, 0.5f);
        pt_label.set_line_wrap(true);
        info_box.attach(pt_label, 1,2,i,i+1,Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL, Gtk.AttachOptions.SHRINK|Gtk.AttachOptions.FILL,0,0);
        i++;


        vbox.pack_start(hbox , false, false, 0);

        /* Lyrics */
        var frame = new Gtk.Frame(null);
        label = new Gtk.Label("");
        label.set_markup(Markup.printf_escaped("<b>%s:</b>", _("Lyrics")));
        label.set_alignment(0.0f, 0.0f);
        frame.set_label_widget(label);
        frame.set_shadow_type(Gtk.ShadowType.ETCHED_IN);
        var text_view = new Gmpc.MetaData.TextView(Gmpc.MetaData.Type.SONG_TXT);
        text_view.query_from_song(song);
        ali = new Gtk.Alignment(0f,0f,1f,0f); ali.set_padding(6,6,12,12); ali.add(text_view); frame.add(ali);;
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

        var frame = new Gtk.Frame(null);
        label = new Gtk.Label("");
        label.set_markup(Markup.printf_escaped("<b>%s:</b>", _("Album information")));
        label.set_alignment(0.0f, 0.0f);
        frame.set_label_widget(label);
        frame.set_shadow_type(Gtk.ShadowType.ETCHED_IN);
        var text_view = new Gmpc.MetaData.TextView(Gmpc.MetaData.Type.ALBUM_TXT);
        text_view.query_from_song(song);
        ali = new Gtk.Alignment(0f,0f,1f,0f); ali.set_padding(6,6,12,12); ali.add(text_view); frame.add(ali);;
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

        var frame = new Gtk.Frame(null);
        label = new Gtk.Label("");
        label.set_markup(Markup.printf_escaped("<b>%s:</b>", _("Artist information")));
        label.set_alignment(0.0f, 0.0f);
        frame.set_label_widget(label);
        frame.set_shadow_type(Gtk.ShadowType.ETCHED_IN);
        var text_view = new Gmpc.MetaData.TextView(Gmpc.MetaData.Type.ARTIST_TXT);
        text_view.query_from_song(song);

        ali = new Gtk.Alignment(0f,0f,1f,0f); ali.set_padding(6,6,12,12); ali.add(text_view); frame.add(ali);;
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
        string song = browser_get_selected_song();

        if(album != null && artist != null && song != null) {
            metadata_box_show_song(artist,album,song);
        }else if(album != null && artist != null) {
            metadata_box_show_album(artist,album);
        }else if (artist != null) {
            metadata_box_show_artist(artist);
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
}
