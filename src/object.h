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


// object.h

#ifndef OBJECT_H
#define OBJECT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <prothon/prothon_dll.h>
#include "lock.h"
#include "prlist.h"
#include <apr_thread_proc.h>
#include <apr_portable.h>


/* Get the root APR pool for prothon, can be used directly or to create
 * subpools. */
apr_pool_t* get_pr_head_pool(void);

apr_thread_t* os_thread_2_apr(apr_os_thread_t os_thread);
obj_p         os_thread_2_obj(apr_os_thread_t os_thread);

void console(isp ist);

extern obj_p		obj_list_root;
extern obj_p        core_object_marker;
extern pr_lock_t	object_list_lock;

// #define NDEBUG
#include <assert.h>

obj_p list_append_no_lock(obj_p list_obj, obj_p item);

void init_symbol(isp ist);

extern pr_services_t dll_services;

// Use OBJ(OBJNAME) to access well-known object pointer
// example:  OBJ(OBJECT) or OBJ(PR_TRUE)
#define OBJ(x) OBJ[x]

// Use SYM(XXX_) instead of sym(ist, "xxx_") for speed.
#define SYM(symenum)  (sym_id_table[symenum].id)

#define ATTRS_MAX_PERCENT_FULL  		66
#define ATTRS_PAD_PERCENT_FACTOR		300
#define ATTRS_PROTO_PADDING				1

#define ATTRS_PINDEX	0
#define ATTRS_AINDEX	1
#define ATTRS_OVERHEAD	2
#define ATTRS_PINIT 	(ATTRS_PROTO_PADDING+1)
#define ATTRS_AINIT 	8
#define ATTRS_TINIT		(ATTRS_OVERHEAD+ATTRS_PINIT+ATTRS_AINIT)

#define attr_psize(attrp)	((*(attrp+ATTRS_PINDEX)).hdr.size)
#define attr_plen(attrp)	((*(attrp+ATTRS_PINDEX)).hdr.len)
#define attr_asize(attrp)	((*(attrp+ATTRS_AINDEX)).hdr.size)
#define attr_alen(attrp)	((*(attrp+ATTRS_AINDEX)).hdr.len)
#define attr_tsize(attrp)	(ATTRS_OVERHEAD+attr_psize(attrp)+attr_asize(attrp))
#define attr_pp(attrp,i)	((attrp)+(ATTRS_OVERHEAD+(i)))
#define attr_ap(attrp,i)	((attrp)+(ATTRS_OVERHEAD+attr_psize(attrp)+((i)%attr_asize(attrp))))
#define attr_proto_item(attrp,i) (((attrp)+(ATTRS_OVERHEAD+(i)))->proto.proto)
#define attr_owns_bin(attrp,i)	 (((attrp)+(ATTRS_OVERHEAD+(i)))->proto.owns_binary)

#define PAUSE_MS 2

#ifdef DEBUG_THREADS
#ifdef WIN32
void SetThreadName(LPCSTR szThreadName);
#else
#define SetThreadName(name)
#endif
#endif

void object_store_init(isp ist, int nop2);
obj_p new_object(isp ist, obj_p proto);
obj_p copy_object(isp ist, obj_p obj);
void free_object(obj_p obj);

obj_p debug_object(obj_p obj);
extern obj_p debug_obj;
extern obj_p debug_obj_proto;

void set_item(isp ist, obj_p ref_self, obj_p ref_key, obj_p value, int prop_flg);
void del_item(isp ist, obj_p ref_self, obj_p ref_key, int prop_flg);

obj_p new_protolist_obj(isp ist, obj_p obj);
obj_p new_attrdict_obj(isp ist, obj_p obj);
obj_p new_float_obj(isp ist, double num);
obj_p new_hash_obj(isp ist, i32_t num);
obj_p new_int_obj(isp ist, i64_t num);
obj_p new_int_long_obj(isp ist, long_p longp);
long_p cstr2longp(isp ist, char *str, char **pend, int base);
obj_p long_format(long_p aa, int base, int uppercase);
long_p new_longp(i64_t ival);
long long_hash(long_p v);
int int_is_zero(obj_p obj);
u8_t* bytes_ptr(obj_p obj);
size_t bytes_len(obj_p obj);
void pr_str2bytes(isp ist, u8_t* buf, obj_p str_obj, int latin);
void pr2pr_strcpy(isp ist, obj_p obj, ch3_p str, data_size_t len);
void c2pr_bytcpy(isp ist, obj_p obj, u8_t* byt, data_size_t len, int latin);

void dict2attrs(isp ist, obj_p dict, obj_p obj);
void list2protos(isp ist, obj_p list, obj_p obj);

attr_p add_attrs(obj_p obj);
char* numonly(char* s, char* buf);

void dump_obj(isp ist, obj_p obj, int dep);

obj_p arginit(isp its, int argc, char* argv[]);
void sysinit(isp ist, char* prog_path, obj_p argv_obj);

isp new_ist(access_t access);

obj_p load_sys_module(isp ist, char* prog_path, obj_p argv_obj);
#define get_sys_path_list()  \
	get_attr(ist, get_attr(ist, OBJ(MODULES), sym(ist, "sys")), sym(ist, "path"))

// common exit point for debug breakpoints
void pr_exit(int n);

/* Some seq stuff from builtins-core.c */
obj_p str_tuple_list(isp ist, obj_p self, char* ldelim, char* rdelim);
obj_p get_sequence_item(isp ist, obj_p self, obj_p slice, int seq_type);

#define SEQ_TYPE_BYTES			0
#define SEQ_TYPE_STRING         1
#define SEQ_TYPE_TUPLE          2
#define SEQ_TYPE_LIST           3

#define READ_NOTIFY		1
#define WRITE_NOTIFY	2
#define	DELETE_NOTIFY	4

void add_notification(isp ist, obj_p obj, obj_p callback_func, int flags);
void del_notification(isp ist, obj_p obj, obj_p callback_func);

void delete_notify(isp ist, obj_p obj);

void finalize_all(void);

extern  pr_lock_t		    del_notify_lock_var;
#define del_notify_lock()	pr_lock(&del_notify_lock_var)
#define del_notify_unlock()	pr_unlock(&del_notify_lock_var)

#endif  // #define OBJECT_H
