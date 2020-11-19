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

MODULE_DECLARE(List);

static int seq_len(isp ist, obj_p seq) {
	if (has_proto(ist, seq, OBJ(STRING_PROTO)))
		return (int) strlen(pr2c_strptr(ist, seq));
	else return (int) list_len(ist, seq);
}

static obj_p seq_item(isp ist, obj_p seq, int i) {
	char s[2];
	if (has_proto(ist, seq, OBJ(STRING_PROTO))) {
		s[0] = pr2c_strptr(ist, seq)[i];
		s[1] = 0;
		return NEW_STRING(s); 
	} else
		return list_item(ist, seq, i);
}

//********************************* new_list_obj ******************************
obj_p new_list_obj(isp ist, size_t initial_size){
	obj_p list_obj = NEW_OBJ(OBJ(LIST_PROTO));
	list_p lstp = list_obj->data.ptr = 
		      pr_malloc(list_sizeof(max(initial_size,2)));
	SET_TYPE_IF_EXC(OBJ(TUPLE_PROTO), list_obj, DATA_TYPE_DATAPTR) return NULL;
	lstpsetsize(lstp, max(initial_size,2));
	lstplen(lstp)  = 0;
	return list_obj;
}

//********************************* init_list_data ****************************
void init_list_data(isp ist, obj_p obj, int initial_size) {
	list_p lstp;
	lstp = obj->data.ptr = 
			pr_malloc(list_sizeof(max(initial_size, 2)));
	SET_TYPE_IF_EXC(OBJ(LIST_PROTO), obj, DATA_TYPE_DATAPTR) return;
	lstpsetsize(lstp, max(initial_size, 2));
	lstplen(lstp)  = 0;
}

//********************************* copy_list_obj ****************************
obj_p copy_list_obj(isp ist, obj_p list_obj) {
	obj_p res;
	size_t llen, size;
	list_p list_ptr;
	rdlock_rtrn(list_obj) NULL;
	llen = listlen(list_obj);
	size = list_sizeof(llen);
	res = copy_object(ist, list_obj);
	res->data.ptr = list_ptr = pr_malloc(size);
	lstpsetsize(list_ptr, llen);
	lstplen(list_ptr)  = llen;
	memcpy( list_ptr->item, ((list_p)(list_obj->data.ptr))->item, 
		                                  size-sizeof(list_hdr_t) );
	read_unlock(ist, list_obj);
	return res;
}

//********************************* list_len **********************************
size_t list_len(isp ist, obj_p list_obj){
	size_t len;
	rdlock_rtrn(list_obj) 0;
	if (list_obj->data.ptr)
		len = listlen(list_obj);
	else
		len = 0;
	read_unlock(ist, list_obj);
	return len;
}

//********************************* list_clear **********************************
void list_clear(isp ist, obj_p list_obj) {
	wrlock_rtrn(list_obj);
	listlen(list_obj) = 0;
	write_unlock(ist, list_obj);
}

//********************************* list_item *********************************
obj_p list_item(isp ist, obj_p list_obj, int i){
	obj_p res;
	rdlock_rtrn(list_obj) NULL;
	res = listitem(list_obj, i);
	read_unlock(ist, list_obj);
	return res;
}

//********************************* list_item_set *****************************
void list_item_set(isp ist, obj_p list_obj, int i, obj_p new_item){
	wrlock_rtrn(list_obj);
	listitem(list_obj, i) = new_item;
	write_unlock(ist, list_obj);
}

//********************************* list_append *******************************
obj_p list_append(isp ist, obj_p list_obj, obj_p item){
	list_p lstp;
	wrlock_rtrn(list_obj) NULL;
	lstp = list_obj->data.ptr;
	if(lstplen(lstp) == lstpsize(lstp)) {
		lstpsetsize(lstp, lstpsize(lstp) * LIST_GROWTH_FACTOR);
		lstp = pr_realloc(lstp, list_sizeof(lstpsize(lstp)));
		list_obj->data.ptr = lstp;
	}
	listitem(list_obj, listlen(list_obj)++) = item;
	write_unlock(ist, list_obj);
	return list_obj;
}

//********************************* list_append_no_lock ***********************
obj_p list_append_no_lock(obj_p list_obj, obj_p item){
	list_p lstp;
	lstp = list_obj->data.ptr;
	if(lstplen(lstp) == lstpsize(lstp)) {
		lstpsetsize(lstp, lstpsize(lstp) * LIST_GROWTH_FACTOR);
		lstp = pr_realloc(lstp, list_sizeof(lstpsize(lstp)));
		list_obj->data.ptr = lstp;
	}
	listitem(list_obj, listlen(list_obj)++) = item;
	return list_obj;
}

