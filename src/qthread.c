#include <stdio.h>
#include <glib.h>
#include <libmpd/debug_printf.h>
#include "qthread.h"

int qthread_is_done (qthread *qt){
	int retval = 0;
	g_static_mutex_lock(&(qt->mutex));
	retval = qt->done;
	g_static_mutex_unlock(&(qt->mutex));
	return retval;
}

void qthread_init()
{
	debug_printf(DEBUG_INFO,"initializing threading\n");
	if (!g_thread_supported ()) g_thread_init (NULL);
}


qthread *qthread_new(GSourceFunc qthread_function, gpointer userdata)
{
	qthread *qt = g_malloc(sizeof(qthread));
	qt->callback = qthread_function;
	qt->userdata = userdata;
	g_static_mutex_init(&(qt->mutex));
	qt->done = 0;
	return qt;
}
void qthread_internall_run(qthread *qt)
{
	debug_printf(DEBUG_INFO,"running in qthread: %p\n", qt);
	if(qt->callback)
	{
		qt->callback(qt->userdata);
	}
	debug_printf(DEBUG_INFO,"qthread done %p\n",qt);
	g_static_mutex_lock(&(qt->mutex));
	qt->done =1;
	g_static_mutex_unlock(&(qt->mutex));
}

void qthread_free(qthread *qt)
{
	debug_printf(DEBUG_INFO,"cleaning up qthread %p\n",qt);
	g_static_mutex_free(&(qt->mutex));
	g_free(qt);
}
void qthread_run(qthread *qt)
{
	debug_printf(DEBUG_INFO,"Running qthread %p\n",qt);
	g_thread_create((GThreadFunc)qthread_internall_run,qt, FALSE,NULL);
}
