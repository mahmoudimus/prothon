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


// object.c

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <apr_strings.h>

#include "object.h"
#include "bytecodes.h"
#include "clist.h"
#include "memory_mgr.h"
#include "interp.h"
#include "dict.h"

obj_p            obj_list_root;
DECLARE_PR_LOCK(object_list_lock);
pr_services_t dll_services;

#if defined(DEBUG_THREADS) && defined(WIN32)
// FROM VISUAL STUDIO HELP PAGE "Setting a Thread Name (Unmanaged)"
//
// Usage: SetThreadName ("MainThread");
//
typedef struct tagTHREADNAME_INFO {
   DWORD dwType;		// must be 0x1000
   LPCSTR szName;		// pointer to name (in user addr space)
   DWORD dwThreadID;	// thread ID (-1=caller thread)
   DWORD dwFlags;		// reserved for future use, must be zero
} THREADNAME_INFO;

void SetThreadName(LPCSTR szThreadName) {
	THREADNAME_INFO info;
	info.dwType		= 0x1000;
	info.szName		= szThreadName;
	info.dwThreadID = -1;
	info.dwFlags	= 0;
	__try {
		RaiseException(0x406D1388, 0, sizeof(info)/sizeof(DWORD), (CONST ULONG_PTR*) &info);
	}
	__except(EXCEPTION_CONTINUE_EXECUTION) {
	}
}
#endif

obj_p OBJ[OBJ_ENUM_COUNT];
obj_p core_object_marker;

