/*****************************************************************************
 * $Id: queue.h,v 1.2 2000/08/18 16:59:15 rf Exp $
 *----------------------------------------------------------------------------
 * NexGenOS v1.2
 * Generic Queue Management
 *----------------------------------------------------------------------------
 *		Copyright (c) 1998,1999,2000 NexGen Software.
 *
 *	All rights reserved. NexGen Software' source code is an unpublished
 *	work and the use of a copyright notice does not imply otherwise.
 *	This source code contains confidential, trade secret material of
 *	NexGen Software. Any attempt or participation in deciphering, decoding,
 *	reverse engineering or in any way altering the source code is
 *	strictly prohibited, unless the prior written consent of
 *	NexGen Software is obtained.
 *
 *   	This software is  supplied  under  the terms of a
 *   	license agreement or nondisclosure agreement with
 *   	NexGen Software, and may not be copied or disclosed
 *   	except  in  accordance  with  the  terms of  that
 *  	agreement.
 *
 *----------------------------------------------------------------------------
 * 08/09/98 - Regis Feneon
 * 18/01/99 -
 *  macros redefinition in respect to the specs
 * 15/02/2000 -
 *  added NG_NODE_INIT()
 *****************************************************************************/

#ifndef __NG_QUEUE_H_INCLUDED__
#define __NG_QUEUE_H_INCLUDED__

NG_BEGIN_DECLS

/*
 * Node
 */
typedef struct NGnode_S {
    struct NGnode_S *prev;
    struct NGnode_S *next;
} NGnode;

/* initialize single node */
#define NG_NODE_INIT( n) { \
    ((NGnode *)(n))->prev       =  (NGnode *)(n); \
    ((NGnode *)(n))->next       =  (NGnode *)(n); \
}

/* insert n2 before n1 in the circular list */
#define NG_NODE_IN( n1, n2) { \
    ((NGnode *)(n2))->prev       = ((NGnode *)(n1))->prev; \
    ((NGnode *)(n2))->next       =  (NGnode *)(n1); \
    ((NGnode *)(n1))->prev->next =  (NGnode *)(n2); \
    ((NGnode *)(n1))->prev       =  (NGnode *)(n2); \
}

/* extract n2 following n1 in the circular list */
#define NG_NODE_OUT( n1, n2) { \
    if( ((NGnode *)(n1))->next == (NGnode *)(n1)) { \
        (n2) = NULL; \
    } \
    else { \
        (n2) = (void *)(((NGnode *)(n1))->next); \
        ((NGnode *)(n1))->next       = ((NGnode *)(n2))->next; \
        ((NGnode *)(n2))->next->prev =  (NGnode *)(n1); \
        ((NGnode *)(n2))->prev       =  (NGnode *)(n2); \
        ((NGnode *)(n2))->next       =  (NGnode *)(n2); \
    } \
}

/* extract n from the circular list */
#define NG_NODE_DETACH( n) { \
    ((NGnode *)(n))->next->prev = ((NGnode *)(n))->prev; \
    ((NGnode *)(n))->prev->next = ((NGnode *)(n))->next; \
    ((NGnode *)(n))->prev       =  (NGnode *)(n); \
    ((NGnode *)(n))->next       =  (NGnode *)(n); \
}


/*
 * Message Buffer Queue
 */
typedef struct {
    NGnode *qu_head;
    NGnode *qu_tail;
    int     qu_nel;
    int     qu_nelmax;
} NGqueue;

/* Initialize the fifo queue */
#define NG_QUEUE_INIT( q, nmax) { \
    (q)->qu_head   = NULL; \
    (q)->qu_tail   = NULL; \
    (q)->qu_nel    = 0; \
    (q)->qu_nelmax = nmax; \
}

/* add node at the tail of the queue */
#define NG_QUEUE_IN( q, n) { \
    ((NGnode *) (n))->next = NULL; \
    ((NGnode *) (n))->prev = (q)->qu_tail; \
    if( (q)->qu_tail) { \
        (q)->qu_tail->next = (NGnode *) (n); \
        (q)->qu_tail       = (NGnode *) (n); \
    } \
    else { \
        (q)->qu_head = (NGnode *) (n); \
        (q)->qu_tail = (NGnode *) (n); \
    } \
    (q)->qu_nel++; \
}

/* get node at the head of the queue */
#define NG_QUEUE_OUT( q, n) { \
    (n) = (void *) ((q)->qu_head); \
    if( n) { \
        (q)->qu_head = ((NGnode *) (n))->next; \
        if( (q)->qu_head ) (q)->qu_head->prev = NULL; \
        else (q)->qu_tail = NULL; \
        ((NGnode *) (n))->prev = NULL; \
        ((NGnode *) (n))->next = NULL; \
        (q)->qu_nel--; \
    } \
}

/* peek the first element */
#define NG_QUEUE_PEEK( q, n) { \
    (n) = (void *) ((q)->qu_head); \
}

/* get the next element following n1 in n2*/
#define NG_QUEUE_NEXT( q, n1, n2) { \
    (n2) = (void *) (((NGnode *)(n1))->next); \
}

/* test if queue is empty */
#define NG_QUEUE_EMPTY( q)     ((q)->qu_nel == 0)

/* test if queue is full */
#define NG_QUEUE_FULL( q)      ((q)->qu_nel >= (q)->qu_nelmax)

NG_END_DECLS

#endif

