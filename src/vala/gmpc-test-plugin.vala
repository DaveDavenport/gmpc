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

using Config;
using Gtk;
using Gmpc;

private const bool use_transition_tp = Gmpc.use_transition;

public class Gmpc.MetaData.EditWindow : Gtk.Window {
    private const string some_unique_name = Config.VERSION;
    private MPD.Song? song = null;
    private Gmpc.MetaData.Type query_type = Gmpc.MetaData.Type.ALBUM_ART;

    private GLib.List<unowned Gmpc.AsyncDownload.Handle> downloads = null;
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

    private Gtk.ScrolledWindow sw = null;
    private Gtk.EventBox ilevent = null;
    
    private Gtk.VBox itemslist = new Gtk.VBox(false, 6);

    construct {
        this.type = Gtk.WindowType.TOPLEVEL;
        int height = config.get_int_with_default(
                "Metadata Selector" , "window_height", 600);
        int width = config.get_int_with_default(
                "Metadata Selector" , "window_width", 480);

        this.resize(width,height);
        this.set_border_width(8);
        /* Connect to allocation changes so I can store new size */
        this.size_allocate.connect((source, alloc) => {
                config.set_int(
                    "Metadata Selector" , "window_width", alloc.width);

                config.set_int(
                    "Metadata Selector" , "window_height", alloc.height);
        });
    }