//********************************* object_store_init ***************************
void object_store_init(isp ist, int nop2){
	apr_status_t aprerr;
	char buf[1024];

	if ((aprerr = apr_thread_cond_create(&obj_wrlock_cond, get_pr_head_pool())) ||
	    (aprerr = apr_thread_cond_create(&write_cond, get_pr_head_pool()))) {
		printf("object_store_init error: apr_thread_cond_create %s\n",
		       apr_strerror(aprerr, buf, sizeof(buf)));
		pr_exit(1);
	}

	if ((aprerr = apr_thread_mutex_create(&obj_wrlock_mutex,
					     APR_THREAD_MUTEX_DEFAULT,
					     get_pr_head_pool())) ||
	    (aprerr = apr_thread_mutex_create(&write_mutex,
					      APR_THREAD_MUTEX_DEFAULT,
					      get_pr_head_pool()))) {
		printf("object_store_init error: apr_thread_mutex_create %s\n",
		       apr_strerror(aprerr, buf, sizeof(buf)));
		pr_exit(1);
	}

	/* Setup the services struct */
	dll_services.new_ist		  = new_ist;		
	dll_services.get_attr		  = get_attr;		
	dll_services.set_attr		  = set_attr;		
	dll_services.del_attr		  = del_attr;		
	dll_services.add_proto		  = add_proto;		
	dll_services.del_proto		  = del_proto;		
	dll_services.has_proto	      = has_proto;	
	dll_services.switch_proto     = switch_proto;
	dll_services.list_append	  = list_append;
	dll_services.list_item		  = list_item;	
	dll_services.list_item_set	  = list_item_set;	
	dll_services.list_len		  = list_len;
	dll_services.list_clear		  = list_clear;
	dll_services.new_hash_obj	  = new_hash_obj;	
	dll_services.new_int_obj	  = new_int_obj;	
	dll_services.new_float_obj	  = new_float_obj;	
	dll_services.new_string_obj	  = new_string_obj;	
	dll_services.new_string_n_obj = new_string_n_obj;	
	dll_services.c2pr_strcpy  = c2pr_strcpy;	
	dll_services.new_tuple_obj	  = new_tuple_obj;	
	dll_services.new_list_obj	  = new_list_obj;
	dll_services.init_list_data	  = init_list_data;
	dll_services.copy_list_obj	  = copy_list_obj;
	dll_services.new_C_func_obj	  = new_C_func_obj;	
	dll_services.call_func		  = call_func;
	dll_services.call_func1_f	  = call_func1_f;
	dll_services.call_func2_f	  = call_func2_f;
	dll_services.sym			  = sym;			
	dll_services.tuple1			  = tuple1;		
	dll_services.tuple2			  = tuple2;		
	dll_services.list1			  = list1;		
	dll_services.list2			  = list2;		
	dll_services.list3			  = list3;		
	dll_services.list4			  = list4;		
	dll_services.list6			  = list6;		
	dll_services.list8			  = list8;		
	dll_services.list10			  = list10;		
	dll_services.list16			  = list16;		
	dll_services.raise_exception  = raise_exception;
	dll_services.new_object       = new_object;
	dll_services.copy_object_data = copy_object_data;
	dll_services.set_obj_doc_f    = set_obj_doc_f;
	dll_services.obj_malloc		  = obj_malloc;
	dll_services.obj_realloc      = obj_realloc;
	dll_services.obj_free		  = obj_free;
	dll_services.OBJ			  = OBJ;
	dll_services.sym_id_table	  = sym_id_table;
	dll_services.pr_malloc	      = pr_malloc;
	dll_services.pr_realloc		  = pr_realloc;	
	dll_services.pr_free		  = pr_free;	
	dll_services.read_lock		  = read_lock;
	dll_services.read_unlock	  = read_unlock;
	dll_services.write_lock		  = write_lock;
	dll_services.write_unlock	  = write_unlock;
	dll_services.del_lock		  = del_lock;
	dll_services.del_unlock	      = del_unlock;
	dll_services.set_immutable	  = set_immutable;
	dll_services.clr_immutable	  = clr_immutable;
	dll_services.set_archived     = set_archived;  
	dll_services.clr_archived	  = clr_archived;	
	dll_services.is_archived	  = is_archived;	
	dll_services.get_pr_head_pool = get_pr_head_pool;
	dll_services.dict_keys_values = dict_keys_values;
	dll_services.set_obj_rdacc    = set_obj_rdacc;
	dll_services.set_obj_wracc	  = set_obj_wracc;
	dll_services.get_obj_rdacc	  = get_obj_rdacc;
	dll_services.get_obj_wracc	  = get_obj_wracc;
	dll_services.check_exceptions = check_exceptions;
	dll_services.attr_first_key    = attr_first_key;
	dll_services.attr_next_key	   = attr_next_key;
	dll_services.attr_value_by_key = attr_value_by_key;
	dll_services.dict_add		   = dict_add;
	dll_services.dict_item		   = dict_item;		  
	dll_services.is_unclonable	   = is_unclonable;
	dll_services.symch   		   = symch;	  
	dll_services.key_to_sym		   = key_to_sym;	  
	dll_services.new_dict_obj	   = new_dict_obj;	  
	dll_services.dict_clear	       = dict_clear;	  
	dll_services.proto_len		   = proto_len;	  
	dll_services.proto_item		   = proto_item;		 
	dll_services.dump			   = dump;		 
	dll_services.int2i32t		   = int2i32t;		 
	dll_services.int2i64t		   = int2i64t;		 
	dll_services.pr2c_strptr	   = pr2c_strptr;		 
	dll_services.proto_owns_binary = proto_owns_binary;		 
	dll_services.obj_set_data_owner	= obj_set_data_owner;		 

	/* Setup the core objects. Order counts here */
	OBJ(OBJECT)             = NEW_OBJ(NULL);
	OBJ(OBJECT)->attr_proto.proto = OBJ(OBJECT);

	OBJ(PR_FALSE)           = NEW_OBJ(NULL);
	OBJ(PR_TRUE)            = NEW_OBJ(NULL);
	OBJ(NONE)               = NEW_OBJ(NULL);
	OBJ(NOKEY)       = NEW_OBJ(NULL);
	OBJ(PARAM_STAR)         = NEW_OBJ(NULL);
	OBJ(PARAM_STAR_STAR)    = NEW_OBJ(NULL);
	OBJ(NODEF)				= NEW_OBJ(NULL);
	OBJ(SEQ_PROTO)          = NEW_OBJ(NULL);
	OBJ(TUPLE_PROTO)        = NEW_OBJ(OBJ(SEQ_PROTO));
	OBJ(LIST_PROTO)         = NEW_OBJ(OBJ(TUPLE_PROTO));
	OBJ(SYMBOL_PROTO)       = NEW_OBJ(NULL);
	OBJ(DATETIME_PROTO)     = NEW_OBJ(NULL);
	OBJ(HASH_PROTO)         = NEW_OBJ(NULL);
	OBJ(SLICE_PROTO)        = NEW_OBJ(NULL);
	OBJ(LOCAL_PROTO)        = NEW_OBJ(NULL);
	OBJ(OUTER_PROTO)        = NEW_OBJ(NULL);
	OBJ(SUPER_PROTO)         = NEW_OBJ(NULL);
	OBJ(THREAD_PROTO)       = NEW_OBJ(NULL);
	OBJ(MUTEX_PROTO)        = NEW_OBJ(NULL);
	OBJ(PARMPTR_PROTO)      = NEW_OBJ(NULL);
	OBJ(PROP_PROTO)         = NEW_OBJ(NULL);
	OBJ(ROOT_GLOBALS)       = NEW_OBJ(NULL);
	OBJ(MODULES)            = NEW_OBJ(NULL);
	OBJ(SYS)				= NEW_OBJ(NULL);
	OBJ(EXCEPTION)          = NEW_OBJ(NULL);
	OBJ(FUNC_PROTO)         = NEW_OBJ(NULL);
	OBJ(ITER_PROTO)         = NEW_OBJ(NULL);
	OBJ(GEN_PROTO)          = NEW_OBJ(OBJ(ITER_PROTO));
	OBJ(INT_PROTO)          = NEW_OBJ(NULL);
	OBJ(BYTES_PROTO)        = NEW_OBJ(OBJ(SEQ_PROTO));
	OBJ(STRING_PROTO)       = NEW_OBJ(OBJ(SEQ_PROTO));
	OBJ(DICT_PROTO)         = NEW_OBJ(NULL);
	OBJ(METHOD_PROTO)       = NEW_OBJ(OBJ(FUNC_PROTO));

	OBJ(SYMBOLS)            = NEW_DICT(SYM_ENUM_COUNT+100);

	init_symbol(ist);

	set_obj_doc(OBJ(SYMBOLS), "Symbols: dictionary of all symbol objects");

	/* Now load the actual modules for the builtin prototypes */
	BUILTIN_LOAD(Core);

	pr_lock(&object_list_lock);
	core_object_marker = obj_list_root;
	pr_unlock(&object_list_lock);

	BUILTIN_LOAD(Dict);
	BUILTIN_LOAD(AttrDict);
	BUILTIN_LOAD(Bytes);
	BUILTIN_LOAD(File);
	BUILTIN_LOAD(Float);
	BUILTIN_LOAD(Int);
	BUILTIN_LOAD(List);
	BUILTIN_LOAD(ProtoList);
	BUILTIN_LOAD(String);
	BUILTIN_LOAD(Thread);
	BUILTIN_LOAD(Tuple);
	BUILTIN_LOAD(DateTime);
}

//********************************* debug_object ******************************
obj_p debug_obj       = NULL;
obj_p debug_obj_proto = NULL;

obj_p debug_object(obj_p obj) {
	debug_obj = obj;
	debug_obj_proto = obj->attr_proto.proto;
	return obj;
}

//********************************* new_object ********************************
obj_p new_object(isp ist, obj_p proto){
	obj_p obj = pr_malloc(sizeof(obj_t));
	if (obj == (obj_p) 0xa192f0) 
		memset(obj, 0, sizeof(obj_t));
	memset(obj, 0, sizeof(obj_t));
	if (!proto) proto = OBJ(OBJECT);
	obj->rd_access = ACC_GUEST;
	if (ist)
		obj->wr_access = ist->access;
	else
		obj->wr_access = ACC_SYSTEM;
	obj->attr_proto.proto = proto;
	lock_init(obj->wrlock);
	obj->del_locked = TRUE;
	pr_lock(&object_list_lock);
	obj->next_obj = obj_list_root;
	obj_list_root = obj;
	pr_unlock(&object_list_lock);
	return obj;
}

