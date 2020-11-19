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


// lock.c

#include <stdlib.h>

#include <apr_time.h>
#include <apr_thread_mutex.h>

#include "object.h"
#include "memory_mgr.h"
#include "lock.h"

apr_thread_cond_t	*obj_wrlock_cond;
apr_thread_mutex_t	*obj_wrlock_mutex;
apr_thread_cond_t	*write_cond;
apr_thread_mutex_t	*write_mutex;

int obj_wrlock_waiting;

DECLARE_PR_LOCK(del_notify_lock_var);

//********************************* notify ************************************
void notify(isp ist, obj_p obj, int flag) {
	int i, llen;
	obj_p callback_list, arr[2];
	arr[0] = OBJ(NOKEY);

	if (ist->notify_flags) {
		ist->notify_flags |= flag;
		return;
	}
	ist->notify_flags = flag;
	callback_list = get_attr(ist, obj, SYM(NOTIFYCALLBACK_));
	if (ist->exception_obj) { ist->notify_flags = 0; return; }
	if (callback_list) {
		while(ist->notify_flags) {
			if (ist->notify_flags & READ_NOTIFY)        flag = READ_NOTIFY;   
			else if (ist->notify_flags & WRITE_NOTIFY)  flag = WRITE_NOTIFY;  
			else if (ist->notify_flags & DELETE_NOTIFY) flag = DELETE_NOTIFY;
			arr[1] = NEW_INT(flag);
			llen = (int) list_len(ist, callback_list);
			for (i=0; i < llen; i += 2)
				if (int2i32t(ist, list_item(ist, callback_list, i)) & flag) {
					call_func(ist, NULL, list_item(ist, callback_list, i+1), NULL, 2, arr, NULL);
					check_exceptions(ist);
				}
			ist->notify_flags &= ~flag;
			del_unlock(arr[1]);
		}
	}
	ist->notify_flags = 0;
}

//********************************* delete_notify *****************************
void delete_notify(isp ist, obj_p obj){
	int del_notify;
	get_wrlock(obj, FALSE);
	del_notify = obj->del_notify;
	free_wrlock(obj);
	if (del_notify) {
		del_notify_lock();
		notify(ist, obj, DELETE_NOTIFY);
		del_notify_unlock();
	}
}

//********************************* add_notification **************************
void add_notification(isp ist, obj_p obj, obj_p callback_func, int flags) {
	int i, llen, new_list = FALSE;
	obj_p callback_list = get_attr(ist, obj, SYM(NOTIFYCALLBACK_));
	if (!callback_list) {
		callback_list = NEW_LIST(2);
		new_list = TRUE;
	}
	llen = (int) list_len(ist, callback_list);
	for (i=0; i < llen; i += 2) {
		if (list_item(ist, callback_list, i+1) == callback_func) {
			list_item_set(ist, callback_list, i, NEW_INT(flags));
			goto done;
		}
	}
	list_append(ist, callback_list, NEW_INT(flags));
	list_append(ist, callback_list, callback_func);
	if (new_list)
		set_attr(ist, obj, SYM(NOTIFYCALLBACK_), callback_list);
done:
	get_wrlock(obj, FALSE);
	if (flags & READ_NOTIFY)   obj->rd_notify = TRUE;
	if (flags & WRITE_NOTIFY)  obj->wr_notify = TRUE;
	if (flags & DELETE_NOTIFY) obj->del_notify = TRUE;
	free_wrlock(obj);
}

//********************************* clr_notification **************************
void del_notification(isp ist, obj_p obj, obj_p callback_func) {
	list_p selfp;
	int i, llen, flags;
	obj_p callback_list = get_attr(ist, obj, SYM(NOTIFYCALLBACK_));
	if (!callback_list) return;
	write_lock(ist, callback_list);
	selfp = callback_list->data.ptr;
	while (TRUE) {
		llen  = (int) lstplen(selfp);
		for (i=0; i < llen; i += 2)
			if (listitem(callback_list, i+1) == callback_func) break;
		if (i == llen) break; 
		if (i < llen-2)
			memmove( selfp->item+i, selfp->item+i+2, 
					(llen - 2 - (size_t)i) * sizeof(obj_p) ); 
		lstplen(selfp) -= 2;
	}
	write_unlock(ist, callback_list); 
	llen = (int) list_len(ist, callback_list);
	flags = 0;
	for(i=0; i < llen; i += 2)
		flags |= int2i32t(ist, list_item(ist, callback_list, i));
	get_wrlock(obj, FALSE);
	if (flags & READ_NOTIFY)   obj->rd_notify  = TRUE; else obj->rd_notify  = FALSE; 
	if (flags & WRITE_NOTIFY)  obj->wr_notify  = TRUE; else obj->wr_notify  = FALSE;
	if (flags & DELETE_NOTIFY) obj->del_notify = TRUE; else obj->del_notify = FALSE;
	free_wrlock(obj);
}

