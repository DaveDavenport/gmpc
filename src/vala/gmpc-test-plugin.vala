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
    private MPD.Song song = null;
    private Gtk.TreeView tree = null;
    private void *handle = null;
    construct {
        this.type = Gtk.WindowType.TOPLEVEL;
        this.set_default_size(650,800);
        this.set_border_width(8);
    }

    public void image_downloaded(Gmpc.AsyncDownload.Handle handle, Gmpc.AsyncDownload.Status status)
    {
        if(status == Gmpc.AsyncDownload.Status.DONE)
        {
            int64 length;
            weak string data = handle.get_data(out length);
            try{
                Gtk.TreeIter iter;
                var load = new Gdk.PixbufLoader();
                load.set_size(150,150);
                try {
                    load.write((uchar[])data);
                }catch (Error e) {
                    stdout.printf("Failed to load file: %s::%s\n",e.message,handle.get_uri());
                }
                load.close();
                Gdk.Pixbuf pb = load.get_pixbuf();//new Gdk.Pixbuf.from_inline((int)length, (uchar[])data, true); 
                this.model.append(out iter);
                this.model.set(iter, 0, pb,1, handle.get_uri(), -1);
            }catch (Error e) {
                stdout.printf("Failed to load file: %s::%s\n",e.message,handle.get_uri());

                return;
            }

        }
    }
    public void callback(void *handle,GLib.List<string>? list)
    {
        if(handle == null) {
            stdout.printf("Done fetching\n");
            this.handle = null;
        }
        foreach(weak string uri in list)
        {
            stdout.printf("Uri: %s\n", uri);
            if(uri[0] == '/'){
                Gtk.TreeIter iter;
                try{
                Gdk.Pixbuf pb = new Gdk.Pixbuf.from_file_at_scale(uri, 150, 150, true);
                this.model.append(out iter);
                this.model.set(iter, 0, pb,1, uri, -1);
                }catch(Error e)
                {

                }

            }else
                Gmpc.AsyncDownload.download(uri, image_downloaded); 
        }
    }

    public void store_image(Gmpc.AsyncDownload.Handle handle, Gmpc.AsyncDownload.Status status)
    {
        stdout.printf("Aap noot mies\n");
        if(status == Gmpc.AsyncDownload.Status.DONE)
        {
            int64 length;
            weak string data = handle.get_data(out length);
            var file = Gmpc.MetaData.get_metadata_filename(Gmpc.MetaData.Type.ALBUM_ART, this.song, "jpg");
            try {
                stdout.printf("Storing into: %s\n", file);
                GLib.FileUtils.set_contents(file, data, (long)length);

                Gmpc.MetaData.set_metadata(this.song, Gmpc.MetaData.Type.ALBUM_ART, Gmpc.MetaData.Result.AVAILABLE, file); 
                metawatcher.data_changed(this.song, Gmpc.MetaData.Type.ALBUM_ART, Gmpc.MetaData.Result.AVAILABLE, file);  
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
                Gmpc.MetaData.set_metadata(this.song, Gmpc.MetaData.Type.ALBUM_ART, Gmpc.MetaData.Result.AVAILABLE, path); 
                metawatcher.data_changed(this.song, Gmpc.MetaData.Type.ALBUM_ART, Gmpc.MetaData.Result.AVAILABLE, path);  
            }else{
                Gmpc.AsyncDownload.download(path, store_image); 
            }

        }
    }
    public
    void
    destroy_popup(Gtk.Button button)
    {
        this.destroy();
    }

    SongWindow (MPD.Song song) {
        var vbox = new Gtk.VBox(false, 6);
        this.song = song;
        this.model = new Gtk.ListStore(2,typeof(Gdk.Pixbuf), typeof(string));
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
        column.add_attribute(rendererpb, "text", 1);

        vbox.pack_start(sw, true, true,0);

        /* Button */
        var hbox = new Gtk.HBox(false, 6);

        var button = new Gtk.Button.from_stock("gtk-quit");
        button.clicked += destroy_popup;
        hbox.pack_end(button, false, false, 0);

        button = new Gtk.Button.with_label("Set cover");
        button.clicked += set_metadata;
        hbox.pack_end(button, false, false, 0);

        vbox.pack_start(hbox, false, false,0);

        this.add(vbox);
        sw.add(iv);
        this.show_all();

        this.handle = Gmpc.MetaData.get_list(song, Gmpc.MetaData.Type.ALBUM_ART, callback);
    }
    ~SongWindow() {
        if(this.handle != null){
            Gmpc.MetaData.get_list_cancel(this.handle);
            this.handle = null;
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

    public void menu_activated(Gtk.MenuItem item)
    {
        weak MPD.Song song = server.playlist_get_current_song();
        new SongWindow(song);
    }
    public int tool_menu_integration(Gtk.Menu menu)
    {
        Gtk.MenuItem item = new Gtk.MenuItem.with_label("Test plugin");
        menu.append(item);
        item.activate += menu_activated;
        return 0;
    }
}