//********************************* copy_object_data **************************
// clobbers data section of dest (copy) object
void copy_object_data(isp ist, obj_p copy, obj_p obj) {
	int wrlock_req_cnt;
	attr_p new_attrp, attrp;
	wrlock_rtrn(obj);
	if_wrlock(copy) { write_unlock(ist, obj); return; }
	wrlock_req_cnt = copy->wrlock_req_cnt;
	memcpy(copy, obj, sizeof(*obj)-2*sizeof(obj->next_obj));
	copy->data = obj->data;
	copy->wrlock_req_cnt = wrlock_req_cnt;
	if (obj->has_attrs) {
		size_t size;
		attrp = obj->attr_proto.attrs;
		size = attr_tsize(attrp)*sizeof(attr_t);
		copy->attr_proto.attrs = new_attrp = pr_malloc(size);
		memcpy(new_attrp, attrp, size);
	}
	write_unlock(ist, copy);
	write_unlock(ist, obj);
}

//********************************* copy_object ******************************
// clones everything but data pointed to by data.ptr
obj_p copy_object(isp ist, obj_p obj) {
	obj_p copy;
	attr_p new_attrp, attrp;
	copy = NEW_OBJ(NULL);
	memcpy(copy, obj, sizeof(*obj)-2*sizeof(obj->next_obj));
	copy->data = obj->data;
	copy->archived       = FALSE;
	copy->del_locked     = TRUE;
	copy->rdlock_cnt     = 0;
	copy->wrlock_req_cnt = 0;
	copy->wrlock		  = 0;
	if (obj->has_attrs) {
		size_t size;
		attrp = obj->attr_proto.attrs;
		size = attr_tsize(attrp)*sizeof(attr_t);
		copy->attr_proto.attrs = new_attrp = pr_malloc(size);
		memcpy(new_attrp, attrp, size);
	}
	return copy;
}

//********************************* obj_set_data_owner ************************
void obj_set_data_owner(isp ist, obj_p obj, obj_p owner) {
	attr_p attrp;
	int i, numprotos, owner_set = FALSE; 
	if (obj == owner || !obj->has_attrs) return;
	attrp = obj->attr_proto.attrs;
	numprotos = attr_plen(attrp);
	for (i=0; i < numprotos; i++) {
		obj_p proto = attr_proto_item(attrp, i);
		owner_set |= ( attr_pp(attrp, i)->proto.owns_binary = 
					  ((proto == owner) || has_proto(ist, proto, owner)) );
	}
	if (!owner_set)
		raise_exception(ist, OBJ(INTERNAL_EXC), "obj_set_data_owner error");
}

//********************************* proto_owns_binary *************************
int proto_owns_binary(isp ist, obj_p proto, obj_p obj) {
	attr_p attrp;
	obj_p owner = NULL;
	int i, numprotos, res; 	
	if (obj->data_type == DATA_TYPE_NONE) return FALSE;
	if (proto == obj) return TRUE;
	if (!obj->has_attrs) owner = obj->attr_proto.proto;
	else {
		attrp = obj->attr_proto.attrs;
		numprotos = attr_plen(attrp);
		for (i=0; i < numprotos; i++)
			if (attr_pp(attrp, i)->proto.owns_binary) {
				owner = attr_proto_item(attrp, i);
				break;
			}
	}
	res =  owner &&
		( (proto == owner) || has_proto(ist, proto, owner) || 
		                      has_proto(ist, owner, proto) );
	if (!res)
		owner = NULL;
	return res;
}

//********************************* free_object *******************************
void free_object(obj_p obj) {
	if (obj->has_attrs)
		pr_free(obj->attr_proto.attrs);
	if (obj->data_type == DATA_TYPE_DATAPTR)
		pr_free(obj->data.ptr);
	pr_free(obj);
}

//********************************* set_immutable *****************************
void set_immutable(obj_p obj){
	get_wrlock(obj, NO_WAIT);
	obj->immutable = TRUE; 
	free_wrlock(obj);
}

//********************************* clr_immutable *****************************
void clr_immutable(obj_p obj){
	get_wrlock(obj, NO_WAIT);
	obj->immutable = FALSE; 
	free_wrlock(obj);
}

//********************************* is_immutable ******************************
int is_immutable(obj_p obj) {
	int res;
	get_wrlock(obj, NO_WAIT);
	res = obj->immutable;
	free_wrlock(obj);
	return res;
}

//********************************* set_unclonable ****************************
void set_unclonable(obj_p obj){
	get_wrlock(obj, NO_WAIT);
	obj->unclonable = TRUE; 
	free_wrlock(obj);
}

//********************************* clr_unclonable ****************************
void clr_unclonable(obj_p obj){
	get_wrlock(obj, NO_WAIT);
	obj->unclonable = FALSE; 
	free_wrlock(obj);
}

//********************************* is_unclonable *****************************
int is_unclonable(obj_p obj) {
	int res;
	get_wrlock(obj, NO_WAIT);
	res = obj->unclonable; 
	free_wrlock(obj);
	return res;
}

//********************************* set_archived ****************************
void set_archived(obj_p obj){
	get_wrlock(obj, NO_WAIT);
	obj->archived = TRUE; 
	free_wrlock(obj);
}

//********************************* clr_archived ****************************
void clr_archived(obj_p obj){
	get_wrlock(obj, NO_WAIT);
	obj->archived = FALSE; 
	free_wrlock(obj);
}

//********************************* is_archived *****************************
int is_archived(obj_p obj) {
	int res;
	get_wrlock(obj, NO_WAIT);
	res = obj->archived; 
	free_wrlock(obj);
	return res;
}

