/*====================================================================
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

MODULE_DECLARE(Object);
MODULE_DECLARE(Symbol);
MODULE_DECLARE(False);
MODULE_DECLARE(True);
MODULE_DECLARE(None);
MODULE_DECLARE(Exception);
MODULE_DECLARE(Gen);
MODULE_DECLARE(Local);
MODULE_DECLARE(Super);
MODULE_DECLARE(Seq);
MODULE_DECLARE(Func);
MODULE_DECLARE(Method);
MODULE_DECLARE(RangeGen);
MODULE_DECLARE(Slice);
MODULE_DECLARE(Prop);
MODULE_DECLARE(WeakRef);
MODULE_DECLARE(UniqueObjects);
MODULE_DECLARE(NoKey);
MODULE_DECLARE(NoDef);
MODULE_DECLARE(Star);
MODULE_DECLARE(StarStar);

// ***************************** Object *********************************

MODULE_START(Object)
{
	Object_OBJ = OBJ(OBJECT);
	MODULE_SET_DOC(Object,  "prototype base for all objects");
	set_obj_id(Object_OBJ, *, Object);

	set_obj_doc(OBJ(SYMBOL_PROTO),  "Symbol object prototype");
	set_obj_doc(OBJ(HASH_PROTO),    "Hash object prototype");
	set_obj_doc(OBJ(LOCAL_PROTO),   "Local object prototype");
	set_obj_doc(OBJ(OUTER_PROTO),   "Outer object prototype");
	set_obj_doc(OBJ(SUPER_PROTO),    "Super object prototype");
	set_obj_doc(OBJ(ROOT_GLOBALS),  "Root_Globals: recursive container of all objects");
	set_obj_doc(OBJ(MODULES),       "Modules: container of all modules");

	set_attr(ist, OBJ(OBJECT), sym(ist, "Root_Globals"),    OBJ(ROOT_GLOBALS));
	set_attr(ist, OBJ(OBJECT), sym(ist, "Modules"),         OBJ(MODULES));
	set_attr(ist, OBJ(OBJECT), sym(ist, "Symbols"),         OBJ(SYMBOLS));
	set_attr(ist, OBJ(OBJECT), sym(ist, "Object"),          OBJ(OBJECT));
}

DEF(Object, call_, FORM_STAR3_PARAM) {
	obj_p new_obj;
	parms[0] = OBJ(PARAM_STAR);  parms[2] = OBJ(PARAM_STAR_STAR);
	new_obj = new_object(ist, self);
	call_func(ist, new_obj, SYM(INIT_), NULL, 4, parms, NULL);
	return new_obj;
}

DEF(Object, init_, FORM_STAR3_PARAM) {
	return OBJ(NONE);
}

DEF(Object, final_, NULL) {
	return OBJ(NONE);
}

DEF(Object, objId_, NULL) {
	return NEW_INT((intptr_t) self);
}

DEF(Object, isMain_QUES, NULL) {
	dump(ist, "self.txt", self);
	if (self == get_attr(ist, OBJ(MODULES), sym(ist, "Main")))
		return OBJ(PR_TRUE);
	else
		return OBJ(PR_FALSE);
}

DEF(Object, debug_, NULL) {
	return debug_object(self);
}

DEF(Object, newObject, FORM_STAR_PARAM) {
	int i, llen = (int) list_len(ist, parms[1]);
	obj_p new_obj;
	if (!llen) {
		raise_exception(ist, OBJ(INTERNAL_EXC), "proto() must have one param");
		return NULL;
	}
	new_obj = new_object(ist, list_item(ist, parms[1], 0));
	for(i=1; i < llen; i++)
		add_proto(ist, new_obj, list_item(ist, parms[1], i));
	return new_obj;
}

DEF(Object, copy, NULL) {
	obj_p copy;
	if (self->unclonable) {
		raise_exception(ist, OBJ(TYPE_EXC), "copy() not supported on this object");
		return NULL;
	}
	copy = copy_object(ist, self);
	if (self->data_type == DATA_TYPE_DATAPTR) {
		data_size_t data_size = DATA_SIZE(self);
		copy->data.ptr = pr_malloc(data_size);
		memcpy(copy->data.ptr, self->data.ptr, data_size);
	}
	return copy;
}

DEF(Object, hasProto_QUES, FORM_RPARAM) {
	if (has_proto(ist, self, parms[1]))
		return OBJ(PR_TRUE);
	else
		return OBJ(PR_FALSE); 
}

DEF(Object, setProto, FORM_RPARAM) {
	int m;
	read_unlock(ist, self);
	if((m = is_immutable(self)))
		clr_immutable(self);
	switch_proto(ist, self, parms[1]);
	if (m) set_immutable(self);
	read_lock(ist, self);
	return NULL;
}

DEF(Object, addProto, FORM_RPARAM) {
	int m;
	read_unlock(ist, self);
	if((m = is_immutable(self)))
		clr_immutable(self);
	add_proto(ist, self, parms[1]);
	if (m) set_immutable(self);
	read_lock(ist, self);
	return NULL;
}

DEF(Object, len, FORM_RPARAM) {
	return call_func0(ist, parms[1], SYM(LEN_));
}

DEF(Object, cmp, FORM_PARAM2) {
	return call_func1(ist, parms[1], SYM(CMP_), parms[3]);
}

DEF(Object, Max, FORM_RPARAM) {
	return call_func0(ist, parms[1], SYM(MAX));
}

DEF(Object, Min, FORM_RPARAM) {
	return call_func0(ist, parms[1], SYM(MIN));
}

DEF(Object, ord, FORM_RPARAM) {
	return call_func0(ist, parms[1], SYM(ORD_));
}

DEF(Object, chr, FORM_RPARAM) {
	return call_func0(ist, parms[1], SYM(CHR_));
}

DEF(Object, setReadAccess, FORM_RPARAM) {
	CHECK_TYPE_EXC(parms[1], OBJ(INT_PROTO), "integer");
	if (ist->access < get_obj_wracc(self)) {
		raise_exception(ist, OBJ(PERMISSION_EXC), "write access permission error");	
		return NULL;
	}
	set_obj_rdacc(self, (int) parms[1]->data.i64);
	return parms[1];
}

DEF(Object, readAccess, NULL) {
	return NEW_INT(get_obj_rdacc(self));
}

DEF(Object, readAccessStr, NULL) {
	switch (get_obj_rdacc(self)) {
		case 0:	return NEW_STRING("guest");
		case 1: return NEW_STRING("user1");
		case 2: return NEW_STRING("user2");
		case 3: return NEW_STRING("system");
		default:return NEW_STRING("<invalid access value>");
	}
}

DEF(Object, setWriteAccess, FORM_RPARAM)  {
	CHECK_TYPE_EXC(parms[1], OBJ(INT_PROTO), "integer");
	if (ist->access < get_obj_wracc(self)) {
		raise_exception(ist, OBJ(PERMISSION_EXC), "write access permission error");	
		return NULL;
	}
	set_obj_wracc(self, (int) parms[1]->data.i64);
	return parms[1];
}

DEF(Object, writeAccess, NULL) {
	return NEW_INT(get_obj_wracc(self));
}

DEF(Object, writeAccessStr, NULL) {
	switch (get_obj_wracc(self)) {
		case 0:	return NEW_STRING("guest");
		case 1: return NEW_STRING("user1");
		case 2: return NEW_STRING("user2");
		case 3: return NEW_STRING("system");
		default:return NEW_STRING("<invalid access value>");
	}
}

DEF(Object, getItem_, FORM_RPARAM) {
	raise_exception(ist, OBJ(TYPE_EXC), "indexing not supported on this object");
	return NULL;
}
DEF(Object, setItem_, FORM_PARAM2) {
	raise_exception(ist, OBJ(TYPE_EXC), "indexing not supported on this object");
	return NULL;
}
DEF(Object, delItem_, FORM_RPARAM) {
	raise_exception(ist, OBJ(TYPE_EXC), "indexing not supported on this object");
	return NULL;
}

DEF(Object, protoChain, NULL) {
	return proto_list(ist, self);
}

DEF(Object, bool__QUES, NULL) { return OBJ(PR_TRUE); }
DEF(Object, not__QUES, NULL)
{
	if (call_func0(ist, self, SYM(BOOL__QUES)) == OBJ(PR_TRUE))
		return OBJ(PR_FALSE);
	else
		return OBJ(PR_TRUE);
}

DEF(Object, is__QUES, FORM_RPARAM) { 
	if (self == parms[1]) return OBJ(PR_TRUE);
	else				  return OBJ(PR_FALSE); 
}

DEF(Object, isNot__QUES, FORM_RPARAM) { 
	if (self != parms[1]) return OBJ(PR_TRUE);
	else				  return OBJ(PR_FALSE); 
}

DEF(Object, str_, NULL) {
	char str[1024];
	obj_p doc = get_attr(ist, self, SYM(DOC_));
	char* p = pr2c_strptr(ist, doc);
	if (doc)
		apr_snprintf(str, sizeof(str), "<Object:%lx:%s>", (unsigned long)(uintptr_t)self, p);
	else
		apr_snprintf(str, sizeof(str), "<Object:%lx>",    (unsigned long)(uintptr_t)self);
	return NEW_STRING(str);
}

DEF(Object, hash_, NULL) {
	return new_hash_obj(ist, (i32_t)(uintptr_t) self);
}

DEF(Object, eq__QUES, FORM_RPARAM) {
	obj_p cmp_obj = call_func1(ist, self, SYM(CMP_), parms[1]); 
	if(catch_exception(ist, OBJ(FUNCNOTFOUND_EXC), NULL))
		return OBJ(PR_FALSE);
	if (cmp_obj && *((i64_t*)(obj_dataptr(cmp_obj))) == 0)
		return OBJ(PR_TRUE);
	else
		return OBJ(PR_FALSE);
}

DEF(Object, ne__QUES, FORM_RPARAM) {
	obj_p cmp_obj = call_func1(ist, self, SYM(EQ__QUES), parms[1]); if_exc_return NULL;
	if (cmp_obj == OBJ(PR_TRUE))
		return OBJ(PR_FALSE);
	else
		return OBJ(PR_TRUE);
}

DEF(Object, lt__QUES, FORM_RPARAM) {
	obj_p cmp_obj = call_func1(ist, self, SYM(CMP_), parms[1]); if_exc_return NULL;
	if (cmp_obj && *((i64_t*)(obj_dataptr(cmp_obj))) < 0)	return OBJ(PR_TRUE);
	else											        return OBJ(PR_FALSE);
}

DEF(Object, le__QUES, FORM_RPARAM) {
	obj_p cmp_obj = call_func1(ist, self, SYM(CMP_), parms[1]); if_exc_return NULL;
	if (cmp_obj && *((i64_t*)(obj_dataptr(cmp_obj))) <= 0)	return OBJ(PR_TRUE);
	else											        return OBJ(PR_FALSE);
}

DEF(Object, gt__QUES, FORM_RPARAM) {
	obj_p cmp_obj = call_func1(ist, self, SYM(CMP_), parms[1]); if_exc_return NULL;
	if (cmp_obj && *((i64_t*)(obj_dataptr(cmp_obj))) > 0)	return OBJ(PR_TRUE);
	else											        return OBJ(PR_FALSE);
}

DEF(Object, ge__QUES, FORM_RPARAM) {
	obj_p cmp_obj = call_func1(ist, self, SYM(CMP_), parms[1]); if_exc_return NULL;
	if (cmp_obj && *((i64_t*)(obj_dataptr(cmp_obj))) >= 0)	return OBJ(PR_TRUE);
	else											        return OBJ(PR_FALSE);
}

DEF(Object, in__QUES, FORM_RPARAM) {
	return call_func1(ist, parms[1], SYM(RIN__QUES), self);
}

DEF(Object, notIn__QUES, FORM_RPARAM) {
	return call_func1(ist, parms[1], SYM(RNOTIN__QUES), self);
}

DEF(Object, currentThread, NULL) {
	return os_thread_2_obj(apr_os_thread_current());
}

DEF(Object, Sleep, FORM_RPARAM) {
	obj_p thd;
	CHECK_TYPE_EXC(parms[1], OBJ(FLOAT_PROTO), "float");
	thd = os_thread_2_obj(apr_os_thread_current());
	call_func1(ist, thd, sym(ist, "sleep"), NEW_FLOAT(parms[1]->data.f64));
	return parms[1];
}

DEF(Object, toStorProxy_, NULL) {
	return copy_object(ist, self);
}

DEF(Object, fromStorProxy_, NULL) {
	return self;
}

DEF(Object, print, FORM_STAR3_PARAM) {
	obj_p arg_list = parms[1];
	obj_p arg_dict = parms[3];
	int i, llen = (int) list_len(ist, arg_list);
	char *sep, *eol, *str;
	obj_p sep_obj, eol_obj;
	sep_obj = dict_item(ist, arg_dict, sym(ist, "sep")); if_exc_return NULL;
	if (sep_obj) {
		CHECK_TYPE_EXC(sep_obj, OBJ(STRING_PROTO), String);
		sep = pr2c_strptr(ist, sep_obj); 
	} else 
		sep = " ";
	eol_obj = dict_item(ist, arg_dict, sym(ist, "eol")); if_exc_return NULL;
	if (eol_obj) {
		CHECK_TYPE_EXC(eol_obj, OBJ(STRING_PROTO), String);
		eol = pr2c_strptr(ist, eol_obj); 
	} else 
		eol = "\n";
	str = pr_malloc(strlen(eol)+1);
	str[0] = 0;
	for (i=0; i < llen; i++)  {
		obj_p str_obj, item = list_item(ist, arg_list, i);
		char* item_str;
		if (!item) item = OBJ(NONE);
		str_obj = call_func0(ist, item, SYM(STR_));
		if_exc_return NULL;
		item_str = pr2c_strptr(ist, str_obj);
		str = pr_realloc(str, strlen(str)+strlen(item_str)+strlen(sep)+strlen(eol)+1);
		strcat(str, item_str);
		if (i < llen-1) strcat(str, sep);
		else            strcat(str, eol);
	}
	if (llen == 0) strcpy(str, eol);
	printf("%s", str);
	//if (ist->in_console) {
		pr_free(str);
		return OBJ(NONE);
	//} else {
	//	res = NEW_STRING(str);
	//	pr_free(str);
	//	return res;
	//}
}


// ***************************** SYMBOL *****************************************

MODULE_START(Symbol)
{
	Symbol_OBJ = OBJ(SYMBOL_PROTO);
	MODULE_SET_DOC(Symbol, "Symbol prototype");
	set_attr(ist, OBJ(OBJECT), sym(ist, "Symbol"), Symbol_OBJ);
	set_obj_id(Symbol_OBJ, *, Symbol);
}

DEF(Symbol, call_,  FORM_RPARAM) {
	char* str;
	STRING_PARAM(1, str);
	return sym(ist, str); 
}

DEF(Symbol, str_,  NULL) { 
	obj_p str_obj = get_attr(ist, self, SYM(STRING_));
	char buf[512];
	BIN_STR_CHK(Symbol);
	apr_snprintf(buf, sizeof(buf), "$%s", pr2c_strptr(ist, str_obj));
	return NEW_STRING(buf); 
}

// ***************************** FALSE *****************************************

MODULE_START(False)
{
	False_OBJ = OBJ(PR_FALSE);
	MODULE_SET_DOC(False, "boolean false identity value");
	set_attr(ist, OBJ(OBJECT), sym(ist, "False"), False_OBJ);
	set_obj_id(False_OBJ, *, False);
}

DEF(False, str_,  NULL) { return NEW_STRING("False"); }
DEF(False, bool__QUES, NULL) { return False_OBJ; }
DEF(False, cmp_, FORM_RPARAM) { 
	if (parms[1] == False_OBJ)
		return NEW_INT(0);
	else
		return NEW_INT(-1); 
}

// ***************************** PR_TRUE *****************************************

MODULE_START(True)
{
	True_OBJ = OBJ(PR_TRUE);
	MODULE_SET_DOC(True, "boolean true identity value");
	set_attr(ist, OBJ(OBJECT), sym(ist, "True"), True_OBJ);
	set_obj_id(True_OBJ, *, True);
}

DEF(True, str_,  NULL)  { return NEW_STRING("True"); }
DEF(True, bool__QUES, NULL)  { return True_OBJ; }
DEF(True, cmp_, FORM_RPARAM) { 
	if (parms[1] == OBJ(PR_FALSE))
		return NEW_INT(1); 
	else if (parms[1] == True_OBJ)
		return NEW_INT(0); 
	else
		return NEW_INT(-1); 
}
DEF(True, getCmp_, NULL) { 
	return get_attr(ist, self, SYM(CMPVALOBJ_)); 
}

// ***************************** NONE *****************************************

MODULE_START(None)
{
	None_OBJ = OBJ(NONE);
	MODULE_SET_DOC(None, "represents empty set");
	set_attr(ist, OBJ(OBJECT), sym(ist, "None"), None_OBJ);
	set_obj_id(None_OBJ, *, None);
}

DEF(None, str_,  NULL) {				
	return NEW_STRING("None"); 
}

DEF(None, bool__QUES, NULL) {			
	return OBJ(PR_FALSE); 
}

DEF(None, eq__QUES, FORM_RPARAM) {	
	if (parms[1] == OBJ(NONE)) return OBJ(PR_TRUE); 
	else                       return OBJ(PR_FALSE); 
}

// ***************************** EXCEPTION ************************************
MODULE_START(Exception)
{
	Exception_OBJ = OBJ(EXCEPTION);
	MODULE_SET_DOC(Exception, "prototype of all exception objects");
	set_attr(ist, OBJ(OBJECT), sym(ist, "Exception"), Exception_OBJ);

#define EXCEPTION_DECLARE(proto, exc_const, name, desc_str)		\
	OBJ(exc_const) = NEW_OBJ(proto);					\
	set_obj_id(OBJ(exc_const), *, name);						\
	set_obj_doc(OBJ(exc_const), desc_str);						\
	set_attr(ist, OBJ(OBJECT), sym(ist, #name), OBJ(exc_const))

/* Other exception objects dependent on this one */
EXCEPTION_DECLARE(Exception_OBJ, INTERNAL_EXC,        InternalError,     "Internal Prothon Error");
EXCEPTION_DECLARE(Exception_OBJ, PARSEERROR_EXC,      ParseError,        "Parse Error");
EXCEPTION_DECLARE(Exception_OBJ, INTERPRETER_EXC,     ProgramError,      "Program Error");
EXCEPTION_DECLARE(Exception_OBJ, ASSERTION_EXC,       AssertionError,    "Assertion Error");
EXCEPTION_DECLARE(Exception_OBJ, NAME_EXC,            NameError,         "Name Error");
EXCEPTION_DECLARE(Exception_OBJ, INDEX_EXC,           IndexError,        "Index Error");
EXCEPTION_DECLARE(Exception_OBJ, FUNCNOTFOUND_EXC,    FunctionNotFound,  "Function not found Error");
EXCEPTION_DECLARE(Exception_OBJ, TYPE_EXC,            TypeError,         "Type Error");
EXCEPTION_DECLARE(Exception_OBJ, VALUE_EXC,           ValueError,        "Value Error");
EXCEPTION_DECLARE(Exception_OBJ, MUTABLE_EXC,         MutableError,      "Mutable Error");
EXCEPTION_DECLARE(Exception_OBJ, DIVIDEZERO_EXC,      DivideByZero,      "Divide by zero Error");
EXCEPTION_DECLARE(Exception_OBJ, OVERFLOW_EXC,        OverflowError,     "Overflow Error");
EXCEPTION_DECLARE(Exception_OBJ, OUTOFMEMORY_EXC,     OutOfMemoryError,  "Out of memory Error");
EXCEPTION_DECLARE(Exception_OBJ, IO_EXC,         IOError,           "IO Error");
EXCEPTION_DECLARE(OBJ(IO_EXC), FILENOTFOUND_EXC, FileNotFound,      "File not found Error");
EXCEPTION_DECLARE(Exception_OBJ, STOP_ITERATION_EXC,  StopIteration,     "Stop iteration (not an error)");
EXCEPTION_DECLARE(Exception_OBJ, LOCK_EXC,            LockError,         "Locking Error");
EXCEPTION_DECLARE(Exception_OBJ, PERMISSION_EXC,      PermissionError,   "Permission Error");
EXCEPTION_DECLARE(Exception_OBJ, COMMAND_EXC,         CommandError,      "Invalid Command");
}

