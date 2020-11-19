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
#include "dict.h"
#include "parser.h"
#include "object.h"
#include <prothon/prothon_dll.h>

//********************************* new_tuple_obj *****************************
obj_p new_tuple_obj(isp ist, int fixed_size) {
	obj_p tuple_obj = NEW_LIST(fixed_size);
	pr_assert(!(tuple_obj->has_attrs));
	tuple_obj->attr_proto.proto = OBJ(TUPLE_PROTO);
	return tuple_obj;
}

//********************************* tuple1 *************************************
obj_p tuple1(isp ist, obj_p item1){
	obj_p tuple_obj = new_tuple_obj(ist, 1);
	list_append_no_lock(tuple_obj, item1);
	set_immutable(tuple_obj);
	return tuple_obj;
}

//********************************* tuple2 *************************************
obj_p tuple2(isp ist, obj_p item1, obj_p item2){
	obj_p tuple_obj = new_tuple_obj(ist, 2);
	list_append_no_lock(tuple_obj, item1); list_append_no_lock(tuple_obj, item2);
	set_immutable(tuple_obj);
	return tuple_obj;
}


MODULE_DECLARE(Tuple);
MODULE_DECLARE(Tgen);

// ***************************** TUPLE ******************************************

MODULE_START(Tuple)
{
	list_p lstp;

	Tuple_OBJ = OBJ(TUPLE_PROTO);
	MODULE_SET_DOC(Tuple,   "object prototype");
	set_obj_id(Tuple_OBJ, *, Tuple);
	set_attr(ist, OBJ(OBJECT), sym(ist, "Tuple"), Tuple_OBJ);

	SET_TYPE_IF_EXC(Tuple_OBJ, Tuple_OBJ, DATA_TYPE_DATAPTR) return;
	lstp = Tuple_OBJ->data.ptr = pr_malloc(list_sizeof(2));
	lstpsetsize(lstp, 2);
	lstplen(lstp)  = 0;
}

DEF(Tuple, init_, FORM_STAR_PARAM) {
	BIN_EMPTY_CHK();
	read_unlock(ist, self);
	read_unlock(ist, parms[1]);
	copy_object_data(ist, self, parms[1]);
	switch_proto(ist, self, OBJ(TUPLE_PROTO));
	set_immutable(self);
	read_lock(ist, parms[1]);
	read_lock(ist, self);
	return OBJ(NONE);
}

DEF(Tuple, str_, NULL) {
	BIN_STR_CHK(Tuple);
	return str_tuple_list(ist, self, "(", ")");
}

DEF(Tuple, getItem_, FORM_RPARAM) {
	BIN_CONTENT_CHK(Tuple);
	return get_sequence_item(ist, self, parms[1], SEQ_TYPE_TUPLE);
}

DEF(Tuple, setItem_, FORM_RPARAM) {
	raise_exception(ist, OBJ(MUTABLE_EXC), "tuples may not be modifed");
	return NULL;
}

DEF(Tuple, delItem_, NULL) {
	raise_exception(ist, OBJ(MUTABLE_EXC), "tuples may not be modifed");
	return NULL;
}

DEF(Tuple, objList_, FORM_RPARAM) {
	if (!self->data.ptr) return parms[1];
	return self;
}

DEF(Tuple, cmp_, FORM_RPARAM) { 
	int i, llen, tlen, mlen;
	obj_p res;
	obj_p self_item, tgt_item, tgt_list;
	BIN_CONTENT_CHK(Tuple);
	if (!has_proto(ist, parms[1], OBJ(SEQ_PROTO))) {
		raise_exception(ist, OBJ(TYPE_EXC), "sequence can only be compared to a sequence");
		return NULL;
	}
	llen = (int) list_len(ist, self);
	tgt_list = parms[1];
	tlen = (int) list_len(ist, tgt_list);
	mlen = min(llen, tlen);
	for(i=0; i < mlen; i++) {
		self_item = list_item(ist, self, i);
		tgt_item  = list_item(ist, tgt_list, i);
		res = call_func1(ist, self_item, SYM(CMP_), tgt_item); if_exc_return NULL;
		if (res->data.i64)
			return NEW_INT(res->data.i64);
	}
	if (llen < tlen) return NEW_INT(-1);
	if (llen > tlen) return NEW_INT(+1);
	return OBJ(ZERO_INT);
}

