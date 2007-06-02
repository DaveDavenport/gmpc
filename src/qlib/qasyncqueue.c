/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * QAsyncQueue: asynchronous queue implementation, based on Gqueue.
 * Copyright (C) 2000 Sebastian Wilhelmi; University of Karlsruhe
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * MT safe
 */
/* Modified by qball
 * Renamed gasync->qasync to stop name colision
 */

#include <glib.h>
#include "qlib/qasyncqueue.h"


struct _QAsyncQueue
{
  GMutex *mutex;
  GCond *cond;
  GQueue *queue;
  guint waiting_threads;
  gint32 ref_count;
};

typedef struct {
  GCompareDataFunc func;
  gpointer         user_data;
} SortData;

/**
 * q_async_queue_new:
 * 
 * Creates a new asynchronous queue with the initial reference count of 1.
 * 
 * Return value: the new #QAsyncQueue.
 **/
QAsyncQueue*
q_async_queue_new (void)
{
  QAsyncQueue* retval = g_new (QAsyncQueue, 1);
  retval->mutex = g_mutex_new ();
  retval->cond = NULL;
  retval->queue = g_queue_new ();
  retval->waiting_threads = 0;
  retval->ref_count = 1;
  return retval;
}

/**
 * q_async_queue_ref:
 * @queue: a #QAsyncQueue.
 *
 * Increases the reference count of the asynchronous @queue by 1. You
 * do not need to hold the lock to call this function.
 *
 * Returns: the @queue that was passed in (since 2.6)
 **/
QAsyncQueue *
q_async_queue_ref (QAsyncQueue *queue)
{
  g_return_val_if_fail (queue, NULL);
  g_return_val_if_fail (g_atomic_int_get (&queue->ref_count) > 0, NULL);
  
  g_atomic_int_inc (&queue->ref_count);

  return queue;
}

/**
 * q_async_queue_ref_unlocked:
 * @queue: a #QAsyncQueue.
 * 
 * Increases the reference count of the asynchronous @queue by 1.
 *
 * @Deprecated: Since 2.8, reference counting is done atomically
 * so q_async_queue_ref() can be used regardless of the @queue's
 * lock.
 **/
void 
q_async_queue_ref_unlocked (QAsyncQueue *queue)
{
  g_return_if_fail (queue);
  g_return_if_fail (g_atomic_int_get (&queue->ref_count) > 0);
  
  g_atomic_int_inc (&queue->ref_count);
}

/**
 * q_async_queue_unref_and_unlock:
 * @queue: a #QAsyncQueue.
 * 
 * Decreases the reference count of the asynchronous @queue by 1 and
 * releases the lock. This function must be called while holding the
 * @queue's lock. If the reference count went to 0, the @queue will be
 * destroyed and the memory allocated will be freed.
 *
 * @Deprecated: Since 2.8, reference counting is done atomically
 * so q_async_queue_unref() can be used regardless of the @queue's
 * lock.
 **/
void 
q_async_queue_unref_and_unlock (QAsyncQueue *queue)
{
  g_return_if_fail (queue);
  g_return_if_fail (g_atomic_int_get (&queue->ref_count) > 0);

  g_mutex_unlock (queue->mutex);
  q_async_queue_unref (queue);
}

/**
 * q_async_queue_unref:
 * @queue: a #QAsyncQueue.
 * 
 * Decreases the reference count of the asynchronous @queue by 1. If
 * the reference count went to 0, the @queue will be destroyed and the
 * memory allocated will be freed. So you are not allowed to use the
 * @queue afterwards, as it might have disappeared. You do not need to
 * hold the lock to call this function.
 **/
void 
q_async_queue_unref (QAsyncQueue *queue)
{
  g_return_if_fail (queue);
  g_return_if_fail (g_atomic_int_get (&queue->ref_count) > 0);
  
  if (g_atomic_int_dec_and_test (&queue->ref_count))
    {
      g_return_if_fail (queue->waiting_threads == 0);
      g_mutex_free (queue->mutex);
      if (queue->cond)
	g_cond_free (queue->cond);
      g_queue_free (queue->queue);
      g_free (queue);
    }
}