//********************************* list1 *************************************
obj_p list1(isp ist, obj_p item1){
	obj_p list_obj = NEW_LIST(1);
	list_append_no_lock(list_obj, item1);
	return list_obj;
}

//********************************* list2 *************************************
obj_p list2(isp ist, obj_p item1, obj_p item2){
	obj_p list_obj = NEW_LIST(2);
	list_append_no_lock(list_obj, item1); list_append_no_lock(list_obj, item2);
	return list_obj;
}

//********************************* list3 *************************************
obj_p list3(isp ist, obj_p item1, obj_p item2, obj_p item3){
	obj_p list_obj = NEW_LIST(3);
	list_append_no_lock(list_obj, item1); list_append_no_lock(list_obj, item2); 
	list_append_no_lock(list_obj, item3);
	return list_obj;
}

//********************************* list4 *************************************
obj_p list4(isp ist, obj_p item1, obj_p item2, obj_p item3, obj_p item4){
	obj_p list_obj = NEW_LIST(4);
	list_append_no_lock(list_obj, item1); list_append_no_lock(list_obj, item2);
	list_append_no_lock(list_obj, item3); list_append_no_lock(list_obj, item4);
	return list_obj;
}

//********************************* list6 *************************************
obj_p list6(isp ist, obj_p item1, obj_p item2, obj_p item3, obj_p item4, 
			         obj_p item5, obj_p item6 ){
	obj_p list_obj = NEW_LIST(6);
	list_append_no_lock(list_obj, item1); list_append_no_lock(list_obj, item2);
	list_append_no_lock(list_obj, item3); list_append_no_lock(list_obj, item4);
	list_append_no_lock(list_obj, item5); list_append_no_lock(list_obj, item6);
	return list_obj;
}

//********************************* list8 *************************************
obj_p list8(isp ist, obj_p item1, obj_p item2, obj_p item3, obj_p item4, 
			         obj_p item5, obj_p item6, obj_p item7, obj_p item8 ){
	obj_p list_obj = NEW_LIST(8);
	list_append_no_lock(list_obj, item1); list_append_no_lock(list_obj, item2);
	list_append_no_lock(list_obj, item3); list_append_no_lock(list_obj, item4);
	list_append_no_lock(list_obj, item5); list_append_no_lock(list_obj, item6);
	list_append_no_lock(list_obj, item7); list_append_no_lock(list_obj, item8);
	return list_obj;
}

//********************************* list10 ************************************
obj_p list10(isp ist, obj_p item1, obj_p item2, obj_p item3, obj_p item4, 
			          obj_p item5, obj_p item6, obj_p item7, obj_p item8,
					  obj_p item9, obj_p item10 ){
	obj_p list_obj = NEW_LIST(10);
	list_append_no_lock(list_obj, item1); list_append_no_lock(list_obj, item2);
	list_append_no_lock(list_obj, item3); list_append_no_lock(list_obj, item4);
	list_append_no_lock(list_obj, item5); list_append_no_lock(list_obj, item6);
	list_append_no_lock(list_obj, item7); list_append_no_lock(list_obj, item8);
	list_append_no_lock(list_obj, item9); list_append_no_lock(list_obj, item10);
	return list_obj;
}

//********************************* list16 ************************************
obj_p list16(isp ist, obj_p  item1, obj_p  item2, obj_p  item3, obj_p  item4, 
			          obj_p  item5, obj_p  item6, obj_p  item7, obj_p  item8,
			          obj_p  item9, obj_p item10, obj_p item11, obj_p item12,
					  obj_p item13, obj_p item14, obj_p item15, obj_p item16) {
	obj_p list_obj = NEW_LIST(16);
	list_append_no_lock(list_obj, item1); list_append_no_lock(list_obj, item2);
	list_append_no_lock(list_obj, item3); list_append_no_lock(list_obj, item4);
	list_append_no_lock(list_obj, item5); list_append_no_lock(list_obj, item6);
	list_append_no_lock(list_obj, item7); list_append_no_lock(list_obj, item8);
	list_append_no_lock(list_obj, item9); list_append_no_lock(list_obj, item10);
	list_append_no_lock(list_obj, item11); list_append_no_lock(list_obj, item12);
	list_append_no_lock(list_obj, item13); list_append_no_lock(list_obj, item14);
	list_append_no_lock(list_obj, item15); list_append_no_lock(list_obj, item16);
	return list_obj;
}

