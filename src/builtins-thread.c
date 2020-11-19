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


// builtins.c

#include <stdio.h>
#include <string.h>

#include <apr_strings.h>

#include <prothon/prothon.h>
#include "thread.h"
#include "object.h"
#include <prothon/prothon_dll.h>

obj_p new_thread_object;
clist_p thread_registry;
DECLARE_PR_LOCK(thread_registry_lock);

MODULE_DECLARE(Thread);
MODULE_DECLARE(Mutex);

void thread_init(isp ist) {
	obj_p thread_obj;
	pr_thread_p thread_ptr;
	thread_obj = NEW_OBJ(OBJ(THREAD_PROTO));
	SET_TYPE_IF_EXC(OBJ(THREAD_PROTO), thread_obj, DATA_TYPE_DATAPTR) return;
	thread_ptr = thread_obj->data.ptr  = pr_malloc(sizeof(pr_thread_t));
	memset(thread_obj->data.ptr, 0, sizeof(pr_thread_t));
	thread_ptr->size          = sizeof(pr_thread_t);
	thread_ptr->ist           = ist;
	thread_ptr->apr_os_thread = apr_os_thread_current();
	thread_ptr->running		  = TRUE;
	set_attr(ist, thread_obj, sym(ist, "name"), NEW_STRING("ConsoleThread"));

	pr_lock(&thread_registry_lock);
	thread_registry = new_clist(5);
	clist_append(thread_registry, thread_obj);
	pr_unlock(&thread_registry_lock);
}

static apr_threadattr_t *thread_attr = NULL;
static apr_pool_t *thread_pool = NULL;

//********************************* new_thread_obj ****************************
obj_p new_thread_obj( isp ist, apr_thread_start_t thread_func, 
					  void *thread_data, obj_p thread_obj, int acc_level ) {
	apr_status_t aprerr;
	apr_thread_t *new_thread = NULL;
	pr_thread_p thread_p;

	if (!thread_pool) {
		aprerr = apr_pool_create(&thread_pool, get_pr_head_pool());
		IF_APR_ERR("apr_pool_create") return NULL;
	}

	if (!thread_attr) {
		aprerr = apr_threadattr_create(&thread_attr, thread_pool);
		IF_APR_ERR("apr_threadattr_create") return NULL;
		aprerr = apr_threadattr_detach_set(thread_attr, 1);
		IF_APR_ERR("apr_threadattr_detach_set") return NULL;
	}
	if (thread_obj)
		thread_p = thread_obj->data.ptr;
	else {
		thread_obj = NEW_OBJ(OBJ(THREAD_PROTO));
		SET_TYPE_IF_EXC(OBJ(THREAD_PROTO), thread_obj, DATA_TYPE_DATAPTR) return NULL;
		thread_p = thread_obj->data.ptr = pr_malloc(sizeof(pr_thread_t));
		memset(thread_obj->data.ptr, 0, sizeof(pr_thread_t));
		((pr_thread_p)(thread_obj->data.ptr))->size = sizeof(pr_thread_t);
	}
	thread_p->ist = new_ist(acc_level);

	/* Lock this, so we don't start a new thread, until the last one
	 * calls register_thread(). */
	pr_lock(&thread_registry_lock);

	new_thread_object = thread_obj;
	
	aprerr = apr_thread_create(&new_thread, thread_attr, thread_func, thread_data, thread_pool);
	IF_APR_ERR("apr_thread_create") {
		pr_unlock(&thread_registry_lock);
		return NULL;
	}
	return thread_obj;
}

//**************************** register_thread ********************************
/* New threads must call this first thing, it will release the
 * thread_registry_lock so that other threads can start. 
 * Returns the matching thread object */
obj_p register_thread(apr_thread_t *this_thread)
{
	obj_p res = new_thread_object;
	pr_thread_p thread_ptr    = res->data.ptr;
	thread_ptr->apr_os_thread = apr_os_thread_current();
	thread_ptr->apr_thread    = this_thread;
	clist_append(thread_registry, res);
	thread_ptr->running		  = TRUE;
	pr_unlock(&thread_registry_lock);
	return res;
}

//********************************* os_thread_2_apr ***************************
apr_thread_t* os_thread_2_apr(apr_os_thread_t os_thread) {
	int i;
	apr_thread_t *res = NULL;

	pr_lock(&thread_registry_lock);
	for (i = 0; i < clist_len(thread_registry); i++) {
		obj_p thread_obj = clist_item(thread_registry, i);
		pr_thread_p thread_ptr = thread_obj->data.ptr;
		if (thread_ptr->apr_os_thread == os_thread) {
			res = thread_ptr->apr_thread;
			break;
		}
	}
	pr_unlock(&thread_registry_lock);

	return res;
}

//********************************* os_thread_2_obj ***************************
obj_p os_thread_2_obj(apr_os_thread_t os_thread) {
	int i, llen;
	obj_p res = NULL;

	pr_lock(&thread_registry_lock);
	llen = clist_len(thread_registry);
	for (i = 0; i < llen; i++) {
		res = clist_item(thread_registry, i);
		if (((pr_thread_p)(res->data.ptr))->apr_os_thread == os_thread) 
			break;
	}
	if (i == llen) res = NULL;
	pr_unlock(&thread_registry_lock);
	return res;
}