/**
 * q_async_queue_lock:
 * @queue: a #QAsyncQueue.
 * 
 * Acquires the @queue's lock. After that you can only call the
 * <function>q_async_queue_*_unlocked()</function> function variants on that
 * @queue. Otherwise it will deadlock.
 **/
void
q_async_queue_lock (QAsyncQueue *queue)
{
  g_return_if_fail (queue);
  g_return_if_fail (g_atomic_int_get (&queue->ref_count) > 0);

  g_mutex_lock (queue->mutex);
}

/**
 * q_async_queue_unlock:
 * @queue: a #QAsyncQueue.
 * 
 * Releases the queue's lock.
 **/
void 
q_async_queue_unlock (QAsyncQueue *queue)
{
  g_return_if_fail (queue);
  g_return_if_fail (g_atomic_int_get (&queue->ref_count) > 0);

  g_mutex_unlock (queue->mutex);
}

/**
 * q_async_queue_push:
 * @queue: a #QAsyncQueue.
 * @data: @data to push into the @queue.
 *
 * Pushes the @data into the @queue. @data must not be %NULL.
 **/
void
q_async_queue_push (QAsyncQueue* queue, gpointer data)
{
  g_return_if_fail (queue);
  g_return_if_fail (g_atomic_int_get (&queue->ref_count) > 0);
  g_return_if_fail (data);

  g_mutex_lock (queue->mutex);
  q_async_queue_push_unlocked (queue, data);
  g_mutex_unlock (queue->mutex);
}

/**
 * q_async_queue_push_unlocked:
 * @queue: a #QAsyncQueue.
 * @data: @data to push into the @queue.
 * 
 * Pushes the @data into the @queue. @data must not be %NULL. This
 * function must be called while holding the @queue's lock.
 **/
void
q_async_queue_push_unlocked (QAsyncQueue* queue, gpointer data)
{
  g_return_if_fail (queue);
  g_return_if_fail (g_atomic_int_get (&queue->ref_count) > 0);
  g_return_if_fail (data);

  g_queue_push_head (queue->queue, data);
  if (queue->waiting_threads > 0)
    g_cond_signal (queue->cond);
}

/**
 * q_async_queue_push_sorted:
 * @queue: a #QAsyncQueue
 * @data: the @data to push into the @queue
 * @func: the #GCompareDataFunc is used to sort @queue. This function
 *     is passed two elements of the @queue. The function should return
 *     0 if they are equal, a negative value if the first element
 *     should be higher in the @queue or a positive value if the first
 *     element should be lower in the @queue than the second element.
 * @user_data: user data passed to @func.
 * 
 * Inserts @data into @queue using @func to determine the new
 * position. 
 * 
 * This function requires that the @queue is sorted before pushing on
 * new elements.
 * 
 * This function will lock @queue before it sorts the queue and unlock
 * it when it is finished.
 * 
 * For an example of @func see q_async_queue_sort(). 
 *
 * Since: 2.10
 **/
void
q_async_queue_push_sorted (QAsyncQueue      *queue,
			   gpointer          data,
			   GCompareDataFunc  func,
			   gpointer          user_data)
{
  g_return_if_fail (queue != NULL);

  g_mutex_lock (queue->mutex);
  q_async_queue_push_sorted_unlocked (queue, data, func, user_data);
  g_mutex_unlock (queue->mutex);
}

static gint 
q_async_queue_invert_compare (gpointer  v1, 
			      gpointer  v2, 
			      SortData *sd)
{
  return -sd->func (v1, v2, sd->user_data);
}

/**
 * q_async_queue_push_sorted_unlocked:
 * @queue: a #QAsyncQueue
 * @data: the @data to push into the @queue
 * @func: the #GCompareDataFunc is used to sort @queue. This function
 *     is passed two elements of the @queue. The function should return
 *     0 if they are equal, a negative value if the first element
 *     should be higher in the @queue or a positive value if the first
 *     element should be lower in the @queue than the second element.
 * @user_data: user data passed to @func.
 * 
 * Inserts @data into @queue using @func to determine the new
 * position.
 * 
 * This function requires that the @queue is sorted before pushing on
 * new elements.
 * 
 * This function is called while holding the @queue's lock.
 * 
 * For an example of @func see q_async_queue_sort(). 
 *
 * Since: 2.10
 **/