//********************************* quicksort *********************************
int obj_lt(obj_p a, obj_p b, isp ist, obj_p cmp_func) {
	obj_p res, arr[4];
	if (cmp_func != OBJ(NONE)) {
		arr[0] = OBJ(NOKEY);
		arr[1] = a;
		arr[2] = OBJ(NOKEY);
		arr[3] = b;
		res = call_func(ist, NULL, cmp_func, NULL, 4, arr, NULL);
		if_exc_return 0;
		return (res->data.i64 < 0);
	}
	arr[0] = OBJ(NOKEY);
	arr[1] = b;
	res = call_func(ist, a, SYM(CMP_), NULL, 2, arr, NULL);
	if_exc_return 0;
	return (res->data.i64 < 0);
}

int obj_eq(obj_p a, obj_p b, isp ist, obj_p cmp_func) {
	obj_p res, arr[4];
	if (cmp_func != OBJ(NONE)) {
		arr[0] = OBJ(NOKEY);
		arr[1] = a;
		arr[2] = OBJ(NOKEY);
		arr[3] = b;
		res = call_func(ist, NULL, cmp_func, NULL, 4, arr, NULL);
		if_exc_return 0;
		return (res->data.i64 == 0);
	}
	arr[0] = OBJ(NOKEY);
	arr[1] = b;
	res = call_func(ist, a, SYM(EQ__QUES), NULL, 2, arr, NULL);
	if_exc_return 0;
	return (res == OBJ(PR_TRUE));
}

void quicksort(obj_p a[], int l, int r, isp ist, obj_p cmp_func) {
	int i=l-1, j=r, k, p=l-1, q=r;
	obj_p tmp, v=a[r];

	if (r <= l) return;
	while (TRUE) {
		while (obj_lt(a[++i], v, ist, cmp_func)) if_exc_return;
		while (obj_lt(v, a[--j], ist, cmp_func)) if (j==l) break;
		if_exc_return;
		if (i >= j) break;
		exch(a[i], a[j]);
		if (obj_eq(a[i], v, ist, cmp_func)) { if_exc_return; p++; exch(a[p], a[i]); }
		if (obj_eq(v, a[j], ist, cmp_func)) { if_exc_return; q--; exch(a[j], a[q]); }
	}
	exch(a[i], a[r]);
	j = i-1;
	i = i+1;
	for (k=l;   k < p; k++, j--) exch(a[k], a[j]);
	for (k=r-1; k > q; k--, i++) exch(a[i], a[k]);
	quicksort(a, l, j, ist, cmp_func); if_exc_return;
	quicksort(a, i, r, ist, cmp_func);
}

// ***************************** LIST ******************************************

MODULE_START(List)
{
	list_p lstp;

	List_OBJ = OBJ(LIST_PROTO);
	MODULE_SET_DOC(List, "object prototype");
	set_obj_id(List_OBJ, *, List);
	set_attr(ist, OBJ(OBJECT), sym(ist, "List"), List_OBJ);

	SET_TYPE_IF_EXC(List_OBJ, List_OBJ, DATA_TYPE_DATAPTR) return;
	lstp = List_OBJ->data.ptr = pr_malloc(list_sizeof(2));
	lstpsetsize(lstp, 2);
	lstplen(lstp)  = 0;
}

DEF(List, init_, FORM_STAR_PARAM) {
	list_p lstp;
	BIN_EMPTY_OR_NOT_CHK(List);
	if (list_len(ist, parms[1])) {
		obj_p item = list_item(ist, parms[1], 0);
		if (has_proto(ist, item, OBJ(ITER_PROTO))) {
			def_write_lock(self);
			SET_TYPE_IF_EXC(List_OBJ, self, DATA_TYPE_DATAPTR) {
				def_write_unlock(self); return NULL; }
			lstp = self->data.ptr = pr_malloc(list_sizeof(2));
			lstpsetsize(lstp, 2);
			lstplen(lstp)  = 0;
			def_write_unlock(self);
			read_unlock(ist, self);
			while (TRUE) {
				obj_p new_item = call_func0(ist, item, SYM(NEXT));
				if (catch_exception(ist, OBJ(STOP_ITERATION_EXC), NULL)) break;
				if (ist->exception_obj) { read_lock(ist, self); return NULL; }
				list_append(ist, self, new_item);
			}
			read_lock(ist, self);
			return OBJ(NONE);
		}
	}
	read_unlock(ist, self);
	read_unlock(ist, parms[1]);
	copy_object_data(ist, self, parms[1]);
	read_lock(ist, parms[1]);
	read_lock(ist, self);
	return OBJ(NONE);
}

