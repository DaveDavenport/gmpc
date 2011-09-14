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

private const bool use_transition_mdsa = Gmpc.use_transition;
private const string some_unique_name_mdsa = Config.VERSION;

public class Gmpc.MetaData.Widgets.SimilarArtists : Gtk.Table
{
    private MPD.Song? song = null;
    private int columns = 1;
    private int button_width = 200;
    private void size_changed(Gdk.Rectangle alloc)
    {
		int t_column = alloc.width/button_width;
        t_column = (t_column < 1)?1:t_column;
        if(t_column != columns )
		{
			var list = this.get_children();
			foreach(Gtk.Widget child in list) {
				child.ref();
				this.remove(child);
			}

			columns = t_column;
			int i = 0;

			this.resize(list.length()/columns+1, columns);
			foreach(Gtk.Widget item in list)
			{
				this.attach(item, 
						i%columns,i%columns+1,i/columns,i/columns+1,
						Gtk.AttachOptions.EXPAND|Gtk.AttachOptions.FILL,
						Gtk.AttachOptions.SHRINK, 0,0);
				i++;
			}
			this.show_all();
		}

    } 

    /**
     * Handle signals from the metadata object.
     */
    private void metadata_changed(MetaWatcher gmw2, 
            MPD.Song song, 
            Gmpc.MetaData.Type type, 
            Gmpc.MetaData.Result result, 
            Gmpc.MetaData.Item? met)
    {
        /* only listen to the same artist and the same type */
        if(type != Gmpc.MetaData.Type.ARTIST_SIMILAR) return;
        if(this.song.artist.collate(song.artist)!=0) return;

        /* clear widgets */
        var child_list = this.get_children();
        foreach(Gtk.Widget child in child_list)
        {
            child.destroy();
        }

        /* if unavailable set that in a label*/
        if(result == Gmpc.MetaData.Result.UNAVAILABLE || met.is_empty() || !met.is_text_list())
        {
            var label = new Gtk.Label(_("Unavailable"));
            this.attach(label, 0,1,0,1,Gtk.AttachOptions.SHRINK, Gtk.AttachOptions.SHRINK, 0,0);
        }
        /* if fetching set that in a label*/
        else if(result == Gmpc.MetaData.Result.FETCHING){
            var label = new Gtk.Label(_("Fetching"));
            this.attach(label, 0,1,0,1,Gtk.AttachOptions.SHRINK, Gtk.AttachOptions.SHRINK, 0,0);
        
        }
        /* Set result */
        else {
            List<Gtk.Widget> in_db_list = null;
            GLib.List<unowned string> list = met.get_text_list().copy();
            list.sort((GLib.CompareFunc)string.collate);


            int items = 30;
            int i = 0;
            if(list != null)
            {
                unowned List<unowned string> liter= null;                 
                MPD.Database.search_field_start(server, MPD.Tag.Type.ARTIST);
                var data = MPD.Database.search_commit(server);

                int q =0;

                if(data != null)
                {

                    data.sort_album_disc_track();
                    unowned MPD.Data.Item iter = data.get_first();

                    liter = list.first();
                    string artist = "";
                    if(iter.tag.validate() == false) {
                        error("Failed to validate"); 
                    }
                    if(iter.tag != null) 
                        artist = iter.tag.casefold(); 
                    do{
                        var res = liter.data.casefold().collate(artist);
                        q++;
                        if(res == 0)
                        {
                            in_db_list.prepend(new_artist_button(iter.tag, true));
                            i++;
                            var d = liter.data;
                            liter = liter.next;
                            list.remove(d);
                            //liter = null;
                            iter.next(false);
                            if(iter != null)
                                artist = iter.tag.casefold();
                        }
                        else if (res > 0) {
                            //list.remove(liter.data);

                            iter.next(false);
                            if(iter != null)
                                artist = iter.tag.casefold(); 
                        }
                        else {
                            liter = liter.next;
                        }
                    }while(iter != null && liter != null && i < items);
                }

                liter= list.first();
                while(liter != null && i < items) 
                {
                    var artist = liter.data;
                    in_db_list.prepend(new_artist_button(artist, false));
                    i++;
                    liter = liter.next; 
                }

            }
            in_db_list.reverse();
            i=0;
            this.hide();
            uint llength = in_db_list.length();
            columns = this.allocation.width/button_width;
	    columns = (columns < 1)?1:columns;
            this.resize(llength/columns+1, columns);
            foreach(Gtk.Widget item in in_db_list)
            {
                this.attach(item, 
                        i%columns,i%columns+1,i/columns,i/columns+1,
                        Gtk.AttachOptions.EXPAND|Gtk.AttachOptions.FILL,
                        Gtk.AttachOptions.SHRINK, 0,0);
                i++;
            }
        }

        this.show_all();
    }
    private
    void
    artist_button_clicked(Gtk.Button button)
    {
        unowned string artist = (string)button.get_data<string>("artist");
        Gmpc.Browser.Metadata.show_artist(artist);
    }
    public
    Gtk.Widget
    new_artist_button(string artist, bool in_db)
    {
        var hbox = new Gtk.HBox(false, 6);
        hbox.border_width = 4;
/*
        var event = new Gtk.Frame(null);
        */

        var event = new Gtk.EventBox();
        event.app_paintable = true;
        event.set_visible_window(true);
        event.expose_event.connect(Gmpc.Misc.misc_header_expose_event);
        event.set_size_request(button_width-20,60);

        var image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ARTIST_ART, 48);
        image.set_no_cover_icon("no-artist");
        image.set_loading_cover_icon("fetching-artist");
        MPD.Song song = new MPD.Song();
        song.artist = artist;
        image.set_squared(true);
        image.update_from_song_delayed(song);
        hbox.pack_start(image,false,false,0);

        var label = new Gtk.Label(artist);
        label.set_tooltip_text(artist);
        label.set_selectable(true);
        label.set_alignment(0.0f, 0.5f);
        label.ellipsize = Pango.EllipsizeMode.END; 
        hbox.pack_start(label,true,true,0);

        if(in_db)
        {
            var find = new Gtk.Button();
            find.add(new Gtk.Image.from_stock("gtk-find", Gtk.IconSize.MENU));
            find.set_relief(Gtk.ReliefStyle.NONE);
            hbox.pack_start(find,false,false,0);

            find.set_data_full("artist",(void *)"%s".printf(artist), (GLib.DestroyNotify) g_free);
            find.clicked.connect(artist_button_clicked);
        }

        event.add(hbox);
        return event;
    }
    public bool first_show_b = false;

    public void first_show()
    {
        GLib.log(np2_LOG_DOMAIN, GLib.LogLevelFlags.LEVEL_DEBUG, "First Show()");

        if(!first_show_b)
        {
            MetaData.Item item = null;
            metawatcher.data_changed.connect(metadata_changed);
            this.size_allocate.connect(size_changed);

            Gmpc.MetaData.Result gm_result = metawatcher.query(song, Gmpc.MetaData.Type.ARTIST_SIMILAR,out item);
            if(gm_result == Gmpc.MetaData.Result.AVAILABLE)
            {
                this.metadata_changed(metawatcher, this.song, Gmpc.MetaData.Type.ARTIST_SIMILAR, gm_result, item); 
            }
            first_show_b = true;
        }
    }
    public SimilarArtists(MPD.Server server, MPD.Song song)
    {
        this.song = song.copy();

        this.set_homogeneous(true);

        this.set_row_spacings(6);
        this.set_col_spacings(6);
        this.show();

    }
}