void
q_async_queue_push_sorted_unlocked (QAsyncQueue      *queue,
				    gpointer          data,
				    GCompareDataFunc  func,
				    gpointer          user_data)
{
  SortData sd;
  
  g_return_if_fail (queue != NULL);

  sd.func = func;
  sd.user_data = user_data;

  g_queue_insert_sorted (queue->queue, 
			 data, 
			 (GCompareDataFunc)q_async_queue_invert_compare, 
			 &sd);
  if (queue->waiting_threads > 0)
    g_cond_signal (queue->cond);
}

static gpointer
q_async_queue_pop_intern_unlocked (QAsyncQueue *queue, 
				   gboolean     try, 
				   GTimeVal    *end_time)
{
  gpointer retval;

  if (!g_queue_peek_tail_link (queue->queue))
    {
      if (try)
	return NULL;
      
      if (!queue->cond)
	queue->cond = g_cond_new ();

      if (!end_time)
        {
          queue->waiting_threads++;
	  while (!g_queue_peek_tail_link (queue->queue))
            g_cond_wait (queue->cond, queue->mutex);
          queue->waiting_threads--;
        }
      else
        {
          queue->waiting_threads++;
          while (!g_queue_peek_tail_link (queue->queue))
            if (!g_cond_timed_wait (queue->cond, queue->mutex, end_time))
              break;
          queue->waiting_threads--;
          if (!g_queue_peek_tail_link (queue->queue))
	    return NULL;
        }
    }

  retval = g_queue_pop_tail (queue->queue);

  g_assert (retval);

  return retval;
}

/**
 * q_async_queue_pop:
 * @queue: a #QAsyncQueue.
 * 
 * Pops data from the @queue. This function blocks until data become
 * available.
 *
 * Return value: data from the queue.
 **/
gpointer
q_async_queue_pop (QAsyncQueue* queue)
{
  gpointer retval;

  g_return_val_if_fail (queue, NULL);
  g_return_val_if_fail (g_atomic_int_get (&queue->ref_count) > 0, NULL);

  g_mutex_lock (queue->mutex);
  retval = q_async_queue_pop_intern_unlocked (queue, FALSE, NULL);
  g_mutex_unlock (queue->mutex);

  return retval;
}

/**
 * q_async_queue_pop_unlocked:
 * @queue: a #QAsyncQueue.
 * 
 * Pops data from the @queue. This function blocks until data become
 * available. This function must be called while holding the @queue's
 * lock.
 *
 * Return value: data from the queue.
 **/
gpointer
q_async_queue_pop_unlocked (QAsyncQueue* queue)
{
  g_return_val_if_fail (queue, NULL);
  g_return_val_if_fail (g_atomic_int_get (&queue->ref_count) > 0, NULL);

  return q_async_queue_pop_intern_unlocked (queue, FALSE, NULL);
}

/**
 * q_async_queue_try_pop:
 * @queue: a #QAsyncQueue.
 * 
 * Tries to pop data from the @queue. If no data is available, %NULL is
 * returned.
 *
 * Return value: data from the queue or %NULL, when no data is
 * available immediately.
 **/
gpointer
q_async_queue_try_pop (QAsyncQueue* queue)
{
  gpointer retval;

  g_return_val_if_fail (queue, NULL);
  g_return_val_if_fail (g_atomic_int_get (&queue->ref_count) > 0, NULL);

  g_mutex_lock (queue->mutex);
  retval = q_async_queue_pop_intern_unlocked (queue, TRUE, NULL);
  g_mutex_unlock (queue->mutex);

  return retval;
}

/**
 * q_async_queue_try_pop_unlocked:
 * @queue: a #QAsyncQueue.
 * 
 * Tries to pop data from the @queue. If no data is available, %NULL is
 * returned. This function must be called while holding the @queue's
 * lock.
 *
 * Return value: data from the queue or %NULL, when no data is
 * available immediately.
 **/