DEF(List, str_, NULL) {
	BIN_STR_CHK(List);
	return str_tuple_list(ist, self, "[", "]");
}

DEF(List, getItem_, FORM_RPARAM) {
	BIN_CONTENT_CHK(List);
	return get_sequence_item(ist, self, parms[1], SEQ_TYPE_LIST);
}

DEF(List, setItem_, FORM_PARAM2) {
	int j;
	obj_p slice_item1, slice_item2, slice_item3;
	obj_p slice = parms[1], value = parms[3];
	i64_t i, new_len, exp_len, index1 = 0, index2 = 0, index3 = 0, slice1_empty=FALSE, slice2_empty=FALSE;
	size_t val_len, self_len, slice_len;
	BIN_CONTENT_CHK(List);
	if (has_proto(ist, slice, OBJ(SLICE_PROTO))) {
		slice_item1 = list_item(ist, slice, 0);
		slice_len = (int) list_len(ist, slice);
	} else {
		slice_len = 1;
		slice_item1 = slice;
	}
	def_write_lock(self);
	self_len = listlen(self);
	slice_item1 = list_item(ist, slice,0);
	if (slice_item1 == SLICEPARAM_EMPTY || slice_item1 == OBJ(NONE)) {
		slice1_empty=TRUE;
		if (slice_len == 1) {
			raise_exception(ist, OBJ(INDEX_EXC), "Indexes missing");
			return NULL;									
		}
	} else {
		if (!has_proto(ist, slice_item1, OBJ(INT_PROTO))) {
			raise_exception(ist, OBJ(TYPE_EXC), "List slice index must be an integer");
			def_write_unlock(self); 
			return NULL;
		}
		index1 = (int)(slice_item1->data.i64);
		if (index1 < 0) index1 += self_len;						
		if (index1 < 0 || index1 >= self_len) {			
			raise_exception(ist, OBJ(INDEX_EXC), "Index (%"APR_INT64_T_FMT") out of range",
					(i64_t)index1);
			def_write_unlock(self);
			return NULL;									
		}
		if (slice_len == 1) {
			listitem(self, index1) = value;
			def_write_unlock(self);
			return value;
		}
	}
	if ( !has_proto(ist, value, List_OBJ) && 
		 !has_proto(ist, value, OBJ(TUPLE_PROTO)) &&
		 !has_proto(ist, value, OBJ(STRING_PROTO)) ) {
		raise_exception(ist, OBJ(TYPE_EXC), "list slices may only be replaced with a sequence");
		def_write_unlock(self);
		return NULL;									
	}
	if (slice_len == 3) {
		slice_item3 = list_item(ist, slice,2);
		if (slice_item3 == SLICEPARAM_EMPTY || slice_item3 == OBJ(NONE)) index3 = 1;
		else {
			if (!has_proto(ist, slice_item3, OBJ(INT_PROTO))) {
				raise_exception(ist, OBJ(TYPE_EXC), "sequence slice index must be an integer");
				def_write_unlock(self);
				return NULL;
			}
			index3 = (int)(slice_item3->data.i64);
			if (index3 == 0) {
				raise_exception(ist, OBJ(INDEX_EXC), "slice step cannot be zero");
				def_write_unlock(self);
				return NULL;
			}
		}
	} else index3 = 1;
	slice_item2 = list_item(ist, slice,1);
	if (slice_item2 == SLICEPARAM_EMPTY || slice_item2 == OBJ(NONE))
		slice2_empty=TRUE;
	else {
		if (!has_proto(ist, slice_item2, OBJ(INT_PROTO))) {
			raise_exception(ist, OBJ(TYPE_EXC), "sequence slice index must be an integer");
			def_write_unlock(self);
			return NULL;
		}
		index2 = (int)(slice_item2->data.i64);
		if (index2 < 0) index2 += self_len;						
		if (index2 < 0 || index2 > self_len) {			
			raise_exception(ist, OBJ(INDEX_EXC), "Second index (%"APR_INT64_T_FMT") out of range",
					(i64_t)index2);
			def_write_unlock(self);
			return NULL;									
		}
	}
	if (index3 > 0) {
		if (slice1_empty) index1 = 0;
		if (slice2_empty) index2 = self_len;
	} else {
		if (slice1_empty) index1 = self_len-1;
		if (slice2_empty) index2 = -1;
	}
	val_len = seq_len(ist, value);
	exp_len = (index2-index1+(index3-1))/index3;
	exp_len = max(exp_len, 0);
	if (slice_len==3) {
		if (exp_len != val_len) {
			raise_exception(ist, OBJ(INDEX_EXC), "extended slice assignment lengths do not match");	
			def_write_unlock(self);
			return NULL;
		}
		if (index3 > 0)
			for(i = index1, j=0; i < index2; i += index3, j++)
				listitem(self, i) = seq_item(ist, value, j);
		else
			for(i = index1, j=0; i > index2; i += index3, j++)
				listitem(self, i) = seq_item(ist, value, j);
	} else {
		new_len = index1 + (i64_t) val_len + ((i64_t)self_len - index2);
		listlen(self) = (size_t) new_len;
		if (new_len+1 > listsize(self)) {
			listsetsize(self, new_len + 1);
			self->data.ptr = 
				pr_realloc(self->data.ptr, list_sizeof(listsize(self)));
		}
		if (exp_len != val_len && index2 != self_len)
			memmove( &(((list_p)self->data.ptr)->item[index1+val_len]),
					 &(((list_p)self->data.ptr)->item[index2]),
					 (self_len - (size_t) index2) * sizeof(obj_p) );
		for(i = index1, j=0; j < (int) val_len; i++, j++)
			listitem(self, i) = seq_item(ist, value, j);
	}
	def_write_unlock(self);
	return NULL;
}