DEF(Exception, init_,  FORM_STAR_PARAM) {
	char buf[512];
	if (list_len(ist, parms[1]) > 0) {
		apr_snprintf(buf, sizeof(buf), "%s", as_str(ist, list_item(ist, parms[1], 0)));
		set_obj_doc(self, buf);
	}
	return OBJ(NONE);
}

DEF(Exception, str_,  NULL) {
	obj_p doc;
	if (!(doc = get_attr(ist, self, SYM(DOC_))))
		doc = proto_item(ist, self, 0);
	return call_func0(ist, doc, SYM(STR_));
}

// ***************************** GEN ******************************************
MODULE_START(Gen)
{
	Gen_OBJ = OBJ(GEN_PROTO);
	MODULE_SET_DOC(Gen, "generator object prototype");
	set_attr(ist, OBJ(OBJECT), sym(ist, "Generator"), Gen_OBJ);
	set_obj_id(Gen_OBJ, *, Generator);
}

DEF(Gen, iter_, NULL) {
	return self; 
}

// ***************************** LOCAL ******************************************
MODULE_START(Local)
{
	Local_OBJ = OBJ(LOCAL_PROTO);
	MODULE_SET_DOC(Local, "local object prototype");
	set_attr(ist, OBJ(OBJECT), sym(ist, "Local"), Local_OBJ);
	set_obj_id(Local_OBJ, *, Local);
}

