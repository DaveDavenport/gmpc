
/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2012 Qball Cow <qball@gmpclient.org>
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
using Gmpc.Plugin;

private const bool use_transition_arp = Gmpc.use_transition;
private const string some_unique_name_arp = Config.VERSION;

public class  Gmpc.Tools.AutoResetPriority : Gmpc.Plugin.Base
{
    private const int[] version = {0,0,2};

    public override unowned int[] get_version()
    {
        return this.version;
    }

    public override unowned string get_name()
    {
        return _("Auto reset priority");
    }

    construct
    {
        /* Mark the plugin as an internal dummy */
        this.plugin_type = 8+4;
        /* Attach status changed signal */
        gmpcconn.status_changed.connect(status_changed);
    }


    private void status_changed(Connection conn, MPD.Server server, MPD.Status.Changed what)
    {
        if(!this.get_enabled()) return;
        if((what&MPD.Status.Changed.SONGID) == MPD.Status.Changed.SONGID)
        {
            weak MPD.Song? song =  server.playlist_get_current_song();
            if(song != null) {
                if(song.priority > 0 ) {
                    MPD.PlayQueue.set_priority(server, song.id, 0);  
                }
            }
        }
    }
}
