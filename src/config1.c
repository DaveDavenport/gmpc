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



