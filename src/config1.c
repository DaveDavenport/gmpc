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
	do
	{
		if(xmlStrEqual(cur->name, class))
		{
			return cur;
		}
	}while (cur != NULL);
	return NULL;
}

xmlNodePtr cfg_get_single_value(config_obj *cfg, char *class, char *key)
{
	xmlNodePtr cur = cfg_get_class(cfg, class);
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

int cfg_get_single_value_as_int(config_obj *cfg, char *class, char *key)
{
	xmlNodePtr cur = cfg_get_single_value(cfg,class,key);
	if(cur != NULL)
	{
		char * value = xmlNodeGetContent(cur);
		if(value != NULL)
		{
			return atoi(value);
		}
	}
	/* make it return an error */
	return 0;
}