DEF(Local, str_, NULL) {
	BIN_STR_CHK(Local);
	return call_func0(ist, self->data.ptr, SYM(STR_));
}

DEF(Local, objList_, FORM_RPARAM) {
	return parms[1];
}

// ***************************** SUPER ******************************************
MODULE_START(Super)
{
	Super_OBJ = OBJ(SUPER_PROTO);
	MODULE_SET_DOC(Super, "super object prototype");
	set_attr(ist, OBJ(OBJECT), sym(ist, "Super"), Super_OBJ);
	set_obj_id(Super_OBJ, *, Super);
}

DEF(Super, str_, NULL) {
	BIN_STR_CHK(Super);
	return call_func0(ist, self->data.ptr, SYM(STR_));
}

DEF(Super, objList_, FORM_RPARAM) {
	return parms[1];
}

// ***************************** SEQUENCE *************************************

MODULE_START(Seq)
{
	Seq_OBJ = OBJ(SEQ_PROTO);
	MODULE_SET_DOC(Seq, "object prototype");
	set_attr(ist, OBJ(OBJECT), sym(ist, "Sequence"), Seq_OBJ);
	set_obj_id(Seq_OBJ, *, Sequence);
}

obj_p str_tuple_list(isp ist, obj_p self, char* ldelim, char* rdelim)
{
	int i, len = (int) list_len(ist, self);
	char msg[64], *res;
	obj_p ret_obj;

	/* Need enough room for just empty list */
	res = pr_malloc(strlen(ldelim) + strlen(rdelim) + 1);

	strcpy(res, ldelim);

	for (i = 0; i < len; i++) {
		char* item_str;
		obj_p item = list_item(ist, self,i);
		int is_str;

		if (item) {
			if ((uintptr_t) item > 10) {
				is_str = has_proto(ist, item, OBJ(STRING_PROTO));
				item_str = as_str(ist, item);
			} else {
				apr_snprintf(msg, sizeof(msg), "<objptr:%lx>", (unsigned long)(uintptr_t)item);
				item_str = msg;
				is_str = FALSE;
			}
		} else {
			item_str = "<NULL>";
			is_str = FALSE;
		}
		res = pr_realloc(res, strlen(res) + strlen(item_str) + strlen(rdelim) + 6);
		if (is_str)
			strcat(res, "'");
		strcat(res, item_str);
		if (is_str)
			strcat(res,"'");

		if (i != len-1)
			strcat(res, ", ");
	}
	strcat(res, rdelim);

	ret_obj = NEW_STRING(res);
	pr_free(res);

	return ret_obj;
}

