/*
 *Copyright (C) 2004 Qball Cow <Qball@qballcow.nl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "config1.h"

config_obj *cfg_open(gchar *url)
{
	config_obj *cfgo = NULL;
	/* check if there is an url passed */
	if(url == NULL)
	{
		return NULL;
	}
	
	cfgo = g_malloc(sizeof(config_obj));
	/* check if malloc went ok */
	if(cfgo == NULL)
	{
		return NULL;
	}

	cfgo->url = g_strdup(url);
	if(g_file_test(cfgo->url, G_FILE_TEST_EXISTS))
	{
		cfgo->xmldoc = xmlParseFile(cfgo->url); 
		cfgo->root = xmlDocGetRootElement(cfgo->xmldoc);
	}
	else
	{
		cfgo->xmldoc = xmlNewDoc("1.0");
		cfgo->root = xmlNewDocNode(cfgo->xmldoc, NULL, "config",NULL);	
		xmlDocSetRootElement(cfgo->xmldoc, cfgo->root);
		/* save it directly */
		cfg_save(cfgo);
	}
	return cfgo;
}

void cfg_save(config_obj *cfgo)
{
	if(cfgo == NULL)
	{
		return;
	}
	xmlSaveFile(cfgo->url, cfgo->xmldoc);
	return;
}

xmlNodePtr cfg_get_class(config_obj *cfg, char *class)
{
	xmlNodePtr cur = cfg->root->xmlChildrenNode;
	if(cur == NULL)
	{
		return NULL;
	}
	do
	{
		if(xmlStrEqual(cur->name, class))
		{
			return cur;
		}
		cur = cur->next;
	}while (cur != NULL);
	return NULL;
}

xmlNodePtr cfg_get_single_value(config_obj *cfg, char *class, char *key)
{
	/* take children */
	xmlNodePtr cur = cfg_get_class(cfg, class);
	
	if(cur == NULL)
	{
		return NULL;
	}
	cur = cur->xmlChildrenNode;
	if(cur == NULL)
	{
		return NULL;
	}
	do
	{
		if(xmlStrEqual(cur->name, key))
		{
			return cur;
		}
		cur = cur->next;
	}while (cur != NULL);
	return NULL;                                     	
}


char * cfg_get_single_value_as_string(config_obj *cfg, char *class, char *key)
{
	xmlNodePtr cur = cfg_get_single_value(cfg,class,key);
	if(cur != NULL)
	{
		return xmlNodeGetContent(cur);
	}
	return NULL;
}

char * cfg_get_single_value_as_string_with_default(config_obj *cfg, char *class, char *key , char *def)
{
	char *retv = cfg_get_single_value_as_string(cfg,class,key);
	if(retv == NULL)
	{
		cfg_set_single_value_as_string(cfg,class,key,def);
		retv = cfg_get_single_value_as_string(cfg,class,key);
	}
	return retv;
}

int cfg_get_single_value_as_int(config_obj *cfg, char *class, char *key)
{
	char * temp = cfg_get_single_value_as_string(cfg,class,key);
	if(temp == NULL)
	{
		return CFG_INT_NOT_DEFINED;
	}
	/* make it return an error */
	return atoi(temp);
}

void cfg_set_single_value_as_int(config_obj *cfg, char *class, char *key, int value)
{
	char *value1 = g_strdup_printf("%i", value);
	cfg_set_single_value_as_string(cfg,class,key,value1);
	g_free(value1);
}
int cfg_get_single_value_as_int_with_default(config_obj *cfg, char *class, char *key, int def)
{
	int retv = cfg_get_single_value_as_int(cfg,class,key);
	if(retv == CFG_INT_NOT_DEFINED)
	{
		cfg_set_single_value_as_int(cfg,class,key,def);
		retv = cfg_get_single_value_as_int(cfg,class,key);		
	}
	/* make it return an error */
	return retv;
}
/* float */
float cfg_get_single_value_as_float(config_obj *cfg, char *class, char *key)
{
	char * temp = cfg_get_single_value_as_string(cfg,class,key);
	if(temp == NULL)
	{
		return CFG_INT_NOT_DEFINED;
	}
	/* make it return an error */
	return (float)g_ascii_strtod(temp,NULL);
}

void cfg_set_single_value_as_float(config_obj *cfg, char *class, char *key, float value)
{
	char *value1 = g_strdup_printf("%f", value);
	cfg_set_single_value_as_string(cfg,class,key,value1);
	g_free(value1);
}

float cfg_get_single_value_as_float_with_default(config_obj *cfg, char *class, char *key, float def)
{
	float retv = cfg_get_single_value_as_float(cfg,class,key);
	if(retv == CFG_INT_NOT_DEFINED)
	{
		cfg_set_single_value_as_float(cfg,class,key,def);
		retv = cfg_get_single_value_as_float(cfg,class,key);		
	}
	/* make it return an error */
	return retv;
}