// ***************************** Thread ***************************************

MODULE_START(Thread)
{
	pr_thread_p thread;

	Thread_OBJ = OBJ(THREAD_PROTO);
	MODULE_SET_DOC(Thread, "thread object prototype");

	set_attr(ist, OBJ(OBJECT), sym(ist,  "Thread"), Thread_OBJ);
	set_attr(ist,  Thread_OBJ, sym(ist,   "GUEST"), NEW_INT( ACC_GUEST));
	set_attr(ist,  Thread_OBJ, sym(ist,   "USER1"), NEW_INT( ACC_USER1));
	set_attr(ist,  Thread_OBJ, sym(ist,   "USER2"), NEW_INT( ACC_USER2));
	set_attr(ist,  Thread_OBJ, sym(ist,  "SYSTEM"), NEW_INT(ACC_SYSTEM));

    thread = obj_malloc(ist, Thread_OBJ, Thread_OBJ, sizeof(pr_thread_t));
	if_exc_return;
    memset(thread, 0, sizeof(pr_thread_t));
	thread->size = sizeof(pr_thread_t);
	Thread_OBJ->unclonable = TRUE;
}


DEF(Thread, init_, list8(ist, sym(ist, "func"),   NULL,
								 sym(ist, "params"), OBJ(NONE),
								 sym(ist, "name"),   OBJ(NONE),
								 sym(ist, "access"), NEW_INT(-1) ) ) {
	char buf[30];
	obj_p name_obj;
	user_thread_p uthread;
	int access_level;

	BIN_EMPTY_CHK();
	CHECK_TYPE_EXC(parms[1], OBJ(FUNC_PROTO),   "function");
	if (parms[3] != OBJ(NONE))
		CHECK_TYPE_EXC(parms[3], OBJ(SEQ_PROTO), "sequence");
	if (parms[5] != OBJ(NONE))
		CHECK_TYPE_EXC(parms[5], OBJ(STRING_PROTO), "string");
	CHECK_TYPE_EXC(parms[7], OBJ(INT_PROTO),    "integer");
	SET_TYPE_IF_EXC(Thread_OBJ, self, DATA_TYPE_DATAPTR) return NULL;
	self->data.ptr  = pr_malloc(sizeof(pr_thread_t));
	memset(self->data.ptr, 0, sizeof(pr_thread_t));
	((pr_thread_p)(self->data.ptr))->size = sizeof(pr_thread_t);
	uthread = &(((pr_thread_p)self->data.ptr)->user_thread_data);
	uthread->func   = parms[1]; 
	uthread->params = parms[3]; 
	access_level = (int) parms[7]->data.i64;
	if (access_level == -1) access_level = ist->access;
	if (ist->access < access_level) {
		raise_exception(ist, OBJ(PERMISSION_EXC), 
			"attempt to start thread with higher access level than own");	
		return NULL;
	}
	if (parms[5] == OBJ(NONE)) {
		apr_snprintf(buf, sizeof(buf), "uThread%lx", (unsigned long)(intptr_t) self);
		name_obj = NEW_STRING(buf);
	} else
		name_obj = parms[5];
	read_unlock(ist, self);
	set_attr(ist, self, sym(ist, "name"), name_obj);
	new_thread_obj(ist, (apr_thread_start_t) user_thread, uthread, self, access_level);
	read_lock(ist, self);
	self->unclonable = TRUE;
	return OBJ(NONE);
}

DEF(Thread, str_, NULL) {
	obj_p name_obj;
	char name[80];
	BIN_STR_CHK(Thread);
	if ((name_obj = get_attr(ist, self, sym(ist, "name"))))
		apr_snprintf(name, sizeof(name), "<Thread:%s>", pr2c_strptr(ist, name_obj));
	else 
		apr_snprintf(name, sizeof(name), "<Thread:%lx>", (unsigned long)(uintptr_t)self);
	return NEW_STRING(name);
}

DEF(Thread, access, NULL) {
	BIN_CONTENT_CHK(Thread);
	return NEW_INT(ist->access);
}

DEF(Thread, setAccess, FORM_RPARAM) {
	int new_access;
	BIN_CONTENT_CHK(Thread);
	CHECK_TYPE_EXC(parms[1], OBJ(INT_PROTO), "integer");
	new_access = (int) parms[1]->data.i64;
	if (new_access > ist->access) {
		raise_exception(ist, OBJ(PERMISSION_EXC), 
			"attempt to raise thread access level");	
		return NULL;
	}
	ist->access = new_access;
	return parms[1];
}

DEF(Thread, accessStr, NULL) {
	BIN_CONTENT_CHK(Thread);
	switch (ist->access) {
		case 0:	return NEW_STRING("guest");
		case 1: return NEW_STRING("user1");
		case 2: return NEW_STRING("user2");
		case 3: return NEW_STRING("system");
		default:return NEW_STRING("<invalid access value>");
	}
}

