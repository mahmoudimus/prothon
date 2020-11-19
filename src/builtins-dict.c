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


// builtins-dict.c

#include <stdio.h>
#include <string.h>

#include <apr_strings.h>

#include <prothon/prothon.h>
#include "dict.h"
#include "parser.h"
#include "object.h"
#include <prothon/prothon_dll.h>

MODULE_DECLARE(Dict);
MODULE_DECLARE(DictGen);
MODULE_DECLARE(DictRestore);

//********************************* new_dict_obj ******************************
obj_p new_dict_obj(isp ist, int initial_size){
	obj_p dict_obj = NEW_OBJ(OBJ(DICT_PROTO));
	dict_p dict;
	int size = max(initial_size, DEFAULT_INITIAL_DICT_SIZE);
	data_size_t data_size = dict_sizeof(size);
	dict = obj_malloc(ist, OBJ(DICT_PROTO), dict_obj, data_size); 
	memset(dict, 0, data_size);
	dictsize_bytes(dict) = data_size;
	dictsize(dict) = size;
	return dict_obj;
}

//********************************* dict_realloc *****************************
dict_p dict_realloc(dict_p dict) {
	dict_p dp, old_dict = dict;
	i32_t  i, j, hash_in, old_size = (int) dictsize(dict);
	i32_t size = max(((int)dictsize(dict) * DICT_PAD_PERCENT_FACTOR) / 100, DEFAULT_INITIAL_DICT_SIZE);
	i32_t len  =  (int) dictlen(dict);
	data_size_t data_size = dict_sizeof(size);
	dict = pr_malloc(data_size);
	memset(dict, 0, data_size);
	dictsize_bytes(dict) = data_size;
	dictsize(dict) = size;
	dictlen(dict)  = len;
	for(i=0; i < old_size; i++) {
		if ((hash_in = dicthash(old_dict, i)) > 0) {
			for (j=hash_in; (dp=dictptr(dict,j))->entry.hash; j++);
			dp->entry.hash  = hash_in;
			dp->entry.key   = dictkey(old_dict, i);
			dp->entry.value = dictitem(old_dict, i);
		}
	}
	pr_free(old_dict);
	return dict;
}

//********************************* dict_clear ********************************
void dict_clear(isp ist, obj_p dict_obj) {
	dict_p dict;
	write_lock(ist, dict_obj);
	dict = dict_d(dict_obj);
	memset(&(dict->entry)+1, 0, dictsize(dict) * sizeof(dict_entry_t));
	dictlen(dict) = 0;
	write_unlock(ist, dict_obj);
}

//********************************* hash_value ********************************
i32_t hash_value(isp ist, obj_p key_in) {
	obj_p hash_obj;
	if (!is_immutable(key_in)) {
		raise_exception(ist, OBJ(TYPE_EXC), "dictionary keys must be immutable");
		return 0;
	}
	if (has_proto(ist, key_in, OBJ(INT_PROTO))) {
		i32_t res;
		long_p v;
		if (key_in->data_type == DATA_TYPE_IMMDATA) {
			res = long_hash(v = new_longp(key_in->data.i64));
			pr_free(v);
			return res;
		}
		else
			return long_hash(key_in->data.ptr);
	}
	hash_obj = call_func0(ist, key_in, SYM(HASH_));
	if_exc_return 0;
	return hash_obj->data.i32[0];
}

