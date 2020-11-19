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


// builtins-attrdict.c

#include <stdio.h>
#include <string.h>

#include <apr_strings.h>

#include <prothon/prothon.h>
#include "dict.h"
#include "parser.h"
#include "object.h"
#include <prothon/prothon_dll.h>

MODULE_DECLARE(AttrDict);

// ***************************** new_attrdict_obj *****************************
obj_p new_attrdict_obj(isp ist, obj_p obj) {
	obj_p self = new_object(ist, AttrDict_OBJ);
	SET_TYPE_IF_EXC(AttrDict_OBJ, self, DATA_TYPE_EXTPTR) return NULL;
	self->data.ptr  = obj;
	set_unclonable(self);
	return self;
}

// ***************************** attr2dict ************************************
obj_p attr2dict(isp ist, obj_p self) {
	int i;
	size_t alen, asize;
	attr_p attrp;
	obj_p dict_obj;
	rdlock_rtrn(self) NULL;
	if (!self->has_attrs) {
		read_unlock(ist, self);
		return NEW_DICT(0);
	}
	attrp = self->attr_proto.attrs;
	asize = attr_asize(attrp);
	alen  = attr_alen(attrp);
	dict_obj = NEW_DICT((int)alen);
	for (i=0; i < (int) asize; i++) {
		if (attr_ap(attrp,i)->attr.key > 0) {
			obj_p sym = key_to_sym(ist, attr_ap(attrp,i)->attr.key); if_exc_return NULL;
			dict_add( ist, dict_obj, sym, attr_ap(attrp,i)->attr.value );
		}
	}
	read_unlock(ist, self);
	return dict_obj;
}

// ***************************** dict2attrs ************************************
void dict2attrs(isp ist, obj_p dict, obj_p obj) {
	obj_p dict_list = dict_keys_values(ist, dict, 2 /*list*/);
	int i, llen = (int) list_len(ist, dict_list);
	if_exc_return;

	write_lock(ist, obj);
	if (obj->has_attrs) {
		attr_p attrp = obj->attr_proto.attrs;
		memset(attr_ap(attrp, 0), 0, attr_asize(attrp) * sizeof(attr_t));
		attr_alen(attrp) = 0;
	} else if (llen) {
		add_attrs(obj);
	} else {
		write_unlock(ist, obj);
		return;
	}
	write_unlock(ist, obj);
	for (i = 0; i < llen; i += 2) {
		obj_p key = list_item(ist, dict_list, i);
		if (has_proto(ist, key, OBJ(STRING_PROTO)))
			key = sym(ist, pr2c_strptr(ist, key));
		if (!has_proto(ist, key, OBJ(SYMBOL_PROTO))) {
			raise_exception(ist, OBJ(TYPE_EXC), "attrs_ key must be a string or symbol");
			return;
		}
		set_attr(ist, obj, key, list_item(ist, dict_list, i+1));
	}
	return ;
}

// ***************************** ATTRDICT *************************************

MODULE_START(AttrDict)
{
	AttrDict_OBJ = NEW_OBJ(NULL);
	MODULE_SET_DOC(AttrDict, "attribute fake dictionary prototype");
	set_obj_id(AttrDict_OBJ, *, AttrDict);
	set_attr(ist, OBJ(OBJECT), sym(ist, "AttrDict"), AttrDict_OBJ);

	SET_TYPE_IF_EXC(AttrDict_OBJ, AttrDict_OBJ, DATA_TYPE_EXTPTR) return;
	AttrDict_OBJ->data.ptr  = OBJ(OBJECT);
	set_unclonable(AttrDict_OBJ);
}

DEF(AttrDict, init_, FORM_RPARAM) {
    BIN_EMPTY_CHK();
	SET_TYPE_IF_EXC(AttrDict_OBJ, self, DATA_TYPE_EXTPTR) return NULL;
	self->data.ptr  = parms[1];
	set_unclonable(self);
	return OBJ(NONE);
}

DEF(AttrDict, str_, NULL) {
	obj_p res, dict;
	BIN_STR_CHK(AttrDict_OBJ);
	dict = attr2dict(ist, self->data.ptr); if_exc_return NULL;
	res = call_func0(ist, dict, SYM(STR_));
	del_unlock(dict);
	return res;
}

DEF(AttrDict, hasKey_QUES, FPARM1(k, OBJ(NODEF))) {
	obj_p res, dict;
	BIN_CONTENT_CHK(AttrDict);
	dict = attr2dict(ist, self->data.ptr); if_exc_return NULL;
	res = call_func1( ist, dict, sym(ist, "hasKey_QUES"), parms[1] );
	del_unlock(dict);
	return res;
}

DEF(AttrDict, get, FPARM2(k, OBJ(NODEF), x, OBJ(NONE))) {
	obj_p res, dict;
	BIN_CONTENT_CHK(AttrDict);
	res = call_func2( ist, dict = attr2dict(ist, self->data.ptr), 
						   sym(ist, "get"), parms[1], parms[3] );
	del_unlock(dict);
	return res;
}

DEF(AttrDict, rIn__QUES, FORM_RPARAM) {
	obj_p res, dict;
	BIN_CONTENT_CHK(AttrDict);
	res = call_func1( ist, dict = attr2dict(ist, self->data.ptr), 
						   sym(ist, "rIn__QUES"), parms[1] );
	del_unlock(dict);
	return res;
}

DEF(AttrDict, rNotIn__QUES, FORM_RPARAM) {
	obj_p res, dict;
	BIN_CONTENT_CHK(AttrDict);
	res = call_func1( ist, dict = attr2dict(ist, self->data.ptr), 
						   sym(ist, "rNotIn__QUES"), parms[1] );
	del_unlock(dict);
	return res;
}