void cfg_del_single_value(config_obj *cfg, char *class, char *key)
{
	xmlNodePtr cur = cfg_get_single_value(cfg,class,key);
	xmlUnlinkNode(cur);
	xmlFreeNode(cur);
	cfg_save(cfg);
}

void cfg_set_single_value_as_string(config_obj *cfg, char *class, char *key, char *value)
{
	xmlNodePtr cur = cfg_get_single_value(cfg,class,key);
	gchar *string;
	if(value == NULL)
	{
		g_print("ERROR: trying to set value NULL\n");
		return;
	}
	if(cur != NULL)
	{
		cfg_del_single_value(cfg,class,key);
		cur = NULL;
	}
	cur = cfg_get_class(cfg,class);
	if(cur == NULL)
	{
		cur = xmlNewChild(cfg->root, NULL, class, NULL);
	}
	string = g_markup_escape_text(value, g_utf8_strlen(value, -1));
	if(string != NULL)
	{
		xmlNewChild(cur,NULL, key, string);
		g_free(string);
	}
	cfg_save(cfg);
}

/* multiple values */


xmlNodePtr cfg_get_multiple_value(config_obj *cfg, char *class, char *key, char* id)
{
	xmlNodePtr cur = cfg_get_class(cfg, class);
	if(cur == NULL)
	{
		return NULL;
	}
	cur = cur->xmlChildrenNode;
	if(cur == NULL)
	{
		return NULL;
	}
	do
	{
		if(xmlStrEqual(cur->name, key) && xmlStrEqual(xmlGetProp(cur, (const xmlChar *)"id"),id))
		{
			return cur;
		}
		cur = cur->next;
	}while (cur != NULL);
	return NULL;                                     	
}

void cfg_del_multiple_value(config_obj *cfg, char *class, char *key,char *id)
{
	xmlNodePtr cur = cfg_get_multiple_value(cfg, class,key,id);
	if(cur != NULL)
	{
		xmlUnlinkNode(cur);
		xmlFreeNode(cur);  				
		cfg_save(cfg);
	}
}

void cfg_set_multiple_value_as_string(config_obj *cfg, char *class, char *key, char *id, char *value)
{
	xmlNodePtr test,cur;
	char *string;
	cur= cfg_get_multiple_value(cfg,class,key,id);
	if(value == NULL)
	{
		return;
	}
	if(cur != NULL)
	{
		xmlUnlinkNode(cur);
		xmlFreeNode(cur);  		
	}

	string = g_markup_escape_text(value, g_utf8_strlen(value, -1));
	cur = cfg_get_class(cfg,class);
	if(cur == NULL)
	{
		cur = xmlNewChild(cfg->root, NULL, class, NULL);
	}
	test = xmlNewChild(cur,NULL, key, string);
	g_free(string);
	string = g_markup_escape_text(id, g_utf8_strlen(id, -1));
	g_print("id: %s\n",string);
	xmlSetProp(test, "id",string); 
	g_free(string);
	cfg_save(cfg);
}

conf_mult_obj * cfg_get_multiple_as_string(config_obj *cfg, char *class, char *key)
{
	xmlNodePtr cur;
	conf_mult_obj *list = NULL, *first= NULL; 
	cur = cfg_get_class(cfg, class);
	if(cur == NULL)
	{
		return NULL;
	}
	cur = cur->xmlChildrenNode;
	if(cur == NULL)
	{
		return NULL;
	}
	do
	{
		if(xmlStrEqual(cur->name, key))
		{
			if(list == NULL)
			{
				first = list = g_malloc(sizeof(conf_mult_obj));
				list->next = NULL;
				list->prev = NULL;
			}
			else
			{
				list->next = g_malloc(sizeof(conf_mult_obj));
				list->next->next =  NULL;
				list->next->prev = list;
				list = list->next;
			}
			list->key = g_strdup(xmlGetProp(cur, "id"));
			list->value = g_strdup(xmlNodeGetContent(cur));
		}
		cur = cur->next;
	}while (cur != NULL);
	return first;                                     	
}

void cfg_free_multiple(conf_mult_obj *data)
{
	conf_mult_obj *list = data;
	while(list != NULL)
	{
		if(list->key != NULL) g_free(list->key);
		if(list->value != NULL) g_free(list->value);
		list = list->next;
	}
	list = data;
	while(list != NULL)
	{
		if(list->next != NULL)
		{
			g_free(list->prev);
			list = list->next;
		}
		else{
			g_free(list);
			list = NULL;
		}
	}

}
