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


// memory_mgr.c

#include "object.h"
#include "interp.h"
#include "memory_mgr.h"
#include "lock.h"

#include <stdio.h>


#include <apr_time.h>
#include <apr_thread_proc.h>


int mem_mgr_running;
int mem_mgr_phase;
int mem_mgr_req_pause;
int mem_mgr_req_unpause;
int mem_mgr_paused;
int mem_mgr_req_termination;
int mem_mgr_terminated;
obj_p mem_mgr_error;

clist_p  to_be_scanned_list;
DECLARE_PR_LOCK(to_be_scanned_list_lock);

i64_t hi_mem_boundary;
i64_t lo_mem_boundary;

//********************************* clr_all_locks *****************************
void clr_all_locks(obj_p obj) {
	apr_atomic_set(&(obj->wrlock), 0);	
	obj->rdlock_cnt = 0;				
	obj->wrlock_req_cnt = 0;			
}

//********************************* mm_read_lock *********************************
int mm_read_lock(isp ist, obj_p obj) {
	get_wrlock(obj, NO_WAIT);
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

//********************************* mm_write_lock *****************************
void mm_write_lock(isp ist, obj_p obj) {
	get_wrlock(obj, NO_WAIT);
	if(obj->rdlock_cnt) {
		apr_thread_mutex_lock(write_mutex);
		while (obj->rdlock_cnt) {
			obj->wrlock_req_cnt++;
			free_wrlock(obj);
			apr_thread_cond_timedwait(write_cond, write_mutex, PAUSE_MS*1000);
			get_wrlock(obj, NO_WAIT);
			obj->wrlock_req_cnt--;
		}
		apr_thread_mutex_unlock(write_mutex);
	}
}

//********************************* mm_write_unlock ***************************
void mm_write_unlock(obj_p obj) {
	int write_ready_flag;
	pr_assert(obj->wrlock);
	write_ready_flag = obj->wrlock_req_cnt;	
	free_wrlock(obj);
	if (write_ready_flag)  apr_thread_cond_broadcast(write_cond);
}

//********************************* mem_mgr_thread ******************************
void *mem_mgr_thread(apr_thread_t *handle, void *dummy) {
	obj_p thread_obj = register_thread(handle);
	pr_thread_p thread_p = thread_obj->data.ptr;
	isp ist = thread_p->ist;
	int i;

	obj_p mm_obj_list = NEW_LIST(100);
	obj_p mm_obj_act_param[] = {OBJ(NOKEY), mm_obj_list}; 
	int last_obj_del_count = 0, last_obj_count = 0;     

#ifdef DEBUG_THREADS
	if (debug_threads) {
		printf("Memory manager thread started\n");
		SetThreadName("mmgr");
	}
#endif
	while(!mem_mgr_req_termination) {
		obj_p obj, last_obj, min_obj = (obj_p) 10;
		int obj_count=0, obj_del_count=0;	

		if ( last_obj_del_count <  last_obj_count / 100) 
			apr_sleep(PAUSE_MS*1000);

#ifdef DEBUG_MEM_MGR
		clock_t time;
		printf("Memory manager phase: Clearing marks\n");
		obj_count = 0; time = clock();
#endif
		mem_mgr_running = TRUE;
		mem_mgr_phase = MM_PHASE_CLEARING_MARKS;
		pr_lock(&object_list_lock); 
		obj=obj_list_root;
		pr_unlock(&object_list_lock); 
		while(obj && !mem_mgr_req_termination) {
			if (obj == obj->next_obj) {
				dump(ist, "!mm-obj-dump.txt", obj);
				pr_assert(FALSE);
			}
			mm_write_lock(ist, obj);
			obj->state = OBJ_STATE_UNMARKED;
			obj->scanned = FALSE;
			mm_write_unlock(obj);
			obj_count++;
			pr_lock(&object_list_lock); 
			obj = obj->next_obj;
			pr_unlock(&object_list_lock); 
		}
#ifdef DEBUG_MEM_MGR
		printf("Memory manager: Cleared %d objects of marks, elapsed time(secs): %g\n", 
			    obj_count, ((clock()-time)*1.0)/CLOCKS_PER_SEC );
		obj_count = 0; time = clock();
		printf("Memory manager phase: Marking used objects\n");
#endif
		mem_mgr_phase = MM_PHASE_MARKING;     
		obj = OBJ(ROOT_GLOBALS);			
		to_be_scanned_list = new_clist(1000); 
		mm_write_lock(ist, obj);
		obj->state = OBJ_STATE_NEW_OR_MARKED;
		obj->scanned = TRUE;
		mm_write_unlock(obj);
markscan:
		if (ist->exception_obj)
			i=0;
		while(TRUE && !mem_mgr_req_termination) {
			obj_p val;
			obj_count++;
			if (obj == obj->next_obj) {
				dump(ist, "!mm-obj-dump.txt", obj);
				pr_assert(FALSE);
			}
			mm_read_lock(ist, obj);
			if (obj->has_attrs) {
				int i;
				attr_p attrs = obj->attr_proto.attrs;
				for(i=ATTRS_OVERHEAD; i < ATTRS_OVERHEAD+attr_plen(attrs); i++){
					val = attrs[i].attr.value;
					if(val > min_obj && val != obj && !val->scanned) {
						mm_write_lock(ist, val);
						val->state = OBJ_STATE_NEW_OR_MARKED;
						val->scanned = TRUE;
						mm_write_unlock(val);
						pr_lock(&to_be_scanned_list_lock);
						clist_append(to_be_scanned_list, val);
						pr_unlock(&to_be_scanned_list_lock);
					}
				}
				for( i=ATTRS_OVERHEAD+attr_psize(attrs); 
					 i < ATTRS_OVERHEAD+attr_psize(attrs)+attr_asize(attrs); i++){
					if ( attrs[i].attr.key > 0 &&
						 (val = attrs[i].attr.value) > min_obj && 
						  val != obj && !val->scanned ) {
						mm_write_lock(ist, val);
						val->state = OBJ_STATE_NEW_OR_MARKED;
						val->scanned = TRUE;
						mm_write_unlock(val);
						pr_lock(&to_be_scanned_list_lock);
						clist_append(to_be_scanned_list, val);
						pr_unlock(&to_be_scanned_list_lock);
					}
				}
			}
			if (obj->data_type == DATA_TYPE_DATAPTR) {
				obj_p obj_list;
				if (ist->exception_obj)
					i=0;
				mm_write_lock(ist, mm_obj_list); 
				if (ist->exception_obj)
					i=0;
				listlen(mm_obj_list) = 0;
				write_unlock(ist, mm_obj_list);
				obj_list = call_func(ist, obj, SYM(OBJLIST_), NULL, 2, mm_obj_act_param, NULL);
				if (ist->exception_obj) {
					printf("\nWarning: memory manager unable to scan object: %lx\n",
						(unsigned long)(uintptr_t)obj);
					printf("Dumping object details to mm-obj-dump.txt\n");
					printf("Aborting Prothon\n");
					dump(ist, "!mm-obj-dump.txt", obj);
					check_exceptions(ist);
					pr_exit(1);
				} else if (obj_list) {
					int i, llen = (int) list_len(ist, obj_list);
					for(i=0; i < llen; i++) {
						obj_p val = listitem(obj_list, i);
						if (val > min_obj && val != obj && !val->scanned) {
							mm_write_lock(ist, val);
							val->state = OBJ_STATE_NEW_OR_MARKED;
							val->scanned = TRUE;
							mm_write_unlock(val);
							pr_lock(&to_be_scanned_list_lock);
							clist_append(to_be_scanned_list, val);
							pr_unlock(&to_be_scanned_list_lock);
						}
					}
					if (obj_list != obj && obj_list != mm_obj_list)
						del_unlock(obj_list);
				}
			}
			read_unlock(ist, obj);
			if (ist->exception_obj)
				i=0;

			pr_lock(&to_be_scanned_list_lock);
			if (clist_len(to_be_scanned_list)) {
				obj = clist_pop_dups(to_be_scanned_list);
				pr_unlock(&to_be_scanned_list_lock);
			} else {
				pr_unlock(&to_be_scanned_list_lock);
				break;
			}
			if (ist->exception_obj)
				i=0;
		}
		apr_sleep(1000);
		pr_lock(&to_be_scanned_list_lock);
		if(clist_len(to_be_scanned_list)){
			obj = clist_pop(to_be_scanned_list);
			pr_unlock(&to_be_scanned_list_lock);
			goto markscan;
		}
#ifdef DEBUG_MEM_MGR
		printf("Memory manager: Marked %d objects as used, elapsed time(secs): %g\n", 
			    obj_count, ((clock()-time)*1.0)/CLOCKS_PER_SEC );
		printf("Object work list length: %d objects\n", to_be_scanned_list->space);
		time = clock();
		printf("Memory manager phase: Deleting unused objects\n");
#endif
		obj_count = 0; obj_del_count = 0;
		free_clist(to_be_scanned_list);
		mem_mgr_phase = MM_PHASE_DELETING;
		pr_unlock(&to_be_scanned_list_lock);
		apr_thread_cond_broadcast(write_cond);

		pr_lock(&object_list_lock); 
		obj=obj_list_root;
		pr_unlock(&object_list_lock); 
		last_obj = NULL;
		while(obj && !mem_mgr_req_termination) {
			if (obj == obj->next_obj) {
				dump(ist, "!mm-obj-dump.txt", obj);
				pr_assert(FALSE);
			}
			obj_count++;
			if (obj->state != OBJ_STATE_NEW_OR_MARKED && !obj->del_locked) {
				delete_notify(ist, obj);
				call_func(ist, obj, SYM(FINAL_), NULL, 0, NULL, NULL);
				check_exceptions(ist);
				mm_write_lock(ist, obj);
				ist->exception_obj = 0;
				obj->state = OBJ_STATE_DELETED;
				pr_lock(&object_list_lock); 
				if (last_obj)
					last_obj->next_obj = obj->next_obj;
				else
					obj_list_root = obj->next_obj;
				pr_unlock(&object_list_lock);
				obj_del_count++;
				free_object(obj);
			} else {
				last_obj = obj;
			}
			pr_lock(&object_list_lock); 
			obj = obj->next_obj;
			pr_unlock(&object_list_lock); 
		}
#ifdef DEBUG_MEM_MGR
		printf("Memory manager: Deleted %d of %d objects, elapsed time(secs): %g\n", 
			    obj_del_count, obj_count, ((clock()-time)*1.0)/CLOCKS_PER_SEC );
#endif
		last_obj_del_count = obj_del_count;
		last_obj_count     = obj_count;
	}
	if (mem_mgr_req_termination) {
#ifdef DEBUG_THREADS
		if (debug_threads) printf("Memory manager thread terminating\n");
#endif
		mem_mgr_error = MMERR_NORMAL_TERMINATION;
		mem_mgr_running = FALSE;
		mem_mgr_terminated = TRUE;
	}
	return NULL;
}

//********************************* stop_mem_mgr ******************************
void stop_mem_mgr() {
	int mem_mgr_count = 0;
	if (!disable_mem_mgr) {
		mem_mgr_req_termination = TRUE;
		while(!mem_mgr_terminated && ++mem_mgr_count < MEMMGR_TIMEOUT) 
			apr_sleep(PAUSE_MS*1000);
	}
}

