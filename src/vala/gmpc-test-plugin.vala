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

private const bool use_transition = Gmpc.use_transition;

public class Gmpc.MetaData.EditWindow : Gtk.Window {
    private const string some_unique_name = Config.VERSION;
    private Gtk.ListStore model = null;
    private Gtk.TreeView tree = null;
    private GLib.List<weak Gmpc.AsyncDownload.Handle> downloads = null;

    private MPD.Song song = null;
    private Gmpc.MetaData.Type query_type = Gmpc.MetaData.Type.ALBUM_ART;

    /** fetching handles */
    private void *handle = null;
    private void *handle2 = null;

    private Gtk.HBox pbox = null;
    private Gtk.Label warning_label = null;
    private Gtk.Entry artist_entry;
    private Gtk.Entry album_entry;
    private Gtk.Entry title_entry;
    private Gtk.Button cancel = null;
    private Gtk.Button refresh = null;
    private Gtk.ComboBox combo = null;
    private Gtk.ProgressBar bar = null;
    
    private Gtk.TreeViewColumn column = null;

    construct {
        this.type = Gtk.WindowType.TOPLEVEL;
        this.set_default_size(650,800);
        this.set_border_width(8);
    }

    private void add_entry_image(string? provider, string uri,Gdk.PixbufFormat? format, Gdk.Pixbuf pb)
    {
        Gtk.TreeIter iter;
        string a;
        a = Markup.printf_escaped("<b>%s</b>: %s",_("Uri"),uri);
        if(provider != null) {
            a+= Markup.printf_escaped("\n<b>%s</b>:  %s",_("Provider"), provider);
        }
        if(format != null)
        {
            a+= Markup.printf_escaped("\n<b>%s</b>: %s",_("Filetype"), format.get_name());
        }
        if(pb != null)
        {
            a+="\n<b>%s</b>: %ix%i (%s)".printf(_("Size"), pb.width, pb.height,_("wxh"));

        }
        int new_h, new_w;
        if(pb.width < pb.height) {
            new_h = 150;
            new_w = (int)((150.0/(double)pb.height)*pb.width);
        }
        else
        {
            new_w = 150;
            new_h = (int)((150.0/(double)pb.width)*pb.height);
        }
        this.model.append(out iter);
        this.model.set(iter, 0, pb.scale_simple(new_w,new_h,Gdk.InterpType.BILINEAR),1, uri,2,a, -1);
    }

