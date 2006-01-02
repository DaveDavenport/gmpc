#ifndef __QTHREAD_H__
#define __QTHREAD_H__

typedef struct _qthread {
	GStaticMutex 	mutex;
	int 		done;
	GSourceFunc	callback;
	gpointer	userdata;
} qthread;

int qthread_is_done(qthread *qt);
void qthread_init();
qthread *qthread_new(GSourceFunc qthread_function, gpointer userdata);
void qthread_free(qthread *qt);
void qthread_run(qthread *qt);

#endif