//********************************* set_obj_rdacc *****************************
void set_obj_rdacc(obj_p obj, int access) {
	get_wrlock(obj, NO_WAIT);
	obj->rd_access = access; 
	free_wrlock(obj);
}

//********************************* set_obj_wracc *****************************
void set_obj_wracc(obj_p obj, int access) {
	get_wrlock(obj, NO_WAIT);
	obj->wr_access = access; 
	free_wrlock(obj);
}

//********************************* get_obj_rdacc *****************************
int get_obj_rdacc(obj_p obj) {
	int res;
	get_wrlock(obj, NO_WAIT);
	res = obj->rd_access;
	free_wrlock(obj);
	return res;
}

//********************************* get_obj_wracc *****************************
int get_obj_wracc(obj_p obj) {
	int res;
	get_wrlock(obj, NO_WAIT);
	res = obj->wr_access;
	free_wrlock(obj);
	return res;
}

//********************************* raise_exception ***************************
void raise_exception(isp ist, obj_p proto_obj, const char *format, ...)
{
	obj_p exc_obj;
	if (!proto_obj)
		proto_obj = OBJ(EXCEPTION);
	exc_obj = NEW_OBJ(proto_obj);
	if (format) {
		char *p, err_buf[512];
		va_list ap;

		va_start(ap, format);
		apr_vsnprintf(err_buf, sizeof(err_buf), format, ap);
		va_end(ap);

		exc_obj->wr_access = ACC_GUEST;

		p = err_buf + strlen(err_buf) - 1;
		while (*p == '.' || *p == ' ' || *p == '\t') { *p = 0; p--; }

		set_obj_doc(exc_obj, err_buf);
	}
	ist->exception_obj = exc_obj;
}

//********************************* if_exc_return ************************************
obj_p catch_exception(isp ist, obj_p proto, obj_p proto_list){
	int i;
	obj_p exc_obj = ist->exception_obj;
	ist->exception_obj = NULL;
	if (exc_obj) {
		if (!proto && !proto_list)
			return exc_obj;
		if (proto && has_proto(ist, exc_obj, proto))
			return exc_obj;
		if (proto_list)
			for (i=0; i < (int) list_len(ist, proto_list); i++){
				if (has_proto(ist, exc_obj, list_item(ist, proto_list, i)))
					return exc_obj;
			}
	}
	ist->exception_obj = exc_obj;
	return NULL;
}

//********************************* get_attr *******************************
obj_p get_attr(isp ist, obj_p obj, obj_p key) {
	rdlock_rtrn(obj) NULL;
	if (obj->has_attrs){
		attr_p ptr, attrs = obj->attr_proto.attrs;
		attr_key_t i, akey, tgt = key->data.key;
		obj_p res;
		for(i=tgt; (akey=((ptr=attr_ap(attrs, i))->attr.key)) && (tgt!=akey); i++);
		if (akey) {
			res = ptr->attr.value;
			read_unlock(ist, obj);
			return res;
		}
	}
	read_unlock(ist, obj);
	return NULL;
}

//********************************* del_attr *******************************
int del_attr(isp ist, obj_p obj, obj_p key) {
	wrlock_rtrn(obj) FALSE;
	if (obj->has_attrs){
		attr_p ptr, attrs = obj->attr_proto.attrs;
		attr_key_t i, akey, tgt = key->data.key;
		for(i=tgt; (akey=((ptr=attr_ap(attrs, i))->attr.key)) && (tgt!=akey); i++);
		if (akey) {
			ptr->attr.key = ENTRY_DELETED;
			attr_alen(attrs)--;
			write_unlock(ist, obj);
			return TRUE;
		}
	}
	write_unlock(ist, obj);
	return FALSE;
}

//********************************* add_attrs *********************************
attr_p add_attrs(obj_p obj) {
	obj_p proto = obj->attr_proto.proto;
	attr_p attrs = pr_malloc(ATTRS_TINIT * sizeof(attr_t));
	memset(attrs, 0, ATTRS_TINIT * sizeof(attr_t));
	attr_psize(attrs)= ATTRS_PINIT;
	attr_asize(attrs)= ATTRS_AINIT;
	attr_plen(attrs) = 1;
	attr_proto_item(attrs, 0) = proto;
	if (obj->data_type != DATA_TYPE_NONE)
		attr_owns_bin(attrs, 0) = TRUE;
	attr_alen(attrs) = 0;
	obj->attr_proto.attrs = attrs;
	obj->has_attrs   = TRUE;
	return attrs;
}

//********************************* attrs_realloc *****************************
attr_p attrs_realloc(obj_p obj, attr_p attrs) {
	attr_key_t i;
	attr_p old_attrs = attrs;
	i32_t  old_bsize = attr_psize(attrs);
	i32_t  old_asize = attr_asize(attrs);
	i32_t bsize =  attr_plen(attrs) + ATTRS_PROTO_PADDING;
	i32_t asize = max((attr_alen(attrs) * ATTRS_PAD_PERCENT_FACTOR) / 100, ATTRS_AINIT);
	i32_t blen  =  attr_plen(attrs);
	i32_t alen  =  attr_alen(attrs);
	size_t tsize = (size_t)(ATTRS_OVERHEAD+bsize+asize) * sizeof(attr_t);
	attrs = pr_malloc(tsize);
	memset(attrs, 0, tsize);
	attr_psize(attrs) = bsize;
	attr_asize(attrs) = asize;
	attr_plen(attrs)  = blen;
	attr_alen(attrs)  = alen;
	memcpy(attrs+ATTRS_OVERHEAD, old_attrs+ATTRS_OVERHEAD, blen * sizeof(attr_t));
	for (i=ATTRS_OVERHEAD+old_bsize; i < ATTRS_OVERHEAD+old_bsize+old_asize; i++){
		attr_key_t j, key;
		attr_p ptr, old_attr = old_attrs+i;
		if ((key=(old_attr->attr.key)) > 0) {
			for(j=key; (ptr=attr_ap(attrs, j))->attr.key; j++);
			ptr->attr.key   = key;
			ptr->attr.value = old_attr->attr.value;
		}
	}
	obj->attr_proto.attrs = attrs;
	pr_free(old_attrs);
	return attrs;
}