    private void add_entry_text(string? provider, string uri,string text) 
    {
        Gtk.TreeIter iter;
        string a;
        a = "<b>%s</b>: %s".printf(_("Uri"),uri);
        if(provider != null) {
            a+="\n<b>%s</b>:  %s".printf(_("Provider"), provider);
        }
        this.model.append(out iter);
        this.model.set(iter, 3,text,1, uri,2,a, -1);
    }
    public void image_downloaded(Gmpc.AsyncDownload.Handle handle, Gmpc.AsyncDownload.Status status)
    {
        if(status == Gmpc.AsyncDownload.Status.PROGRESS) return;
        this.downloads.remove(handle);
        this.bar.pulse();
        if(status == Gmpc.AsyncDownload.Status.DONE)
        {
            weak uchar[] data = handle.get_data();
            if(this.query_type == Gmpc.MetaData.Type.ALBUM_ART || this.query_type == Gmpc.MetaData.Type.ARTIST_ART)
            {
                try{
                    var load = new Gdk.PixbufLoader();
                    try {
                        load.write(data);
                    }catch (Error e) {
                        stdout.printf("Failed to load file: %s::%s\n",e.message,handle.get_uri());
                    }
                    load.close();

                    Gdk.Pixbuf pb = load.get_pixbuf();//new Gdk.Pixbuf.from_inline((int)length, (uchar[])data, true); 
                    if(pb!= null)
                        this.add_entry_image((string)handle.get_user_data(),handle.get_uri(),load.get_format(),pb);
                }catch (Error e) {
                    stdout.printf("Failed to load file: %s::%s\n",e.message,handle.get_uri());
                }
            }
            else
            {
                this.add_entry_text((string)handle.get_user_data(),handle.get_uri(),(string)data);
            }
        }
        
        if(this.handle == null && this.handle2 == null && this.downloads == null)
        {
            this.pbox.hide();
            this.refresh.sensitive = true;
            this.combo.sensitive = true;
        }

    }
    public void callback(void *handle,string? plugin_name,GLib.List<MetaData.Item>? list)
    {
        bar.pulse();
        if(list == null) {
            if(this.handle == handle)
            {
                this.handle = null;
                if(this.handle == null && this.downloads == null)
                {
                    this.pbox.hide();
                    this.refresh.sensitive = true;
                    this.combo.sensitive = true;
                }
            }
            if(this.handle2 == handle)
            {
                this.handle2 = null;


                if(this.handle == null && this.downloads == null)
                {
                    this.pbox.hide();
                    this.combo.sensitive = true;
                    this.refresh.sensitive = true;
                }
            }
        }
        foreach(weak MetaData.Item md in list)
        {

            if(this.query_type == Gmpc.MetaData.Type.ALBUM_ART || this.query_type == Gmpc.MetaData.Type.ARTIST_ART)
            {
                weak string uri = md.get_uri();
                if(md.content_type == Gmpc.MetaData.ContentType.URI)
                {
                    if(uri[0] == '/'){
                        try{
                            Gdk.Pixbuf pb = new Gdk.Pixbuf.from_file(uri);
                            if(pb != null)
                            {
                                int w,h;
                                add_entry_image(plugin_name, uri,Gdk.Pixbuf.get_file_info(uri,out w, out h), pb);
                                }
                        }catch(Error e)
                        {

                        }
                    }else{
                        var h =  Gmpc.AsyncDownload.download(uri, image_downloaded); 
                        if(h!=null)
                        {
                            h.set_user_data(md.plugin_name);
                            this.downloads.append(h);
                        } 
                        else stdout.printf("async download returned NULL\n");
                    }
                }
            }else{

                if(md.content_type == Gmpc.MetaData.ContentType.TEXT)
                {
                    weak string uri = md.get_text();
                    add_entry_text(plugin_name, "n/a", uri);
                }
                else 
                if(md.content_type == Gmpc.MetaData.ContentType.URI)
                {
                    weak string uri = md.get_uri();
                    if(uri[0] == '/'){
                        try {
                            string content;
                            if(GLib.FileUtils.get_contents(uri,out content))
                            {
                               add_entry_text(plugin_name,uri, content);
                            }
                        }catch (Error e) {

                        }
                    }

                }
            }

        }
    }