DEF(List, delItem_, FORM_RPARAM) {
	int j;
	obj_p slice_item1, slice_item2, slice_item3;
	obj_p slice = parms[1];
	i64_t i, new_len, index1 = 0, index2 = 0, index3 = 0, slice1_empty=FALSE, slice2_empty=FALSE;
	size_t exp_len, self_len, slice_len = list_len(ist, slice);
	BIN_CONTENT_CHK(List);
	def_write_lock(self);
	self_len = listlen(self);
	if (has_proto(ist, slice, OBJ(SLICE_PROTO))) {
		slice_item1 = list_item(ist, slice, 0);
		slice_len = (int) list_len(ist, slice);
	} else {
		slice_len = 1;
		slice_item1 = slice;
	}
	if (slice_item1 == SLICEPARAM_EMPTY || slice_item1 == OBJ(NONE)) {
		slice1_empty=TRUE;
		if (slice_len == 1) {
			raise_exception(ist, OBJ(INDEX_EXC), "Indexes missing");
			return NULL;									
		}
	} else {
		if (!has_proto(ist, slice_item1, OBJ(INT_PROTO))) {
			raise_exception(ist, OBJ(TYPE_EXC), "List slice index must be an integer");
			def_write_unlock(self);
			return NULL;
		}
		index1 = (int)(slice_item1->data.i64);
		if (index1 < 0) index1 += self_len;						
		if (index1 < 0 || index1 >= self_len) {			
			raise_exception(ist, OBJ(INDEX_EXC), "Index (%"APR_INT64_T_FMT") out of range",
					(i64_t)index1);
			def_write_unlock(self);
			return NULL;									
		}
		if (slice_len == 1) {
			if (index1 != self_len-1)
				memmove( &(((list_p)self->data.ptr)->item[index1]),
					     &(((list_p)self->data.ptr)->item[index1+1]),
						 (self_len-1 - (size_t)index1) * sizeof(obj_p) );
			listlen(self)--;
			def_write_unlock(self);
			return NULL;
		}
	}
	if (slice_len == 3) {
		slice_item3 = list_item(ist, slice,2);
		if (slice_item3 == SLICEPARAM_EMPTY || slice_item3 == OBJ(NONE)) index3 = 1;
		else {
			if (!has_proto(ist, slice_item3, OBJ(INT_PROTO))) {
				raise_exception(ist, OBJ(TYPE_EXC), "sequence slice index must be an integer");
				def_write_unlock(self);
				return NULL;
			}
			index3 = (int)(slice_item3->data.i64);
			if (index3 == 0) {
				raise_exception(ist, OBJ(INDEX_EXC), "slice step cannot be zero");
				def_write_unlock(self);
				return NULL;
			}
		}
	} else index3 = 1;
	slice_item2 = list_item(ist, slice,1);
	if (slice_item2 == SLICEPARAM_EMPTY || slice_item2 == OBJ(NONE))
		slice2_empty=TRUE;
	else {
		if (!has_proto(ist, slice_item2, OBJ(INT_PROTO))) {
			raise_exception(ist, OBJ(TYPE_EXC), "sequence slice index must be an integer");
			def_write_unlock(self);
			return NULL;
		}
		index2 = (int)(slice_item2->data.i64);
		if (index2 < 0) index2 += self_len;						
		if (index2 < 0 || index2 > self_len) {			
			raise_exception(ist, OBJ(INDEX_EXC), "Second index (%"APR_INT64_T_FMT") out of range",
					(i64_t)index2);
			def_write_unlock(self);
			return NULL;									
		}
	}
	if (index3 > 0) {
		if (slice1_empty) index1 = 0;
		if (slice2_empty) index2 = self_len;
	} else {
		if (slice1_empty) index1 = self_len-1;
		if (slice2_empty) index2 = -1;
	}
	exp_len = (size_t)((index2-index1+(index3-1))/index3);
	exp_len = max(exp_len, 0);
	if (index3 != 1) {
		if (index3 > 0) {
			for(i = index1, j=0; i < index2; i += index3, j++)
				if (i != self_len-1)
					memmove( &(((list_p)self->data.ptr)->item[i-j]),
							 &(((list_p)self->data.ptr)->item[i-j+1]),
								(self_len - 1 - (size_t)i) * sizeof(obj_p) );
		} else {
			for(i = index1, j=0; i > index2; i += index3, j++)
				if (i != self_len-1)
					memmove( &(((list_p)self->data.ptr)->item[i]),
							 &(((list_p)self->data.ptr)->item[i+1]),
								(self_len - 1 - (size_t)i) * sizeof(obj_p) );
		}
	} else {
		new_len = index1 + ((i64_t)self_len - index2);
		listlen(self) = (size_t) new_len;
		if (exp_len != 0 && index2 != self_len)
			memmove( &(((list_p)self->data.ptr)->item[index1]),
					 &(((list_p)self->data.ptr)->item[index2]),
					 (self_len - (size_t)index2) * sizeof(obj_p) );
		def_write_unlock(self);
		return NULL;
	}
	listlen(self) -= exp_len;
	def_write_unlock(self);
	return NULL;
}

