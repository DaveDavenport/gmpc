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

private const bool use_transition_ppldb = Gmpc.use_transition;
private const string some_unique_name_ppldb = Config.VERSION;
private const string log_domain_ppldb = "Gmpc.Provider.LyrDB";

public class Gmpc.Provider.LyrDB: Gmpc.Plugin.Base,Gmpc.Plugin.MetaDataIface
{
	private const int[] version = {0,0,2};

	public override unowned int[] get_version()
	{
		return this.version;
	}

	public override unowned string get_name()
	{
		return N_("Lyrics DB Plugin");
	}

	construct
	{
		this.plugin_type = 8+32;
	}

	public void set_priority(int priority)
	{
		config.set_int(this.get_name(),"priority",priority);
	}

	public int get_priority()
	{
		return config.get_int_with_default(this.get_name(),"priority",60);
	}


	public void get_metadata (MPD.Song song,
		Gmpc.MetaData.Type type,
		MetaDataCallback callback)
	{
		if(song == null || song.artist == null || song.title == null)
		{
			log(log_domain_ppldb, GLib.LogLevelFlags.LEVEL_DEBUG,
				"Insufficient information. doing nothing");
			/* Tell that we found nothing */
			callback(null);
			return;
		}
		switch(type)
		{
			case Gmpc.MetaData.Type.SONG_TXT:
				/* A request for artist art came in. */
				this.get_song_txt(song, callback);
				return;
			case  Gmpc.MetaData.Type.ALBUM_ART:
			case  Gmpc.MetaData.Type.ARTIST_ART:
			case  Gmpc.MetaData.Type.ALBUM_TXT:
			case  Gmpc.MetaData.Type.ARTIST_TXT:
			case  Gmpc.MetaData.Type.ARTIST_SIMILAR:
			case  Gmpc.MetaData.Type.SONG_SIMILAR:
			case  Gmpc.MetaData.Type.GENRE_SIMILAR:
			case  Gmpc.MetaData.Type.SONG_GUITAR_TAB:
			case  Gmpc.MetaData.Type.QUERY_DATA_TYPES:
			case  Gmpc.MetaData.Type.QUERY_NO_CACHE:
			default:
				break;
		}

		/* Tell what we found */
		callback(null);
	}

	/* HT Backdrops specific code */
	const string query = "http://webservices.lyrdb.com/lookup.php?q=%s|%s&for=match&agent=gmpc-lyrdb";
	const string download_url = "http://webservices.lyrdb.com/getlyr.php?q=%s";

    private string __convert_raw_data(string raw_data)
    {
            string data = null;
			if(!(raw_data.validate()))
			{
				log(log_domain_ppldb, GLib.LogLevelFlags.LEVEL_WARNING,
						"Input is invalid utf-8, try converting");

				try
				{
					data = GLib.convert ((string)raw_data, -1,
						"UTF-8",  // to
						"ISO-8859-15", //from
						null,null);
				}catch (GLib.Error e)
				{
					log(log_domain_ppldb, GLib.LogLevelFlags.LEVEL_WARNING,
						"Unable to convert input data: %s", e.message);
				}
			}
			else
			{
				data = (string)raw_data;
			}
			if(data != null) {
			    data.chomp();
   			    /* Replace the \r\n by \n */
    			data.replace("\r\n", "\n");
 
			}
        return data;
    }

	private void handle_lyrics_download(Gmpc.AsyncDownload.Handle handle,
		Gmpc.AsyncDownload.Status status,
		void *d)
	{
		Prop *p = (Prop *)d;
		if(status == AsyncDownload.Status.PROGRESS)
		{
			return;
		}
		else if(status == AsyncDownload.Status.DONE)
		{
			unowned string raw_data = handle.get_data_as_string();
			var data = __convert_raw_data(raw_data);
            if(data != null)
            {
    			MetaData.Item pitem = new MetaData.Item();
	    		pitem.type = Gmpc.MetaData.Type.SONG_TXT;
	    		pitem.plugin_name = get_name();
	    		pitem.content_type = MetaData.ContentType.TEXT;
	    		pitem.set_text((string)data);
	    		p->list.append((owned)pitem);

	    		log(log_domain_ppldb, GLib.LogLevelFlags.LEVEL_DEBUG,
	    			"Added item to the list");
   			}
		}
		download_lyrics(p);
	}
	private  void download_lyrics(Prop *p)
	{
		/* still items to download, download */
		if(p->results.length > 0 )
		{
			string path = p->results.pop_tail();
				log(log_domain_ppldb, GLib.LogLevelFlags.LEVEL_DEBUG,
	    			"Added Downloading: '%s'", path);
   			
			Gmpc.AsyncDownload.download_vala(path,
				p,
				handle_lyrics_download);
		}else
		{
			p->callback((owned)(p->list));
			delete p;
		}
	}

	private void parse_data(Prop *p, string raw_data)
	{
		string data = __convert_raw_data(raw_data);
		if(data == null) return;
		string[] splitted = ((string)data).split("\n");
		foreach(string entry in splitted)
		{
			string[] fields = entry.split("\\");
			if(fields.length == 3)
			{
				string path = download_url.printf(fields[0]);
				p->results.push_head((owned)path);
			}
		}
	}

	private void handle_download(Gmpc.AsyncDownload.Handle handle,
		Gmpc.AsyncDownload.Status status,
		void *d)
	{
		Prop *p = (Prop *)d;
		if(status == Gmpc.AsyncDownload.Status.DONE)
		{
			unowned string data = handle.get_data_as_string();
			if(data != null)
				parse_data(p, data);

			log(log_domain_ppldb, GLib.LogLevelFlags.LEVEL_DEBUG,
				"Download done: results: %u ", p->results.length);
			if(p->results.length == 0)
			{
				p->callback(null);
				/* Cleanup */
				delete p;
			}
			else
			{
				/* Download the lyrics */
				download_lyrics(p);
			}
		}
		else if (status == Gmpc.AsyncDownload.Status.PROGRESS)
		{
			// do nothing when downloading
		}
		else
		{
			/* Nothying found, or error, so cleanup */
			p->callback(null);
			/* delete Prop */
			delete p;
		}
	}
	/**
	 * Get artist art
	 */
	private void get_song_txt(MPD.Song song, MetaDataCallback callback)
	{

		Prop *p = new Prop();
		p->this = this;
		p->song = song;
		p->callback = callback;
		var path = query.printf(Gmpc.AsyncDownload.escape_uri(song.artist),
			Gmpc.AsyncDownload.escape_uri(song.title));
		log(log_domain_ppldb, GLib.LogLevelFlags.LEVEL_DEBUG,
			"Query song txt: %s ", path);
		Gmpc.AsyncDownload.download_vala(path, p, handle_download);
	}
	/**
	 * This class is used to go around the limitations of
	 *  GmpcEasyAsyncDownload (it is no GLibSimpleAsync implementation.)
	 */
	[Compact]
    private class Prop
	{
		public MPD.Song song;
		public LyrDB this;
		public MetaDataCallback callback;
		public Queue<string> results = new Queue<string>();
		public List<MetaData.Item> list = null;
	}
}