    public void store_image(Gmpc.AsyncDownload.Handle handle, Gmpc.AsyncDownload.Status status)
    {
        if(status == Gmpc.AsyncDownload.Status.PROGRESS){
            weak uchar[] data =handle.get_data();
            this.sensitive = false;
            this.pbox.show();
            int64 total_size = handle.get_content_size(); 
            if(data.length > 0 && total_size > 0){
                double progress = data.length/(double)total_size;
                this.bar.set_fraction(progress);
            }
            else this.bar.pulse();
            return;
        }
        this.downloads.remove(handle);
        if(status == Gmpc.AsyncDownload.Status.DONE)
        {
            weak uchar[] data = handle.get_data();
            var file = Gmpc.MetaData.get_metadata_filename(this.query_type, this.song,null); 
            try {
                GLib.FileUtils.set_contents(file, (string)data, (long)data.length);
                var met = new MetaData.Item();
                met.type = this.query_type;
                met.plugin_name = "User set";
                met.content_type = MetaData.ContentType.URI;
                met.set_uri(file);
                Gmpc.MetaData.set_metadata(this.song, Gmpc.MetaData.Result.AVAILABLE, met); 
    
                var met_false = new MetaData.Item();
                met_false.type = this.query_type;
                met_false.plugin_name = "User set";
                met_false.content_type = MetaData.ContentType.EMPTY;
                metawatcher.data_changed(this.song, this.query_type, Gmpc.MetaData.Result.UNAVAILABLE, met_false);  
                metawatcher.data_changed(this.song, this.query_type, Gmpc.MetaData.Result.AVAILABLE, met);  
            }catch (Error e) {

            }
        }
        this.pbox.hide();
        this.sensitive = true;

    }
    private void
    set_metadata(Gtk.Button button)
    {
        Gtk.TreeIter iter;
        var sel = this.tree.get_selection();
        string path = null;
        Gtk.TreeModel model;
        if(sel.get_selected(out model,out iter))
        {
            this.model.get(iter,1,out path);

            if(this.query_type == Gmpc.MetaData.Type.ALBUM_ART || this.query_type == Gmpc.MetaData.Type.ARTIST_ART)
            {
                if(path[0]  == '/')
                {
                    var met = new MetaData.Item();
                    met.type = this.query_type;
                    met.plugin_name = "User set";
                    met.content_type = MetaData.ContentType.URI;
                    met.set_uri(path);
                    Gmpc.MetaData.set_metadata(this.song, Gmpc.MetaData.Result.AVAILABLE, met); 

                    var met_false = new MetaData.Item();
                    met_false.type = this.query_type;
                    met_false.plugin_name = "User set";
                    met_false.content_type = MetaData.ContentType.EMPTY;
                    metawatcher.data_changed(this.song, this.query_type, Gmpc.MetaData.Result.UNAVAILABLE, met_false);  
                    metawatcher.data_changed(this.song, this.query_type, Gmpc.MetaData.Result.AVAILABLE, met);  
                }else{
                    var h = Gmpc.AsyncDownload.download(path, store_image); 
                    if(h!=null)
                        this.downloads.append(h);
                    else stdout.printf("async download returned NULL");
                }
            }else{
                string lyric;
                this.model.get(iter,3,out lyric);
                var file = Gmpc.MetaData.get_metadata_filename(this.query_type, this.song,null); 
                try {
                    stdout.printf("Storing into: %s\n", file);
                    GLib.FileUtils.set_contents(file,lyric, -1); 
                    var met = new MetaData.Item();
                    met.type = this.query_type;
                    met.plugin_name = "User set";
                    met.content_type = MetaData.ContentType.URI;
                    met.set_uri(file);
                    Gmpc.MetaData.set_metadata(this.song, Gmpc.MetaData.Result.AVAILABLE, met); 

                    var met_false = new MetaData.Item();
                    met_false.type = this.query_type;
                    met_false.plugin_name = "User set";
                    met_false.content_type = MetaData.ContentType.EMPTY;
                    metawatcher.data_changed(this.song, this.query_type, Gmpc.MetaData.Result.UNAVAILABLE, met_false);  
                    metawatcher.data_changed(this.song, this.query_type, Gmpc.MetaData.Result.AVAILABLE, met);  
                }catch (Error e) {

                }

            }

        }
    }
    public
    void
    destroy_popup(Gtk.Button button)
    {
        this.destroy();
    }

