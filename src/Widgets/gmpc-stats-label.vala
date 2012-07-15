/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2012 Qball Cow <qball@gmpclient.org>
 * Copyright (C) 2012 Quentin "Sardem FF7" Glidic
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

private const string some_unique_name_slb = Config.VERSION;

namespace Gmpc.MetaData
{
    class StatsLabel : Gtk.Label
    {
        public enum Type
        {
            ARTIST_NUM_SONGS,
            ARTIST_PLAYTIME_SONGS,
            ARTIST_GENRES_SONGS,
            ARTIST_DATES_SONGS,
            ALBUM_NUM_SONGS,
            ALBUM_PLAYTIME_SONGS,
            ALBUM_GENRES_SONGS,
            ALBUM_DATES_SONGS
        }

        private Type ltype = Type.ARTIST_NUM_SONGS;
        private MPD.Song song;
        private ulong ll_signal = 0;

        private
        bool
        idle_handler()
        {
            switch(this.ltype)
            {
                case Type.ARTIST_NUM_SONGS:
                case Type.ALBUM_NUM_SONGS:
                    MPD.Database.search_stats_start(server);
                    MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, this.song.artist);
                    if(this.ltype == Type.ALBUM_NUM_SONGS)
                    {
                        MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, this.song.album);
                    }
                    var stats = MPD.Database.search_stats_commit(server);
                    if(stats != null)
                    {
                        this.set_text(stats.numberOfSongs.to_string());
                    }
                    else
                    {
                        this.set_text("");
                    }
                    break;
                case Type.ARTIST_PLAYTIME_SONGS:
                case Type.ALBUM_PLAYTIME_SONGS:
                    MPD.Database.search_stats_start(server);
                    MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, this.song.artist);
                    if(this.ltype ==Type. ALBUM_PLAYTIME_SONGS)
                    {
                        MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, this.song.album);
                    }
                    var stats = MPD.Database.search_stats_commit(server);
                    if(stats != null)
                    {
                        this.set_text(Misc.format_time(stats.playTime));
                    }
                    else
                    {
                        this.set_text("");
                    }
                    break;
                case Type.ARTIST_GENRES_SONGS:
                case Type.ALBUM_GENRES_SONGS:
                    MPD.Database.search_field_start(server, MPD.Tag.Type.GENRE);
                    MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, this.song.artist);
                    if(this.ltype == Type.ALBUM_GENRES_SONGS)
                    {
                        MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, this.song.album);
                    }
                    string[] text = {};
                    for(var data = MPD.Database.search_commit(server); data != null; data.next_free())
                    {
                        if(data.tag.length > 0)
                            text += data.tag;
                    }
                    if(text.length >0)
                    {
                        this.set_text(string.joinv(", ", text));
                    }
                    else
                    {
                        this.set_text(_("n/a"));
                    }
                    break;
                case Type.ARTIST_DATES_SONGS:
                case Type.ALBUM_DATES_SONGS:
                    MPD.Database.search_field_start(server, MPD.Tag.Type.DATE);
                    MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, this.song.artist);
                    if(this.ltype == Type.ALBUM_GENRES_SONGS)
                    {
                        MPD.Database.search_add_constraint(server, MPD.Tag.Type.ALBUM, this.song.album);
                    }
                    string[] text = {};
                    for(var data = MPD.Database.search_commit(server); data != null; data.next_free())
                    {
                        if(data.tag.length > 0)
                            text += data.tag;
                    }
                    if(text.length >0)
                    {
                        this.set_text(string.joinv(", ", text));
                    }
                    else
                    {
                        this.set_text(_("n/a"));
                    }
                    break;
            }

            return false;
        }

        /* This implements lazy loading, the data will only be fetched when the widget is visible for the first time. */
        private
        bool
        expose_event_handler(Gdk.EventExpose event)
        {
            var text = GLib.Markup.printf_escaped("<i>%s</i>", _("Loading"));
            this.set_markup(text);
            this.idle_handler();
            if(this.ll_signal != 0)
            {
                GLib.SignalHandler.disconnect(this, this.ll_signal);
                this.ll_signal = 0;
            }
            return false;
        }

        /* Constructor */
        public
        StatsLabel(Type ltype, MPD.Song song)
        {
            this.song = song.copy();
            this.ltype = ltype;
            /* Set expose signal, so I know when it is visible for the first time */
            this.ll_signal = this.expose_event.connect(this.expose_event_handler);
        }
    }
}