//********************************* get_wrlock ********************************
void get_wrlock(obj_p obj, int force_wait) {
	if (!force_wait && lock((obj)->wrlock)) return;
	obj_wrlock_waiting++;
	apr_thread_mutex_lock(obj_wrlock_mutex);
	while(TRUE) {
		apr_thread_cond_timedwait(obj_wrlock_cond, obj_wrlock_mutex, PAUSE_MS*1000);
		if (lock((obj)->wrlock)) {
			obj_wrlock_waiting--;
			apr_thread_mutex_unlock(obj_wrlock_mutex);
			return;
		}
	}
}

//********************************* read_lock_try *****************************
int read_lock_try(isp ist, obj_p obj) {
	if (!lock((obj)->wrlock)) return FALSE;
	if (ist->access < obj->rd_access) {
		raise_exception(ist, OBJ(PERMISSION_EXC), "read-try permission exception");
		free_wrlock(obj);
		return FALSE;
	}
	if (obj->rd_notify) {
		free_wrlock(obj);
		notify(ist, obj, READ_NOTIFY);
		get_wrlock(obj, NO_WAIT);
	}
	if (obj->immutable) {
		free_wrlock(obj);
		return TRUE;
	}
	if (obj->wrlock_req_cnt) {
		free_wrlock(obj);
		return FALSE;
	}
	clist_push(lockstack, obj);
	obj->rdlock_cnt++;
	obj->state = OBJ_STATE_NEW_OR_MARKED;
	free_wrlock(obj);
	return TRUE;
}

//********************************* read_lock *********************************
int read_lock(isp ist, obj_p obj) {
	get_wrlock(obj, NO_WAIT);
	if (ist->access < obj->rd_access) {
		raise_exception(ist, OBJ(PERMISSION_EXC), "read permission exception");
		free_wrlock(obj);
		return TRUE;
	}
	if (obj->rd_notify) {
		free_wrlock(obj);
		notify(ist, obj, READ_NOTIFY);
		get_wrlock(obj, NO_WAIT);
	}
	if (obj->immutable) {
		free_wrlock(obj);
		return FALSE;
	}
	if (obj->wrlock_req_cnt && !clist_in(lockstack, obj)) { 
		while(obj->wrlock_req_cnt) {
			free_wrlock(obj);
			get_wrlock(obj, WAIT);
		}
	}
	clist_push(lockstack, obj);
	obj->rdlock_cnt++;
	obj->state = OBJ_STATE_NEW_OR_MARKED;
	free_wrlock(obj);
	return FALSE;
}

//********************************* read_unlock *******************************
void read_unlock(isp ist, obj_p obj) {
	int	write_ready_flag;
	get_wrlock(obj, NO_WAIT);
	if (obj->immutable) {
		free_wrlock(obj);
		return;
	}
	write_ready_flag = !(--(obj->rdlock_cnt)) && obj->wrlock_req_cnt;
	free_wrlock(obj);
	if (write_ready_flag) apr_thread_cond_broadcast(write_cond);
	if (clist_pop_item(lockstack, obj)) {
		get_wrlock(obj, NO_WAIT);
		obj->rdlock_cnt++;
		free_wrlock(obj);
		raise_exception(ist, OBJ(LOCK_EXC), "read_unlock performed on a non-read_locked object");
	}
}

//********************************* write_lock_try ****************************
int write_lock_try(isp ist, obj_p obj) {
	if (!lock((obj)->wrlock)) return FALSE;
	if (ist->access < obj->wr_access) {
		raise_exception(ist, OBJ(PERMISSION_EXC), "write-try permission exception");
		free_wrlock(obj);
		return FALSE;
	}
	if (obj->immutable) {
		raise_exception(ist, OBJ(LOCK_EXC), "write_lock attempted on an immutable object");
		free_wrlock(obj);
		return FALSE;
	}
	if (obj->rdlock_cnt || obj->wrlock_req_cnt) {
		free_wrlock(obj);
		return FALSE;
	}
	return TRUE;
}

