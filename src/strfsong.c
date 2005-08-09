/* 
 * $Id: strfsong.c 1502 2004-06-15 15:32:18Z kaw $
 *
 * Based on mpc's songToFormatedString modified for glib and ncmpc
 *
 *
 * (c) 2003-2004 by normalperson and Warren Dukes (shank@mercury.chem.pitt.edu)
 *              and Daniel Brown (danb@cs.utexas.edu)
 *              and Kalle Wallin (kaw@linux.se)
 *              and Qball Cow (Qball@qballcow.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include "misc.h"
#include <libmpd/libmpdclient.h>
#include "strfsong.h"

static gchar * skip(gchar * p) 
{
	gint stack = 0;

	while (*p != '\0') {
		if(*p == '[') stack++;
		if(*p == '#' && p[1] != '\0') {
			/* skip escaped stuff */
			++p;
		}
		else if(stack) {
			if(*p == ']') stack--;
		}
		else {
			if(*p == '&' || *p == '|' || *p == ']') {
				break;
			}
		}
		++p;
	}

	return p;
}

static gsize _strfsong(gchar *s, 
		gsize max, 
		const gchar *format, 
		mpd_Song *song, 
		gchar **last)
{
	gchar *p, *end;
	gchar *temp;
	gsize n, length = 0;
	gint i;
	gboolean found = FALSE;

	memset(s, 0, max);
	if( song==NULL )
		return 0;

	for( p=(gchar *) format; *p != '\0' && length<max; )
	{
		/* OR */
		if (p[0] == '|') 
		{
			++p;
			if(!found) 
			{
				memset(s, 0, max);
				length = 0;
			}
			else 
			{
				p = skip(p);
			}
			continue;
		}

		/* AND */
		if (p[0] == '&') 
		{
			++p;
			if(!found) 
			{
				p = skip(p);
			}
			else 
			{
				found = FALSE;
			}
			continue;
		}

		/* EXPRESSION START */
		if (p[0] == '[')
		{
			temp = g_malloc0(max);
			if( _strfsong(temp, max, p+1, song, &p) >0 )
			{
				strncat(s, temp, max-length);
				length = strlen(s);
				found = TRUE;
			}
			g_free(temp);
			continue;
		}

		/* EXPRESSION END */
		if (p[0] == ']')
		{
			if(last) *last = p+1;
			if(!found && length) 
			{
				memset(s, 0, max);
				length = 0;
			}
			return length;
		}

		/* pass-through non-escaped portions of the format string */
		if (p[0] != '#' && p[0] != '%' && length<max)
		{
			strncat(s, p, 1);
			length++;
			++p;
			continue;
		}

		/* let the escape character escape itself */
		if (p[0] == '#' && p[1] != '\0' && length<max)
		{
			strncat(s, p+1, 1);
			length++;
			p+=2;
			continue;
		}

		/* advance past the esc character */

		/* find the extent of this format specifier (stop at \0, ' ', or esc) */
		temp = NULL;
		end  = p+1;
		while(*end >= 'a' && *end <= 'z')
		{
			end++;
		}
		n = end - p + 1;
		if(*end != '%')
			n--;
		else if (memcmp("%file%", p, n) == 0)
			temp = g_strdup(song->file);
		else if (memcmp("%artist%", p, n) == 0)
			temp = song->artist ? g_strdup(song->artist) : NULL;
		else if (memcmp("%title%", p, n) == 0)
			temp = song->title ? g_strdup(song->title) : NULL;
		else if (memcmp("%album%", p, n) == 0)
			temp = song->album ? g_strdup(song->album) : NULL;
		else if (memcmp("%track%", p, n) == 0)
			temp = song->track ? g_strdup(song->track) : NULL;
		else if (memcmp("%name%", p, n) == 0)
			temp = song->name ? g_strdup(song->name) : NULL;
		else if (memcmp("%date%", p, n) == 0)
			temp = song->date ? g_strdup(song->date) : NULL;		
		else if (memcmp("%genre%", p, n) == 0)
			temp = song->genre ? g_strdup(song->genre) : NULL;		
		
		else if (memcmp("%shortfile%", p, n) == 0)
		{
			if( strstr(song->file, "://") )
				temp = g_strdup(song->file);
			else
				temp = remove_extention_and_basepath(song->file);
		}
		else if (memcmp("%time%", p, n) == 0)
		{
			if (song->time != MPD_SONG_NO_TIME) {
				temp = g_strdup_printf("%02d:%02d", song->time/60,song->time % 60);;
			}
		}
/*
		if( temp == NULL)
		{
	*/
			/*gsize templen=n;*/
			/* just pass-through any unknown specifiers (including esc) */
			/* drop a null char in so printf stops at the end of this specifier,
			   but put the real character back in (pseudo-const) */
/*			if( length+templen > max )
				templen = max-length;
			strncat(s, p, templen);
			length+=templen;
		}
*/		if(temp != NULL) {
			gsize templen = strlen(temp);

			found = TRUE;
			if( length+templen > max )
				templen = max-length;
			strncat(s, temp, templen);
			length+=templen;
			g_free(temp);
		}

		/* advance past the specifier */
		p += n;
	}

	for(i=0; i < max;i++)
	{
		if(s[i] == '_') s[i] = ' ';
	}	

	if(last) *last = p;

	return length;
}

	gsize
strfsong(gchar *s, gsize max, const gchar *format, mpd_Song *song)
{
	return _strfsong(s, max, format, song, NULL);
}