    private void add_entry_image(string? provider, string uri,Gdk.PixbufFormat? format, Gdk.Pixbuf pb, bool is_raw = false, bool is_thumbnail = false)
    {
        string a;
        a = "";// Markup.printf_escaped("<b>%s</b>: %s",_("Uri"),uri);
        if(provider != null) {
            a+= Markup.printf_escaped("\n<b>%s</b>:  %s",_("Provider"), provider);
        }
        if(format != null)
        {
            a+= Markup.printf_escaped("\n<b>%s</b>: %s",_("Filetype"), format.get_name());
        }
        if(pb != null)
        {
            if(is_thumbnail) {
                a+="\n<b>%s</b>: %ix%i (%s) (%s)".printf(_("Size"), pb.width, pb.height,_("width x height"), 
                        _("high-res image will be downloaded"));
            }else{
                a+="\n<b>%s</b>: %ix%i (%s)".printf(_("Size"), pb.width, pb.height,_("width x height"));
           }

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


        var hbox = new Gtk.HBox(false, 6);
        var label = new Gtk.Label("");
        label.set_ellipsize(Pango.EllipsizeMode.MIDDLE);
        label.set_markup(a);
        label.set_line_wrap(true);
        label.set_alignment(0.0f, 0.5f);

        var image = new Gtk.Image.from_pixbuf(pb.scale_simple(new_w,new_h,Gdk.InterpType.BILINEAR));
        image.set_size_request(180,-1);
        hbox.pack_start(image, false, true, 0);
        hbox.pack_start(label, true, true, 0);

        
        var ali = new Gtk.Alignment(0f,0f,0f,0f);
        var button = new Gtk.Button.with_label(_("Set"));
        if(!is_raw) {
            button.set_data_full("path",(void *)uri.dup(),(GLib.DestroyNotify)g_free);
        }else{
            button.set_data_full("data",(void *)uri.dup(),(GLib.DestroyNotify)g_free);
        }
        ali.add(button);
        hbox.pack_start(ali, false, true, 0);
        button.clicked.connect(set_metadata);

        hbox.show_all();
        this.itemslist.pack_start(hbox,false, true, 0);
        var sep = new Gtk.HSeparator();
        sep.set_size_request(-1, 4);
        sep.show();
        this.itemslist.pack_start(sep, false, true, 0);

    }

    private void add_entry_text(string? provider, string uri,string text) 
    {
        string a;
        a = "";// "<b>%s</b>: %s".printf(_("Uri"),uri);
        if(provider != null) {
            a+="\n<b>%s</b>:  %s".printf(_("Provider"), provider);
        }
        var hbox = new Gtk.HBox(false, 6);
        var label = new Gtk.Label("");
        label.set_markup(a);
        label.set_ellipsize(Pango.EllipsizeMode.MIDDLE);
        label.set_line_wrap(true);
        label.set_alignment(0.0f, 0.0f);
        label.set_size_request(280,-1);
        hbox.pack_start(label, false, true, 0);
        
        var text_label = new Gtk.Label(text); 
        text_label.set_ellipsize(Pango.EllipsizeMode.END);
        text_label.set_alignment(0.0f, 0.0f);
        text_label.set_size_request(180,-1);
        hbox.pack_start(text_label, true, true, 0);

        var ali = new Gtk.Alignment(0f,0f,0f,0f);
        var button = new Gtk.Button.with_label(_("Set"));
        button.set_data_full("lyrics",(void *)text.dup(),(GLib.DestroyNotify)g_free);
        ali.add(button);
        hbox.pack_start(ali, false, true, 0);
        button.clicked.connect(set_metadata);

        hbox.show_all();
        this.itemslist.pack_start(hbox,false, true, 0);
        var sep = new Gtk.HSeparator();
        sep.set_size_request(-1, 4);
        sep.show();
        this.itemslist.pack_start(sep, false, true, 0);
    }

    public void image_downloaded(Gmpc.AsyncDownload.Handle handle, Gmpc.AsyncDownload.Status status, void *p)
    {
        MetaData.Item *item = (MetaData.Item *)p;
        GLib.assert(item != null);
        if(status == Gmpc.AsyncDownload.Status.PROGRESS) return;
        this.downloads.remove(handle);
        this.bar.pulse();
        if(status == Gmpc.AsyncDownload.Status.DONE)
        {
            unowned uchar[] data = handle.get_data<uchar[]>();
            if(this.query_type == Gmpc.MetaData.Type.ALBUM_ART || this.query_type == Gmpc.MetaData.Type.ARTIST_ART)
            {
                try{
                    var load = new Gdk.PixbufLoader();
                    try {
                        load.write(data);
                    }catch (Error e) {
                        GLib.debug("Failed to load file: %s::%s\n",e.message,handle.get_uri());
                    }
                    load.close();

                    Gdk.Pixbuf pb = load.get_pixbuf();
                    if(pb!= null)
                        this.add_entry_image((string)handle.get_user_data(),
                                            item->get_uri(),
                                            load.get_format(),
                                            pb,
                                            false,
                                            (item->get_thumbnail_uri() != null));
                }catch (Error e) {
                    GLib.debug("Failed to load file: %s::%s\n",e.message,handle.get_uri());
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
            this.ilevent.sensitive = true;
            this.combo.sensitive = true;
        }
        delete item;
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
                    this.ilevent.sensitive = true;
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
                    this.ilevent.sensitive = true;
                    this.refresh.sensitive = true;
                }
            }
        }
        foreach(unowned MetaData.Item md in list)
        {

            if(this.query_type == Gmpc.MetaData.Type.ALBUM_ART || this.query_type == Gmpc.MetaData.Type.ARTIST_ART)
            {
                if(md.content_type == Gmpc.MetaData.ContentType.URI)
                {
                    if(md.content_type == Gmpc.MetaData.ContentType.URI)
                    {
                        unowned string uri = null;
                        if(md.get_thumbnail_uri() != null) {
                            uri = md.get_thumbnail_uri();
                        }else{
                            uri = md.get_uri();
                        }
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
                            MetaData.Item *item = MetaData.Item.copy(md);
                            unowned Gmpc.AsyncDownload.Handle h =  Gmpc.AsyncDownload.download_vala(uri,(void *)item,image_downloaded);                                 
                            if(h!=null)
                            {
                                h.set_user_data(md.plugin_name);
                                this.downloads.append(h);
                            } 
                        }
                    }
                }else if (md.content_type == Gmpc.MetaData.ContentType.RAW){
                  var data = md.get_raw();  
                    var load = new Gdk.PixbufLoader();
                    try {
                        load.write(data);
                    }catch (Error e) {
                        GLib.debug("Failed to load raw data: %s\n",e.message);
                    }
                    try {
                        load.close();
                    }catch (Error e) {
                        GLib.debug("Failed to close loader: %s\n",e.message);
                    }

                    Gdk.Pixbuf pb = load.get_pixbuf();
                    if(pb!= null){
                        var base16 = GLib.Base64.encode(data); 
                        this.add_entry_image(plugin_name,base16,load.get_format(),pb,true);
                    }
                }
            }else{

                if(md.content_type == Gmpc.MetaData.ContentType.TEXT)
                {
                    unowned string uri = md.get_text();
                    add_entry_text(plugin_name, "n/a", uri);
                }
                else 
                if(md.content_type == Gmpc.MetaData.ContentType.HTML)
                {
                    string uri = md.get_text_from_html();
                    add_entry_text(plugin_name, "n/a", uri);
                }
                else
                if(md.content_type == Gmpc.MetaData.ContentType.URI)
                {
                    unowned string uri = md.get_uri();
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
            unowned uchar[] data =handle.get_data<uchar[]>();
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
            unowned uchar[] data = handle.get_data<uchar[]>();
            var file = Gmpc.MetaData.get_metadata_filename(this.query_type, this.song,null); 
            try {
                GLib.FileUtils.set_contents(file, (string)data, (long)data.length);
                var met = new MetaData.Item();
                met.type = this.query_type;
                met.plugin_name = "User set";
                met.content_type = MetaData.ContentType.URI;
                met.set_uri(file);
				// @TODO fix this
				GLib.error("needs implementing");
                //Gmpc.MetaData.set_metadata(this.song, Gmpc.MetaData.Result.AVAILABLE, met); 
    
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
        string path = null;
        if(this.query_type == Gmpc.MetaData.Type.ALBUM_ART || this.query_type == Gmpc.MetaData.Type.ARTIST_ART)
        {
            path = (string)button.get_data<string>("path");
            if(path != null)
            {
                if(path[0]  == '/')
                {
                    var met = new MetaData.Item();
                    met.type = this.query_type;
                    met.plugin_name = "User set";
                    met.content_type = MetaData.ContentType.URI;
                    met.set_uri(path);
					// @TODO fix this
					GLib.error("needs implementing");
					//Gmpc.MetaData.set_metadata(this.song, Gmpc.MetaData.Result.AVAILABLE, met); 

                    var met_false = new MetaData.Item();
                    met_false.type = this.query_type;
                    met_false.plugin_name = "User set";
                    met_false.content_type = MetaData.ContentType.EMPTY;
                    metawatcher.data_changed(this.song, this.query_type, Gmpc.MetaData.Result.UNAVAILABLE, met_false);  
                    metawatcher.data_changed(this.song, this.query_type, Gmpc.MetaData.Result.AVAILABLE, met);  
                }else{
                    unowned Gmpc.AsyncDownload.Handle h = Gmpc.AsyncDownload.download(path, store_image); 
                    if(h!=null)
                        this.downloads.append(h);
                }
            }else{
                string base64 =(string)button.get_data<string>("data");
                if(base64 != null) {
                    string filename = Gmpc.MetaData.get_metadata_filename(this.query_type,this.song, null); 
                    uchar[] data = GLib.Base64.decode(base64 );
                    try{
                    GLib.FileUtils.set_contents(filename, (string)data, (ssize_t)data.length);

                    var met = new MetaData.Item();
                    met.type = this.query_type;
                    met.plugin_name = "User set";
                    met.content_type = MetaData.ContentType.URI;
                    met.set_uri(filename);
					// @TODO fix this
					GLib.error("needs implementing");
					//Gmpc.MetaData.set_metadata(this.song, Gmpc.MetaData.Result.AVAILABLE, met); 

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
        }else{
            string lyric;

            lyric = (string)button.get_data<string>("lyrics");
            var file = Gmpc.MetaData.get_metadata_filename(this.query_type, this.song,null); 
            try {
                GLib.FileUtils.set_contents(file,lyric, -1); 
                var met = new MetaData.Item();
                met.type = this.query_type;
                met.plugin_name = "User set";
                met.content_type = MetaData.ContentType.URI;
                met.set_uri(file);
				// @TODO fix this
				GLib.error("needs implementing");
				//Gmpc.MetaData.set_metadata(this.song, Gmpc.MetaData.Result.AVAILABLE, met); 

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
    public
    void
    destroy_popup(Gtk.Button button)
    {
        this.destroy();
    }
    private void clear_itemslist()
    {
        foreach(Gtk.Widget client in this.itemslist.get_children())
        {
           client.destroy();
        }

    }
    public void refresh_query(Gtk.Button button) 
    {
        clear_itemslist();
        GLib.log("MetadataSelector", GLib.LogLevelFlags.LEVEL_DEBUG, "Query metadata");
        MPD.Song ss = this.song.copy();
        ss.artist = this.artist_entry.get_text();
        ss.album = this.album_entry.get_text();
        ss.title = this.title_entry.get_text();

        if(this.handle == null && this.handle2 == null) {
            this.pbox.show();
            this.refresh.sensitive = false;
            this.combo.sensitive = false;
            this.ilevent.sensitive = false;
            GLib.log("MetadataSelector", GLib.LogLevelFlags.LEVEL_DEBUG,"Start metdata get_list query");
            this.handle = Gmpc.MetaData.get_list(ss, this.query_type, callback);
            GLib.log("MetadataSelector", GLib.LogLevelFlags.LEVEL_DEBUG,"Wait");
        }
        
    }
    private void combo_box_changed(Gtk.ComboBox comb)
    {
        clear_itemslist();
        int active = comb.active;
        this.title_entry.sensitive = false;
        this.album_entry.sensitive = false;
        this.artist_entry.sensitive = false;
        this.refresh.sensitive = false;
        this.warning_label.hide();
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
    }

    public
    EditWindow (MPD.Song song, Gmpc.MetaData.Type type) {
        var vbox = new Gtk.VBox(false, 6);
        this.song = song.copy();
        this.query_type = type;
        this.pbox = new Gtk.HBox(false, 6); 
        this.bar = new Gtk.ProgressBar();
        vbox.pack_start(this.pbox, false, false, 0);
        this.cancel = new Gtk.Button.from_stock("gtk-cancel");
        this.cancel.clicked.connect(this.b_cancel);
        this.pbox.pack_start(this.bar, true, true, 0);
        this.pbox.pack_start(this.cancel, false, false, 0);
        this.bar.show();
        this.cancel.show();
        this.pbox.no_show_all = true;
        this.pbox.hide();

        sw = new Gtk.ScrolledWindow(null, null);

        sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
        sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN);
        

        /* Button */
        var hbox = new Gtk.HBox(false, 6);

        var button = new Gtk.Button.from_stock("gtk-quit");
        button.clicked.connect(destroy_popup);
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
        Gtk.Label label = null;

        var list = new Gtk.ListStore(2,typeof(string), typeof(string),-1);
        list.insert_with_values(null, -1, 0, "media-artist",    1, _("Artist art"));
        list.insert_with_values(null, -1, 0, "media-album",     1, _("Album art"));
        list.insert_with_values(null, -1, 0, "gtk-dnd",         1, _("Song lyrics"));
        list.insert_with_values(null, -1, 0, "media-album",     1, _("Album Info"));
        list.insert_with_values(null, -1, 0, "media-artist",    1, _("Artist Biography"));
        list.insert_with_values(null, -1, 0, "media-album",     1, _("Guitar Tab"));

        this.combo = new Gtk.ComboBox.with_model(list); 
       
        Gtk.CellRenderer renderer = new Gtk.CellRendererPixbuf();
        this.combo.pack_start(renderer,false);
        this.combo.add_attribute(renderer, "icon-name", 0);
        renderer = new Gtk.CellRendererText();
        this.combo.pack_start(renderer,true);
        this.combo.add_attribute(renderer, "text", 1);

        qhbox.pack_start(this.combo, true,true,0);
        this.combo.changed.connect(combo_box_changed);

        vbox.pack_start(qhbox, false, false, 0);
       
        
        qhbox = new Gtk.HBox(false, 6);

        label = new Gtk.Label(_("Artist"));
        label.set_alignment(0.0f, 0.5f);
        group.add_widget(label);
        qhbox.pack_start(label, false, false, 0);
        var image =  new Gtk.Image.from_icon_name("media-artist", Gtk.IconSize.BUTTON);
        qhbox.pack_start(image, false, false, 0);

        this.artist_entry = new Gtk.Entry();
        this.artist_entry.set_text(song.artist);
        qhbox.pack_start(this.artist_entry, true, true, 0);
        vbox.pack_start(qhbox, false, false, 0);
       
        qhbox = new Gtk.HBox(false, 6);
        label = new Gtk.Label(_("Album"));
        label.set_alignment(0.0f, 0.5f);
        group.add_widget(label);
        qhbox.pack_start(label, false, false, 0);
        image =  new Gtk.Image.from_icon_name("media-album", Gtk.IconSize.BUTTON);
        qhbox.pack_start(image, false, false, 0);
        this.album_entry = new Gtk.Entry();
        if(song.album != null)
            this.album_entry.set_text(song.album);
        qhbox.pack_start(this.album_entry, true, true, 0);
        vbox.pack_start(qhbox, false, false, 0);

        qhbox = new Gtk.HBox(false, 6);
        label = new Gtk.Label(_("Title"));
        label.set_alignment(0.0f, 0.5f);
        group.add_widget(label);
        qhbox.pack_start(label, false, false, 0);
        image =  new Gtk.Image.from_icon_name("media-audiofile", Gtk.IconSize.BUTTON);
        qhbox.pack_start(image, false, false, 0);
        this.title_entry = new Gtk.Entry();
        if(song.title != null)
            this.title_entry.set_text(song.title);
        qhbox.pack_start(this.title_entry, true, true, 0);

        vbox.pack_start(qhbox, false, false, 0);

        if(type != Gmpc.MetaData.Type.ALBUM_ART)
            this.album_entry.sensitive = false;


        this.refresh = button = new Gtk.Button.with_label(_("Query"));
        var ali = new Gtk.Alignment(1.0f, 0.5f, 0.0f, 0.0f);
        ali.add(button);
        vbox.pack_start(ali, false, false, 0);
        button.clicked.connect(refresh_query);



        vbox.pack_start(sw, true, true,0);
        this.add(vbox);
        this.hide_on_delete();
        //sw.add(iv);
        ilevent = new Gtk.EventBox();
        this.itemslist.border_width = 8;
        ilevent.visible_window = true;
        ilevent.modify_bg(Gtk.StateType.NORMAL,this.style.base[Gtk.StateType.NORMAL]);
        this.style_set.connect((source, old) => {
                this.itemslist.get_parent().modify_bg(Gtk.StateType.NORMAL,this.style.base[Gtk.StateType.NORMAL]);
        });
    //            ilevent.modify_bg(Gtk.StateType.NORMAL,this.style.base[Gtk.StateType.NORMAL]);
     //           });
        ilevent.add(itemslist);
        sw.add_with_viewport(ilevent);
        this.show_all();

        if(type == Gmpc.MetaData.Type.ARTIST_ART)  this.combo.set_active(0);
        else if(type == Gmpc.MetaData.Type.ALBUM_ART) this.combo.set_active(1);
        else if(type == Gmpc.MetaData.Type.SONG_TXT)  this.combo.set_active(2);
        else if(type == Gmpc.MetaData.Type.ALBUM_TXT)  this.combo.set_active(3);
        else if(type == Gmpc.MetaData.Type.ARTIST_TXT)this.combo.set_active(4);
        else if(type == Gmpc.MetaData.Type.SONG_GUITAR_TAB)this.combo.set_active(5);
        refresh_query(button);
    }
    public void b_cancel(){
        if(this.handle != null){
            Gmpc.MetaData.get_list_cancel(this.handle);
            this.handle = null;
        }
        if(this.handle2 != null){
            Gmpc.MetaData.get_list_cancel(this.handle2);
            this.handle2 = null;
        }
        this.downloads.first();
        while(this.downloads != null){
            unowned Gmpc.AsyncDownload.Handle handle = this.downloads.data;
            
            handle.cancel(); 
            this.downloads.first();
        }

        this.pbox.hide();
        this.refresh.sensitive = true;
        this.combo.sensitive = true;
    }
    ~EditWindow() {
        this.b_cancel();
    }
}



public class  Gmpc.TestPlugin : Gmpc.Plugin.Base,Gmpc.Plugin.ToolMenuIface, Gmpc.Plugin.SongListIface {
    private const int[] version = {0,0,2};
    /*********************************************************************************
     * Plugin base functions 
     * These functions are required.
     ********************************************************************************/
    construct {
        /* Mark the plugin as an internal dummy */
        this.plugin_type = 8+4;
    }
    public override unowned int[] get_version() {
        return this.version;
    }
    /**
     * The name of the plugin
     */
    public override unowned string get_name() {
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
        Gtk.TreeIter iter;
        Gtk.TreeView tv = (Gtk.TreeView)item.get_data<string>("treeview");
        Gtk.TreeModel model = tv.get_model();
        var selection = tv.get_selection();
        foreach(unowned Gtk.TreePath path in selection.get_selected_rows(out model))
        {
            if(model.get_iter(out iter, path))
            {
                unowned MPD.Song? song = null;
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

            item.activate.connect(menu_activate_tree);
            return 1;
        }
        return 0;
    }

    public void menu_activated_album(Gtk.MenuItem item)
    {
        unowned MPD.Song song = server.playlist_get_current_song();
        new Gmpc.MetaData.EditWindow(song,Gmpc.MetaData.Type.ALBUM_ART);
    }
    public int tool_menu_integration(Gtk.Menu menu)
    {
        Gtk.MenuItem item = new Gtk.MenuItem.with_label(_("Edit metadata current song"));
        menu.append(item);
        item.activate.connect(menu_activated_album);
        return 2;
    }
}