//********************************* set_attr **********************************
void set_attr(isp ist, obj_p obj, obj_p key, obj_p value) {
	attr_p attrs, ptr, hole=0;
	attr_key_t i, akey, tgt = key->data.key;
	if (tgt%3 != 0)
		ptr = NULL;
	pr_assert((tgt%3)==0);
	wrlock_rtrn(obj);
	if (obj->has_attrs) attrs = obj->attr_proto.attrs;
	else				attrs = add_attrs(obj);
	if (((attr_alen(attrs) * 100) / attr_asize(attrs)) > ATTRS_MAX_PERCENT_FULL)
		attrs = attrs_realloc(obj, attrs);
	for(i=tgt; (akey=((ptr=attr_ap(attrs,i))->attr.key)); i++) {
		if (akey < 0) hole = ptr;
		else if (tgt == akey) break;
	}
	if (tgt != akey) {
		attr_alen(attrs)++;
		if (hole) ptr = hole;
		ptr->attr.key = tgt;
	}
	ptr->attr.value = value;
	write_unlock(ist, obj);
}

//********************************* attr_first_key ****************************
attr_key_t attr_first_key(isp ist, obj_p obj) {
	int i;
	size_t asize;
	attr_p attrp;
	if (!obj->has_attrs) return 0;
	attrp = obj->attr_proto.attrs;
	asize = attr_asize(attrp);
	for (i=0; i < (int) asize; i++) {
		if (attr_ap(attrp,i)->attr.key > 0)
			return attr_ap(attrp,i)->attr.key;
	}
	return 0;
}

//********************************* attr_next_key *****************************
attr_key_t attr_next_key(isp ist, obj_p obj, attr_key_t key) {
	if (obj->has_attrs){
		attr_p attrs = obj->attr_proto.attrs;
		attr_key_t i, akey, tgt = key;
		int asize = (int) attr_asize(attrs);
		for(i=tgt; (akey=(attr_ap(attrs, i))->attr.key) && (tgt!=akey); i++);
		if (akey) {
			for(i=(i+1)%asize; (akey=(attr_ap(attrs, i))->attr.key) <= 0 && i < asize; i++);
			if (akey > 0 && i < asize) return akey;
		}
	} 
	return 0;
}

//********************************* attr_value_by_key *************************
obj_p attr_value_by_key(isp ist, obj_p obj, attr_key_t key) {
	if (obj->has_attrs){
		attr_p ptr, attrs = obj->attr_proto.attrs;
		attr_key_t i, akey, tgt = key;
		for(i=tgt; (akey=((ptr=attr_ap(attrs, i))->attr.key)) && (tgt!=akey); i++);
		if (akey) return ptr->attr.value;
	} 
	return NULL;
}

//********************************* switch_proto ******************************
void switch_proto(isp ist, obj_p obj, obj_p new_proto) {
	attr_p attrs;
	wrlock_rtrn(obj);
	if (!proto_owns_binary(ist, new_proto, obj)) {
		if (obj->data_type == DATA_TYPE_DATAPTR)
			pr_free(obj->data.ptr);
		obj->data_type = DATA_TYPE_NONE;
	}
	if (!(obj->has_attrs)) {
		obj->attr_proto.proto = new_proto;
		write_unlock(ist, obj);
		return;
	}
	attrs = obj->attr_proto.attrs;
	attr_plen(attrs) = 1;
	attr_proto_item(attrs, 0) = new_proto;
	write_unlock(ist, obj);
	return;
}

//********************************* add_proto **********************************
int add_proto(isp ist, obj_p obj, obj_p proto){
	attr_p attrs;
	i32_t i, len;
	wrlock_rtrn(obj) FALSE;
	if (obj->has_attrs) attrs = obj->attr_proto.attrs;
	else				attrs = add_attrs(obj);
	len = attr_plen(attrs);
	for (i=0; i < len; i++)
		if (attr_proto_item(attrs, i) == proto) {
			write_unlock(ist, obj);
			return FALSE;
		}
	if (len == attr_psize(attrs)) {
		attrs = attrs_realloc(obj, attrs);
		len = attr_plen(attrs);
	}
	attr_proto_item(attrs, attr_plen(attrs)++) = proto;
	write_unlock(ist, obj);
	return TRUE;
}

//********************************* del_proto **********************************
int del_proto(isp ist, obj_p obj, obj_p proto) {
	attr_p attrs;
	i32_t i, len;
	wrlock_rtrn(obj) FALSE;
	if (!obj->has_attrs) {
		write_unlock(ist, obj);
		return FALSE;
	}
	attrs = obj->attr_proto.attrs;
	len = attr_plen(attrs);
	if (len < 2)  {
		write_unlock(ist, obj);
		return FALSE;
	}
	for (i=0; i < len; i++)
		if (attr_proto_item(attrs, i) == proto) {
			if (attr_owns_bin(attrs, i)) {
				if (obj->data_type == DATA_TYPE_DATAPTR)
					pr_free(obj->data.ptr);
				obj->data_type = DATA_TYPE_NONE;
			}
			if (i != len-1) 
				memmove(attr_pp(attrs,i),attr_pp(attrs,i+1),((len-1)-i)*sizeof(attr_t));
			attr_plen(attrs)--;
			write_unlock(ist, obj);
			return TRUE;
		}
	write_unlock(ist, obj);
	return FALSE;
}

//********************************* proto_len **********************************
int proto_len(isp ist, obj_p obj){
	int res;
	rdlock_rtrn(obj) 0;
	if (!obj->has_attrs) {
		read_unlock(ist, obj);
		return 1;
	}
	res = attr_plen(obj->attr_proto.attrs);
	read_unlock(ist, obj);
	return res;
}

