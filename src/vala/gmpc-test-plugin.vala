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
using GLib;
using Gtk;
using Gmpc;

public class  Gmpc.TestPlugin : Gmpc.Plugin.Base, Gmpc.Plugin.PreferencesIface, Gmpc.Plugin.ToolMenuIface  {
    public const int[3] version = {0,0,2};
    private Gtk.ListStore model = null;
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
        stdout.printf("%s: Destroy preferences panel\n",this.get_name());
    }

    /*********************************************************************************
     * Private  
     ********************************************************************************/

    /* Plugin functions */
    private void connection_changed(Gmpc.Connection conn,MPD.Server server, int connect){
        stdout.printf("%s: Connection changed: %i\n",this.get_name(), connect);
    }
    construct {
        stdout.printf("create %s\n", this.get_name());
        gmpcconn.connection_changed += connection_changed;
    }
    ~TestPlugin() {
        stdout.printf("Destroying %s\n", this.get_name());

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
    public void callback(GLib.List<string> list)
    {
        foreach(weak string uri in list)
        {
            stdout.printf("Uri: %s\n", uri);
            if(uri[0] == '/'){
                Gtk.TreeIter iter;
                try{
                Gdk.Pixbuf pb = new Gdk.Pixbuf.from_file(uri);
                this.model.append(out iter);
                this.model.set(iter, 0, pb,1, uri, -1);
                }catch(Error e)
                {

                }

            }else
                Gmpc.AsyncDownload.download(uri, image_downloaded); 
        }
    }
    public void menu_activated(Gtk.MenuItem item)
    {
        var window = new Gtk.Window(Gtk.WindowType.TOPLEVEL);
        this.model = new Gtk.ListStore(2,typeof(Gdk.Pixbuf), typeof(string));
        var iv = new Gtk.IconView();
        iv.set_model(this.model);
        iv.pixbuf_column = 0;
        window.add(iv);
        window.show_all();
        var song = server.playlist_get_current_song();
        if(song != null){
            Gmpc.MetaData.get_list(song, Gmpc.MetaData.Type.ALBUM_ART, callback);
        }
    }
    public int tool_menu_integration(Gtk.Menu menu)
    {
        Gtk.MenuItem item = new Gtk.MenuItem.with_label("Test plugin");
        menu.append(item);
        item.activate += menu_activated;
        return 0;
    }
}