DEF(AttrDict, getItem_, FORM_RPARAM) {
	obj_p res = NULL, index;
	BIN_CONTENT_CHK(AttrDict);
	if (has_proto(ist, parms[1], OBJ(SLICE_PROTO))) {
		if (list_len(ist, parms[1]) > 1) {
			raise_exception(ist, OBJ(INTERPRETER_EXC), "slice not allowed in attr dictionary indexing");	
			return NULL;
		}
		index = list_item(ist, parms[1], 0);
	} else
		index = parms[1];
	if (has_proto(ist, index, OBJ(SYMBOL_PROTO)))
		res = get_attr(ist, self->data.ptr, index);
	else if (has_proto(ist, index, OBJ(STRING_PROTO))) {
		res = get_attr(ist, self->data.ptr, sym(ist, pr2c_strptr(ist, index)));
	} else
		raise_exception(ist, OBJ(TYPE_EXC), "attrs_ index must be a string or symbol");
	return res;
}

DEF(AttrDict, setItem_, FORM_PARAM2) {
	obj_p index;
	BIN_CONTENT_CHK(AttrDict);
	if (has_proto(ist, parms[1], OBJ(SLICE_PROTO))) {
		if (list_len(ist, parms[1]) > 1) {
			raise_exception(ist, OBJ(INTERPRETER_EXC), "slice not allowed in attr dictionary indexing");	
			return NULL;
		}
		index = list_item(ist, parms[1], 0);
	} else
		index = parms[1];
	if (has_proto(ist, index, OBJ(SYMBOL_PROTO)))
		set_attr(ist, self->data.ptr, index, parms[3]);
	else if (has_proto(ist, index, OBJ(STRING_PROTO))) {
		set_attr(ist, self->data.ptr, sym(ist, pr2c_strptr(ist, index)), parms[3]);
	} else
		raise_exception(ist, OBJ(TYPE_EXC), "attrs_ index must be a string or symbol");
	return NULL;
}

DEF(AttrDict, delItem_, FORM_RPARAM) {
	obj_p index;
	BIN_CONTENT_CHK(AttrDict);
	if (has_proto(ist, parms[1], OBJ(SLICE_PROTO))) {
		if (list_len(ist, parms[1]) > 1) {
			raise_exception(ist, OBJ(INTERPRETER_EXC), "slice not allowed in attr dictionary indexing");	
			return NULL;
		}
		index = list_item(ist, parms[1], 0);
	} else
		index = parms[1];
	if (has_proto(ist, index, OBJ(SYMBOL_PROTO)))
		del_attr(ist, self->data.ptr, index);
	else if (has_proto(ist, index, OBJ(STRING_PROTO))) {
		del_attr(ist, self->data.ptr, sym(ist, pr2c_strptr(ist, index)));
	} else
		raise_exception(ist, OBJ(TYPE_EXC), "attrs_ index must be a string or symbol");
	return NULL;
}

DEF(AttrDict, len_, NULL) {
	obj_p obj;
	int res;
	BIN_CONTENT_CHK(AttrDict);
	obj = self->data.ptr;
	if (obj->has_attrs)
		res = attr_alen(obj->attr_proto.attrs);
	else
		res = 0;
	return NEW_INT(res);
}

DEF(AttrDict, items, NULL) {
	obj_p res, dict;
	BIN_CONTENT_CHK(AttrDict);
	res = dict_keys_values(ist, dict = attr2dict(ist, self->data.ptr), 3 );
	del_unlock(dict);
	return res;
}

DEF(AttrDict, keys, NULL) {
	obj_p res, dict;
	BIN_CONTENT_CHK(AttrDict);
	res = dict_keys_values(ist, dict = attr2dict(ist, self->data.ptr), 1 );
	del_unlock(dict);
	return res;
}

DEF(AttrDict, values, NULL) {
	obj_p res, dict;
	BIN_CONTENT_CHK(AttrDict);
	res = dict_keys_values(ist, dict = attr2dict(ist, self->data.ptr), 0 );
	del_unlock(dict);
	return res;
}

DEF(AttrDict, clear_BANG, NULL) {
	obj_p obj;
	attr_p attrp;
	BIN_CONTENT_CHK(AttrDict);
	obj = self->data.ptr;
	write_lock(ist, obj);
	attrp = obj->attr_proto.attrs;
	memset(attr_ap(attrp, 0), 0, attr_asize(attrp) * sizeof(attr_t));
	attr_alen(attrp) = 0;
	write_unlock(ist, obj);
	return self;
}

DEF(AttrDict, objList_, FORM_RPARAM) {
	return parms[1];
}

MAIN_MODULE_INIT(AttrDict)
{
	MODULE_SUB_INIT(AttrDict);
	MODULE_ADD_SYM(AttrDict, init_);
	MODULE_ADD_SYM(AttrDict, str_);
	MODULE_ADD_SYM(AttrDict, hasKey_QUES);
	MODULE_ADD_SYM(AttrDict, items);
	MODULE_ADD_SYM(AttrDict, len_);
	MODULE_ADD_SYM(AttrDict, keys);
	MODULE_ADD_SYM(AttrDict, values);
	MODULE_ADD_SYM(AttrDict, get);
	MODULE_ADD_SYM(AttrDict, clear_BANG);
	MODULE_ADD_SYM(AttrDict, rIn__QUES);
	MODULE_ADD_SYM(AttrDict, rNotIn__QUES);
	MODULE_ADD_SYM(AttrDict, getItem_);
	MODULE_ADD_SYM(AttrDict, setItem_);
	MODULE_ADD_SYM(AttrDict, delItem_);

	MODULE_ADD_SYM(AttrDict, objList_);

	check_exceptions(ist);
}
