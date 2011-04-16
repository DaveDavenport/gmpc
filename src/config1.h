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
#include <glib.h>

#ifndef __CONFIG_1_H__
#define __CONFIG_1_H__

/** 
 * \defgroup Config1 Config
 * \brief GMPC config system.
 */
/* @{@ */
#define CFG_INT_NOT_DEFINED -65536


typedef struct _config_obj config_obj;


typedef struct conf_mult_obj
{
    char *key;
    char *value;
    struct conf_mult_obj *next;
    struct conf_mult_obj *prev;
} conf_mult_obj;

/**
 * @param url The path to the config file to open.
 *
 * Open the config file.
 * 
 * @returns A #config_obj if opened succesful #NULL on failure.
 */
config_obj *cfg_open(const gchar * url);

/**
 * @param cfgo The #config_obj to close
 * 
 * Free's the #config_obj and if needed saves it.
 */
void cfg_close(config_obj * cfgo);


/**
 * @param cfg The #config_obj.
 * @param class The config subclass.
 * @param key The key to get.
 * 
 * Get a single config value as a string.
 *
 * F.e. #cfg_get_single_value_as_string(cfg, "aap", "mies");
 * Will get the value of key mies in class aap.
 * 
 *
 * @returns NULL when the value is not availible, an allocated string if 
 * found. (needs to be free'ed)
 */
char *cfg_get_single_value_as_string(config_obj * cfg, const char *class,
                                     const char *key);
/**
 * @param cfg The #config_obj.
 * @param class The config subclass.
 * @param key The key to get.
 * @param value The new value for class:key.
 *
 * Set single value.
 */
void cfg_set_single_value_as_string(config_obj * cfg, const char *class,
                                    const char *key, const char *value);
/**
 * @param cfg The #config_obj.
 * @param class The config subclass.
 * @param key The key to get.
 * @param def The value to return if class:key is not found.
 * 
 * Get single string value.
 *
 * @returns The value off class:key converted to a string or def is not found.
 */
char *cfg_get_single_value_as_string_with_default(config_obj * cfg,
                                                  const char *class,
                                                  const char *key,
                                                  const char *def);
/**
 * @param cfg The #config_obj.
 * @param class The config subclass.
 * @param key The key to get.
 * 
 * Get a single config value as a int.
 *
 *
 * @returns CFG_INT_NOT_DEFINED when the value is not availible.
 */
int cfg_get_single_value_as_int(config_obj * cfg, const char *class,
                                const char *key);
/**
 * @param cfg The #config_obj.
 * @param class The config subclass.
 * @param key The key to get.
 * @param value The new value for class:key.
 *
 * Set single value.
 */
void cfg_set_single_value_as_int(config_obj * cfg, const char *class,
                                 const char *key, int value);
/**
 * @param cfg The #config_obj.
 * @param class The config subclass.
 * @param key The key to get.
 * @param def The value to return if class:key is not found.
 * 
 * Get single int value.
 * @returns The value off class:key converted to a int or def is not found.
 */
int cfg_get_single_value_as_int_with_default(config_obj * cfg,
                                             const char *class, const char *key,
                                             int def);

/**
 * @param cfg The #config_obj.
 * @param class The config subclass.
 * @param key The key to get.
 * 
 * Get a single config value as a float.
 *
 *
 * @returns CFG_INT_NOT_DEFINED when the value is not availible.
 */
float cfg_get_single_value_as_float(config_obj * cfg, const char *class,
                                    const char *key);
/**
 * @param cfg The #config_obj.
 * @param class The config subclass.
 * @param key The key to get.
 * @param value The new value for class:key.
 *
 * Set single value.
 */
void cfg_set_single_value_as_float(config_obj * cfg, const char *class,
                                   const char *key, float value);
/**
 * @param cfg The #config_obj.
 * @param class The config subclass.
 * @param key The key to get.
 * @param def The value to return if class:key is not found.
 * 
 * Get single float value.
 * @returns The value off class:key converted to a float or def is not found.
 */
float cfg_get_single_value_as_float_with_default(config_obj * cfg,
                                                 const char *class,
                                                 const char *key, float def);

/**
 * @param cfg The #config_obj.
 * @param class The config subclass.
 * @param key The key to remove.
 *
 * Removes a single value.
 */
void cfg_del_single_value(config_obj * cfg, const char *class, const char *key);

/**
 * @param cfg The #config_obj
 *
 * Get a list of all the classes.
 *
 * @returns a linked list (type #conf_mult_obj) of all the classes.
 */
conf_mult_obj *cfg_get_class_list(config_obj * data);

/**
 * @param cfg The #config_obj
 * @param class The class to get the keys from.
 *
 * Get a list of all the keys in class class.
 *
 * @returns a linked list (type #conf_mult_obj) of all the keys.
 */
conf_mult_obj *cfg_get_key_list(config_obj * data, const char *class);

/**
 * @param data The #conf_mult_obj to free
 * 
 * Free's the #conf_mult_obj.
 */
void cfg_free_multiple(conf_mult_obj * data);

/**
 * @param cfg the #config_obj.
 *
 * Remove class from the config file, including all sub-items.
 */
void cfg_remove_class(config_obj * cfg, const char *class);

#define cfg_free_string(a)  g_free(a);a=NULL;
/* @{@ */
#endif