DEF(Thread, running_QUES, NULL) {
	BIN_CONTENT_CHK(Thread);
	if (thread_running(self)) return OBJ(PR_TRUE);
	else                      return OBJ(PR_FALSE);
}

DEF(Thread, sleep, FORM_RPARAM) {
	BIN_CONTENT_CHK(Thread);
	CHECK_TYPE_EXC(parms[1], OBJ(FLOAT_PROTO), "float");
	apr_sleep((apr_interval_time_t)((parms[1]->data.f64)*1000000));
	return parms[1];
}

DEF(Thread, join, NULL) {
	pr_thread_p thread_ptr = self->data.ptr;
	apr_status_t retval, aprerr;
	BIN_CONTENT_CHK(Thread);
	if (!(thread_ptr->apr_thread)) {
		raise_exception(ist, OBJ(INTERPRETER_EXC), "cannot join this thread");
		return NULL;
	}
	aprerr = apr_thread_join(&retval, thread_ptr->apr_thread);
	IF_APR_ERR("error in thread join") return NULL;
	return NEW_INT(retval);
}

DEF(Thread, objList_, FORM_RPARAM) {
	return parms[1];
}

// ***************************** Mutex ***************************************

MODULE_START(Mutex)
{
	Mutex_OBJ = OBJ(MUTEX_PROTO);
	MODULE_SET_DOC(Mutex, "mutex object prototype");

	set_attr(ist, OBJ(OBJECT), sym(ist,  "Mutex"), Mutex_OBJ);
	Mutex_OBJ->unclonable = TRUE;
}


DEF(Mutex, init_, NULL) {
	apr_status_t aprerr;
	apr_pool_t *subpool;

	aprerr = apr_pool_create(&subpool, get_pr_head_pool());
	IF_APR_ERR("out of memory creating mutex") return NULL;
	SET_TYPE_IF_EXC(Mutex_OBJ, self, DATA_TYPE_DATAPTR) return NULL;
	aprerr = apr_thread_mutex_create( (apr_thread_mutex_t**)(&self->data.ptr), 
		                               APR_THREAD_MUTEX_UNNESTED, subpool );
	IF_APR_ERR("error creating mutex in apr_thread_mutex_create") return NULL;
	self->unclonable = TRUE;
	return OBJ(NONE);
}

DEF(Mutex, str_, NULL) {
	char name[30];
	BIN_STR_CHK(Mutex);
	apr_snprintf(name, sizeof(name), "<Mutex:%lx>", (unsigned long)(intptr_t)self);
	return NEW_STRING(name);
}

DEF(Mutex, lock, NULL) {
	apr_status_t aprerr;
	BIN_CONTENT_CHK(Mutex);
	aprerr = apr_thread_mutex_lock((apr_thread_mutex_t*)(self->data.ptr));
	IF_APR_ERR("error in apr_thread_mutex_lock") return NULL;
	return self;
}

DEF(Mutex, tryLock, NULL) {
	apr_status_t aprerr;
	BIN_CONTENT_CHK(Mutex);
	aprerr = apr_thread_mutex_trylock((apr_thread_mutex_t*)(self->data.ptr));
	if (APR_STATUS_IS_EBUSY(aprerr)) return OBJ(PR_FALSE);
	else if (aprerr == APR_SUCCESS)	 return OBJ(PR_TRUE);
	IF_APR_ERR("error in apr_thread_mutex_trylock") return NULL;
	return NULL;
}

DEF(Mutex, unlock, NULL) {
	apr_status_t aprerr;
	BIN_CONTENT_CHK(Mutex);
	aprerr = apr_thread_mutex_unlock((apr_thread_mutex_t*)(self->data.ptr));
	IF_APR_ERR("error in apr_thread_mutex_unlock") return NULL;
	return self;
}

DEF(Mutex, final_, NULL) {
	if (self->data.ptr)
		apr_thread_mutex_destroy((apr_thread_mutex_t*)(self->data.ptr));
	return OBJ(NONE);
}

DEF(Mutex, objList_, FORM_RPARAM) {
	return parms[1];
}


MAIN_MODULE_INIT(Thread) {
	MODULE_SUB_INIT(Thread);
	MODULE_ADD_SYM(Thread, init_);
	MODULE_ADD_SYM(Thread, str_);
	MODULE_ADD_SYM(Thread, access);
	MODULE_ADD_SYM(Thread, accessStr);
	MODULE_ADD_SYM(Thread, setAccess);
	MODULE_ADD_SYM(Thread, running_QUES);
	MODULE_ADD_SYM(Thread, sleep);
	MODULE_ADD_SYM(Thread, join);
	MODULE_ADD_SYM(Thread, objList_);

	MODULE_SUB_INIT(Mutex);
	MODULE_ADD_SYM(Mutex, init_);
	MODULE_ADD_SYM(Mutex, str_);
	MODULE_ADD_SYM(Mutex, lock);
	MODULE_ADD_SYM(Mutex, tryLock);
	MODULE_ADD_SYM(Mutex, unlock);
	MODULE_ADD_SYM(Mutex, final_);
	MODULE_ADD_SYM(Mutex, objList_);

	check_exceptions(ist);
}
