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
using Gmpc;
using Gmpc.Plugin;

/* Needed to solve some vala odities */
private const bool use_transition_ppclyr = Gmpc.use_transition;
private const string some_unique_name_ppclyr = Config.VERSION;

/* Log domain, do gmpc gmpc --log-filter=Gmpc.Provider.Chartlyrics to see this output */
private const string log_domain_ppclyr = "Gmpc.Provider.ChartLyrics";

/**
 * Plugin implements the Base class and the Metadata interface
 */
public class Gmpc.Provider.ChartLyrics: Gmpc.Plugin.Base,Gmpc.Plugin.MetaDataIface
{

    /**
     * Gmpc.Plugin.Base
     */
    private const int[] version = {0,0,2};

    /** Return the plugin version. For an internal plugin this is not that interresting.
     * But we implement it anyway 
     */
    public override unowned int[] get_version()
    {
        return this.version;
    }
    
    /**
     * The name of the plugin 
     */
    public override unowned string get_name()
    {
        return N_("ChartLyrics Plugin");
    }

    /**
     * Constructor
     */
    construct
    {
        // Not needed, but lets do it anyway. This means metadata provider
        // and internal plugin.
        this.plugin_type = 8+32;
    }

    /**
     * Gmpc.Plugin.MetaDataIface
     */
    /**
     * Priority of the plugin, default 50
     */
    public void set_priority(int priority)
    {
        config.set_int(this.get_name(),"priority",priority);
    }

    public int get_priority()
    {
        return config.get_int_with_default(this.get_name(),"priority",50);
    }

    const string query = "http://api.chartlyrics.com/apiv1.asmx/SearchLyricDirect?artist=%s&song=%s";

    public void get_metadata (MPD.Song song,
        Gmpc.MetaData.Type type,
        MetaDataCallback callback)
    {
        /* Check request type */
        if(type != Gmpc.MetaData.Type.SONG_TXT) {
            /* Signal that we do not find anything */
            callback(null);
            return;
        }
        /* Check if we have enough metadata to fetch lyrics */
        if(song == null || song.artist == null || song.title == null)
        {
            log(log_domain_ppclyr, GLib.LogLevelFlags.LEVEL_DEBUG,
                "Insufficient information. doing nothing");
            /* Tell that we found nothing */
            callback(null);
            return;
        }
        /* intergrate fetcher here */
        Prop *p = new Prop();
        p->this = this;
        p->song = song;
        p->callback = callback;
        var path = query.printf(Gmpc.AsyncDownload.escape_uri(song.artist),
            Gmpc.AsyncDownload.escape_uri(song.title));
        log(log_domain_ppclyr, GLib.LogLevelFlags.LEVEL_DEBUG,
            "Query song txt: %s ", path);
        Gmpc.AsyncDownload.download_vala(path, p, this.handle_chart_download);
    }

    /** 
     * Parse the result 
     */
   private void parse_data(Prop *p, uchar[] data)
   {

       Xml.Doc *doc = Xml.Parser.parse_memory((string)data, data.length); 
       /* nothing found */
       if(doc == null) return;

       Xml.Node *root = doc->get_root_element();
       if(root != null)
       {
           for(Xml.Node *child = root->children; 
                   child != null ; 
                   child = child->next)
           {
                /* first song is parsed, then lyric */
                if(child->name == "LyricSong")
                {
                    string tsong = child->get_content();
                    tsong = tsong.down();
                    string ot_song = p->song.title.down();
                    if(ot_song.collate(tsong) != 0) {
                        log(log_domain_ppclyr, GLib.LogLevelFlags.LEVEL_DEBUG,
                                "Skipping: %s-%s ",tsong, ot_song);
                        /* skip this result*/
                        return;
                    }

                }
                if(child->name == "Lyric")
                {
                    string lyric = child->get_content();
                    /* it returns empty lyrics when no hit, so catch that */
                    if(lyric.length > 0) 
                    {
                        /* Create a new metadata item, of th right type
                         * and add it to the list */
                        MetaData.Item pitem = new MetaData.Item();
                        pitem.type = Gmpc.MetaData.Type.SONG_TXT;
                        pitem.plugin_name = get_name();
                        pitem.content_type = MetaData.ContentType.TEXT;
                        pitem.set_text(lyric);
                        p->list.append((owned)pitem);
                    }
                }
           }
       }
   }
    /**
     * This function handles the callback from Gmpc.AsyncDownload 
     */
    private void handle_chart_download(Gmpc.AsyncDownload.Handle handle,
        Gmpc.AsyncDownload.Status status,
        void *d)
    {
        Prop *p = (Prop *)d;
        if(status == Gmpc.AsyncDownload.Status.DONE)
        {
            var data = handle.get_data();
            /* If there is results parse it */
            if(data != null)
                parse_data(p, data);

            log(log_domain_ppclyr, GLib.LogLevelFlags.LEVEL_DEBUG,
                "Download done: results: %u ", p->list.length());

            /* Call the callback with the results,
             * this will take over the reference
             * of the result list, and takes care of freeing it
             */
            p->callback((owned)(p->list));
            delete p;
        }
        else if (status == Gmpc.AsyncDownload.Status.PROGRESS)
        {
            // do nothing when downloading
        }
        else
        {
            /* Nothying found, or error, so return this and cleanup */
            p->callback(null);
            /* delete Prop */
            delete p;
        }
    }
    /**
     * This class is used to go around the limitations of
     *  GmpcEasyAsyncDownload (it is no GLibSimpleAsync implementation.)
     */
    [Compact]
    private class Prop
    {
        public MPD.Song song;
        public ChartLyrics this;
        public MetaDataCallback callback;
        public List<MetaData.Item> list = null;
    }
}
