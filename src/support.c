/* 
 * $Id: support.c 1481 2004-06-14 18:32:31Z kaw $
 *
 * (c) 2004 by Kalle Wallin <kaw@linux.se>
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
 *
 */

#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "support.h"

#define BUFSIZE 1024

extern void screen_status_printf(char *format, ...);

static gboolean noconvert = TRUE;

char *
trim(char *str)
{
  char *end;

  if( str==NULL )
    return NULL;

  while( IS_WHITESPACE(*str) )
    str++;

  end=str+strlen(str)-1;
  while( end>str && IS_WHITESPACE(*end) )
    {
      *end = '\0';
      end--;
    }
  return str;
}

char *
remove_trailing_slash(char *path)
{
  int len;

  if( path==NULL )
    return NULL;

  len=strlen(path);
  if( len>1 && path[len-1] == '/' )
    path[len-1] = '\0';

  return path;
}

char *
lowerstr(char *str)
{
  size_t i;
  size_t len = strlen(str);

  if( str==NULL )
    return NULL;

  i=0;
  while(i<len && str[i])
    {
      str[i] = tolower(str[i]);
      i++;
    }
  return str;
}


#ifndef HAVE_BASENAME
char *
basename(char *path)
{
  char *end;

  path = remove_trailing_slash(path);
  end = path + strlen(path);

  while( end>path && *end!='/' )
    end--;

  if( *end=='/' && end!=path )
    return end+1;

  return path;
}
#endif /* HAVE_BASENAME */


#ifndef HAVE_STRCASESTR
char *
strcasestr(char *haystack,char *needle)
{
  return strstr(lowerstr(haystack), lowerstr(needle));
}
#endif /* HAVE_STRCASESTR */

char *
strscroll(char *str, char *separator, int width, scroll_state_t *st)
{
  char *tmp, *buf;
  size_t len;

  if( st->offset==0 )
    {
      st->offset++;
      return g_strdup(str);
    }
  
  /* create a buffer containing the string and the separator */
  tmp = g_malloc(strlen(str)+strlen(separator)+1);
  strcpy(tmp, str);
  strcat(tmp, separator);
  len = strlen(tmp);

  if( st->offset >= len )
    st->offset = 0;
  
  /* create the new scrolled string */
  buf = g_malloc(width+1);
  strncpy(buf, tmp+st->offset, width);
  if( strlen(buf) < width )
    strncat(buf, tmp, width-strlen(buf));

  if( time(NULL)-st->t >= 1 )
    {
      st->t = time(NULL);
      st->offset++;
    }
  g_free(tmp);
  return buf;
  
}


void
charset_init(gboolean disable)
{
  noconvert = disable;
}

char *
utf8_to_locale(char *utf8str)
{
  gchar *str;
  gsize rb, wb;
  GError *error;

  if( noconvert )
    return g_strdup(utf8str);

  rb = 0; /* bytes read */
  wb = 0; /* bytes written */
  error = NULL;
  str=g_locale_from_utf8(utf8str, 
			 strlen(utf8str),
			 &wb, &rb,
			 &error);
  if( error )
    {
      const char *charset;

      g_get_charset(&charset);
//      screen_status_printf(_("Error: Unable to convert characters to %s"),
//			   charset);
  //    D("utf8_to_locale(): %s\n", error->message);
      g_error_free(error);
      return g_strdup(utf8str);
    }
  
  return str;
}

char *
locale_to_utf8(char *localestr)
{
  gchar *str;
  gsize rb, wb;
  GError *error;

  if( noconvert )
    return g_strdup(localestr);

  rb = 0; /* bytes read */
  wb = 0; /* bytes written */
  error = NULL;
  str=g_locale_to_utf8(localestr, 
		       strlen(localestr), 
		       &wb, &rb,
		       &error);
  if( error )
    {
      //screen_status_printf(_("Error: Unable to convert characters to UTF-8"));
//      D("locale_to_utf8: %s\n", error->message);
      g_error_free(error);
      return g_strdup(localestr);
    }

  return str;
}