obj_p get_sequence_item(isp ist, obj_p self, obj_p slice, int seq_type) {
	obj_p res = NULL, slice_item1, slice_item2, slice_item3;
	int i, index1 = 0, index2 = 0, index3 = 0, slice1_empty=FALSE, slice2_empty=FALSE;
	int exp_len, self_len, slice_len;
	ch3_p  self_str   = NULL;
	u8_t* self_bytes = NULL;

	CHECK_TYPE_EXC(self,    OBJ(SEQ_PROTO), sequence);

	if (seq_type == SEQ_TYPE_BYTES) {
		self_bytes = bytes_ptr(self);
		self_len   = (int) bytes_len(self);
	} else if (seq_type == SEQ_TYPE_STRING) {
		self_str = pr_ch3ptr(self);
		self_len = (int) pr_strlen(self);
	} else
		self_len = (int) list_len(ist, self);
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
		CHECK_TYPE_EXC(slice_item1, OBJ(INT_PROTO), "integer");
			
		index1 = int2i32t(ist, slice_item1);
		if (index1 < 0) index1 += self_len;						
		if (index1 < 0 || index1 >= self_len) {			
			raise_exception(ist, OBJ(INDEX_EXC), "Index (%d) out of range", index1);
			return NULL;									
		}
		if (slice_len == 1) {
			if (seq_type == SEQ_TYPE_BYTES) {
				return NEW_INT(*(self_bytes+index1));
			} else if (seq_type == SEQ_TYPE_STRING) {
				return NEW_STRING_CH3(self_str+index1, 1);
			} else
				return list_item(ist, self, index1);
		}
	}
	if (slice_len == 3) {
		slice_item3 = list_item(ist, slice, 2);
		if (slice_item3 == SLICEPARAM_EMPTY || slice_item3 == OBJ(NONE)) index3 = 1;
		else {
			CHECK_TYPE_EXC(slice_item3, OBJ(INT_PROTO), "integer");

			index3 = int2i32t(ist, slice_item3);
			if (index3 == 0) {
				raise_exception(ist, OBJ(INDEX_EXC), "Slice step cannot be zero");		
				return NULL;
			}
		}
	} else index3 = 1;
	slice_item2 = list_item(ist, slice, 1);
	if (slice_item2 == SLICEPARAM_EMPTY || slice_item2 == OBJ(NONE))
		slice2_empty = TRUE;
	else {
		CHECK_TYPE_EXC(slice_item2, OBJ(INT_PROTO), "integer");

		index2 = int2i32t(ist, slice_item2);
		if (index2 < 0) index2 += self_len;						
		if (index2 < 0 || index2 > self_len) {			
			raise_exception(ist, OBJ(INDEX_EXC), "Second index (%d) out of range", index2);
			return NULL;									
		}
	}
	if (index3 > 0) {
		if (slice1_empty) index1 = 0;
		if (slice2_empty) index2 = self_len;
		exp_len = (index2-index1+(index3-1))/index3;
		exp_len = max(exp_len, 0);
		switch(seq_type) {
		case SEQ_TYPE_BYTES: {
			int j;
			u8_t* s = pr_malloc(exp_len);
			for(i = index1, j=0; i < index2; i += index3, j++)
				s[j] = self_bytes[i];
			res = NEW_BYTES(s, j);
			pr_free(s);
		}   break;
		case SEQ_TYPE_STRING: {
			int j;
			ch3_p s = pr_malloc(exp_len*3+3);
			for(i = index1, j=0; i < index2; i += index3, j++)
				s[j] = self_str[i];
			res = NEW_STRING_CH3(s, j);
			pr_free(s);
		}   break;
		case SEQ_TYPE_TUPLE:
			res = NEW_TUPLE(exp_len);
			for(i = index1; i < index2; i += index3)
				list_append(ist, res, list_item(ist, self, i));
			break;
		case SEQ_TYPE_LIST:
			res = NEW_LIST(exp_len);
			for(i = index1; i < index2; i += index3)
				list_append(ist, res, list_item(ist, self, i));
			break;
		}
	} else {
		if (slice1_empty) index1 = self_len-1;
		if (slice2_empty) index2 = -1;
		exp_len = (index2-index1+(index3-1))/index3;
		exp_len = max(exp_len, 0)+1;
		switch(seq_type) {
		case SEQ_TYPE_BYTES: {
			int j;
			u8_t* s = pr_malloc(exp_len);
			for(i = index1, j=0; i > index2; i += index3, j++)
				s[j] = self_bytes[i];
			res = NEW_BYTES(s, j);
			pr_free(s);
		}   break;
		case SEQ_TYPE_STRING: {
			int j;
			ch3_p s = pr_malloc(exp_len*3+3);
			for(i = index1, j=0; i > index2; i += index3, j++)
				s[j] = self_str[i];
			res = NEW_STRING_CH3(s, j);
			pr_free(s);
		}   break;
		case SEQ_TYPE_TUPLE:
			res = NEW_TUPLE(exp_len);
			for(i = index1; i > index2; i += index3)
				list_append(ist, res, list_item(ist, self, i));
			break;
		case SEQ_TYPE_LIST:
			res = NEW_LIST(exp_len);
			for(i = index1; i > index2; i += index3)
				list_append(ist, res, list_item(ist, self, i));
			break;
		}
	}
	return res;
}