    public void refresh_query(Gtk.Button button) 
    {
        this.model.clear();
        MPD.Song ss = this.song;
        ss.artist = this.artist_entry.get_text();
        ss.album = this.album_entry.get_text();
        ss.title = this.title_entry.get_text();

        if(this.handle == null && this.handle2 == null) {
            this.handle = Gmpc.MetaData.get_list(ss, this.query_type, callback);
            this.refresh.sensitive = false;
            this.combo.sensitive = false;
            this.pbox.show();
        }
        
    }
    private void combo_box_changed(Gtk.ComboBox comb)
    {
        int active = comb.active;
        this.model.clear();
        this.title_entry.sensitive = false;
        this.album_entry.sensitive = false;
        this.artist_entry.sensitive = false;
        this.refresh.sensitive = false;
        this.warning_label.hide();
        if(this.column != null)
        {
            this.tree.remove_column(this.column);
            this.column.destroy();
            this.column = null;
        }
        if(active == 0)
        {
           this.query_type = Gmpc.MetaData.Type.ARTIST_ART;  
           if(this.song.artist != null)
           {
                this.artist_entry.sensitive = true;
                this.refresh.sensitive = true;
           }
           else this.warning_label.show();
        }else  if (active == 1) {
           this.query_type = Gmpc.MetaData.Type.ALBUM_ART;  

           if(this.song.artist != null && this.song.album != null)
           {
                this.artist_entry.sensitive = true;
                this.album_entry.sensitive = true;
                this.refresh.sensitive = true;
           }
           else this.warning_label.show();
        } else  if (active == 2) {
           this.query_type = Gmpc.MetaData.Type.SONG_TXT;  

           if(this.song.artist != null && this.song.title != null)
           {
                this.artist_entry.sensitive = true;
                this.title_entry.sensitive = true;
                this.refresh.sensitive = true;
           }
           else this.warning_label.show();
        }else  if (active == 3) {
           this.query_type = Gmpc.MetaData.Type.ALBUM_TXT;  

           if(this.song.artist != null && this.song.album != null)
           {
                this.artist_entry.sensitive = true;
                this.album_entry.sensitive = true;
                this.refresh.sensitive = true;
           }
           else this.warning_label.show();
        }else  if (active == 4) {
           this.query_type = Gmpc.MetaData.Type.ARTIST_TXT;  

           if(this.song.artist != null)
           {
                this.artist_entry.sensitive = true;
                this.refresh.sensitive = true;
           }
           else this.warning_label.show();
        }else  if (active == 5) {
           this.query_type = Gmpc.MetaData.Type.SONG_GUITAR_TAB;  

           if(this.song.artist != null && this.song.title != null)
           {
                this.artist_entry.sensitive = true;
                this.title_entry.sensitive = true;
                this.refresh.sensitive = true;
           }
           else this.warning_label.show();
        }


        if(active <2 )
        {
            var renderer = new Gtk.CellRendererPixbuf();
            this.column = new Gtk.TreeViewColumn();
            column.resizable = true;
            this.column.pack_start(renderer,false);
            renderer.set("xalign",0.0f);
            renderer.set("yalign",0.0f);
            this.tree.append_column(this.column);
            this.column.set_title(_("Cover"));
            this.column.add_attribute(renderer, "pixbuf", 0);
        }
        else 
        {
            var renderer = new Gtk.CellRendererText();
            this.column = new Gtk.TreeViewColumn();
            column.resizable = true;
            this.column.pack_start(renderer,false);
            this.tree.append_column(this.column);

            renderer.set("wrap-mode",Pango.WrapMode.WORD);
            renderer.set("wrap-width",300);
            this.column.set_title(_("Lyric"));
            this.column.add_attribute(renderer, "text", 3);
        }
    }

    public
    EditWindow (MPD.Song song, Gmpc.MetaData.Type type) {
        var vbox = new Gtk.VBox(false, 6);
        this.song = song;
        this.query_type = type;
        this.pbox = new Gtk.HBox(false, 6); 
        this.bar = new Gtk.ProgressBar();
        vbox.pack_start(this.pbox, false, false, 0);
        this.cancel = new Gtk.Button.from_stock("gtk-cancel");
        this.cancel.clicked += this.b_cancel;
        this.pbox.pack_start(this.bar, true, true, 0);
        this.pbox.pack_start(this.cancel, false, false, 0);
        this.bar.show();
        this.cancel.show();
        this.pbox.no_show_all = true;
        this.pbox.hide();

        this.model = new Gtk.ListStore(4,typeof(Gdk.Pixbuf), typeof(string),typeof(string),typeof(string));
        var sw = new Gtk.ScrolledWindow(null, null);
        var iv = new Gtk.TreeView();

        this.tree = iv;
        sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
        sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN);
        iv.set_model(this.model);
        this.tree.rules_hint = true;
        

        var rendererpb = new Gtk.CellRendererText();
        var column = new Gtk.TreeViewColumn();
        column.resizable = true;
        column.min_width = 250;
        rendererpb.set("xalign",0.0f);
        rendererpb.set("yalign",0.0f);
        rendererpb.set("ellipsize", Pango.EllipsizeMode.END);
        column.pack_start(rendererpb,true);
        iv.append_column(column);
        column.set_title(_("Information"));
        column.add_attribute(rendererpb, "markup", 2);
        



        /* Button */
        var hbox = new Gtk.HBox(false, 6);