gpointer
q_async_queue_try_pop_unlocked (QAsyncQueue* queue)
{
  g_return_val_if_fail (queue, NULL);
  g_return_val_if_fail (g_atomic_int_get (&queue->ref_count) > 0, NULL);

  return q_async_queue_pop_intern_unlocked (queue, TRUE, NULL);
}

/**
 * q_async_queue_timed_pop:
 * @queue: a #QAsyncQueue.
 * @end_time: a #GTimeVal, determining the final time.
 *
 * Pops data from the @queue. If no data is received before @end_time,
 * %NULL is returned.
 *
 * To easily calculate @end_time a combination of g_get_current_time()
 * and g_time_val_add() can be used.
 *
 * Return value: data from the queue or %NULL, when no data is
 * received before @end_time.
 **/
gpointer
q_async_queue_timed_pop (QAsyncQueue* queue, GTimeVal *end_time)
{
  gpointer retval;

  g_return_val_if_fail (queue, NULL);
  g_return_val_if_fail (g_atomic_int_get (&queue->ref_count) > 0, NULL);

  g_mutex_lock (queue->mutex);
  retval = q_async_queue_pop_intern_unlocked (queue, FALSE, end_time);
  g_mutex_unlock (queue->mutex);

  return retval;  
}

/**
 * q_async_queue_timed_pop_unlocked:
 * @queue: a #QAsyncQueue.
 * @end_time: a #GTimeVal, determining the final time.
 *
 * Pops data from the @queue. If no data is received before @end_time,
 * %NULL is returned. This function must be called while holding the
 * @queue's lock.
 *
 * To easily calculate @end_time a combination of g_get_current_time()
 * and g_time_val_add() can be used.
 *
 * Return value: data from the queue or %NULL, when no data is
 * received before @end_time.
 **/
gpointer
q_async_queue_timed_pop_unlocked (QAsyncQueue* queue, GTimeVal *end_time)
{
  g_return_val_if_fail (queue, NULL);
  g_return_val_if_fail (g_atomic_int_get (&queue->ref_count) > 0, NULL);

  return q_async_queue_pop_intern_unlocked (queue, FALSE, end_time);
}

/**
 * q_async_queue_length:
 * @queue: a #QAsyncQueue.
 * 
 * Returns the length of the queue, negative values mean waiting
 * threads, positive values mean available entries in the
 * @queue. Actually this function returns the number of data items in
 * the queue minus the number of waiting threads. Thus a return value
 * of 0 could mean 'n' entries in the queue and 'n' thread waiting.
 * That can happen due to locking of the queue or due to
 * scheduling.  
 *
 * Return value: the length of the @queue.
 **/
gint
q_async_queue_length (QAsyncQueue* queue)
{
  gint retval;

  g_return_val_if_fail (queue, 0);
  g_return_val_if_fail (g_atomic_int_get (&queue->ref_count) > 0, 0);

  g_mutex_lock (queue->mutex);
  retval = queue->queue->length - queue->waiting_threads;
  g_mutex_unlock (queue->mutex);

  return retval;
}

/**
 * q_async_queue_length_unlocked:
 * @queue: a #QAsyncQueue.
 * 
 * Returns the length of the queue, negative values mean waiting
 * threads, positive values mean available entries in the
 * @queue. Actually this function returns the number of data items in
 * the queue minus the number of waiting threads. Thus a return value
 * of 0 could mean 'n' entries in the queue and 'n' thread waiting.
 * That can happen due to locking of the queue or due to
 * scheduling. This function must be called while holding the @queue's
 * lock.
 *
 * Return value: the length of the @queue.
 **/
gint
q_async_queue_length_unlocked (QAsyncQueue* queue)
{
  g_return_val_if_fail (queue, 0);
  g_return_val_if_fail (g_atomic_int_get (&queue->ref_count) > 0, 0);

  return queue->queue->length - queue->waiting_threads;
}

