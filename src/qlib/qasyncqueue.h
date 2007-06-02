/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */
/* 
 * Modified by Qball <Qball@Sarine.nl> For usage in gmpc.
 */

#ifndef __Q_ASYNCQUEUE_H__
#define __Q_ASYNCQUEUE_H__

#include <glib/gthread.h>

G_BEGIN_DECLS

typedef struct _QAsyncQueue QAsyncQueue;

/* Asyncronous Queues, can be used to communicate between threads */

/* Get a new QAsyncQueue with the ref_count 1 */
QAsyncQueue*  q_async_queue_new                 (void);

/* Lock and unlock a QAsyncQueue. All functions lock the queue for
 * themselves, but in certain cirumstances you want to hold the lock longer,
 * thus you lock the queue, call the *_unlocked functions and unlock it again.
 */
void         q_async_queue_lock                 (QAsyncQueue      *queue);
void         q_async_queue_unlock               (QAsyncQueue      *queue);

/* Ref and unref the QAsyncQueue. */
QAsyncQueue* q_async_queue_ref                  (QAsyncQueue      *queue);
void         q_async_queue_unref                (QAsyncQueue      *queue);

#ifndef G_DISABLE_DEPRECATED
/* You don't have to hold the lock for calling *_ref and *_unref anymore. */
void         q_async_queue_ref_unlocked         (QAsyncQueue      *queue);
void         q_async_queue_unref_and_unlock     (QAsyncQueue      *queue);
#endif /* !G_DISABLE_DEPRECATED */

/* Push data into the async queue. Must not be NULL. */
void         q_async_queue_push                 (QAsyncQueue      *queue,
						 gpointer          data);
void         q_async_queue_push_unlocked        (QAsyncQueue      *queue,
						 gpointer          data);

void         q_async_queue_push_sorted          (QAsyncQueue      *queue,
						 gpointer          data,
						 GCompareDataFunc  func,
						 gpointer          user_data);
void         q_async_queue_push_sorted_unlocked (QAsyncQueue      *queue,
						 gpointer          data,
						 GCompareDataFunc  func,
						 gpointer          user_data);

/* Pop data from the async queue. When no data is there, the thread is blocked
 * until data arrives.
 */
gpointer     q_async_queue_pop                  (QAsyncQueue      *queue);
gpointer     q_async_queue_pop_unlocked         (QAsyncQueue      *queue);

/* Try to pop data. NULL is returned in case of empty queue. */
gpointer     q_async_queue_try_pop              (QAsyncQueue      *queue);
gpointer     q_async_queue_try_pop_unlocked     (QAsyncQueue      *queue);



/* Wait for data until at maximum until end_time is reached. NULL is returned
 * in case of empty queue. 
 */
gpointer     q_async_queue_timed_pop            (QAsyncQueue      *queue,
						 GTimeVal         *end_time);
gpointer     q_async_queue_timed_pop_unlocked   (QAsyncQueue      *queue,
						 GTimeVal         *end_time);

/* Return the length of the queue. Negative values mean that threads
 * are waiting, positve values mean that there are entries in the
 * queue. Actually this function returns the length of the queue minus
 * the number of waiting threads, q_async_queue_length == 0 could also
 * mean 'n' entries in the queue and 'n' thread waiting. Such can
 * happen due to locking of the queue or due to scheduling. 
 */
gint         q_async_queue_length               (QAsyncQueue      *queue);
gint         q_async_queue_length_unlocked      (QAsyncQueue      *queue);
void         q_async_queue_sort                 (QAsyncQueue      *queue,
						 GCompareDataFunc  func,
						 gpointer          user_data);
void         q_async_queue_sort_unlocked        (QAsyncQueue      *queue,
						 GCompareDataFunc  func,
						 gpointer          user_data);

/* Private API */
GMutex*      _q_async_queue_get_mutex           (QAsyncQueue      *queue);

G_END_DECLS


/**
 * q_async_queue_remove_data_unlocked:
 * @queue: a #QAsyncQueue.
 * 
 * Pops data from the @queue. 
 *
 * Return value: data from the queue.
 **/
gpointer
q_async_queue_remove_data_unlocked (QAsyncQueue* queue, GCompareFunc func, gpointer data);


gboolean
q_async_queue_has_data (QAsyncQueue *queue, GCompareFunc func, gpointer data);
#endif /* __Q_ASYNCQUEUE_H__ */