//********************************* proto_item *********************************
obj_p proto_item(isp ist, obj_p obj, int i){
	obj_p res;
	rdlock_rtrn(obj) NULL;
	if (!obj->has_attrs)
		res = obj->attr_proto.proto;
	else
		res = attr_proto_item(obj->attr_proto.attrs, i);
	read_unlock(ist, obj);
	return res;
}

//********************************* proto_list *****************************
obj_p proto_list(isp ist, obj_p obj) {
	i32_t i, j, k = 0, len;
	obj_p proto, list_obj;
	clist_t *pao_list;
	rdlock_rtrn(obj) NULL;
	pao_list = new_clist(10);
	ins_clist(pao_list, obj, 0);
	for (i=0; i < clist_len(pao_list); i++) {
		obj = clist_item(pao_list,i);
		len = proto_len(ist, obj);
		for(j=0; j < len; j++) {
			proto = proto_item(ist, obj, j);
			if (proto != OBJ(OBJECT)) {
				if_rdlock(proto){
					clist_read_unlock(ist, pao_list);
					free_clist(pao_list);
					return NULL;
				}
				clist_append(pao_list, proto);
			}
		}
		if (i > 100) {
			raise_exception(ist, OBJ(INTERNAL_EXC), "Proto list has circular reference");
			clist_read_unlock(ist, pao_list);
			free_clist(pao_list);
			return NULL;
		}
	}
	if_rdlock(OBJ(OBJECT)){
		clist_read_unlock(ist, pao_list);
		free_clist(pao_list);
		return NULL;
	}
	clist_append(pao_list, OBJ(OBJECT));
	for (j = clist_len(pao_list)-1; j > 0; j--)
		if ((proto = clist_item(pao_list, j)))
			for(i=0, k=0; i < j; i++)
				if(clist_item(pao_list,i) == proto) {
					clist_item(pao_list,i) = 0;
					k++;
				}
	list_obj = NEW_LIST(clist_len(pao_list)-k);
	for (i=0; i < clist_len(pao_list); i++)
		if ((proto = clist_item(pao_list, i)))
			list_append(ist, list_obj, proto);
	clist_read_unlock(ist, pao_list);
	free_clist(pao_list);
	return list_obj;
}

//********************************* has_proto **********************************
int has_proto(isp ist, obj_p obj, obj_p obj_proto){
	i32_t i, j, len;
	obj_p proto;
	clist_t *pao_list;

	if (obj == obj_proto) return TRUE;

	rdlock_rtrn(obj) 0;

	if ( (!obj->has_attrs && obj->attr_proto.proto == obj_proto) ||
		 obj_proto == OBJ(OBJECT) ) {
		read_unlock(ist, obj);
		return TRUE;
	}
	pao_list = new_clist(10);
	clist_append(pao_list, obj);
	for (i=0; i < clist_len(pao_list); i++) {
		obj = clist_item(pao_list, i);
		len = proto_len(ist, obj);
		for(j=0; j < len; j++) {
			proto = proto_item(ist, obj, j);
			if (proto != OBJ(OBJECT)) {
				if (proto == obj_proto) goto end_true;
				if_rdlock(proto){
					clist_read_unlock(ist, pao_list);
					free_clist(pao_list);
					return FALSE;
				}
				clist_append(pao_list, proto);
			}
		}
		if (i > 100) {
			raise_exception(ist, OBJ(INTERNAL_EXC), "Proto list has circular reference");
			goto end_false;
		}
	}
end_false:
	clist_read_unlock(ist, pao_list);
	pr_free(pao_list);
	return FALSE;
end_true:
	clist_read_unlock(ist, pao_list);
	pr_free(pao_list);
	return TRUE;
}

//********************************* get_proto_attr *****************************
// same as get_attr but will look in all protos for key as well
// if proto_pp not null, then will return proto obj that attr was found in
// if super_obj specified, then only objects after super_obj searched
obj_p get_proto_attr(isp ist, obj_p objid, obj_p key, obj_p *proto_pp, obj_p super_obj) {
	obj_p  obj=objid, proto, res = NULL;
	int i, j, len;
	clist_t *pao_list;
	if (proto_pp) *proto_pp = NULL;
	while (TRUE){
		rdlock_rtrn(obj) NULL;
		if((!super_obj && (res = get_attr(ist, obj, key))) || obj == OBJ(OBJECT)) {
			read_unlock(ist, obj);
			break;
		}
		if (obj == super_obj)
			super_obj = NULL;
		if ((len = proto_len(ist, obj)) == 1) {
			proto = proto_item(ist, obj, 0);
			read_unlock(ist, obj);
			if (obj == proto) {
				raise_exception(ist, OBJ(INTERNAL_EXC), "only prototype is self");
				return NULL;
			}
			obj = proto;
			continue;
		}
		pao_list = new_clist(len*2+4);
		for (i=0; i < len; i++)
			if ((proto = proto_item(ist, obj, i)) != OBJ(OBJECT)) {
				if_rdlock(proto){
					clist_read_unlock(ist, pao_list);
					free_clist(pao_list);
					return NULL;
				}
				clist_append(pao_list, proto);
			}					
		read_unlock(ist, obj);
		for (i=0; i < clist_len(pao_list); i++){
			obj = clist_item(pao_list, i);
			if_rdlock(obj){
				clist_read_unlock(ist, pao_list);
				free_clist(pao_list);
				return NULL;
			}
			len = proto_len(ist, obj);
			for(j=0; j < len; j++) {
				proto = proto_item(ist, obj, j);
				if (proto != OBJ(OBJECT)) {
					if_rdlock(proto){
						clist_read_unlock(ist, pao_list);
						free_clist(pao_list);
						return NULL;
					}
					clist_append(pao_list, proto);
				}
			}
			if (i == 100){
				raise_exception(ist, OBJ(INTERNAL_EXC), "Proto list has circular reference");
				read_unlock(ist, obj);
				clist_read_unlock(ist, pao_list);
				free_clist(pao_list);
				return NULL;
			}
			read_unlock(ist, obj);
		}
		if_rdlock(OBJ(OBJECT)){
			clist_read_unlock(ist, pao_list);
			free_clist(pao_list);
			return NULL;
		}
		clist_append(pao_list, OBJ(OBJECT));
		for (j = clist_len(pao_list)-1; j > 0; j--) {
			if ((proto = clist_item(pao_list,j))) {
				for(i=0; i < j; i++) {
					if(clist_item(pao_list,i) == proto) {
						read_unlock(ist, proto);
						clist_item(pao_list,i) = NULL;
					}
				}
			}
		}
		for (i=0; i < clist_len(pao_list); i++) {
			if ((obj = clist_item(pao_list,i))) {
				if (!super_obj && (res = get_attr(ist, obj, key))) {
					if (proto_pp) *proto_pp = obj;
					clist_read_unlock(ist, pao_list);
					free_clist(pao_list);
					return res;
				}
				if (obj == super_obj)
					super_obj = NULL;
			}
		}
		clist_read_unlock(ist, pao_list);
		free_clist(pao_list);
		res = NULL;
		goto done;
	}
done:
	if (res) {
		if (proto_pp) *proto_pp = obj;
		return res;
	}
	return NULL;
}