//********************************* write_lock ********************************
int write_lock(isp ist, obj_p obj) {
	int sav_cnt = 0;
	get_wrlock(obj, NO_WAIT);
	if (ist->access < obj->wr_access) {
		raise_exception(ist, OBJ(PERMISSION_EXC), "write permission exception");
		free_wrlock(obj);
		return TRUE;
	}
	if (obj->immutable) {
		raise_exception(ist, OBJ(LOCK_EXC), "write_lock attempted on an immutable object");
		free_wrlock(obj);
		return TRUE;
	}
	if(obj->rdlock_cnt) {
		if(clist_in(lockstack, obj)) {
			raise_exception(ist, OBJ(LOCK_EXC), "write_lock attempted on a read_locked object");
			obj->rdlock_cnt -= sav_cnt = clist_in_cnt(lockstack, obj);
			free_wrlock(obj);
			return TRUE;
		}
		apr_thread_mutex_lock(write_mutex);
		while (obj->rdlock_cnt) {
			obj->wrlock_req_cnt++;
			free_wrlock(obj);
			apr_thread_cond_timedwait(write_cond, write_mutex, PAUSE_MS*1000);
			get_wrlock(obj, NO_WAIT);
			obj->wrlock_req_cnt--;
		}
		apr_thread_mutex_unlock(write_mutex);
		obj->rdlock_cnt += sav_cnt;
	}
	return FALSE;
}

//********************************* write_unlock ******************************
void write_unlock(isp ist, obj_p obj) {
	int write_ready_flag, write_notify_flag;
	if (obj->immutable) return;
	pr_assert(obj->wrlock);
	obj->archived = FALSE;
	obj->state = OBJ_STATE_NEW_OR_MARKED;
	if (mem_mgr_phase == MM_PHASE_MARKING && obj->scanned) {
		obj->scanned = FALSE;
		pr_lock(&to_be_scanned_list_lock);
		if (mem_mgr_phase == MM_PHASE_MARKING)
			clist_append(to_be_scanned_list, obj);
		pr_unlock(&to_be_scanned_list_lock);
	}
	write_ready_flag  = obj->wrlock_req_cnt;
	write_notify_flag = obj->wr_notify;
	free_wrlock(obj);
	if (write_ready_flag)  apr_thread_cond_broadcast(write_cond);
	if (write_notify_flag) notify(ist, obj, WRITE_NOTIFY);
}

//********************************* unlock_all_objs ***************************
void unlock_all_objs() {
	obj_p obj;
	pr_lock(&object_list_lock); 
	obj = obj_list_root;
	while(obj) {
		if (obj->state == OBJ_STATE_INVALID) {
			obj->next_obj = NULL;
			break;
		}
		obj->state           = OBJ_STATE_INVALID;
		obj->rdlock_cnt      = 0;
		obj->wrlock_req_cnt  = 0;
		obj->wrlock          = 0;
		obj = obj->next_obj;
	}
	pr_unlock(&object_list_lock); 
}

//********************************* pr_lock ********************************
void pr_lock(pr_lock_t* lockp) {
	if (lock(lockp->lock))
		return;

	if (!(lockp->cond)) {
		while (!lock(lockp->lock)) apr_sleep(PAUSE_MS*1000);
		if (!(lockp->cond)) {
			char buf[1024];
			apr_status_t aprerr;

			if ((aprerr = apr_thread_mutex_create(&lockp->mutex, APR_THREAD_MUTEX_DEFAULT,
							      get_pr_head_pool()))) {
				printf("pr_lock error: apr_thread_mutex_create: %s\n",
				       apr_strerror(aprerr, buf, sizeof(buf)));
				pr_exit(1);
			}
			if ((aprerr = apr_thread_cond_create(&lockp->cond, get_pr_head_pool()))) {
				printf("pr_lock error: apr_thread_cond_create: %s\n",
				       apr_strerror(aprerr, buf, sizeof(buf)));
				pr_exit(1);
			}
		}
		unlock(lockp->lock);
	}
	lockp->waiting++;
	apr_thread_mutex_lock(lockp->mutex);
	while (TRUE) {
		apr_thread_cond_timedwait(lockp->cond, lockp->mutex, PAUSE_MS*1000);
		if (lock(lockp->lock)) {
			lockp->waiting--;
			apr_thread_mutex_unlock(lockp->mutex);
			return;
		}
	}
}

//********************************* pr_unlock *********************************
void pr_unlock(pr_lock_t* lockp)
{
	unlock(lockp->lock);
	if (lockp->waiting)
		apr_thread_cond_broadcast(lockp->cond);
}

//********************************* del_lock **********************************
void del_lock(obj_p obj) {
	get_wrlock(obj, NO_WAIT);
	obj->del_locked = TRUE; 
	free_wrlock(obj);
}

void del_unlock(obj_p obj) {
}

int is_del_locked(obj_p obj) {
	int res;
	get_wrlock(obj, NO_WAIT);
	res = obj->del_locked;
	free_wrlock(obj);
	return res;
}