//********************************* dict_add **********************************
int dict_add(isp ist, obj_p dict_obj, obj_p key_in, obj_p value_in) {
	i32_t i, hash, hash_in, match=FALSE;
	obj_p key, rp[2];
	dict_p dict, dp, hole=NULL;
	rp[0]=OBJ(NOKEY); rp[1]=key_in;

	hash_in = hash_value(ist, key_in);
	hash_in = hash_in & HASH_MASK;
	if (!hash_in) hash_in++;

	dict = dict_d(dict_obj);

	if (((dictlen(dict) * 100) / dictsize(dict)) > DICT_MAX_PERCENT_FULL)
		dict_obj->data.ptr = dict = (void *)dict_realloc(dict);

	for (i=hash_in; (key=((dp=dictptr(dict,i))->entry.key)); i++){
		hash = dp->entry.hash;
		if (hash < 0) { hole = dp; continue; }
		if ( hash == hash_in && ( key == key_in ||
			call_func(ist, key, SYM(EQ__QUES), NULL, 2, rp, NULL) == OBJ(PR_TRUE) ) ) {
			match = PR_TRUE;
			break;
		}
		if (ist->exception_obj) return FALSE;
	}
	if (ist->exception_obj) return FALSE;
	if (!match) {
		dictlen(dict)++;
		if(hole) dp = hole;
		dp->entry.hash = hash_in;
		dp->entry.key  = key_in;
	}
	dp->entry.value = value_in;
	return !match;
}

//********************************* dict_len **********************************
size_t dict_len(isp ist, obj_p dict_obj){
	size_t res;
	rdlock_rtrn(dict_obj) 0;
	res = dictlen(dict_d(dict_obj));
	read_unlock(ist, dict_obj);
	return res;
}

#define dict_rd_chk()			\
if (ist->exception_obj) {		\
	read_unlock(ist, dict_obj);	\
	return NULL;				\
}

//********************************* dict_item **********************************
obj_p dict_item(isp ist, obj_p dict_obj, obj_p key_in){
	i32_t i, hash_in;
	obj_p key, res, rp[2];
	dict_p dict, dp;
	int new_sym = FALSE;
try_again:
	rp[0]=OBJ(NOKEY); rp[1]=key_in;
	rdlock_rtrn(dict_obj) NULL;
	hash_in = hash_value(ist, key_in); 
	if (ist->exception_obj) {
		read_unlock(ist, dict_obj);	
		if (new_sym) del_unlock(key_in);
		return NULL;				
	}
	if (!hash_in) hash_in++;
	dict = (dict_p) obj_dataptr(dict_obj);
	for (i=hash_in; (key=((dp=dictptr(dict,i))->entry.key)); i++){
		if ( dp->entry.hash == hash_in && ( key == key_in ||
			 call_func(ist, key, SYM(EQ__QUES), NULL, 2, rp, NULL) == OBJ(PR_TRUE) ) ) {
			dict_rd_chk();
			res = dp->entry.value;
			read_unlock(ist, dict_obj);
			if (new_sym) del_unlock(key_in);
			return res;
		}
		dict_rd_chk();
	}
	read_unlock(ist, dict_obj);
	if (has_proto(ist, key_in, OBJ(STRING_PROTO))) {
		key_in = sym(ist, pr2c_strptr(ist, key_in));
		if (!ist->exception_obj) {
			new_sym = TRUE;
			goto try_again;
		}
		ist->exception_obj = NULL;
	}
	if (new_sym) del_unlock(key_in);
	return NULL;
}

//********************************* dict_keys_values **************************
obj_p dict_keys_values(isp ist, obj_p dict_obj, int key_flg){
	size_t i, size, len, llen, cnt=0;
	int no_tuple = (key_flg == 4);
	obj_p list_obj;
	dict_p dict, dp;
	rdlock_rtrn(dict_obj) NULL;
	dict = dict_d(dict_obj);
	size = dictsize(dict);
	len  = dictlen(dict);
	switch (key_flg) {
		case 2:  llen = 2 * len; break;
		case 4:  llen = 2;       break;
		default: llen = len;     break;
	}
	list_obj = NEW_LIST((int)llen);
	if_exc_return NULL;
	for(i=0; i < size; i++) {
		if ((dp=dictptr(dict,i))->entry.hash > 0) {
			assert (cnt++ < len);
			if (key_flg == 3) {
				obj_p tuple = NEW_LIST(2);
				list_append(ist, tuple, dp->entry.key);
				list_append(ist, tuple, dp->entry.value);
				switch_proto(ist, tuple, OBJ(TUPLE_PROTO));
				set_immutable(tuple);
				list_append(ist, list_obj, tuple);
			} else {
				if (key_flg == 1 || key_flg == 2 || key_flg == 4)
				list_append(ist, list_obj, dp->entry.key);
				if (key_flg == 0 || key_flg == 2 || key_flg == 4)
				list_append(ist, list_obj, dp->entry.value);
			}
			if (key_flg == 4) {
				switch_proto(ist, list_obj, OBJ(TUPLE_PROTO));
				set_immutable(list_obj);
				dp->entry.hash = ENTRY_DELETED;
			    dictlen(dict)--;
				no_tuple = FALSE;
				break;
			}
		}
	}
	if (no_tuple) return OBJ(NONE);
	read_unlock(ist, dict_obj);
	return list_obj;
}

