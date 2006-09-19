#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <curl/curl.h>
#include "gmpc_easy_download.h"
#define CURL_TIMEOUT 5 

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
	/*int res;*/
	if(!dld) return 0;
	/**
	 * Make sure it's clean
	 */
	gmpc_easy_download_clean(dld);
	/* initialize curl */
	curl = curl_easy_init();
	if(!curl) return 0;
	/* set uri */
	curl_easy_setopt(curl, CURLOPT_URL, url);
	/* set callback data */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, dld);
	/* set callback function */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	/* set timeout */
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CURL_TIMEOUT);

	curlm = curl_multi_init();

	curl_multi_add_handle(curlm, curl);
	do{
		curl_multi_perform(curlm, &running);
		g_usleep(100000);
		if(dld->callback)
			dld->callback(dld->callback_data);
		while ((msg = curl_multi_info_read(curlm, &msgs_left))) {	
			if (msg->msg == CURLMSG_DONE)
			{
				if( (!msg->data.result|| msg->data.result == 23)) {
					success = TRUE;
				}
				else
				{
					printf("Error: %i %s\n",msg->data.result, curl_multi_strerror(msg->data.result));
				}
			}
			else if (msg->msg == CURLINFO_CONTENT_LENGTH_DOWNLOAD)
			{
				printf("total size: %lf\n", msg->data.whatever);
			}
		}
	}while(running);
	/* cleanup */
	curl_easy_cleanup(curl);
	curl_multi_cleanup(curlm);
	printf("downloaded: %i\n", dld->size);
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

