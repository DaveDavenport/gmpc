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


		if (!strncmp (buffer, "timeout:", 8))
		{
			preferences.timeout = (float) g_strtod (&buffer[8], NULL);
		}
		else if (!strncmp (buffer, "auto-connect:", 13))
		{
			preferences.autoconnect = atoi (&buffer[13]);
		}
		else if (!strncmp (buffer, "do tray:", 8))
		{
			gchar *buf = g_strstrip (&buffer[8]);
			if (buf != NULL)
			{
				info.do_tray = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "do tray popup:", 14))
		{
			gchar *buf = g_strstrip (&buffer[14]);
			if (buf != NULL)
			{
				info.do_tray_popup = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "time format:", 12))
		{
			gchar *buf = g_strstrip (&buffer[12]);
			if (buf != NULL)
			{
				info.time_format = atoi (buf);
			}
		}


		else if (!strncmp (buffer, "do popup:", 9))
		{
			gchar *buf = g_strstrip (&buffer[9]);
			if (buf != NULL)
			{
				info.popup.do_popup = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "popup pos:", 10))
		{
			gchar *buf = g_strstrip (&buffer[10]);
			if (buf != NULL)
			{
				info.popup.position = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "popup stay:", 11))
		{
			gchar *buf = g_strstrip (&buffer[11]);
			if (buf != NULL)
			{
				info.popup.popup_stay = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "popup state:", 12))
		{
			gchar *buf = g_strstrip (&buffer[12]);
			if (buf != NULL)
			{
				info.popup.show_state = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "popup timeout:", 14))
		{
			gchar *buf = g_strstrip (&buffer[14]);
			if (buf != NULL)
			{
				info.popup.timeout = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "use auth:", 9))
		{
			gchar *buf = g_strstrip (&buffer[9]);
			if (buf != NULL)
			{
				preferences.user_auth = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "auth pass:", 10))
		{
			gchar *buf = g_strstrip (&buffer[10]);
			if (buf != NULL)
			{
				memset (preferences.password, '\0', 256);
				strncpy (preferences.password, buf, MIN (strlen (buf), 256));
			}
		}

		else if (!strncmp (buffer, "markup main display:", 20))
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
		else if (!strncmp (buffer, "markup playlist:", 16))
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
		else if (!strncmp (buffer, "markup song browser:", 20))
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
		else if (!strncmp (buffer, "pl2 tooltip timeout:", 20))
		{
			gchar *buf = g_strstrip (&buffer[20]);
			if (buf != NULL)
			{
				info.pl2_tooltip = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "pl3_scroll_to_open:", 19))
		{
			gchar *buf = g_strstrip (&buffer[19]);
			if (buf != NULL)
			{
				preferences.pl3_scroll_to_open = atoi (buf);
			}
		}


		else if (!strncmp (buffer, "pl2 do tooltip:", 15))
		{
			gchar *buf = g_strstrip (&buffer[15]);
			if (buf != NULL)
			{
				info.pl2_do_tooltip = atoi (buf);
			}
		}
		else if (!strncmp (buffer, "rounded corners:", 16))
		{
			gchar *buf = g_strstrip (&buffer[16]);
			if (buf != NULL)
			{
				info.rounded_corners = atoi (buf);
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
	fprintf (fp, "timeout: %.2f\n", preferences.timeout);
	fprintf (fp, "auto-connect: %i\n", preferences.autoconnect);
	fprintf (fp, "do tray: %i\n", info.do_tray);
	fprintf (fp, "do tray popup: %i\n", info.do_tray_popup);
	fprintf (fp, "time format: %i\n", info.time_format);
	fprintf (fp, "do popup: %i\n", info.popup.do_popup);
	fprintf (fp, "popup pos: %i\n", info.popup.position);
	fprintf (fp, "popup stay: %i\n", info.popup.popup_stay);
	fprintf (fp, "popup state: %i\n", info.popup.show_state);
	fprintf (fp, "popup timeout: %i\n", info.popup.timeout);
	fprintf (fp, "use auth: %i\n", preferences.user_auth);
	fprintf (fp, "auth pass: %s\n", preferences.password);
	escaped = g_strescape(preferences.markup_main_display, "");
	fprintf (fp, "markup main display: %s\n", escaped);
	g_free(escaped);
	escaped = g_strescape(preferences.markup_playlist, "");
	fprintf (fp, "markup playlist: %s\n", escaped);
	g_free(escaped);
	escaped = g_strescape(preferences.markup_song_browser, "");
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
	fprintf (fp, "pl2 do tooltip: %i\n", info.pl2_do_tooltip);
	fprintf (fp, "pl2 tooltip timeout: %i\n", info.pl2_tooltip);	
	fprintf (fp, "rounded corners: %i\n", info.rounded_corners);
	fprintf (fp, "pl3_scroll_to_open: %i\n", preferences.pl3_scroll_to_open);
	fclose (fp);
	g_free (filename);
}