// ***************************** FUNC ******************************************

MODULE_START(Func)
{
	Func_OBJ = OBJ(FUNC_PROTO);
	MODULE_ADD_TO_OBJ(Func, OBJ(OBJECT), "Func");
	MODULE_SET_DOC(Func, "function object prototype");
	set_obj_id(Func_OBJ, *, Function);

	Method_OBJ = OBJ(METHOD_PROTO);
	MODULE_ADD_TO_OBJ(Method, OBJ(OBJECT), "Method");
	MODULE_SET_DOC(Method, "method object prototype");
	set_obj_id(Method_OBJ, *, Method);
}

DEF(Func, init_, FORM_RPARAM) {
	obj_p other, fparms;
	code_p optr;
	int size;
	
	BIN_EMPTY_CHK();
	FUNC___PARAM(1, other);
	optr = other->data.ptr;
	if (!optr) {
		raise_exception(ist, OBJ(INTERNAL_EXC), "Func init_ param not initialized");
		return NULL;
	}
	self->data_type = other->data_type;
	if (self->data_type == DATA_TYPE_DATAPTR) {
		size = DATA_SIZE(other);
		self->data.ptr = pr_malloc(size);
		memcpy(self->data.ptr, other->data.ptr, size);
	} else 
		self->data.ptr = other->data.ptr;
	read_unlock(ist, self);
	if ((fparms = get_attr(ist,  other, SYM(FPARAMS_))))
		 set_attr(ist, self, SYM(FPARAMS_), copy_list_obj(ist, fparms));
	read_lock(ist, self);
	return OBJ(NONE);
}

DEF(Func, str_, NULL) {
	obj_p bobj;
	char str[128];

	BIN_STR_CHK(Func);
	apr_snprintf(str, sizeof(str), "<func:%"APR_UINT64_T_HEX_FMT, (apr_uint64_t)(intptr_t) self);
	if (self->data_type == DATA_TYPE_EXTPTR)
		strcat(str, ":c");
	if ((bobj = get_attr(ist, self, SYM(BINDOBJ_)))) {
		strcat(str, ":bound:");
		strcat(str, as_str(ist, bobj));
	}
	strcat(str, ">");
	return new_string_obj(ist, str);
}

DEF(Func, unbind, NULL) {
	BIN_CONTENT_CHK(Func);
	read_unlock(ist, self);
	del_attr(ist, self, SYM(BINDOBJ_));
	read_lock(ist, self);
	return self;
}

DEF(Func, objList_, FORM_RPARAM) {
	return parms[1];
}
// ***************************** RANGE ****************************************

typedef struct {
	data_size_t	size;
	i64_t		value;
	i64_t		stop;
	i64_t		step;
} range_t;

typedef	range_t*	range_p;

MODULE_START(RangeGen)
{
	RangeGen_OBJ = NEW_OBJ(OBJ(ITER_PROTO));
	MODULE_ADD_TO_OBJ(RangeGen, OBJ(OBJECT), "Range");
	MODULE_SET_DOC(RangeGen, "range generator object prototype");
	set_obj_id(RangeGen_OBJ, *, RangeGen);
}

DEF(Object, range, FPARM3(start, NEW_INT(0), stop, OBJ(NONE), step, OBJ(NONE))) {
	obj_p range_obj;
	range_p rangep;
	i64_t start, stop, step=1;

	INT_64_PARAM(1, start);
	if (parms[3] != OBJ(NONE)) INT_64_PARAM(2, stop);
	else { stop = start; start = 0; }
	if (parms[5] != OBJ(NONE)) INT_64_PARAM(3, step);
	if (!step) {
		raise_exception(ist, OBJ(VALUE_EXC), "range increment must be non-zero");
		return NULL;
	}
	range_obj = NEW_OBJ(RangeGen_OBJ);
	SET_TYPE_IF_EXC(RangeGen_OBJ, range_obj, DATA_TYPE_DATAPTR) return NULL;
	range_obj->data.ptr = rangep = (range_p) pr_malloc(sizeof(range_t));
	rangep->size  = sizeof(range_t);
	rangep->value = start;
	rangep->stop  = stop;
	rangep->step  = step;
	return range_obj;
}