// ***************************** DICT ******************************************

MODULE_START(Dict)
{
	dict_p dict;
	data_size_t dsize = dict_sizeof(DEFAULT_INITIAL_DICT_SIZE);

	Dict_OBJ = OBJ(DICT_PROTO);
	MODULE_SET_DOC(Dict, "dictionary object prototype");
	set_obj_id(Dict_OBJ, *, Dict);
	set_attr(ist, OBJ(OBJECT), sym(ist, "Dict"), Dict_OBJ);

    dict = obj_malloc(ist, OBJ(DICT_PROTO), Dict_OBJ, dsize);
	if_exc_return;
    memset(dict, 0, dsize);
    dictsize_bytes(dict) = dsize;
    dictsize(dict) = DEFAULT_INITIAL_DICT_SIZE;
}

#define __ADD_QUOTE(__is_str)  \
do {					       \
	if (__is_str)			   \
		strcat(res, "'");	   \
} while(0)

DEF(Dict, init_, FORM_STAR3_PARAM) {
	dict_p dict;
	obj_p list, dict_in;
	int i, llen, size;
	data_size_t dsize;

    BIN_EMPTY_OR_NOT_CHK(Dict);
	llen = (int) list_len(ist, parms[1]);
	if (llen == 1 && (dict_in = list_item(ist, parms[1], 0))) {
		CHECK_TYPE_EXC(dict_in, OBJ(DICT_PROTO), dictionary);
	} else if (llen) {
		raise_exception(ist, OBJ(TYPE_EXC), "invalid dictionary init_ arguments");
		return NULL;
	} else
		dict_in = parms[3];
	size  = max(DEFAULT_INITIAL_DICT_SIZE, (int) dict_len(ist, dict_in)*3);
	dsize = dict_sizeof(size);
	dict  = obj_malloc(ist, OBJ(DICT_PROTO), self, dsize);
    memset(dict, 0, dsize);
    dictsize_bytes(dict) = dsize;
    dictsize(dict) = size;
	list = dict_keys_values(ist, dict_in, 2 /*list*/);
	llen = (int) list_len(ist, list);
	for(i=0; i < llen; i += 2)
		dict_add(ist, self, list_item(ist, list, i), list_item(ist, list, i+1));
	del_unlock(list);
	return OBJ(NONE);
}

