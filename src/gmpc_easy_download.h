typedef void (*ProgressCallback)(int downloaded, int total, gpointer data);

typedef struct _gmpc_easy_download_struct{
	char *data;
	int size;
	int max_size;
	ProgressCallback callback;
	gpointer callback_data;
}gmpc_easy_download_struct;


int gmpc_easy_download(const char *url,gmpc_easy_download_struct *dld);
void gmpc_easy_download_clean(gmpc_easy_download_struct *dld);