//********************************* get_scope_attr *****************************
// same as get_attr but will look in all surrounding syntactical scopes as well
// if scope_pp not null, then will return scope obj that attr was found in
// if super_obj specified, then only objects after super_obj after searched
obj_p get_scope_attr(isp ist, obj_p scope, obj_p key, obj_p *scope_pp, obj_p super_obj) {
	obj_p res = NULL, last_scope = NULL;
	while (scope && !res) {
		if (!super_obj && (res = get_attr(ist, scope, key))) {
			if (scope_pp) *scope_pp = scope;
			break;
		}
		if (scope == super_obj) super_obj = NULL;
		last_scope = scope;
		scope = get_attr(ist, scope, SYM(PREVSCOPE_));
		pr_assert(last_scope != scope);
	}
	if (!res && last_scope)
		return get_proto_attr(ist, last_scope, key, scope_pp, super_obj);
	return res;
}

//********************************* obj_alloc ***************************
void* obj_malloc(isp ist, obj_p proto, obj_p obj, size_t size) {
	if (size <= sizeof(obj_data_t)){
		SET_TYPE_IF_EXC(proto, obj, DATA_TYPE_IMMDATA) 
			return NULL;
		obj->imm_data_len = (int) size;
		return &obj->data;
	} else {
		SET_TYPE_IF_EXC(proto, obj, DATA_TYPE_DATAPTR) return NULL;
		obj->data.ptr  = pr_malloc(size);
		return obj->data.ptr;
	}
}

//********************************* obj_realloc *******************************
void* obj_realloc(isp ist, obj_p obj, size_t size){
	wrlock_rtrn(obj) NULL;
	if(obj->data_type == DATA_TYPE_DATAPTR){
		if(size <= sizeof(obj_data_t)){
			char* ptr = obj->data.ptr;
			obj->data_type = DATA_TYPE_IMMDATA;
			obj->imm_data_len = (int) size;
			memcpy(&obj->data, ptr, size);
			pr_free(ptr);
		} else 
			obj->data.ptr = pr_realloc(obj->data.ptr, size);
	} else {
		if(size <= sizeof(obj_data_t))
			obj->imm_data_len = (int) size;
		else {
			char* ptr = pr_malloc(size);
			memcpy(ptr, &obj->data, obj->imm_data_len);
			obj->data_type = DATA_TYPE_DATAPTR;
			obj->data.ptr = ptr;
		}
	}
	write_unlock(ist, obj);
	return obj_dataptr(obj);
}

//********************************* obj_free **********************************
void obj_free(isp ist, obj_p obj) {
	wrlock_rtrn(obj);
	if(obj->data_type == DATA_TYPE_DATAPTR)
		pr_free(obj->data.ptr);
	obj->data.ptr = NULL;
	obj->data_type = DATA_TYPE_NONE;
	write_unlock(ist, obj);
}


//********************************* new_hash_obj ******************************
obj_p new_hash_obj(isp ist, i32_t num) {
	obj_p hash_obj = NEW_OBJ(OBJ(HASH_PROTO));
	SET_TYPE_IF_EXC(OBJ(HASH_PROTO), hash_obj, DATA_TYPE_IMMDATA) return NULL;
	hash_obj->imm_data_len = 4;
	hash_obj->data.i32[0] = num & HASH_MASK;
	if (!(hash_obj->data.i32[0])) hash_obj->data.i32[0] = 1;
	hash_obj->immutable = TRUE;
	return hash_obj;
}

//********************************* call_func1_f ********************************
obj_p call_func1_f(isp ist, obj_p self, obj_p sym, obj_p parm) {
	obj_p arr[2];
	arr[0] = OBJ(NOKEY); 
	arr[1] = parm; 
	return call_func(ist, self, sym, NULL, 2, arr, NULL);
}

//********************************* call_func2_f ********************************
obj_p call_func2_f(isp ist, obj_p self, obj_p sym, obj_p parm1, obj_p parm2) {
	obj_p arr[4];
	arr[0] = OBJ(NOKEY);  arr[1] = parm1; 
	arr[2] = OBJ(NOKEY);  arr[3] = parm2; 
	return call_func(ist, self, sym, NULL, 4, arr, NULL);
}