DEF(List, add_, FORM_RPARAM) { 
	int i;
	obj_p res = copy_list_obj(ist, self);
	BIN_CONTENT_CHK(List);
	if (has_proto(ist, parms[1], OBJ(SEQ_PROTO)) && !has_proto(ist, parms[1], OBJ(STRING_PROTO)))
		for(i=0; i < (int) list_len(ist, parms[1]); i++) 
			list_append(ist, res, list_item(ist, parms[1], i));
	else
		list_append(ist, res, parms[1]);
	return res;
}

DEF(List, iadd_, FORM_RPARAM) { 
	int i;
	BIN_CONTENT_CHK(List);
	read_unlock(ist, self);
	if (has_proto(ist, parms[1], OBJ(SEQ_PROTO)) && !has_proto(ist, parms[1], OBJ(STRING_PROTO)))
		for(i=0; i < (int) list_len(ist, parms[1]); i++) 
			list_append(ist, self, list_item(ist, parms[1], i));
	else
		list_append(ist, self, parms[1]);
	read_lock(ist, self);
	return self;
}

DEF(List, mul_, FORM_RPARAM){
	obj_p res;
	list_p lstp, selfp = (list_p)(self->data.ptr);
	int i, size, times, len = (int) list_len(ist, self);
	BIN_CONTENT_CHK(List);
	if (!has_proto(ist, parms[1], OBJ(INT_PROTO))) {
		raise_exception(ist, OBJ(TYPE_EXC), "multiply times parameter must be an integer");
		return NULL;
	}
	times = (int)parms[1]->data.i64;
	size  = len*times;
	if(times == 0) return NEW_LIST(0);
	if(times == 1) return self;
	res = NEW_OBJ(List_OBJ);
	lstp = res->data.ptr = pr_malloc(list_sizeof(size));
	if(!lstp) {
		raise_exception(ist, OBJ(OUTOFMEMORY_EXC), "memory allocation failed for list multiplication");
		return NULL;
	}
	SET_TYPE_IF_EXC(List_OBJ, res, DATA_TYPE_DATAPTR) return NULL;
	lstpsetsize(lstp, size);
	lstplen(lstp)  = size;
	for(i=0; i < times; i++)
		memcpy(&(lstp->item[i*len]), selfp->item, len*sizeof(obj_p));
	return res;
}