DEF(Dict, str_, NULL) {
	int i;
	size_t len;
	char* res;
	obj_p res_obj, keys, vals;

	BIN_STR_CHK(Dict);

	len = dict_len(ist, self);
	keys = dict_keys(ist, self);
	vals = dict_values(ist, self);
	pr_assert(list_len(ist, keys) == list_len(ist, vals));

	res = pr_malloc(3);
	strcpy(res, "{");
	for (i=0; i < (int) len; i++) {
		obj_p   item1,   item2;
		char    *str1,   *str2;
		int   is_str1, is_str2;

		item1 = list_item(ist, keys,i);
		item2 = list_item(ist, vals,i);
		pr_assert (item2);

		str1 = as_str(ist, item1);
		str2 = as_str(ist, item2);
		is_str1 = has_proto(ist, item1, OBJ(STRING_PROTO));
		is_str2 = has_proto(ist, item2, OBJ(STRING_PROTO));

		if (str1 == NULL)
			str1 = "(null)";
		if (str2 == NULL)
			str2 = "(null)";

		res = pr_realloc(res, strlen(res)+strlen(str1)+1+strlen(str2)+( i != len-1 ? 3 : 2)+6);

		__ADD_QUOTE(is_str1);
		strcat(res, str1);
		__ADD_QUOTE(is_str1);
		strcat(res, ":");
		__ADD_QUOTE(is_str2);
		strcat(res, str2);
		__ADD_QUOTE(is_str2);

		if (i != len-1) strcat(res, ", ");
	}
	strcat(res, "}");

	res_obj = NEW_STRING(res);
	pr_free(res);

	return res_obj;
}

DEF(Dict, hasKey_QUES, FPARM1(k, OBJ(NODEF))) {
	obj_p res;
	BIN_CONTENT_CHK(Dict);
	if ((res = dict_item(ist, self, parms[1]))) {
		del_unlock(res);
		return OBJ(PR_TRUE);
	}
	return OBJ(PR_FALSE);
}

DEF(Dict, get, FPARM2(k, OBJ(NODEF), x, OBJ(NONE))) {
	obj_p res;
	BIN_CONTENT_CHK(Dict);
	if ((res = dict_item(ist, self, parms[1]))) {
		return res;
	}
	return parms[3];
}

DEF(Dict, rIn__QUES, FORM_RPARAM) {
	obj_p res;
	BIN_CONTENT_CHK(Dict);
	if ((res = dict_item(ist, self, parms[1]))) {
		del_unlock(res);
		return OBJ(PR_TRUE);
	}
	return OBJ(PR_FALSE);
}

DEF(Dict, rNotIn__QUES, FORM_RPARAM) {
	obj_p res;
	BIN_CONTENT_CHK(Dict);
	if ((res = dict_item(ist, self, parms[1]))) {
		del_unlock(res);
		return OBJ(PR_FALSE);
	}
	return OBJ(PR_TRUE);
}

DEF(Dict, objList_, FORM_RPARAM) {
	if (!self->data.ptr) return parms[1];
	return dict_keys_values(ist, self, 0 /*values*/);
}

DEF(Dict, getItem_, FORM_RPARAM) {
	obj_p res, index;

	BIN_CONTENT_CHK(Dict);
	CHECK_TYPE_EXC(self, OBJ(DICT_PROTO), "dict");
	if (has_proto(ist, parms[1], OBJ(SLICE_PROTO))) {
		if (list_len(ist, parms[1]) > 1) {
			raise_exception(ist, OBJ(INTERPRETER_EXC), "slice not allowed in attr dictionary indexing");	
			return NULL;
		}
		index = list_item(ist, parms[1], 0);
	} else
		index = parms[1];
	if (!(res = dict_item(ist, self, index))) {
		raise_exception( ist, OBJ(INDEX_EXC), "no entry found with key: %s",
				              as_str(ist, index) );
		return NULL;
	}
	return res;
}

DEF(Dict, setItem_, FORM_PARAM2) {
	obj_p index;
	BIN_CONTENT_CHK(Dict);
	if (has_proto(ist, parms[1], OBJ(SLICE_PROTO))) {
		if (list_len(ist, parms[1]) > 1) {
			raise_exception(ist, OBJ(INTERPRETER_EXC), "slice not allowed in attr dictionary indexing");	
			return NULL;
		}
		index = list_item(ist, parms[1], 0);
	} else
		index = parms[1];
	def_write_lock(self);
	dict_add(ist, self, index, parms[3]);
	def_write_unlock(self);
	return NULL;
}

