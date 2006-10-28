#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <curl/curl.h>
#include <libmpd/debug_printf.h>
#include "gmpc_easy_download.h"
#define CURL_TIMEOUT 10 

static size_t write_data(void *buffer, size_t size, size_t nmemb, gmpc_easy_download_struct *dld)
{
	if(!size || !nmemb)
		return 0;
	if(dld->data == NULL)
	{
		dld->size = 0;
	}
	dld->data = g_realloc(dld->data,(gulong)(size*nmemb+dld->size)+1);

	memset(&(dld->data)[dld->size], '\0', (size*nmemb)+1);
	memcpy(&(dld->data)[dld->size], buffer, size*nmemb);

	dld->size += size*nmemb;
	if(dld->size >= dld->max_size && dld->max_size > 0)
	{
		return 0;
	}
	return size*nmemb;
}

int gmpc_easy_download(const char *url,gmpc_easy_download_struct *dld)
{
	int running = 0;
	int msgs_left = 0;
	int success = FALSE;
	CURL *curl = NULL;
	CURLM *curlm = NULL;
	CURLMsg *msg = NULL;
	double total_size = 0;
	/*int res;*/
	if(!dld) return 0;
	if(url == NULL) return 0;
	if(url[0] == '\0') return 0;
	/**
	 * Make sure it's clean
	 */
	gmpc_easy_download_clean(dld);
	/* initialize curl */
	curl = curl_easy_init();
	if(!curl) return 0;
	curlm = curl_multi_init();
	if(!curlm) return 0;

	/* set uri */
	curl_easy_setopt(curl, CURLOPT_URL, url);
	/* set callback data */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, dld);
	/* set callback function */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	/* set timeout */
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CURL_TIMEOUT);
	/* set redirect */
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION ,1);
	/* set NO SIGNAL */
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, TRUE);

	
	curl_multi_add_handle(curlm, curl);
	do{
		curl_multi_perform(curlm, &running);
		g_usleep(100000);
		if(dld->callback)
		{
			if(!(total_size > 0 ))
			{
				curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &total_size); 
			}
			dld->callback(dld->size, (int)total_size, dld->callback_data);
		}
		while ((msg = curl_multi_info_read(curlm, &msgs_left))) {	
			if (msg->msg == CURLMSG_DONE)
			{
				if( (!msg->data.result|| msg->data.result == 23)) {
					success = TRUE;
				}
				else
				{
					debug_printf(DEBUG_ERROR,"Error: %i '%s' url: %s",
						msg->data.result,
						curl_easy_strerror(msg->data.result),
						url);
				}
			}
		}
	}while(running);
	/**
	 * remove handler
	 */
	curl_multi_remove_handle(curlm, curl);
	/* cleanup */
	curl_easy_cleanup(curl);
	curl_multi_cleanup(curlm);
	debug_printf(DEBUG_INFO,"Downloaded: %i\n", dld->size);
	if(success) return 1;
	if(dld->data) g_free(dld->data);
	dld->data = NULL;
	return 0;
}

void gmpc_easy_download_clean(gmpc_easy_download_struct *dld)
{
	if(dld->data)g_free(dld->data);
	dld->data = NULL;
	dld->size = 0;
}

