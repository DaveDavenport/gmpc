#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glade/glade.h>

#include "libmpdclient.h"
#include "main.h"
#define CONFIG ".gmpc.cfg"

void load_config()
    {
    gchar *filename = g_strdup_printf("%s/%s", g_getenv("HOME"), CONFIG);
    FILE *fp;
    gchar buffer[1024];
    fp = fopen(filename, "r");
    if(fp == NULL)
	{
	g_free(filename);
	return;
	}    
/*    bzero(buffer, 1024);*/
    memset(buffer, '\0', 1024);
    while(fgets(buffer, 1024, fp))
	{
	
	if(!strncmp(buffer, "host:", 5))
	    {
	    gchar *buf = g_strstrip(&buffer[5]);
	    if(buf != NULL)
		{
/*		bzero(preferences.host, 256);*/
		memset(preferences.host, '\0', 256);
	        strncpy(preferences.host, buf, MIN(strlen(buf),256));
		}
	    }
	
	else if (!strncmp(buffer, "port:", 5))
	    {
	    preferences.port = atoi(&buffer[5]);
	    }
	else if (!strncmp(buffer, "timeout:", 8))
	    {
	    preferences.timeout = (float)g_strtod(&buffer[8], NULL);
	    }
	else if (!strncmp(buffer, "auto-connect:", 13))
	    {
	    preferences.autoconnect = atoi(&buffer[13]);
	    }
	    
	else if (!strncmp(buffer, "show filter:", 12))
	    {
	    info.show_filter = atoi(&buffer[12]);
	    }	
	else if (!strncmp(buffer, "filter field:", 13))
	    {
	    info.filter_field = atoi(&buffer[13]);
	    }	
	else if(!strncmp(buffer, "filter entry:", 13))
	    {
	    gchar *buf = g_strstrip(&buffer[13]);
	    if(buf != NULL)
		{
		/*bzero(info.filter_entry, 256);*/
		memset(info.filter_entry, '\0', 256);
		strncpy(info.filter_entry, buf, strlen(buf));
		}
	    }
	else if(!strncmp(buffer, "do tray:", 8))
	    {
	    gchar *buf = g_strstrip(&buffer[8]);
	    if(buf != NULL)
		{
		info.do_tray = atoi(buf);
		}
	    }
	else if(!strncmp(buffer, "do popup:", 9))
	    {
	    gchar *buf = g_strstrip(&buffer[9]);
	    if(buf != NULL)
		{
		info.popup.do_popup = atoi(buf);
		}
	    }
	else if(!strncmp(buffer, "popup pos:", 10))
	    {
	    gchar *buf = g_strstrip(&buffer[10]);
	    if(buf != NULL)
		{
		info.popup.position = atoi(buf);
		}                                	
	    }                                    
	else if(!strncmp(buffer, "popup stay:", 11))
	    {
	    gchar *buf = g_strstrip(&buffer[11]);
	    if(buf != NULL)
		{
		info.popup.popup_stay = atoi(buf);
		}                                	
	    }                                    	
	else if(!strncmp(buffer, "popup state:", 12))
	    {
	    gchar *buf = g_strstrip(&buffer[12]);
	    if(buf != NULL)
		{
		info.popup.show_state = atoi(buf);
		}                                	
	    }                                    	
	else if(!strncmp(buffer, "use auth:", 9))
	    {
	    gchar *buf = g_strstrip(&buffer[9]);
	    if(buf != NULL)
		{
		preferences.user_auth = atoi(buf);
		}                                	
	    }                                    	
	else if(!strncmp(buffer, "auth pass:", 10))
	    {
	    gchar *buf = g_strstrip(&buffer[10]);
	    if(buf != NULL)
		{
		/*bzero(preferences.password, 256);*/
		memset(preferences.password, '\0', 256);
		strncpy(preferences.password, buf, MIN(strlen(buf),256));
		}
	    }
	
	memset(buffer, '\0',1024);
	
    	}
    fclose(fp);
    g_free(filename);
    }

void save_config()
    {
    gchar *filename = g_strdup_printf("%s/%s", g_getenv("HOME"), CONFIG);    
    FILE *fp;

    fp = fopen(filename, "w");
    if(fp == NULL)
	{
	g_free(filename);
	return;
	}    
    fprintf(fp, "host: %s\n", preferences.host);
    fprintf(fp, "port: %i\n", preferences.port);    
    fprintf(fp, "timeout: %.2f\n", preferences.timeout);    
    fprintf(fp, "auto-connect: %i\n", preferences.autoconnect);    
    fprintf(fp, "show filter: %i\n", info.show_filter);    
    fprintf(fp, "filter field: %i\n", info.filter_field);    
    fprintf(fp, "filter entry: %s\n", info.filter_entry);    
    fprintf(fp, "do tray: %i\n", info.do_tray);    
    fprintf(fp, "do popup: %i\n", info.popup.do_popup);   
    fprintf(fp, "popup pos: %i\n", info.popup.position);
    fprintf(fp, "popup stay: %i\n", info.popup.popup_stay);
    fprintf(fp, "popup state: %i\n", info.popup.show_state);
    fprintf(fp, "use auth: %i\n", preferences.user_auth);
    fprintf(fp, "auth pass: %s\n", preferences.password);
    fclose(fp);
    g_free(filename);    
    }