DEF(List, imul_, FORM_RPARAM){
	list_p lstp, selfp = (list_p)(self->data.ptr);
	int i, size, times, len = (int) list_len(ist, self);
	BIN_CONTENT_CHK(List);
	if (!has_proto(ist, parms[1], OBJ(INT_PROTO))) {
		raise_exception(ist, OBJ(TYPE_EXC), "multiply times parameter must be an integer");
		return NULL;
	}
	times = (int)parms[1]->data.i64;
	size  = len*times;
	if(times == 0) { read_unlock(ist, self); list_clear(ist, self); read_lock(ist, self); return self; }
	if(times == 1) return self;
	lstp = pr_malloc(list_sizeof(size));
	if(!lstp) {
		raise_exception(ist, OBJ(OUTOFMEMORY_EXC), "memory allocation failed for list multiplication");
		return NULL;
	}
	lstpsetsize(lstp, size);
	lstplen(lstp)  = size;
	for(i=0; i < times; i++)
		memcpy(&(lstp->item[i*len]), selfp->item, len*sizeof(obj_p));
	def_write_lock(self);
	pr_free(self->data.ptr);
	self->data.ptr = lstp;
	def_write_unlock(self);
	return self;
}

DEF(List, append_BANG, FORM_RPARAM) {
	list_p lstp;
	BIN_CONTENT_CHK(List);
	def_write_lock(self);
	lstp = self->data.ptr;
	if(lstplen(lstp) == lstpsize(lstp)) {
		lstpsetsize(lstp, lstpsize(lstp) * LIST_GROWTH_FACTOR);
		lstp = pr_realloc(lstp, list_sizeof(lstpsize(lstp)));
		self->data.ptr = lstp;
	}
	listitem(self, listlen(self)++) = parms[1];
	def_write_unlock(self);
	return self;
}

DEF(List, extend_BANG, FORM_RPARAM) {
	list_p selfp, otherp;
	size_t self_len, other_len;
	BIN_CONTENT_CHK(List);
	def_write_lock(self);
	if (!has_proto(ist, parms[1], OBJ(TUPLE_PROTO)) && !has_proto(ist, parms[1], List_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "extend parameter must be a tuple or list");
		return NULL;
	}
	selfp     = self->data.ptr;
	self_len  = lstplen(selfp);
	otherp    = parms[1]->data.ptr;
	other_len = lstplen(otherp);
	if(self_len+other_len > lstpsize(selfp)) {
		lstpsetsize(selfp, (self_len+other_len) * LIST_GROWTH_FACTOR);
		selfp = pr_realloc(selfp, list_sizeof(lstpsize(selfp)));
		self->data.ptr = selfp;
	}
	memcpy( selfp->item+self_len, otherp->item, other_len*sizeof(obj_p));
	lstplen(selfp) = self_len+other_len;
	def_write_unlock(self);
	return self;
}

DEF(List, insert_BANG, FORM_PARAM2) {
	i64_t index;
	size_t self_len;
	list_p selfp;
	BIN_CONTENT_CHK(List);
	if (!has_proto(ist, parms[1], OBJ(INT_PROTO))) {
		raise_exception(ist, OBJ(TYPE_EXC), "index (i) parameter must be an integer");
		return NULL;
	}
	def_write_lock(self);
	index = (int)parms[1]->data.i64;
	selfp    = self->data.ptr;
	self_len = lstplen(selfp);
	if (index < 0 || index > self_len) {
		raise_exception(ist, OBJ(INDEX_EXC), "index (i) parameter out of range");
		def_write_unlock(self);
		return NULL;
	}
	if(lstplen(selfp) == lstpsize(selfp)) {
		lstpsetsize(selfp, lstpsize(selfp) * LIST_GROWTH_FACTOR);
		selfp = pr_realloc(selfp, list_sizeof(lstpsize(selfp)));
		self->data.ptr = selfp;
	}
	if (index < self_len)
		memmove( selfp->item+index+1, selfp->item+index, 
				  (self_len - (size_t)index) * sizeof(obj_p) ); 
	listitem(self, index) = parms[3];
	lstplen(selfp)++;
	def_write_unlock(self);
	return self;
}

