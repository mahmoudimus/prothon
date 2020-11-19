/* ====================================================================
 * The Prothon License Agreement, Version 1.1
 *
 * Copyright (c) 2004 Hahn Creative Applications, http://hahnca.com.
 * All rights reserved. 
 *
 * 1. This LICENSE AGREEMENT is between Hahn Creative Applications ("HCA"),
 * and the Individual or Organization ("Licensee") accessing and otherwise
 * using Prothon software in source or binary form and its associated
 * documentation.
 * 
 * 2. Subject to the terms and conditions of this License Agreement, HCA
 * hereby grants Licensee a nonexclusive, royalty-free, world-wide license
 * to reproduce, analyze, test, perform and/or display publicly, prepare
 * derivative works, distribute, and otherwise use Prothon alone or in any
 * derivative version, provided, however, that HCA's License Agreement and
 * HCA's notice of copyright, i.e., "Copyright (c) 2004 Hahn Creative
 * Applications; All Rights Reserved" are retained in Prothon alone or
 * in any derivative version prepared by Licensee.
 * 
 * 3. In the event Licensee prepares a derivative work that is based on or
 * incorporates Prothon or any part thereof, and wants to make the
 * derivative work available to others as provided herein, then Licensee
 * hereby agrees to include in any such work a brief summary of the
 * changes made to Prothon.
 * 
 * 4. HCA is making Prothon available to Licensee on an "AS IS" basis.
 * HCA MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED.  BY WAY
 * OF EXAMPLE, BUT NOT LIMITATION, HCA MAKES NO AND DISCLAIMS ANY
 * REPRESENTATION OR WARRANTY OF MERCHANTABILITY OR FITNESS FOR ANY
 * PARTICULAR PURPOSE OR THAT THE USE OF PROTHON WILL NOT INFRINGE ANY
 * THIRD PARTY RIGHTS.
 * 
 * 5. HCA SHALL NOT BE LIABLE TO LICENSEE OR ANY OTHER USERS OF PROTHON
 * FOR ANY INCIDENTAL, SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS AS A
 * RESULT OF MODIFYING, DISTRIBUTING, OR OTHERWISE USING PROTHON, OR ANY
 * DERIVATIVE THEREOF, EVEN IF ADVISED OF THE POSSIBILITY THEREOF.
 * 
 * 6. This License Agreement will automatically terminate upon a material
 * breach of its terms and conditions.
 * 
 * 7. Nothing in this License Agreement shall be deemed to create any
 * relationship of agency, partnership, or joint venture between HCA and
 * Licensee.  This License Agreement does not grant permission to use HCA
 * trademarks or trade name in a trademark sense to endorse or promote
 * products or services of Licensee, or any third party.
 * 
 * 8. By copying, installing or otherwise using Prothon, Licensee agrees
 * to be bound by the terms and conditions of this License Agreement.
 * ====================================================================
 */


// lock.h

#ifndef LOCK_H
#define LOCK_H

#include "object.h"
#include <apr_atomic.h>
#include <apr_thread_cond.h>
#include <apr_version.h>

#if APR_MAJOR_VERSION == 0
#define lock(lock_var)		(apr_atomic_cas(&lock_var, 1, 0) == 0)
#define unlock(lock_var)	(apr_atomic_set(&lock_var, 0))
#define lock_init(lock_var)	(apr_atomic_set(&lock_var, 0))
#else
#define lock(lock_var)		(apr_atomic_cas32(&lock_var, 1, 0) == 0)
#define unlock(lock_var)	(apr_atomic_set32(&lock_var, 0))
#define lock_init(lock_var)	(apr_atomic_set32(&lock_var, 0))
#endif

#define lockstack	((clist_p)(ist->lock_stack))
#define astrstack	((clist_p)(ist->as_str_stack))

typedef struct {
	int					waiting;
	apr_uint32_t		lock;
	apr_thread_cond_t*	cond;
	apr_thread_mutex_t*	mutex;
} pr_lock_t;

#define DECLARE_PR_LOCK(name)  pr_lock_t name = { 0, 0, NULL, NULL }

void pr_lock(pr_lock_t* lockp);
void pr_unlock(pr_lock_t* lockp);

int read_lock_try(isp ist, obj_p obj);
int write_lock_try(isp ist, obj_p obj);

extern int obj_wrlock_waiting;

extern apr_thread_cond_t       *obj_wrlock_cond;
extern apr_thread_mutex_t      *obj_wrlock_mutex;
extern apr_thread_cond_t       *write_cond;
extern apr_thread_mutex_t      *write_mutex;

#define free_wrlock(obj)						\
	unlock((obj)->wrlock);							\
	if (obj_wrlock_waiting)							\
		apr_thread_cond_broadcast(obj_wrlock_cond)

#define NO_WAIT	0
#define WAIT	1

void get_wrlock(obj_p obj, int force_wait);

void unlock_all_objs();

#endif // #ifndef LOCK_H


