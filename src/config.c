#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glade/glade.h>

#include "libmpdclient.h"
#include "main.h"
#define CONFIG ".gmpc.cfg"

void load_config ()
{
	gchar *filename = g_strdup_printf ("%s/%s", g_getenv ("HOME"), CONFIG);
	FILE *fp;
	gchar buffer[1024];
	fp = fopen (filename, "r");
	if (fp == NULL)
	{
		g_free (filename);
		return;
	}
	memset (buffer, '\0', 1024);
	while (fgets (buffer, 1024, fp))
	{

		if (!strncmp (buffer, "markup main display:", 20))
		{
			gchar *buf = g_strstrip (&buffer[20]);
			if (buf != NULL)
			{
				if (preferences.markup_main_display != NULL)
				{
					g_free (preferences.markup_main_display);
				}
				preferences.markup_main_display = g_strcompress(buf);
			}
		}
/*		else if (!strncmp (buffer, "markup playlist:", 16))
		{
			gchar *buf = g_strstrip (&buffer[16]);
			if (buf != NULL)
			{
				if (preferences.markup_playlist != NULL)
				{
					g_free (preferences.markup_playlist);
				}
				preferences.markup_playlist = g_strcompress(buf);
			}
		}
*/		else if (!strncmp (buffer, "markup song browser:", 20))
		{
			gchar *buf = g_strstrip (&buffer[20]);
			if (buf != NULL)
			{
				if (preferences.markup_song_browser != NULL)
				{
					g_free (preferences.markup_song_browser);
				}
				preferences.markup_song_browser = g_strcompress (buf);
			}
		}
		else if (!strncmp (buffer, "xiph_url:", 9))
		{
			gchar *buf = g_strstrip (&buffer[9]);
			if (buf != NULL)
			{
				if (info.xiph_url != NULL)
				{
					g_free (info.xiph_url);
				}
				info.xiph_url = g_strcompress (buf);
			}
		}
		else if (!strncmp (buffer, "online stream url:", 18))
		{
			gchar *buf = g_strstrip (&buffer[18]);
			if (buf != NULL)
			{
				info.online_streams++;
				info.online_stream_list = g_realloc(info.online_stream_list, info.online_streams*sizeof(gchar *));
				info.online_stream_list[info.online_streams-1] = g_strcompress (buf);
			}
		}                                                   		
		memset (buffer, '\0', 1024);

	}
	fclose (fp);
	g_free (filename);
}

void save_config ()
{
	gchar *filename = g_strdup_printf ("%s/%s", g_getenv ("HOME"), CONFIG);
	FILE *fp;
	char *escaped;
	int i = 0;

	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		g_free (filename);
		return;
	}
	escaped = g_strescape(preferences.markup_main_display, "");
	fprintf (fp, "markup main display: %s\n", escaped);
	g_free(escaped);
/*	escaped = g_strescape(preferences.markup_playlist, "");
	fprintf (fp, "markup playlist: %s\n", escaped);
	g_free(escaped);
*/	escaped = g_strescape(preferences.markup_song_browser, "");
	fprintf (fp, "markup song browser: %s\n", escaped);
	g_free(escaped);
	escaped = g_strescape(info.xiph_url, "");
	fprintf(fp, "xiph_url: %s\n", escaped);
	g_free(escaped);
	for(i=0; i < info.online_streams;i++)
	{
		escaped = g_strescape(info.online_stream_list[i], "");
		fprintf(fp, "online stream url: %s\n", escaped);
		g_free(escaped);

	}
	fclose (fp);
	g_free (filename);
}