DEF(List, remove_BANG, FORM_RPARAM) {
	i64_t i;
	size_t self_len;
	list_p selfp;
	BIN_CONTENT_CHK(List);
	def_write_lock(self);
	selfp    = self->data.ptr;
	self_len = lstplen(selfp);
	for (i=0; i < self_len; i++) {
		if (call_func1(ist, listitem(self, i), SYM(EQ__QUES), parms[1]) == OBJ(PR_TRUE)) break;
		if_exc_return NULL;
	}
	if (i == self_len) {
		raise_exception(ist, OBJ(INDEX_EXC), "item not found to remove from list");
		def_write_unlock(self);
		return NULL;
	}
	if (i < self_len-1)
		memmove( selfp->item+i, selfp->item+i+1, 
		        (self_len - 1 - (size_t)i) * sizeof(obj_p) ); 
	lstplen(selfp)--;
	def_write_unlock(self);
	return self;
}

DEF(List, pop_BANG, list2(ist, SYM(RPARAM),OBJ(NONE))) {
	i64_t index;
	size_t self_len;
	list_p selfp;
	obj_p res;
	BIN_CONTENT_CHK(List);
	if (parms[1] != OBJ(NONE) && !has_proto(ist, parms[1], OBJ(INT_PROTO))) {
		raise_exception(ist, OBJ(TYPE_EXC), "pop index (i) parameter must be an integer");
		return NULL;
	}
	def_write_lock(self);
	selfp    = self->data.ptr;
	self_len = lstplen(selfp);
	if (parms[1] == OBJ(NONE))
		index = self_len-1;
	else
		index = (int)(parms[1]->data.i64);
	if (index < 0 || index > self_len-1) {
		raise_exception(ist, OBJ(INDEX_EXC), "index (i) parameter out of range");
		def_write_unlock(self);
		return NULL;
	}
	res = listitem(self, index);
	if (index < self_len-1)
		memmove( selfp->item+index, selfp->item+index+1, 
				 (self_len - 1 - (size_t)index) * sizeof(obj_p) ); 
	lstplen(selfp)--;
	def_write_unlock(self);
	return res;
}

DEF(List, reverse_BANG, NULL) {
	size_t self_len;
	list_p selfp;
	obj_p *p1, *p2;
	BIN_CONTENT_CHK(List);
	def_write_lock(self);
	selfp    = self->data.ptr;
	self_len = lstplen(selfp);
	p1 = selfp->item;
	p2 = selfp->item+self_len-1;
	while(p1<p2) {
		obj_p tmp = *p2;
		*p2 = *p1;
		*p1 = tmp;
		p1++; p2--;
	}
	def_write_unlock(self);
	return self;
}

DEF(List, sort_BANG, FPARM1(f, OBJ(NONE))) {
	int	self_len;
	list_p	selfp;
	obj_p*	a;

	BIN_CONTENT_CHK(List);
	if (parms[1] != OBJ(NONE))
		CHECK_TYPE_EXC(self, OBJ(FUNC_PROTO), "function");
	selfp = self->data.ptr;
	self_len = (int) lstplen(selfp);
	a = selfp->item;
	quicksort(a, 0, self_len-1, ist, parms[1]);
	return self;
}

MAIN_MODULE_INIT(List)
{
	MODULE_SUB_INIT(List);
	MODULE_ADD_SYM(List, init_);
	MODULE_ADD_SYM(List, str_);
	MODULE_ADD_SYM(List, getItem_);
	MODULE_ADD_SYM(List, setItem_);
	MODULE_ADD_SYM(List, delItem_);
	MODULE_ADD_SYM(List, add_);
	MODULE_ADD_SYM(List, iadd_);
	MODULE_ADD_SYM(List, mul_);
	MODULE_ADD_SYM(List, imul_);
	MODULE_ADD_SYM(List, append_BANG);
	MODULE_ADD_SYM(List, extend_BANG);
	MODULE_ADD_SYM(List, insert_BANG);
	MODULE_ADD_SYM(List, remove_BANG);
	MODULE_ADD_SYM(List, pop_BANG);
	MODULE_ADD_SYM(List, reverse_BANG);
	MODULE_ADD_SYM(List, sort_BANG);

	check_exceptions(ist);
}
