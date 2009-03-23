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


private class SongWindow : Gtk.Window {
    private const string some_unique_name = Config.VERSION;
    private Gtk.ListStore model = null;
    private Gtk.TreeView tree = null;
    private GLib.List<weak Gmpc.AsyncDownload.Handle> downloads = null;

    private MPD.Song song = null;
    private Gmpc.MetaData.Type query_type = Gmpc.MetaData.Type.ALBUM_ART;

    /** fetching handles */
    private void *handle = null;
    private void *handle2 = null;


    private Gtk.Entry artist_entry;
    private Gtk.Entry album_entry;
    private Gtk.Button refresh = null;

    construct {
        this.type = Gtk.WindowType.TOPLEVEL;
        this.set_default_size(650,800);
        this.set_border_width(8);
    }

    private void add_entry(string uri,Gdk.PixbufFormat? format, Gdk.Pixbuf pb)
    {
        Gtk.TreeIter iter;
        string a;
        a = "<b>%s</b>: %s".printf(_("Uri"),uri);
        if(format != null)
        {
            a+="\n<b>%s</b>: %s".printf(_("Filetype"), format.get_name());
            stdout.printf("%s\n",format.get_name()); 
        }
        if(pb != null)
        {
            a+="\n<b>%s</b>: %ix%i (%s)".printf(_("Size"), pb.width, pb.height,_("wxh"));

        }
        int new_h, new_w;
        if(pb.width > pb.height) {
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
    public void image_downloaded(Gmpc.AsyncDownload.Handle handle, Gmpc.AsyncDownload.Status status)
    {
        if(status == Gmpc.AsyncDownload.Status.PROGRESS) return;
        stdout.printf("Result in: %s\n", handle.get_uri());
        this.downloads.remove(handle);
        if(status == Gmpc.AsyncDownload.Status.DONE)
        {
            weak uchar[] data = handle.get_data();
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
                    this.add_entry(handle.get_uri(),load.get_format(),pb);
            }catch (Error e) {
                stdout.printf("Failed to load file: %s::%s\n",e.message,handle.get_uri());

                return;
            }

        }

    }
    public void callback(void *handle,string? plugin_name,GLib.List<string>? list)
    {
        if(list == null) {
            stdout.printf("Done fetching\n");
            if(this.handle == handle)
            {
                stdout.printf("done 1\n");
                this.handle = null;
                if(this.handle == null)
                    this.refresh.sensitive = true;
            }
            if(this.handle2 == handle)
            {
                stdout.printf("done 1\n");
                this.handle2 = null;


                if(this.handle == null)
                    this.refresh.sensitive = true;
            }
        }
        foreach(weak string uri in list)
        {
            stdout.printf("Uri: %s\n", uri);
            if(uri[0] == '/'){
                try{
                    Gdk.Pixbuf pb = new Gdk.Pixbuf.from_file(uri);
                    if(pb != null)
                        add_entry(uri,Gdk.Pixbuf.get_file_info(uri,null, null), pb);
                }catch(Error e)
                {

                }

            }else{
                var h =  Gmpc.AsyncDownload.download(uri, image_downloaded); 
                this.downloads.append(h);
            }
        }
    }

    public void store_image(Gmpc.AsyncDownload.Handle handle, Gmpc.AsyncDownload.Status status)
    {
        if(status == Gmpc.AsyncDownload.Status.PROGRESS) return;
        this.downloads.remove(handle);
        stdout.printf("Aap noot mies: %s\n", handle.get_uri());
        if(status == Gmpc.AsyncDownload.Status.DONE)
        {
            weak uchar[] data = handle.get_data();
            var file = Gmpc.MetaData.get_metadata_filename(this.query_type, this.song, "jpg");
            try {
                stdout.printf("Storing into: %s\n", file);
                GLib.FileUtils.set_contents(file, (string)data, (long)data.length);

                Gmpc.MetaData.set_metadata(this.song,this.query_type, Gmpc.MetaData.Result.AVAILABLE, file); 
                metawatcher.data_changed(this.song, this.query_type, Gmpc.MetaData.Result.UNAVAILABLE, null);  
                metawatcher.data_changed(this.song, this.query_type, Gmpc.MetaData.Result.AVAILABLE, file);  
            }catch (Error e) {

            }
        }
    }
    private void
    set_metadata(Gtk.Button button)
    {
        Gtk.TreeIter iter;
        var sel = this.tree.get_selection();
        string path;
        if(sel.get_selected(out this.model,out iter))
        {
            this.model.get(iter,1,out path);
            stdout.printf("clicked %s\n",path);
            if(path[0]  == '/')
            {
                Gmpc.MetaData.set_metadata(this.song, this.query_type, Gmpc.MetaData.Result.AVAILABLE, path); 
                metawatcher.data_changed(this.song, this.query_type, Gmpc.MetaData.Result.UNAVAILABLE, null);  
                metawatcher.data_changed(this.song, this.query_type, Gmpc.MetaData.Result.AVAILABLE, path);  
            }else{
                var h = Gmpc.AsyncDownload.download(path, store_image); 
                this.downloads.append(h);
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
        if(this.query_type == Gmpc.MetaData.Type.ALBUM_ART)
        {
            ss.album = this.album_entry.get_text();
        }
        if(this.handle == null && this.handle2 == null) {
            this.handle = Gmpc.MetaData.get_list(ss, this.query_type, callback);
            this.refresh.sensitive = false;
        }
        
    }

    SongWindow (MPD.Song song, Gmpc.MetaData.Type type) {
        var vbox = new Gtk.VBox(false, 6);
        this.song = song;
        this.query_type = type;

        this.model = new Gtk.ListStore(3,typeof(Gdk.Pixbuf), typeof(string),typeof(string));
        var sw = new Gtk.ScrolledWindow(null, null);
        var iv = new Gtk.TreeView();

        this.tree = iv;
        sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
        sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN);
        iv.set_model(this.model);
        
        var renderer = new Gtk.CellRendererPixbuf();
        var column = new Gtk.TreeViewColumn();
        column.pack_start(renderer,false);
        iv.append_column(column);
        column.set_title(_("Cover"));
        column.add_attribute(renderer, "pixbuf", 0);

        var rendererpb = new Gtk.CellRendererText();
        column = new Gtk.TreeViewColumn();
        column.pack_start(rendererpb,true);
        iv.append_column(column);
        column.set_title(_("Url"));
        column.add_attribute(rendererpb, "markup", 2);

        vbox.pack_start(sw, true, true,0);

        /* Button */
        var hbox = new Gtk.HBox(false, 6);

        var button = new Gtk.Button.from_stock("gtk-quit");
        button.clicked += destroy_popup;
        hbox.pack_end(button, false, false, 0);

        button = new Gtk.Button.with_label("Set cover");
        button.clicked += set_metadata;
        hbox.pack_end(button, false, false, 0);

        vbox.pack_end(hbox, false, false,0);

        /** Change */
        var group = new Gtk.SizeGroup(Gtk.SizeGroupMode.HORIZONTAL);
        var qhbox = new Gtk.HBox(false, 6);
        var label = new Gtk.Label(_("Artist"));
        group.add_widget(label);
        qhbox.pack_start(label, false, false, 0);
        this.artist_entry = new Gtk.Entry();
        this.artist_entry.set_text(song.artist);
        qhbox.pack_start(this.artist_entry, true, true, 0);

        vbox.pack_start(qhbox, false, false, 0);
       
        if(type == Gmpc.MetaData.Type.ALBUM_ART)
        {
            qhbox = new Gtk.HBox(false, 6);
            label = new Gtk.Label(_("Album"));
            group.add_widget(label);
            qhbox.pack_start(label, false, false, 0);
            this.album_entry = new Gtk.Entry();
            this.album_entry.set_text(song.album);
            qhbox.pack_start(this.album_entry, true, true, 0);

            vbox.pack_start(qhbox, false, false, 0);
        }
        this.refresh = button = new Gtk.Button.from_stock("gtk-refresh");
        var ali = new Gtk.Alignment(1.0f, 0.5f, 0.0f, 0.0f);
        ali.add(button);
        vbox.pack_start(ali, false, false, 0);
        button.clicked += refresh_query;
        this.refresh.sensitive = false;

        this.add(vbox);
        this.hide_on_delete();
        sw.add(iv);
        this.show_all();

        this.handle = Gmpc.MetaData.get_list(song, this.query_type, callback);
        stdout.printf("Query 1\n");
        if(this.song.albumartist != null){
            MPD.Song song2  = song;
            song2.artist = song2.albumartist;
            stdout.printf("query 2\n");
            this.handle2 = Gmpc.MetaData.get_list(song2, this.query_type, callback);
        }
    }
    ~SongWindow() {


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
        stdout.printf("song window destroy\n");
    }
}



public class  Gmpc.TestPlugin : Gmpc.Plugin.Base, Gmpc.Plugin.PreferencesIface, Gmpc.Plugin.ToolMenuIface  {
    public const int[3] version = {0,0,2};
    /*********************************************************************************
     * Plugin base functions 
     * These functions are required.
     ********************************************************************************/
    public override weak int[] get_version() {
        return (int[3])this.version;
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
     * Plugin preferences functions 
     ********************************************************************************/
    public void pane_construct(Gtk.Container container)
    {
        var box = new Gtk.HBox(false,6);
        var label = new Gtk.Label ("This is a test preferences pane");
        box.pack_start(label, false, false,0);

        container.add(box);

        container.show_all();
        stdout.printf("%s: Create preferences panel\n",this.get_name());
    }
    public void pane_destroy(Gtk.Container container)
    {
        Gtk.Bin bin = (Gtk.Bin) container;
        bin.child.destroy();
    }

    /*********************************************************************************
     * Private  
     ********************************************************************************/

    public void menu_activated_album(Gtk.MenuItem item)
    {
        weak MPD.Song song = server.playlist_get_current_song();
        new SongWindow(song,Gmpc.MetaData.Type.ALBUM_ART);
    }

    public void menu_activated_artist(Gtk.MenuItem item)
    {
        weak MPD.Song song = server.playlist_get_current_song();
        new SongWindow(song,Gmpc.MetaData.Type.ARTIST_ART);
    }
    public int tool_menu_integration(Gtk.Menu menu)
    {
        Gtk.MenuItem item = new Gtk.MenuItem.with_label("Test plugin album");
        menu.append(item);
        item.activate += menu_activated_album;

        item = new Gtk.MenuItem.with_label("Test plugin artist");
        menu.append(item);
        item.activate += menu_activated_artist;
        return 2;
    }
}

