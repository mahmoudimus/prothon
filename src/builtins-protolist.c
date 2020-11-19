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

MODULE_DECLARE(ProtoList);

// ***************************** new_protolist_obj ****************************
obj_p new_protolist_obj(isp ist, obj_p obj) {
	obj_p self = new_object(ist, ProtoList_OBJ);
	SET_TYPE_IF_EXC(ProtoList_OBJ, self, DATA_TYPE_EXTPTR) return NULL;
	self->data.ptr  = obj;
	set_unclonable(self);
	return self;
}

// ***************************** protos2list **********************************
obj_p protos2list(isp ist, obj_p obj) {
	int i, llen;
	obj_p list;
	rdlock_rtrn(obj) NULL;
	llen = proto_len(ist, obj);
	list = new_list_obj(ist, llen);
	for (i=0; i < llen; i++)
		list_append(ist, list, proto_item(ist, obj, i));
	read_unlock(ist, obj);
	return list;
}

// ***************************** list2protos **********************************
void list2protos(isp ist, obj_p list, obj_p obj) {
	int i, llen = (int) list_len(ist, list);
	if (!llen) {
		raise_exception(ist, OBJ(INTERPRETER_EXC), "protos_ list must have at least one entry");
		return;
	}
	switch_proto(ist, obj, list_item(ist, list, 0));
	for (i=1; i < llen; i++)
		add_proto(ist, obj, list_item(ist, list, i));
}

// ***************************** PROTOLIST ************************************

MODULE_START(ProtoList)
{
	ProtoList_OBJ = NEW_OBJ(NULL);
	MODULE_SET_DOC(ProtoList, "protos_ list prototype");
	set_obj_id(ProtoList_OBJ, *, ProtoList);
	set_attr(ist, OBJ(OBJECT), sym(ist, "ProtoList"), ProtoList_OBJ);

	ProtoList_OBJ->data_type = DATA_TYPE_EXTPTR;
	ProtoList_OBJ->data.ptr = OBJ(OBJECT);
	set_unclonable(ProtoList_OBJ);
}

DEF(ProtoList, init_, FORM_RPARAM) {
	BIN_EMPTY_CHK();
	SET_TYPE_IF_EXC(ProtoList_OBJ, self, DATA_TYPE_EXTPTR) return NULL;
	self->data.ptr = parms[1];
	set_unclonable(self);
	return OBJ(NONE);
}

DEF(ProtoList, str_, NULL) {
	obj_p res, list;
	BIN_STR_CHK(ProtoList);
	res = call_func0(ist, list = protos2list(ist, self->data.ptr), SYM(STR_));
	del_unlock(list);
	return res;
}
DEF(ProtoList, eq__QUES, FORM_RPARAM) { 
	obj_p res, list;
	BIN_CONTENT_CHK(ProtoList);
	res = call_func1(ist, list = protos2list(ist, self->data.ptr), 
		                  sym(ist, "eq__QUES"), parms[1]);
	del_unlock(list);
	return res;
}

DEF(ProtoList, getItem_, FORM_RPARAM) {
	obj_p res, list;
	BIN_CONTENT_CHK(ProtoList);
	res = call_func1(ist, list = protos2list(ist, self->data.ptr), 
		                  sym(ist, "getItem_"), parms[1]);
	del_unlock(list);
	return res;
}

DEF(ProtoList, setItem_, FORM_PARAM2) {
	obj_p list;
	BIN_CONTENT_CHK(ProtoList);
	call_func2(ist, list = protos2list(ist, self->data.ptr), 
	                sym(ist, "setItem_"), parms[1], parms[3]);
	list2protos(ist, list, self->data.ptr);
	del_unlock(list);
	return NULL;
}

DEF(ProtoList, delItem_, FORM_RPARAM) {
	obj_p list;
	BIN_CONTENT_CHK(ProtoList);
	call_func1(ist, list = protos2list(ist, self->data.ptr), 
	                sym(ist, "delItem_"), parms[1]);
	list2protos(ist, list, self->data.ptr);
	del_unlock(list);
	return NULL;
}

DEF(ProtoList, append_BANG, FORM_RPARAM) {
	obj_p list;
	BIN_CONTENT_CHK(ProtoList);
	call_func1(ist, list = protos2list(ist, self->data.ptr), 
	                sym(ist, "append_BANG"), parms[1]);
	list2protos(ist, list, self->data.ptr);
	del_unlock(list);
	return NULL;
}

DEF(ProtoList, insert_BANG, FORM_PARAM2) {
	obj_p list;
	BIN_CONTENT_CHK(ProtoList);
	call_func2(ist, list = protos2list(ist, self->data.ptr), 
	                sym(ist, "insert_BANG"), parms[1], parms[3]);
	list2protos(ist, list, self->data.ptr);
	del_unlock(list);
	return NULL;
}

DEF(ProtoList, remove_BANG, FORM_RPARAM) {
	obj_p list;
	BIN_CONTENT_CHK(ProtoList);
	call_func1(ist, list = protos2list(ist, self->data.ptr), 
	                sym(ist, "remove_BANG"), parms[1]);
	list2protos(ist, list, self->data.ptr);
	del_unlock(list);
	return NULL;
}

DEF(ProtoList, rIn__QUES, FORM_RPARAM) {
	obj_p res, list;
	BIN_CONTENT_CHK(ProtoList);
	res = call_func1(ist, list = protos2list(ist, self->data.ptr), 
		                  sym(ist, "rIn__QUES"), parms[1]);
	del_unlock(list);
	return res;
}

DEF(ProtoList, rNotIn__QUES, FORM_RPARAM) {
	obj_p res, list;
	BIN_CONTENT_CHK(ProtoList);
	res = call_func1(ist, list = protos2list(ist, self->data.ptr), 
		                  sym(ist, "rNotIn__QUES"), parms[1]);
	del_unlock(list);
	return res;
}

DEF(ProtoList, len_, NULL) {
	BIN_CONTENT_CHK(ProtoList);
	return NEW_INT(proto_len(ist, self->data.ptr));
}

DEF(ProtoList, objList_, FORM_RPARAM) {
	return parms[1];
}

MAIN_MODULE_INIT(ProtoList)
{
	MODULE_SUB_INIT(ProtoList);
	MODULE_ADD_SYM(ProtoList, init_);
	MODULE_ADD_SYM(ProtoList, str_);
	MODULE_ADD_SYM(ProtoList, eq__QUES);
	MODULE_ADD_SYM(ProtoList, len_);
	MODULE_ADD_SYM(ProtoList, rIn__QUES);
	MODULE_ADD_SYM(ProtoList, rNotIn__QUES);
	MODULE_ADD_SYM(ProtoList, getItem_);
	MODULE_ADD_SYM(ProtoList, setItem_);
	MODULE_ADD_SYM(ProtoList, delItem_);
	MODULE_ADD_SYM(ProtoList, append_BANG);
	MODULE_ADD_SYM(ProtoList, insert_BANG);
	MODULE_ADD_SYM(ProtoList, remove_BANG);

	MODULE_ADD_SYM(ProtoList, objList_);
	check_exceptions(ist);
}