DEF(Dict, delItem_, FORM_RPARAM) {
	i32_t i, hash_in;
	obj_p key, key_in, rp[2], index;
	dict_p dict, dp;
	BIN_CONTENT_CHK(Dict);
	if (has_proto(ist, parms[1], OBJ(SLICE_PROTO))) {
		if (list_len(ist, parms[1]) > 1) {
			raise_exception(ist, OBJ(INTERPRETER_EXC), "slice not allowed in attr dictionary indexing");	
			return NULL;
		}
		index = list_item(ist, parms[1], 0);
	} else
		index = parms[1];
	rp[0]=OBJ(NOKEY); rp[1] = key_in = index;
	def_write_lock(self);
	hash_in = hash_value(ist, key_in);
	if (ist->exception_obj) {	
		def_write_unlock(self);
		return NULL;
	}
	dict = (dict_p) self->data.ptr;
	for(i=hash_in; (key=((dp=dictptr(dict,i))->entry.key)); i++){
		if ( dp->entry.hash == hash_in && ( key == key_in ||
			 call_func(ist, key, SYM(EQ__QUES), NULL, 2, rp, NULL) == OBJ(PR_TRUE) ) ) {
			dp->entry.hash = ENTRY_DELETED;
			dictlen(dict)--;
			def_write_unlock(self);
			return NULL;
		}
		if (ist->exception_obj) {	
			def_write_unlock(self);
			return NULL;
		}
	}
	raise_exception(ist, OBJ(INDEX_EXC), "no entry found to delete");
	return NULL;
}

DEF(Dict, len_, NULL) {
	BIN_CONTENT_CHK(Dict);
	return NEW_INT(dict_len(ist, self));
}

DEF(Dict, bool__QUES, NULL) {
	BIN_CONTENT_CHK(Dict);
	if (dict_len(ist, self)) return OBJ(PR_TRUE);
	else                     return OBJ(PR_FALSE);
}

DEF(Dict, items, NULL) {
	BIN_CONTENT_CHK(Dict);
	return dict_keys_values(ist, self, 3 /*tuples*/);
}

DEF(Dict, keys, NULL) {
	BIN_CONTENT_CHK(Dict);
	return dict_keys_values(ist, self, 1 /*keys*/);
}

DEF(Dict, values, NULL) {
	BIN_CONTENT_CHK(Dict);
	return dict_keys_values(ist, self, 0 /*values*/);
}

DEF(Dict, clear_BANG, NULL) {
	dict_p dict;
	BIN_CONTENT_CHK(Dict);
	def_write_lock(self);
	dict = dict_d(self);
	memset(&(dict->entry)+1, 0, dictsize(dict) * sizeof(dict_entry_t));
	dictlen(dict) = 0;
	def_write_unlock(self);
	return self;
}

DEF(Dict, update_BANG, FPARM1(D1, OBJ(NODEF))) {
	size_t i, llen;
	obj_p list;

	BIN_CONTENT_CHK(Dict);
	CHECK_TYPE_EXC(parms[1], OBJ(DICT_PROTO), "dict");
	list = dict_keys_values(ist, parms[1], 2 /*list*/);
	llen = list_len(ist, list);
	def_write_lock(self);
	for(i=0; i < llen; i += 2)
		dict_add(ist, self, list_item(ist, list, (int) i), list_item(ist, list, (int) i+1));
	def_write_unlock(self);
	del_unlock(list);
	return self;
}

DEF(Dict, setDefault_BANG, FPARM2(k, OBJ(NODEF), x, OBJ(NONE))) {
	obj_p res;
	BIN_CONTENT_CHK(Dict);
	if ((res = dict_item(ist, self, parms[1]))) {
		return res;
	}
	def_write_lock(self);
	dict_add(ist, self, parms[1], parms[3]);
	def_write_unlock(self);
	return parms[3];
}

DEF(Dict, popItem_BANG, NULL) {
	BIN_CONTENT_CHK(Dict);
	return dict_keys_values(ist, self, 4 /*tuple*/);
}