        var button = new Gtk.Button.from_stock("gtk-quit");
        button.clicked += destroy_popup;
        hbox.pack_end(button, false, false, 0);

        button = new Gtk.Button.with_mnemonic("_Set");
        button.clicked += set_metadata;
        hbox.pack_end(button, false, false, 0);

        vbox.pack_end(hbox, false, false,0);

        /** Change */
        this.warning_label = new Gtk.Label("");
        this.warning_label.set_markup("<span size='x-large'>%s</span>".printf(_("Insufficient information to store/fetch this metadata")));
        this.warning_label.set_alignment(0.0f, 0.5f);
        vbox.pack_start(this.warning_label, false, false, 0);
        this.warning_label.hide();

        var group = new Gtk.SizeGroup(Gtk.SizeGroupMode.HORIZONTAL);
        var qhbox = new Gtk.HBox(false, 6);
        var label = new Gtk.Label("Type");

        group.add_widget(label);
        qhbox.pack_start(label, false,false,0);

        this.combo = new Gtk.ComboBox.text(); 
        qhbox.pack_start(this.combo, false,false,0);
        this.combo.append_text(_("Artist art"));
        this.combo.append_text(_("Album art"));
        this.combo.append_text(_("Song Lyrics"));
        this.combo.append_text(_("Album Info"));
        this.combo.append_text(_("Artist Biography"));
        this.combo.append_text(_("Guitar Tab"));

        this.combo.changed += combo_box_changed;

        vbox.pack_start(qhbox, false, false, 0);
       
        
        qhbox = new Gtk.HBox(false, 6);
        label = new Gtk.Label(_("Artist"));
        group.add_widget(label);
        qhbox.pack_start(label, false, false, 0);
        this.artist_entry = new Gtk.Entry();
        this.artist_entry.set_text(song.artist);
        qhbox.pack_start(this.artist_entry, true, true, 0);

        vbox.pack_start(qhbox, false, false, 0);
       
//        if(type == Gmpc.MetaData.Type.ALBUM_ART)
        {
            qhbox = new Gtk.HBox(false, 6);
            label = new Gtk.Label(_("Album"));
            group.add_widget(label);
            qhbox.pack_start(label, false, false, 0);
            this.album_entry = new Gtk.Entry();
            if(song.album != null)
                this.album_entry.set_text(song.album);
            qhbox.pack_start(this.album_entry, true, true, 0);

            vbox.pack_start(qhbox, false, false, 0);
        }
        {
            qhbox = new Gtk.HBox(false, 6);
            label = new Gtk.Label(_("Title"));
            group.add_widget(label);
            qhbox.pack_start(label, false, false, 0);
            this.title_entry = new Gtk.Entry();
            if(song.title != null)
                this.title_entry.set_text(song.title);
            qhbox.pack_start(this.title_entry, true, true, 0);

            vbox.pack_start(qhbox, false, false, 0);
        }
        if(type != Gmpc.MetaData.Type.ALBUM_ART)
            this.album_entry.sensitive = false;


        this.refresh = button = new Gtk.Button.with_label(_("Query"));
        var ali = new Gtk.Alignment(1.0f, 0.5f, 0.0f, 0.0f);
        ali.add(button);
        vbox.pack_start(ali, false, false, 0);
        button.clicked += refresh_query;



        vbox.pack_start(sw, true, true,0);
        this.add(vbox);
        this.hide_on_delete();
        sw.add(iv);
        this.show_all();

