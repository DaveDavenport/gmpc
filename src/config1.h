/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2010 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
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

#ifndef __CONFIG_1_H__
#define __CONFIG_1_H__
#include <glib.h>
#define CFG_INT_NOT_DEFINED -65536


typedef struct _config_obj config_obj;
extern config_obj *config;

typedef struct conf_mult_obj
{
	char *key;
	char *value;
	struct conf_mult_obj *next;
	struct conf_mult_obj *prev;
} conf_mult_obj;

/**
 * open the config file
 */
config_obj *cfg_open(gchar *url);


/**
 * Close the object
 */
void cfg_close(config_obj *cfgo);
/* string */

/**
 * Get a single config value as a string.
 * It returns NULL when the value is not availible
 */
char * cfg_get_single_value_as_string(config_obj *cfg, const char *class, const char *key);
/**
 * set single value 
 */
void cfg_set_single_value_as_string(config_obj *cfg, const char *class, const char *key, const char *value);
/**
 * get with default 
 */
char * cfg_get_single_value_as_string_with_default(config_obj *cfg, const char *class, const char *key , const char *def);

int cfg_get_single_value_as_int(config_obj *cfg, const char *class, const char *key);
void cfg_set_single_value_as_int(config_obj *cfg, const char *class, const char *key, int value);
int cfg_get_single_value_as_int_with_default(config_obj *cfg, const char *class, const char *key, int def);

/* float */
float cfg_get_single_value_as_float(config_obj *cfg, const char *class, const char *key);
void cfg_set_single_value_as_float(config_obj *cfg, const char *class, const char *key, float value);
float cfg_get_single_value_as_float_with_default(config_obj *cfg, const char *class, const char *key, float def);

/* del */
void cfg_del_single_value(config_obj *cfg, const char *class, const char *key);


conf_mult_obj *cfg_get_class_list(config_obj *data);
conf_mult_obj *cfg_get_key_list(config_obj *data, const char *class);

/* multiple */
void cfg_free_multiple(conf_mult_obj *data);
conf_mult_obj * cfg_get_multiple_as_string(config_obj *cfg, const char *class, const char *key);
void cfg_set_multiple_value_as_string(config_obj *cfg, const char *class, const char *key, const char *id, const char *value);
void cfg_del_multiple_value(config_obj *cfg, const char *class, const char *key, const char *id);
void cfg_free_string(char *string);


void cfg_remove_class(config_obj *cfg, const char *class);

void cfg_do_special_cleanup(config_obj *cfg);



#define cfg_free_string(a)  g_free(a);a=NULL;




void cfg_del_single_value_mm(config_obj *cfg, const char *class, const char *sclass, const char *ssclass, const char *key);
void cfg_set_single_value_as_int_mm(config_obj *cfg, const char *class, const char *sclass, const char *ssclass, const char *key, int value);
int cfg_get_single_value_as_int_with_default_mm(config_obj *cfg, const char *class, const char *sclass, const char *ssclass, const char *key, int def);
float cfg_get_single_value_as_float_mm(config_obj *cfg, const char *class, const char *sclass, const char *ssclass, const char *key);
/* String */
void cfg_set_single_value_as_string_mm(config_obj *cfg, const char *class, const char *sclass, const char *ssclass, const char *key, const char *value);
char * cfg_get_single_value_as_string_mm(config_obj *cfg, const char *class, const char *sclass, const char *ssclass, const char *key);
char * cfg_get_single_value_as_string_with_default_mm(config_obj *cfg, const char *class, const char *sclass, const char *ssclass, const char *key , const char *def);








#endif
