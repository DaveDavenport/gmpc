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
		g_print("get_class: %s-%s\n", cur->name,class);
		if(xmlStrEqual(cur->name, class))
		{
			g_print("returning from get_class: %s\n",cur->name);
			return cur->xmlChildrenNode;
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
	g_print("no value found\n");
	return NULL;
}

int cfg_get_single_value_as_int(config_obj *cfg, char *class, char *key)
{
	char * temp = cfg_get_single_value_as_string(cfg,class,key);
	if(temp == NULL)
	{
		return 0;
	}
	/* make it return an error */
	return atoi(temp);
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
	if(value == NULL)
	{
		return;
	}
	if(cur != NULL)
	{
		g_print("found one\n");
		cfg_del_single_value(cfg,class,key);
	}
	cur = cfg_get_class(cfg,class);
	if(cur == NULL)
	{
		g_print("found no class: %s\n", cur->name);
		cur = xmlNewChild(cfg->root, NULL, class, NULL);
	}
	xmlNewChild(cur,NULL, key, value);
	cfg_save(cfg);
}
