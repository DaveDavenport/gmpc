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
using MPD;

private const bool use_transition_gsl = Gmpc.use_transition;
private const string some_unique_name_gsl = Config.VERSION;

public class Gmpc.Widgets.Songlist : Gmpc.Widgets.Qtable
{
    private const int MAX_RESULTS           = 125;
    /* Constructor function  */
    public Songlist()
    {
        this.spacing = 6;
        this.max_columns = 3;

    }
    /**
     * Add a artist entry
     */
    private void add_artist_entry(MPD.Song song, int level=0)
    {
        /* Event box */
        var event = new Gtk.EventBox();
        event.set_visible_window(false);

        var box = new Gtk.HBox(false, 6);
        /* Disc image */
        /* add padding */
        var ali = new Gtk.Alignment(0.0f, 0.5f,0f,0f);
        ali.set_padding(0,0,level*32,0);
        box.pack_start(ali, false, false, 0);

        var image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ARTIST_ART, 32);
        image.set_no_cover_icon("no-artist");
        image.set_loading_cover_icon("fetching-artist");
        image.set_squared(false);
        image.update_from_song(song);
        /* add the image */
        ali.add(image);

        /* Create lLabel */
        string label = null;
        if(song.albumartist != null && song.albumartist.length > 0 )
        {
            label = "%s: %s".printf(_("Artist"), song.albumartist);
        }else{
            label = "%s: %s".printf(_("Artist"), song.artist);
        }
        var wlabel = new Clicklabel(label);
        wlabel.set_do_bold(true);
        wlabel.set_can_focus(true);

        MPD.Song song_file = song.copy();
        wlabel.clicked.connect((source, event) => {
                artist_song_clicked(song_file);
        });
        /* add the label */
        box.pack_start(wlabel, false, false, 0);