DEF(RangeGen, next, NULL) {
	obj_p res;
	range_p rangep;
	BIN_CONTENT_CHK(RangeGen);
	rangep = self->data.ptr;
	if ( (rangep->step > 0 && rangep->value >= rangep->stop) ||
		 (rangep->step < 0 && rangep->value <= rangep->stop) ) {
		raise_exception(ist, OBJ(STOP_ITERATION_EXC), NULL);
		return NULL;
	}
	res = NEW_INT(rangep->value);
	rangep->value += rangep->step;
	return res;
}

DEF(RangeGen, iter_, NULL) {
	return self;
}

// ***************************** Slice *********************************

obj_p new_slice_obj(isp ist, obj_p ind1, obj_p ind2, obj_p ind3) {
	obj_p slice_obj = NEW_LIST(3);
	pr_assert(!(slice_obj->has_attrs));
	slice_obj->attr_proto.proto = OBJ(SLICE_PROTO);
	if (ind1 == SLICEPARAM_EMPTY)
		list_append_no_lock(slice_obj, OBJ(NONE));
	else
		list_append_no_lock(slice_obj, ind1);
	if (ind2 != SLICEPARAM_MISSING) {
		if (ind2 == SLICEPARAM_EMPTY)
			list_append_no_lock(slice_obj, OBJ(NONE));
		else
			list_append_no_lock(slice_obj, ind2);
		if (ind3 != SLICEPARAM_MISSING) {
			if (ind3 == SLICEPARAM_EMPTY)
				list_append_no_lock(slice_obj, OBJ(NONE));
			else
				list_append_no_lock(slice_obj,ind3);
		}
	}
	slice_obj->immutable = TRUE;
	return slice_obj;
}

MODULE_START(Slice)
{
	Slice_OBJ = OBJ(SLICE_PROTO);
	MODULE_SET_DOC(Slice,  "Slice prototype object");
	MODULE_ADD_TO_OBJ(Slice, OBJ(OBJECT), "Slice");
	set_obj_id(Slice_OBJ, *, Object);
	set_obj_doc(Slice_OBJ, "Slice object prototype");
}

DEF(Slice, call_, FPARM3(idx1, NEW_INT(0), idx2, SLICEPARAM_MISSING, idx3, SLICEPARAM_MISSING)) { 
	return new_slice_obj(ist, parms[1], parms[3], parms[5]);
}

DEF(Slice, str_, NULL) {
	char buf[300];
	int llen;

	BIN_STR_CHK(Slice);
	llen = (int) list_len(ist, self);
	if (llen == 1)
		apr_snprintf(buf, sizeof(buf), "<Slice:%s>",       as_str(ist, list_item(ist, self, 0)));
	else if (llen == 2)
		apr_snprintf(buf, sizeof(buf), "<Slice:%s:%s>",    as_str(ist, list_item(ist, self, 0)), 
			                                               as_str(ist, list_item(ist, self, 1)));
	else if (llen == 3)
		apr_snprintf(buf, sizeof(buf), "<Slice:%s:%s:%s>", as_str(ist, list_item(ist, self, 0)),
														   as_str(ist, list_item(ist, self, 1)), 
														   as_str(ist, list_item(ist, self, 2)));
	else
		strcpy(buf, "<Slice:invalid>");
	return NEW_STRING(buf);
}

// ***************************** WeakRef **************************************

MODULE_CONSTANT_DECLARE(WeakRef, READ ); 
MODULE_CONSTANT_DECLARE(WeakRef, WRITE); 
MODULE_CONSTANT_DECLARE(WeakRef, DELETE ); 

typedef struct {
	data_size_t	data_size;
	int			exists;
	int			notify_flags;
	obj_p		ref;
} weak_ref_t;

typedef weak_ref_t* weak_ref_p;

MODULE_START(WeakRef)
{
	WeakRef_OBJ  = NEW_OBJ(NULL);
	MODULE_ADD_TO_OBJ(WeakRef, OBJ(OBJECT), "WeakRef");

	MODULE_CONSTANT_INT(WeakRef, READ,   READ_NOTIFY);
	MODULE_CONSTANT_INT(WeakRef, WRITE,  WRITE_NOTIFY);
	MODULE_CONSTANT_INT(WeakRef, DELETE, DELETE_NOTIFY);
}

DEF(WeakRef, str_, NULL) { 
	weak_ref_p wrp;
	char *str_obj, buf[256];
	if (self == WeakRef_OBJ) return NEW_STRING("<Prototype:WeakRef>");
	BIN_STR_CHK(WeakRef);
	wrp = self->data.ptr;
	if (wrp->exists) {
		str_obj = as_str(ist, wrp->ref);
		apr_snprintf(buf, sizeof(buf), "<WeakRef:%s>", str_obj);
	} else
		apr_snprintf(buf, sizeof(buf), "<WeakRef:deleted:%lx>",
			     (unsigned long)(intptr_t) wrp->ref);
	set_unclonable(self);
	return NEW_STRING(buf);
}
DEF(WeakRef, init_, FORM_RPARAM) { 
	weak_ref_p wrp;
	obj_p callback_func;
	int i, immutable_flag;

	BIN_EMPTY_CHK();
	SET_TYPE_IF_EXC(WeakRef_OBJ, self, DATA_TYPE_DATAPTR) return NULL;
	wrp = self->data.ptr = pr_malloc(sizeof(weak_ref_t));
	wrp->data_size	  = sizeof(weak_ref_t);
	wrp->exists       = TRUE;
	wrp->notify_flags = DELETE_NOTIFY;
	wrp->ref          = parms[1];
	callback_func     = get_attr(ist, WeakRef_OBJ, sym(ist, "callback_"));  if_exc_return NULL;
	immutable_flag    = is_immutable(wrp->ref);
	clr_immutable(wrp->ref);
	su(i);
	set_attr(ist, callback_func, SYM(BINDOBJ_), self); if_exc_return NULL;
	un_su(i);
	read_unlock(ist, wrp->ref);
	add_notification(ist, wrp->ref, callback_func, DELETE_NOTIFY);
	read_lock(ist, wrp->ref);
	if(immutable_flag) set_immutable(wrp->ref);
	set_unclonable(self);
	return OBJ(NONE);
}