        if(type == Gmpc.MetaData.Type.ARTIST_ART)  this.combo.set_active(0);
        else if(type == Gmpc.MetaData.Type.ALBUM_ART) this.combo.set_active(1);
        else if(type == Gmpc.MetaData.Type.SONG_TXT)  this.combo.set_active(2);
        else if(type == Gmpc.MetaData.Type.ALBUM_TXT)  this.combo.set_active(3);
        else if(type == Gmpc.MetaData.Type.ARTIST_TXT)this.combo.set_active(4);
        else if(type == Gmpc.MetaData.Type.SONG_GUITAR_TAB)this.combo.set_active(5);
/*
        this.refresh.sensitive = false;
        this.combo.sensitive = false;
        
        this.handle = Gmpc.MetaData.get_list(song, this.query_type, callback);
        stdout.printf("Query 1\n");
        if(this.song.albumartist != null){
            MPD.Song song2  = song;
            song2.artist = song2.albumartist;
            stdout.printf("query 2\n");
            this.handle2 = Gmpc.MetaData.get_list(song2, this.query_type, callback);
        }
        */
    }
    public void b_cancel(){
        if(this.handle != null){
            stdout.printf("cancel 1\n");
            Gmpc.MetaData.get_list_cancel(this.handle);
            this.handle = null;
        }
        if(this.handle2 != null){
            stdout.printf("cancel 2\n");
            Gmpc.MetaData.get_list_cancel(this.handle2);
            this.handle2 = null;
        }
        this.downloads.first();
        while(this.downloads != null){
            Gmpc.AsyncDownload.Handle handle = this.downloads.data;
            
            stdout.printf("cancel download: %s\n", handle.get_uri());
            handle.cancel(); 
            this.downloads.first();
        }

        this.pbox.hide();
        this.refresh.sensitive = true;
        this.combo.sensitive = true;
    }
    ~EditWindow() {
        this.b_cancel();
        stdout.printf("song window destroy\n");
    }
}



public class  Gmpc.TestPlugin : Gmpc.Plugin.Base,Gmpc.Plugin.ToolMenuIface, Gmpc.Plugin.SongListIface {
    public const int[] version = {0,0,2};
    /*********************************************************************************
     * Plugin base functions 
     * These functions are required.
     ********************************************************************************/
    construct {
        /* Mark the plugin as an internal dummy */
        this.plugin_type = 8+4;
    }
    public override weak int[] get_version() {
        return this.version;
    }
    /**
     * The name of the plugin
     */
    public override weak string get_name() {
        return "Vala test plugin";
    }

    /**
     * Tells the plugin to save itself
     */
    public override void save_yourself()
    {
        /* nothing to save */
    }

    /**
     * Get set enabled
     */
    public override bool get_enabled() {
        return (bool)config.get_int_with_default(this.get_name(), "enabled", 1);
    }
    public override void set_enabled(bool state) {
       config.set_int(this.get_name(), "enabled", (int)state); 
    }

     

    /*********************************************************************************
     * Private  
     ********************************************************************************/
    private void menu_activate_tree(Gtk.MenuItem item)
    {
        Gtk.TreeView tv = (Gtk.TreeView)item.get_data("treeview");
        Gtk.TreeModel model = tv.get_model();
        var selection = tv.get_selection();
        foreach(weak Gtk.TreePath path in selection.get_selected_rows(out model))
        {
            Gtk.TreeIter iter;
            if(model.get_iter(out iter, path))
            {
                weak MPD.Song song = null;
                model.get(iter,0, out song);
                if(song != null)
                {
                    new Gmpc.MetaData.EditWindow(song,Gmpc.MetaData.Type.ALBUM_ART);
                }
            }
        }

    }

    public int song_list(Gtk.Widget tree, Gtk.Menu menu)
    {
        Gtk.TreeView tv = (Gtk.TreeView)tree;
        var selection = tv.get_selection();
        if(selection.count_selected_rows() > 0)
        {
            Gtk.ImageMenuItem item = new Gtk.ImageMenuItem.with_label(_("Metadata selector"));
            item.set_image( new Gtk.Image.from_stock("gtk-edit", Gtk.IconSize.MENU));
            item.set_data("treeview", tv);
            menu.append(item);

            item.activate += menu_activate_tree;
            return 1;
        }
        return 0;
    }

    public void menu_activated_album(Gtk.MenuItem item)
    {
        weak MPD.Song song = server.playlist_get_current_song();
        new Gmpc.MetaData.EditWindow(song,Gmpc.MetaData.Type.ALBUM_ART);
    }
    public int tool_menu_integration(Gtk.Menu menu)
    {
        Gtk.MenuItem item = new Gtk.MenuItem.with_label("Edit metadata current song");
        menu.append(item);
        item.activate += menu_activated_album;
        return 2;
    }
}