        /* Add the entry */
        event.add(box);
        //this.pack_start(event, false, false, 0);
        this.add_header(event);
    }

    /**
     * Add a album entry
     */
    private void add_album_entry(MPD.Song song, int level=0)
    {
        /* Event box */
        var event = new Gtk.EventBox();
        event.set_visible_window(false);

        var box = new Gtk.HBox(false, 6);
        /* Disc image */
        /* add padding */
        var ali = new Gtk.Alignment(0.0f, 0.5f,0f,0f);
        ali.set_padding(0,0,level*32,0);
        box.pack_start(ali, false, false, 0);


        //var image = new Gtk.Image.from_icon_name("media-album", Gtk.IconSize.BUTTON);
        var image = new Gmpc.MetaData.Image(Gmpc.MetaData.Type.ALBUM_ART, 32);
        image.set_squared(false);
        image.update_from_song(song);
        /* add the image */
        ali.add(image);

        /* Create lLabel */
        string label = "%s: %s".printf(_("Album"), song.album);
        var wlabel = new Clicklabel(label);
        wlabel.set_do_bold(true);
        wlabel.set_can_focus(true);

        MPD.Song song_file = song.copy();
        wlabel.clicked.connect((source) => {
                album_song_clicked(song_file);
        });
        /* add the label */
        box.pack_start(wlabel, false, false, 0);

        /* Add the entry */
        event.add(box);
//        this.pack_start(event, false, false, 0);
        this.add_header(event);
    }
    /**
     * Add a disc entry
     */
    private void add_disc_entry(string entry, int level=0)
    {
        GLib.debug("Disc entry add: %s", entry);

        /* */
        var box = new Gtk.HBox(false, 6);

        /* add padding */
        var ali = new Gtk.Alignment(0.0f, 0.5f,0f,0f);
        ali.set_padding(0,0,level*32,0);
        box.pack_start(ali, false, false, 0);

        /* Disc image */
        var image = new Gtk.Image.from_icon_name("media-album", Gtk.IconSize.BUTTON);
        /* add the image */
        ali.add(image);

        /* Create lLabel */
        string label = "<b>%s: %s</b>".printf(_("Disc"), entry);
        var wlabel = new Gtk.Label("");
        wlabel.set_markup(label);

        /* add the label */
        box.pack_start(wlabel, false, false, 0);

        /* Add the entry */
        //this.pack_start(box, false, false, 0);
        this.add_header(box);
    }


    /**
     * Add a song entry
     */

     private void add_song_entry(MPD.Song song, int level=0)
     {
        var event = new Gtk.EventBox();
        event.set_visible_window(false);
        GLib.debug("Song entry add: %s", song.file);

        var box = new Gtk.HBox(false, 6);

        /* add padding */
        var ali = new Gtk.Alignment(0.0f, 0.5f,0f,0f);
        ali.set_padding(0,0,level*32,0);
        box.pack_start(ali, false, false, 0);

        /* Title image */
        var image = new Gtk.Image.from_icon_name("media-audiofile", Gtk.IconSize.MENU);

        event.enter_notify_event.connect((source, event) => {
            image.set_from_stock("gtk-media-play", Gtk.IconSize.MENU);
            return false;

        });

        event.leave_notify_event.connect((source, event) => {
            image.set_from_icon_name("media-audiofile",Gtk.IconSize.MENU);
             return false;
        });

        /* add the image */
        event.add(image);
        ali.add(event);


        MPD.Song song_file = song.copy();
        event.button_release_event.connect((source, event) => {
            if(event.button == 1) {
                play_song_clicked(song_file);
                return true;
                }
            return false;
        });

        /* add title label */
        string label = null;
        if(song.track != null && song.title != null) {
            label = "%02s. %s".printf(song.track, song.title);
        }else if (song.title != null) {
            label = song.title;
        }else {
            label = GLib.Path.get_basename(song.file);
        }
        var wlabel = new Gmpc.Clicklabel(label);
        wlabel.set_can_focus(true);

        /* add the label */
        box.pack_start(wlabel, false, false, 0);

        wlabel.clicked.connect((source, alt) => {
            if(alt)
                play_song_clicked(song_file);
            else
                song_clicked(song_file);
        });

        /* Add the entry */
        //this.pack_start(box, false, false, 0);
        this.add(box);
     }

     /* User click on the title */
     public signal void song_clicked(MPD.Song song);
     /* user clicked on the play */
     public signal void play_song_clicked(MPD.Song song);
     /* user clicked on the album */
     public signal void album_song_clicked(MPD.Song song);
     /* user clicked on the artist */
     public signal void artist_song_clicked(MPD.Song song);

     /**
      * Fill the widget from a song list
      */
     public void set_from_data(owned MPD.Data.Item? list, bool show_album=false, bool show_artist=false)
     {
        int results = 0;
         /* Removing everything it contains */
         foreach(var child in this.get_children()) {
             child.destroy();
         }

         if(list == null) return;

         /* Sort the list so we can display it correctly */
         list.sort_album_disc_track();
         weak MPD.Data.Item iter = list;
         /* itterating items */
         string disc = null;
         string album = null;
         string artist = null;
         /* Itterate over the songs */
         int level = -1;
         for(;iter != null; iter.next(false))
         {
             /* if it is no MPD.Song, skip */
             if(iter.song == null) continue;
             if(results > this.MAX_RESULTS) {
                var bar = new Gtk.InfoBar();
                var label = new Gtk.Label(_("Only the first %i results are shown. Please refine your search.".printf(MAX_RESULTS)));
                label.set_alignment(0.0f,0.5f);
                bar.set_message_type(Gtk.MessageType.WARNING);
                (bar.get_content_area() as Gtk.Container).add(label);

//                this.pack_start(bar, false, false);
                this.add_header(bar);
                break;
             }
             results++;

             if(show_artist) {
                if(iter.song.albumartist != null &&
                        iter.song.albumartist.length > 0)
                {
                    if(artist == null || artist != iter.song.albumartist)
                    {
                        this.add_artist_entry(iter.song,0);
                        artist = iter.song.albumartist;
                        disc = null;
                        album = null;
                    }
                }else if(iter.song.artist != null && (artist == null || artist != iter.song.artist))
                {
                    this.add_artist_entry(iter.song,0);
                    artist = iter.song.artist;
                    disc = null;
                    album = null;
                }
             }
             if(show_album) {
                if(iter.song.album != null && (album == null || album != iter.song.album))
                {
                    this.add_album_entry(iter.song,(artist != null)?1:0);
                    album = iter.song.album;
                    disc = null;
                }
             }
             /* Check for a new disc */
             if(iter.song.disc != null && (disc == null  || disc != iter.song.disc))
             {
                 this.add_disc_entry(iter.song.disc,((artist != null)?1:0)+((album!=null)?1:0));
                 /* Add a  new disc button */
                 disc = iter.song.disc;
             }
             /* add song row */
             level = 0;
             if(disc != null) level++;
             if(artist != null) level++;
             if(album != null) level++;
             this.add_song_entry(iter.song,level);
         }

         /* show everything */
         this.show_all();
     }
}