DEF(WeakRef, ref, NULL) {
	weak_ref_p wrp = self->data.ptr;
	BIN_CONTENT_CHK(WeakRef);
	if(!wrp->exists) {
		raise_exception(ist, OBJ(INTERPRETER_EXC), "object doesn't exist anymore");
		del_notify_unlock();
	}
	return wrp->ref;
}

DEF(WeakRef, exists_QUES, NULL) {
	weak_ref_p wrp = self->data.ptr;
	BIN_CONTENT_CHK(WeakRef);
	if (wrp->exists) return OBJ(PR_TRUE);
	else			 return OBJ(PR_FALSE);
}

DEF(WeakRef, flags, NULL) {
	weak_ref_p wrp = self->data.ptr;
	BIN_CONTENT_CHK(WeakRef);
	return NEW_INT(wrp->notify_flags);
}

DEF( WeakRef, notify, FPARM3( func,  OBJ(NODEF), 
							  param, OBJ(NONE), 
							  flags, NEW_INT(WRITE_NOTIFY|DELETE_NOTIFY) ) ) { 
	weak_ref_p wrp = self->data.ptr;
	obj_p callback_func;
	int flags, immutable_flag;
	BIN_CONTENT_CHK(WeakRef);
	CHECK_TYPE_EXC(parms[1], OBJ(FUNC_PROTO), function);
	INT_32_PARAM(3, flags);
	del_notify_lock();
	if (!wrp->exists) {
		raise_exception(ist, OBJ(INTERPRETER_EXC), "object doesn't exist anymore");
		del_notify_unlock();
		return NULL;									
	}
	immutable_flag = is_immutable(wrp->ref);
	clr_immutable(wrp->ref);
	read_unlock(ist, self);
	if (flags & READ_NOTIFY) {
		set_attr(ist, self, sym(ist, "onRead_"), parms[1]); if_exc_return NULL;
		if (parms[3] != OBJ(NONE))
			set_attr(ist, self, sym(ist, "onReadParam_"), parms[3]); if_exc_return NULL;
	} else {
		del_attr(ist, wrp->ref, sym(ist, "onRead_"));
		del_attr(ist, wrp->ref, sym(ist, "onReadParam_"));
	}
	if (flags & WRITE_NOTIFY) {
		set_attr(ist, self, sym(ist, "onWrite_"), parms[1]); if_exc_return NULL;
		if (parms[3] != OBJ(NONE))
			set_attr(ist, self, sym(ist, "onWriteParam_"), parms[3]); if_exc_return NULL;
	} else {
		del_attr(ist, wrp->ref, sym(ist, "onWrite_"));
		del_attr(ist, wrp->ref, sym(ist, "onWriteParam_"));
	}
	if (flags & DELETE_NOTIFY) {
		set_attr(ist, self, sym(ist, "onDelete_"), parms[1]); if_exc_return NULL;
		if (parms[3] != OBJ(NONE))
			set_attr(ist, self, sym(ist, "onDeleteParam_"), parms[3]); if_exc_return NULL;
	} else {
		del_attr(ist, wrp->ref, sym(ist, "onDelete_"));
		del_attr(ist, wrp->ref, sym(ist, "onDeleteParam_"));
	}
	read_lock(ist, self);
	flags |= DELETE_NOTIFY;
	if (wrp->notify_flags != flags) {
		wrp->notify_flags = flags;
		callback_func = get_attr(ist, WeakRef_OBJ, sym(ist, "callback_")); if_exc_return NULL;
		add_notification(ist, wrp->ref, callback_func, flags); if_exc_return NULL;
	}
	if(immutable_flag)
		set_immutable(wrp->ref);
	del_notify_unlock();
	return self;
}

DEF(WeakRef, callback_, FORM_RPARAM) {
	obj_p arr[4]; 
	int flags = int2i32t(ist, parms[1]);
	arr[0] = OBJ(NOKEY); arr[1] = parms[1]; arr[2] = OBJ(NOKEY);
	if (flags & READ_NOTIFY) {
		obj_p on_read_func  = get_attr(ist, self, sym(ist, "onRead_"));
		obj_p on_read_param = get_attr(ist, self, sym(ist, "onReadParam_"));
		if (on_read_func && on_read_param) {
			arr[3] = on_read_param;
			call_func(ist, NULL, on_read_func, NULL, 4, arr, NULL);
		} else if (on_read_func)
			call_func(ist, NULL, on_read_func, NULL, 2, arr, NULL);
	}
	if (flags & WRITE_NOTIFY) {
		obj_p on_write_func  = get_attr(ist, self, sym(ist, "onWrite_"));
		obj_p on_write_param = get_attr(ist, self, sym(ist, "onWriteParam_"));
		if (on_write_func && on_write_param) {
			arr[3] = on_write_param;
			call_func(ist, NULL, on_write_func, NULL, 4, arr, NULL);
		} else if (on_write_func)
			call_func(ist, NULL, on_write_func, NULL, 2, arr, NULL);
	}
	if (flags & DELETE_NOTIFY) {
		weak_ref_p wrp = self->data.ptr;
		obj_p on_delete_func  = get_attr(ist, self, sym(ist, "onDelete_"));
		obj_p on_delete_param = get_attr(ist, self, sym(ist, "onDeleteParam_"));
		wrp->exists = FALSE;
		if (on_delete_func && on_delete_param) {
			arr[3] = on_delete_param;
			call_func(ist, NULL, on_delete_func, NULL, 4, arr, NULL);
			check_exceptions(ist);
		} else if (on_delete_func) {
			call_func(ist, NULL, on_delete_func, NULL, 2, arr, NULL);
			check_exceptions(ist);
		}
	}
	return OBJ(NONE);
}
DEF(WeakRef, objList_, FORM_RPARAM) {
	return parms[1];
}

// ***************************** PROP MODULE ********************************
MODULE_START(Prop)
{
	Prop_OBJ = OBJ(PROP_PROTO);
	MODULE_SET_DOC(Prop,  "Prop prototype object");
	MODULE_ADD_TO_OBJ(Prop, OBJ(OBJECT), "Prop");
	set_obj_id(Prop_OBJ, *, Object);
	set_obj_doc(Prop_OBJ, "Prop object prototype");
}

DEF(Prop, str_, NULL) {
	return NEW_STRING("<Prop>");
}

// ***************************** UNIQUEOBJECTS ********************************
MODULE_START(UniqueObjects)
{
	UniqueObjects_OBJ = NEW_OBJ(NULL);
	NoKey_OBJ    = OBJ(NOKEY);	
	NoDef_OBJ    = OBJ(NODEF);				
	Star_OBJ	 = OBJ(PARAM_STAR);		
	StarStar_OBJ = OBJ(PARAM_STAR_STAR);

	MODULE_ADD_TO_OBJ(NoKey, OBJ(OBJECT), "NoKey_");
	set_obj_id(NoKey_OBJ, *, NoKey_);
	MODULE_ADD_TO_OBJ(NoDef, OBJ(OBJECT), "NoDefVal_");
	set_obj_id(NoDef_OBJ, *, NoDefVal_);
	MODULE_ADD_TO_OBJ(Star, OBJ(OBJECT), "Star_");
	set_obj_id(Star_OBJ, *, Star_);
	MODULE_ADD_TO_OBJ(StarStar, OBJ(OBJECT), "StarStar_");
	set_obj_id(StarStar_OBJ, *, StarStar_);
}