DEF(Tuple, eq__QUES, FORM_RPARAM) { 
	int i, llen, tlen;
	obj_p self_item, tgt_item, tgt_list, false_obj = OBJ(PR_FALSE);
	BIN_CONTENT_CHK(Tuple);
	if (!has_proto(ist, parms[1], OBJ(SEQ_PROTO))) return false_obj;
	llen = (int) list_len(ist, self);
	tgt_list = parms[1];
	tlen = (int) list_len(ist, tgt_list);
	if (llen != tlen)  return false_obj;
	for(i=0; i < llen; i++) {
		self_item = list_item(ist, self, i);
		tgt_item  = list_item(ist, tgt_list, i);
		if (call_func1(ist, self_item, SYM(EQ__QUES), tgt_item) == false_obj) 
			return false_obj;
		else if_exc_return NULL;
	}
	return OBJ(PR_TRUE);
}

DEF(Tuple, rIn__QUES, FORM_RPARAM) { 
	int i, llen = (int) list_len(ist, self);
	obj_p item, tgt = parms[1];
	BIN_CONTENT_CHK(Tuple);
	for(i=0; i < llen; i++) {
		item = list_item(ist, self, i);
		if (call_func1(ist, item, SYM(EQ__QUES), tgt) == OBJ(PR_TRUE)) 
			return OBJ(PR_TRUE);
		else if_exc_return NULL;
	}
	return OBJ(PR_FALSE);
}

DEF(Tuple, rNotIn__QUES, FORM_RPARAM) { 
	int i, llen = (int) list_len(ist, self);
	obj_p item, tgt = parms[1];
	BIN_CONTENT_CHK(Tuple);
	for(i=0; i < llen; i++) {
		item = list_item(ist, self, i);
		if (call_func1(ist, item, SYM(EQ__QUES), tgt) == OBJ(PR_TRUE)) 
			return OBJ(PR_FALSE);
		else if_exc_return NULL;
	}
	return OBJ(PR_TRUE);
}

DEF(Tuple, add_, FORM_RPARAM) { 
	int i;
	obj_p res = copy_list_obj(ist, self);
	BIN_CONTENT_CHK(Tuple);
	if (has_proto(ist, parms[1], OBJ(SEQ_PROTO)) && !has_proto(ist, parms[1], OBJ(STRING_PROTO)))
		for(i=0; i < (int) list_len(ist, parms[1]); i++) 
			list_append(ist, res, list_item(ist, parms[1], i));
	else
		list_append(ist, res, parms[1]);
	switch_proto(ist, res, Tuple_OBJ);
	return res;
}

DEF(Tuple, mul_, FORM_RPARAM){
	obj_p res;
	list_p lstp, selfp = (list_p)(self->data.ptr);
	int i, size, times, len = (int) list_len(ist, self);
	BIN_CONTENT_CHK(Tuple);
	if (!has_proto(ist, parms[1], OBJ(INT_PROTO))) {
		raise_exception(ist, OBJ(TYPE_EXC), "multiply times parameter must be an integer");
		return NULL;
	}
	times = (int)parms[1]->data.i64;
	size  = len*times;
	if(times == 0) { 
		obj_p res = NEW_TUPLE(0);
		res ->immutable = TRUE;
		return res;
	}
	if(times == 1) return self;
	res = NEW_OBJ(OBJ(TUPLE_PROTO));
	lstp = res->data.ptr = pr_malloc(list_sizeof(size));
	if(!lstp) {
		raise_exception(ist, OBJ(OUTOFMEMORY_EXC), "memory allocation failed for tuple multiplication");
		return NULL;
	}
	SET_TYPE_IF_EXC(Tuple_OBJ, res, DATA_TYPE_DATAPTR) return NULL;
	lstpsetsize(lstp, size);
	lstplen(lstp)  = size;
	for(i=0; i < times; i++)
		memcpy(lstp->item+(i*len), selfp->item, len*sizeof(obj_p));
	return res;
}

DEF(Tuple, len_, NULL) { 
	BIN_CONTENT_CHK(Tuple);
	return NEW_INT(list_len(ist, self));
}

DEF(Tuple, min, NULL) { 
	int i, llen = (int) list_len(ist, self);
	obj_p min_item, cmp_obj;
	BIN_CONTENT_CHK(Tuple);
	if (llen == 0) return OBJ(NONE);
	if (llen == 1) return list_item(ist, self, 0);
	cmp_obj = call_func1(ist, list_item(ist, self, 0), SYM(CMP_), list_item(ist, self, 1));  if_exc_return NULL;
	if (cmp_obj->data.i64 < 0) min_item = list_item(ist, self, 0);
	else					   min_item = list_item(ist, self, 1);
	for(i=2; i < llen; i++) {
		cmp_obj = call_func1(ist, list_item(ist, self, i), SYM(CMP_), min_item);  if_exc_return NULL;
		if (cmp_obj->data.i64 < 0) min_item = list_item(ist, self, i);
	}
	return min_item;
}