/**
 * q_async_queue_sort:
 * @queue: a #QAsyncQueue
 * @func: the #GCompareDataFunc is used to sort @queue. This
 *     function is passed two elements of the @queue. The function
 *     should return 0 if they are equal, a negative value if the
 *     first element should be higher in the @queue or a positive
 *     value if the first element should be lower in the @queue than
 *     the second element. 
 * @user_data: user data passed to @func
 *
 * Sorts @queue using @func. 
 *
 * This function will lock @queue before it sorts the queue and unlock
 * it when it is finished.
 *
 * If you were sorting a list of priority numbers to make sure the
 * lowest priority would be at the top of the queue, you could use:
 * <informalexample><programlisting> 
 *  gint32 id1;
 *  gint32 id2;
 *   
 *  id1 = GPOINTER_TO_INT (element1);
 *  id2 = GPOINTER_TO_INT (element2);
 *   
 *  return (id1 > id2 ? +1 : id1 == id2 ? 0 : -1);
 * </programlisting></informalexample>
 *
 * Since: 2.10
 **/
void
q_async_queue_sort (QAsyncQueue      *queue,
		    GCompareDataFunc  func,
		    gpointer          user_data)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (func != NULL);

  g_mutex_lock (queue->mutex);
  q_async_queue_sort_unlocked (queue, func, user_data);
  g_mutex_unlock (queue->mutex);
}

/**
 * q_async_queue_sort_unlocked:
 * @queue: a #QAsyncQueue
 * @func: the #GCompareDataFunc is used to sort @queue. This
 *     function is passed two elements of the @queue. The function
 *     should return 0 if they are equal, a negative value if the
 *     first element should be higher in the @queue or a positive
 *     value if the first element should be lower in the @queue than
 *     the second element. 
 * @user_data: user data passed to @func
 *
 * Sorts @queue using @func. 
 *
 * This function is called while holding the @queue's lock.
 * 
 * Since: 2.10
 **/
void
q_async_queue_sort_unlocked (QAsyncQueue      *queue,
			     GCompareDataFunc  func,
			     gpointer          user_data)
{
  SortData sd;

  g_return_if_fail (queue != NULL);
  g_return_if_fail (func != NULL);

  sd.func = func;
  sd.user_data = user_data;

  g_queue_sort (queue->queue, 
		(GCompareDataFunc)q_async_queue_invert_compare, 
		&sd);
}

/*
 * Private API
 */

GMutex*
_q_async_queue_get_mutex (QAsyncQueue* queue)
{
  g_return_val_if_fail (queue, NULL);
  g_return_val_if_fail (g_atomic_int_get (&queue->ref_count) > 0, NULL);

  return queue->mutex;
}





/**
 * q_async_queue_remove_data_unlocked:
 * @queue: a #QAsyncQueue.
 * 
 * Pops data from the @queue. 
 *
 * Return value: data from the queue.
 **/
gpointer
q_async_queue_remove_data_unlocked (QAsyncQueue* queue, GCompareFunc func, gpointer data)
{
	gint length;
	GList *list = NULL;
  gpointer retval= NULL;

  g_return_val_if_fail (queue, NULL);
  g_return_val_if_fail (g_atomic_int_get (&queue->ref_count) > 0, NULL);

	/* get the first */
	list = g_queue_peek_nth_link(queue->queue, 0);
	for(length = g_queue_get_length(queue->queue);list;length++)
	{
		/* if we find it, then pop it off and return it... */
		if(!func(list->data, data))
		{
			retval = list->data;
			g_queue_delete_link(queue->queue, list);
			return retval;
		}
		list = list->next;
	}
  return NULL;
}

gboolean
q_async_queue_has_data (QAsyncQueue *queue, GCompareFunc func, gpointer data)
{
	gint length;
	GList *list = NULL;
  gpointer retval= NULL;

  g_return_val_if_fail (queue,FALSE);
  g_return_val_if_fail (g_atomic_int_get (&queue->ref_count) > 0, FALSE);

	/* get the first */
	list = g_queue_peek_nth_link(queue->queue, 0);
	for(length = g_queue_get_length(queue->queue);list;length++)
	{
		if(!func(list->data, data))
		{
			return TRUE;	
		}
		list = list->next;
	}
  return FALSE;
}

#define __Q_ASYNCQUEUE_C__