DEF(Dict, toStorProxy_, NULL) {
	obj_p proxy_obj = dict_keys_values(ist, self, 2 /*list*/);
	BIN_CONTENT_CHK(Dict);
	switch_proto(ist, proxy_obj, DictRestore_OBJ);
	return proxy_obj;
}

MODULE_START(DictRestore) {
	DictRestore_OBJ = NEW_OBJ(OBJ(LIST_PROTO));
	MODULE_ADD_TO_OBJ(DictRestore, OBJ(OBJECT), "DictRestore");
	set_obj_id(DictRestore_OBJ, *, DictRestore);
}

DEF(DictRestore, fromStorProxy_, NULL) {
	int i, llen = (int)list_len(ist, self);
	obj_p dict = NEW_DICT(2*llen);
	BIN_CONTENT_CHK(DictRestore);
	for (i=0; i < llen; i += 2)
		dict_add(ist, dict, list_item(ist, self, i), list_item(ist, self, i+1));
	return dict;
}

MODULE_START(DictGen)
{
	DictGen_OBJ = NEW_OBJ(OBJ(ITER_PROTO));
	set_obj_id(DictGen_OBJ, *, DictGen);
}

DEF(Dict, iter_, NULL) {
	obj_p list_obj, gen_obj = NEW_OBJ(DictGen_OBJ);

	BIN_CONTENT_CHK(Dict);
	SET_TYPE_IF_EXC(DictGen_OBJ, gen_obj, DATA_TYPE_DATAPTR) return NULL;
	gen_obj->data.ptr = list_obj = dict_keys(ist, self);

	listlen(list_obj) = 0;
	return gen_obj;
}

DEF(DictGen, next, NULL) {
	obj_p res, list_obj = self->data.ptr;

	BIN_CONTENT_CHK(DictGen);
	CHECK_TYPE_EXC(self, DictGen_OBJ, "DictGen");
	CHECK_TYPE_EXC(list_obj, OBJ(SEQ_PROTO), "list");

	if (listsize(list_obj) == listlen(list_obj)) {
		raise_exception(ist, OBJ(STOP_ITERATION_EXC), NULL);
		return NULL;
	}

	res = listitem(list_obj, listlen(list_obj));
	listlen(list_obj)++;

	return res;
}

MAIN_MODULE_INIT(Dict)
{
	MODULE_SUB_INIT(Dict);
	MODULE_ADD_SYM(Dict, init_);
	MODULE_ADD_SYM(Dict, str_);
	MODULE_ADD_SYM(Dict, hasKey_QUES);
	MODULE_ADD_SYM(Dict, items);
	MODULE_ADD_SYM(Dict, len_);
	MODULE_ADD_SYM(Dict, bool__QUES);
	MODULE_ADD_SYM(Dict, keys);
	MODULE_ADD_SYM(Dict, values);
	MODULE_ADD_SYM(Dict, get);
	MODULE_ADD_SYM(Dict, clear_BANG);
	MODULE_ADD_SYM(Dict, update_BANG);
	MODULE_ADD_SYM(Dict, setDefault_BANG);
	MODULE_ADD_SYM(Dict, popItem_BANG);
	MODULE_ADD_SYM(Dict, rIn__QUES);
	MODULE_ADD_SYM(Dict, rNotIn__QUES);
	MODULE_ADD_SYM(Dict, getItem_);
	MODULE_ADD_SYM(Dict, setItem_);
	MODULE_ADD_SYM(Dict, delItem_);
	MODULE_ADD_SYM(Dict, iter_);
	MODULE_ADD_SYM(Dict, toStorProxy_);
	MODULE_ADD_SYM(Dict, objList_);

	MODULE_SUB_INIT(DictRestore);
	MODULE_ADD_SYM(DictRestore, fromStorProxy_);

	MODULE_SUB_INIT(DictGen);
	MODULE_ADD_SYM(DictGen, next);

	check_exceptions(ist);
}