DEF(NoKey, str_, NULL) { return NEW_STRING("NoKey_"); }
DEF(NoDef, str_, NULL) { return NEW_STRING("NoDefVal_"); }
DEF(Star, str_, NULL) { return NEW_STRING("Star_"); }
DEF(StarStar, str_, NULL) { return NEW_STRING("StarStar_"); }

// ***************************** MAIN_MODULE_INIT *****************************

MAIN_MODULE_INIT(Core)
{
	MODULE_SUB_INIT(Object);
	MODULE_ADD_SYM(Object, call_);
	MODULE_ADD_SYM(Object, init_);
	MODULE_ADD_SYM(Object, final_);
	MODULE_ADD_SYM(Object, objId_);
	MODULE_ADD_SYM(Object, isMain_QUES);
	MODULE_ADD_SYM(Object, debug_);
	MODULE_ADD_SYM(Object, newObject);
	MODULE_ADD_SYM(Object, copy);
	MODULE_ADD_SYM(Object, setReadAccess);
	MODULE_ADD_SYM(Object, readAccess);
	MODULE_ADD_SYM(Object, readAccessStr);
	MODULE_ADD_SYM(Object, setWriteAccess);
	MODULE_ADD_SYM(Object, writeAccess);
	MODULE_ADD_SYM(Object, writeAccessStr);
	MODULE_ADD_SYM(Object, hasProto_QUES);
	MODULE_ADD_SYM(Object, setProto);
	MODULE_ADD_SYM(Object, addProto);
	MODULE_ADD_SYM(Object, protoChain);
	MODULE_ADD_SYM(Object, cmp);
	MODULE_ADD_SYM(Object, len);
	MODULE_ADD_SYM(Object, Max);
	MODULE_ADD_SYM(Object, Min);
	MODULE_ADD_SYM(Object, ord);
	MODULE_ADD_SYM(Object, chr);
	MODULE_ADD_SYM(Object, getItem_);
	MODULE_ADD_SYM(Object, setItem_);
	MODULE_ADD_SYM(Object, delItem_);
	MODULE_ADD_SYM(Object, bool__QUES);
	MODULE_ADD_SYM(Object, not__QUES);
	MODULE_ADD_SYM(Object, is__QUES);
	MODULE_ADD_SYM(Object, isNot__QUES);
	MODULE_ADD_SYM(Object, str_);
	MODULE_ADD_SYM(Object, hash_);
	MODULE_ADD_SYM(Object, eq__QUES);
	MODULE_ADD_SYM(Object, ne__QUES);
	MODULE_ADD_SYM(Object, lt__QUES);
	MODULE_ADD_SYM(Object, le__QUES);
	MODULE_ADD_SYM(Object, gt__QUES);
	MODULE_ADD_SYM(Object, ge__QUES);
	MODULE_ADD_SYM(Object, in__QUES);
	MODULE_ADD_SYM(Object, notIn__QUES);
	MODULE_ADD_SYM(Object, currentThread);
	MODULE_ADD_SYM(Object, Sleep);
	MODULE_ADD_SYM(Object, range);
	MODULE_ADD_SYM(Object, toStorProxy_);
	MODULE_ADD_SYM(Object, fromStorProxy_);
	MODULE_ADD_SYM(Object, print);  MARK_AS_COMMAND;


	MODULE_SUB_INIT(Symbol);
	MODULE_ADD_SYM(Symbol, call_);
	MODULE_ADD_SYM(Symbol, str_);

	MODULE_SUB_INIT(Seq);

	MODULE_SUB_INIT(False);
	MODULE_ADD_SYM(False, str_);
	MODULE_ADD_SYM(False, bool__QUES);
	MODULE_ADD_SYM(False, cmp_);

	MODULE_SUB_INIT(True);
	MODULE_ADD_SYM(True, str_);
	MODULE_ADD_SYM(True, bool__QUES);
	MODULE_ADD_SYM(True, cmp_);
	MODULE_ADD_SYM(True, getCmp_);

	MODULE_SUB_INIT(None);
	MODULE_ADD_SYM(None, str_);
	MODULE_ADD_SYM(None, bool__QUES);
	MODULE_ADD_SYM(None, eq__QUES);

	MODULE_SUB_INIT(Exception);
	MODULE_ADD_SYM(Exception, init_);
	MODULE_ADD_SYM(Exception, str_);

	MODULE_SUB_INIT(Gen);
	MODULE_ADD_SYM(Gen, iter_);

	MODULE_SUB_INIT(Local);
	MODULE_ADD_SYM(Local, str_);
	MODULE_ADD_SYM(Local, objList_);

	MODULE_SUB_INIT(Super);
	MODULE_ADD_SYM(Super, str_);
	MODULE_ADD_SYM(Super, objList_);

	MODULE_SUB_INIT(Func);
	MODULE_ADD_SYM(Func, init_);
	MODULE_ADD_SYM(Func, str_);
	MODULE_ADD_SYM(Func, unbind);
	MODULE_ADD_SYM(Func, objList_);

	MODULE_SUB_INIT(RangeGen);
	MODULE_ADD_SYM(RangeGen, iter_);
	MODULE_ADD_SYM(RangeGen, next);

	MODULE_SUB_INIT(Slice);
	MODULE_ADD_SYM(Slice, call_);
	MODULE_ADD_SYM(Slice, str_);

	MODULE_SUB_INIT(Prop);
	MODULE_ADD_SYM(Prop, str_);

	MODULE_SUB_INIT(WeakRef);
	MODULE_ADD_SYM(WeakRef, init_);
	MODULE_ADD_SYM(WeakRef, str_);
	MODULE_ADD_SYM(WeakRef, ref);
	MODULE_ADD_SYM(WeakRef, exists_QUES);
	MODULE_ADD_SYM(WeakRef, flags);
	MODULE_ADD_SYM(WeakRef, notify);
	MODULE_ADD_SYM(WeakRef, callback_);
	MODULE_ADD_SYM(WeakRef, objList_);

	MODULE_SUB_INIT(UniqueObjects);
	MODULE_ADD_SYM(NoKey, str_);
	MODULE_ADD_SYM(NoDef, str_);
	MODULE_ADD_SYM(Star, str_);
	MODULE_ADD_SYM(StarStar, str_);

	check_exceptions(ist);
}						  