//********************************* set_obj_doc ***********************************
void set_obj_doc_f(isp ist, obj_p obj, char* str){
	if (str[strlen(str)-1] == '\n') {
		char* ns = pr_malloc(strlen(str)+1);
		strcpy(ns, str);
		ns[strlen(str)-1] = 0;
		set_attr(ist, obj, SYM(DOC_), NEW_STRING(ns));
		pr_free(ns);
	} else
		set_attr(ist, obj, SYM(DOC_), NEW_STRING(str));
}

//********************************* new_C_func_obj ******************************
obj_p new_C_func_obj(isp ist, pr_func* func_ptr, obj_p label_values_list) {
	obj_p func_obj = NEW_OBJ(OBJ(FUNC_PROTO));
	func_obj->data_type = DATA_TYPE_EXTPTR;
	func_obj->data.ptr = func_ptr;
	if (label_values_list)
		set_attr(ist,  func_obj, SYM(FPARAMS_), copy_list_obj(ist, label_values_list) );
	return func_obj;
}

//********************************* finalize_all ******************************
void finalize_all(void) {
	isp ist = new_ist(ACC_SYSTEM);
	obj_p obj;
	stop_mem_mgr();
	unlock_all_objs();
	pr_lock(&object_list_lock); 
	obj = obj_list_root;
	pr_unlock(&object_list_lock); 
	while(obj) {
		call_func(ist, obj, SYM(FINAL_), NULL, 0, NULL, NULL);
		check_exceptions(ist);
		pr_lock(&object_list_lock); 
		obj = obj->next_obj;
		pr_unlock(&object_list_lock); 
	}
}

//********************************* dump **************************************
clist_p object_list;
int first_symbol, prt_flg=0;
FILE* fout;
void dump(isp ist, char* dumpfilename, obj_p obj){
	fout = fopen(dumpfilename, "w");
	object_list = new_clist(10);
	first_symbol = PR_TRUE;
	fprintf(fout,"\nDumping (%lx)\n",(unsigned long)(uintptr_t)obj);
	fprintf(fout,  "-------------------\n");
	dump_obj(ist, obj, 0);
	fclose(fout);
}

//********************************* key_to_sym *************************************
obj_p key_to_sym(isp ist, attr_key_t key) {
	obj_p res, key_obj = NEW_INT(key);
	obj_p sym_obj = dict_item(ist, OBJ(SYMBOLS), key_obj);
	if (!sym_obj) {
		raise_exception(ist, OBJ(INTERNAL_EXC), "key_to_sym lookup error");
		res = NULL;
	} else
		res = sym_obj;
	del_unlock(key_obj);
	return res;
}

//********************************* dump_obj **********************************
void ind(int n){int i; for(i=0; i<n; i++) fprintf(fout,".   ");}
void blnk(){
	if(prt_flg){
		prt_flg = 0;
		fprintf(fout,"\n");
	}
}
void dump_obj(isp ist, obj_p obj, int dep){
	int i;
	obj_p doc;
	doc = get_attr(ist, obj, SYM(DOC_));
	if (doc) {fprintf(fout,"obj(%lx) doc: %s\n", (unsigned long)(uintptr_t)obj, pr2c_strptr(ist, doc)); prt_flg = 1;}
	else     {fprintf(fout,"%s\n",as_str(ist, obj)); prt_flg = 1;}
	for(i=0; i < clist_len(object_list); i++)
		if (clist_item(object_list, i) == obj)
			return;
	clist_append(object_list, obj);
	ind(dep+1); fprintf(fout,"State: %s\n",
		( obj->state == OBJ_STATE_NEW_OR_MARKED ? "New/Marked" :
		( obj->state == OBJ_STATE_UNMARKED ? "Unmarked" :
		( obj->state == OBJ_STATE_DELETED  ? "Deleted" : "*** ERROR ***" ))));
	if(obj->data_type != DATA_TYPE_NONE){
		ind(dep+1); fprintf(fout,"Data %s",
		( obj->data_type == DATA_TYPE_IMMDATA   ? "immediate: " :
		( obj->data_type == DATA_TYPE_DATAPTR   ? "pointer:   " : 
		                                           "func ptr:  " )));
	}
	if(obj->data_type == DATA_TYPE_IMMDATA){
		fprintf(fout, "%08x %08x \"%s\"\n", 
							obj->data.i32[0], obj->data.i32[1], 
							(char *)obj->data.str );
	} else if(obj->data_type != DATA_TYPE_NONE){
		fprintf(fout,"%08x\n", obj->data.i32[0]);
	}
	prt_flg = 1;
	if (obj->has_attrs) {
		attr_p attrs = obj->attr_proto.attrs;
		blnk();
		for(i=ATTRS_OVERHEAD; i < ATTRS_OVERHEAD+attr_plen(attrs); i++){
			obj_p val = attrs[i].attr.value;
			ind(dep+1); fprintf(fout,"PROTO: "); prt_flg = 1;
			dump_obj(ist, val, dep+1);
			if_exc_return;
		}
		for( i=ATTRS_OVERHEAD+attr_psize(attrs); 
			 i < ATTRS_OVERHEAD+attr_psize(attrs)+attr_asize(attrs); i++){
			if (attrs[i].attr.key > 0) {
				char* sstr;
				attr_key_t key = attrs[i].attr.key;
				obj_p val = attrs[i].attr.value;
				sstr = symch(ist, key_to_sym(ist, key));
				if (catch_exception(ist, OBJ(INTERNAL_EXC), NULL))
					sstr = "<***err in key_to_sym>";
				ind(dep+1); fprintf(fout,"KEY: %s\n", sstr); prt_flg = 1;
				ind(dep+1); fprintf(fout,"VALUE: ");prt_flg = 1;
				if (val) dump_obj(ist, val, dep+1);
				else     fprintf(fout,"0\n");prt_flg = 1;
				if_exc_return;
			}
		}
	} else if (obj != OBJ(OBJECT)) {
		blnk();
		ind(dep+1); fprintf(fout,"PROTO: "); prt_flg = 1;
		dump_obj(ist, obj->attr_proto.proto, dep+1);
		if_exc_return;
	}
	blnk();
}