DEF(Tuple, max, NULL) { 
	int i, llen = (int) list_len(ist, self);
	obj_p max_item, cmp_obj;
	BIN_CONTENT_CHK(Tuple);
	if (llen == 0) return OBJ(NONE);
	if (llen == 1) return list_item(ist, self, 0);
	cmp_obj = call_func1(ist, list_item(ist, self, 0), SYM(CMP_), list_item(ist, self, 1));  if_exc_return NULL;
	if (cmp_obj->data.i64 > 0) max_item = list_item(ist, self, 0);
	else					   max_item = list_item(ist, self, 1);
	for(i=2; i < llen; i++) {
		cmp_obj = call_func1(ist, list_item(ist, self, i), SYM(CMP_), max_item);  if_exc_return NULL;
		if (cmp_obj->data.i64 > 0) max_item = list_item(ist, self, i);
	}
	return max_item;
}

DEF(Tuple, count, FORM_RPARAM) { 
	int i, cnt=0, llen = (int) list_len(ist, self);
	BIN_CONTENT_CHK(Tuple);
	for (i=0; i < llen; i++) {
		if (call_func1(ist, list_item(ist, self, i), SYM(EQ__QUES), parms[1]) == OBJ(PR_TRUE))
			cnt++;
		if_exc_return NULL;
	}
	return NEW_INT(cnt);
}

DEF(Tuple, index, FORM_RPARAM) { 
	int i, llen = (int) list_len(ist, self);
	BIN_CONTENT_CHK(Tuple);
	for (i=0; i < llen; i++) {
		if (call_func1(ist, list_item(ist, self, i), SYM(EQ__QUES), parms[1]) == OBJ(PR_TRUE))
			return NEW_INT(i);
		if_exc_return NULL;
	}
	raise_exception(ist, OBJ(INDEX_EXC), "item not found in index search");
	return NULL;
}

DEF(Tuple, iter_, NULL) {
	obj_p list_obj, gen_obj = NEW_OBJ(Tgen_OBJ);
	BIN_CONTENT_CHK(Tuple);
	SET_TYPE_IF_EXC(Tgen_OBJ, gen_obj, DATA_TYPE_DATAPTR) return NULL;
	gen_obj->data.ptr = list_obj = copy_list_obj(ist, self);
	listlen(list_obj) = 0;
	return gen_obj;
}

DEF(Tuple, hash_, NULL) {
	obj_p hash_obj;
	i32_t hash=0x937456;
	int i, llen;

	BIN_CONTENT_CHK(Tuple);
	llen = (int) list_len(ist, self);
	for (i=0; i < llen; i++) {
		hash_obj = call_func0(ist, list_item(ist, self, i), SYM(HASH_));
		if_exc_return NULL;
		hash += hash_obj->data.i32[0];
	}
	return new_hash_obj(ist, hash);
}


//*************************** TUPLE GENERATOR OBJECT **************************
MODULE_START(Tgen)
{
	Tgen_OBJ = NEW_OBJ(OBJ(ITER_PROTO));
}

DEF(Tgen, next, NULL) {
	obj_p res, list_obj = self->data.ptr;
	size_t lsiz = listsize(list_obj);
	size_t llen = listlen(list_obj);
	BIN_CONTENT_CHK(Tgen);
	if (llen == lsiz) {
		raise_exception(ist, OBJ(STOP_ITERATION_EXC), NULL);
		return NULL;
	}
	res = listitem(list_obj, llen);
	listlen(list_obj)++;
	return res;
}


//********************************* INITIALIZATION ****************************
MAIN_MODULE_INIT(Tuple)
{
	MODULE_SUB_INIT(Tuple);
	MODULE_ADD_SYM(Tuple, init_);	
	MODULE_ADD_SYM(Tuple, str_);	
	MODULE_ADD_SYM(Tuple, hash_);
	MODULE_ADD_SYM(Tuple, len_);
	MODULE_ADD_SYM(Tuple, min);
	MODULE_ADD_SYM(Tuple, max);
	MODULE_ADD_SYM(Tuple, cmp_);
	MODULE_ADD_SYM(Tuple, eq__QUES);
	MODULE_ADD_SYM(Tuple, count);
	MODULE_ADD_SYM(Tuple, index);
	MODULE_ADD_SYM(Tuple, getItem_);
	MODULE_ADD_SYM(Tuple, add_);
	MODULE_ADD_SYM(Tuple, mul_);
	MODULE_ADD_SYM(Tuple, rIn__QUES);
	MODULE_ADD_SYM(Tuple, rNotIn__QUES);
	MODULE_ADD_SYM(Tuple, iter_);
	MODULE_ADD_SYM(Tuple, setItem_);
	MODULE_ADD_SYM(Tuple, delItem_);
	MODULE_ADD_SYM(Tuple, objList_);

	MODULE_SUB_INIT(Tgen);
	MODULE_ADD_SYM(Tgen, next);

	check_exceptions(ist);
}
