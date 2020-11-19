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


// interp.c

#include <stdlib.h>
#include <stdio.h>
#include "parser.h"
#include "interp.h"
#include <prothon/prothon_dll.h>

#include <apr.h>
#include <apr_thread_proc.h>
#include <apr_strings.h>
#include <apr_dso.h>
#include <string.h>

#define STACK_PADDING 4

static DECLARE_PR_LOCK(start_main_lock);
i32_t main_threads_started=0;
i32_t main_threads_running=0;

int debug_threads = 0;
int dump_module_code = 0;
int trace_interpreter = 0;
int dump_objects_at_end = 0;
int disable_mem_mgr = 0;

static FILE* tfout=NULL;

static int prt_code_line(frame_p frame, code_p code, int pc, FILE* fout);

isp new_ist(access_t access) {
	isp ist = pr_malloc(sizeof(interp_state_t));
	memset(ist, 0, sizeof(interp_state_t));
	ist->lock_stack   = new_clist(20);
	ist->as_str_stack = new_clist(20);
	ist->access = access;
	return ist;
}

void reset_as_str_stack(isp ist) {
	pr_free(ist->as_str_stack);
	ist->as_str_stack = new_clist(20);
}

void trace_step(frame_p frame) {
	int i;
	if (!tfout) tfout = fopen("!code_trace.txt", "w");
	prt_code_line(frame, fr_code, fr_pc, tfout);
	fprintf(tfout,"%6d>>>", fr_sp);
	for(i=0; i <  fr_sp; i++) 
		fprintf(tfout," %x", (unsigned)(uintptr_t) fr_stack[i]);
	fprintf(tfout,"\n");
	fflush(tfout);
}

//******************************** as_str_chk *********************************
int as_str_chk(isp ist, obj_p sym) {
	if (sym != SYM(STR_)) return FALSE;
	if (clist_len(astrstack) >= AS_STR_MAX_DEPTH)
		return TRUE;
	clist_append(astrstack, ist->frame);
	return FALSE;
}

//******************************** str_chk_clr ********************************
void str_chk_clr(isp ist, frame_p frame) {
	if (frame)
		clist_pop_to(astrstack, frame);
}

//******************************** new_parmptr_obj ****************************
obj_p new_parmptr_obj(isp ist, int ofs) {
	obj_p new_obj = new_object(ist, OBJ(PARMPTR_PROTO));
	SET_TYPE_IF_EXC(OBJ(PARMPTR_PROTO), new_obj, DATA_TYPE_IMMDATA) return NULL;
	new_obj->data.i64 = ofs;
	set_immutable(new_obj);
	return new_obj;
}

//******************************** create_frame *******************************
frame_p create_frame( isp ist, obj_p self, obj_p syn_locals, obj_p dyn_locals, 
							   obj_p locals, obj_p func_obj, code_p code_in, char* name ) {
    obj_p prev_scope;
	frame_p frame;
	code_p code = NULL;
	size_t flen;
	if (code_in)       code = code_in;
	else if (func_obj) code = obj_dataptr(func_obj);
	flen = sizeof(frame_t) + ( code ? (code->max_stack_depth+STACK_PADDING) * sizeof(obj_p) : 0 );
	frame = pr_malloc(flen);
	memset(frame, 0, flen);
	frame->fparam_proc_state_idx = -1;

	frame->func_name		  = name;
	if (func_obj) {
		obj_p name_obj;
		frame->func = func_obj;
		if ((name_obj = get_attr(ist, func_obj, SYM(NAME_))))
			frame->func_name = as_str(ist, name_obj);
	}
	frame->self				  = self;
	frame->dyn_locals		  = dyn_locals;
	frame->locals			  = locals;
	frame->code				  = code;
	if(!frame->dyn_locals) frame->dyn_locals = locals;
	pr_assert(frame->dyn_locals && frame->locals);

	prev_scope = (func_obj ? get_attr(ist, func_obj, SYM(PREVSCOPE_)) : syn_locals);
	if (!prev_scope && syn_locals) prev_scope = syn_locals;
	if (prev_scope) {
		pr_assert(frame->locals != prev_scope);
		set_attr(ist, frame->locals, SYM(PREVSCOPE_), prev_scope);
	}
	//pr_assert(frame->func_name);
	return frame;																				  
}

#define ERR 1
#define OK  0

//********************************* new_local_obj *****************************
obj_p new_local_obj(isp ist, obj_p symbol){
	obj_p local_obj = NEW_OBJ(OBJ(LOCAL_PROTO));
	SET_TYPE_IF_EXC(OBJ(LOCAL_PROTO), local_obj, DATA_TYPE_EXTPTR) return NULL;
	local_obj->data.ptr = symbol;
	local_obj->immutable = TRUE;
	return local_obj;
}

//********************************* new_super_obj *****************************
obj_p new_super_obj(isp ist, obj_p container, obj_p symbol) {
	int i;
	obj_p super_obj = NEW_OBJ(OBJ(SUPER_PROTO));
	SET_TYPE_IF_EXC(OBJ(SUPER_PROTO), super_obj, DATA_TYPE_EXTPTR) return NULL;
	super_obj->data.ptr = symbol;
	su(i);
	set_attr(ist, super_obj, SYM(CONT_), container);
	un_su(i);
	super_obj->immutable = TRUE;
	return super_obj;
}

//********************************* new_outer_obj *****************************
obj_p new_outer_obj(isp ist, obj_p symbol){
	obj_p outer_obj = NEW_OBJ(OBJ(OUTER_PROTO));
	SET_TYPE_IF_EXC(OBJ(OUTER_PROTO), outer_obj, DATA_TYPE_EXTPTR) return NULL;
	outer_obj->data.ptr = symbol;
	outer_obj->immutable = TRUE;
	return outer_obj;
}

//********************************* get_item ******************************
obj_p get_item(isp ist, obj_p ref_self, obj_p ref_key, int prop_flg){
	if (prop_flg && get_proto_attr(ist, ref_self, SYM(GET_), NULL, NULL)) {
		return call_func1(ist, ref_self, SYM(GET_), ref_key);
	}
	if (has_proto(ist, ref_key, OBJ(SYMBOL_PROTO))) {
		obj_p res;
		if_exc_return NULL;
		res = get_proto_attr(ist, ref_self, ref_key, NULL, NULL);
		if (!res) {
			raise_exception(ist, OBJ(NAME_EXC), "Attribute %s not found",
					             symch(ist, ref_key));
			return NULL;
		}
		if (prop_flg && has_proto(ist, res, OBJ(PROP_PROTO))) {
			if (get_proto_attr(ist, res, SYM(GET_), NULL, NULL)) {
				return call_func(ist, res, SYM(GET_), ref_self, 0, NULL, NULL);
			}
			raise_exception(ist, OBJ(TYPE_EXC), "read attempt on a property with no get_ method");
			return NULL;
		}
		return res;
	} else if (has_proto(ist, ref_key, OBJ(LOCAL_PROTO))) {
		if_exc_return NULL;
        return get_scope_attr(ist, ref_self, ref_key->data.ptr, NULL, NULL);
	} else if (has_proto(ist, ref_key, OBJ(SLICE_PROTO))) {
		obj_p param[2];  
		if_exc_return NULL;
		param[0]=OBJ(NOKEY); param[1]=ref_key;
        return call_func(ist, ref_self, SYM(GETITEM_), NULL, 2, param, 0);
	} else if (has_proto(ist, ref_key, OBJ(OUTER_PROTO))) {
		obj_p res;
		if_exc_return NULL;
        res = get_scope_attr(ist, ref_self, ref_key->data.ptr, NULL, ref_self);
		if (!res) {
			raise_exception(ist, OBJ(INTERPRETER_EXC), "outer.%s not found", as_str(ist, ref_key->data.ptr));
			return NULL;
		}
		return res;
	} else if (has_proto(ist, ref_key, OBJ(INT_PROTO))) {
		obj_p param[2];  
		param[0]=OBJ(NOKEY); param[1]=slice1(ref_key);
        return call_func(ist, ref_self, SYM(GETITEM_), NULL, 2, param, 0);
	} else if (has_proto(ist, ref_key, OBJ(SUPER_PROTO))) {
		obj_p res, cont = get_attr(ist, ref_key, SYM(CONT_));
		if (!cont) {
			raise_exception(ist, OBJ(NAME_EXC), "Container obj for attribute %s not found",
					                             symch(ist, ref_key->data.ptr));
			return NULL;
		}
		if_exc_return NULL;
		res = get_proto_attr(ist, ref_self, ref_key->data.ptr, NULL, cont);
		if (!res) {
			raise_exception(ist, OBJ(NAME_EXC), "Super attribute %s not found",
					                             symch(ist, ref_key->data.ptr));
		}
		return res;
	} else {
		raise_exception(ist, OBJ(INTERNAL_EXC), "Invalid reference type");
		return NULL;
	}
}

//********************************* set_item ******************************
void set_item(isp ist, obj_p ref_self, obj_p ref_key, obj_p value, int prop_flg){
	if (prop_flg && get_proto_attr(ist, ref_self, SYM(SET_), NULL, NULL)) {
		call_func2(ist, ref_self, SYM(SET_), ref_key, value);
		return;
	}
	if (ref_key == SYM(ATTRS_)) {
		if (!has_proto(ist, value, OBJ(DICT_PROTO))) {
			raise_exception(ist, OBJ(TYPE_EXC), "attempt to replace attrs_ with non-dictionary");
			return;
		}
		dict2attrs(ist, value, ref_self);
	} else if (ref_key == SYM(PROTOS_)) {
		if (!has_proto(ist, value, OBJ(LIST_PROTO))) {
			raise_exception(ist, OBJ(TYPE_EXC), "attempt to replace protos_ with non-list");
			return;
		}
		list2protos(ist, value, ref_self);
	} else if (has_proto(ist, ref_key, OBJ(SYMBOL_PROTO))) {
		if (prop_flg) {
			obj_p res = get_proto_attr(ist, ref_self, ref_key, NULL, NULL);
			if_exc_return;
			if (res && has_proto(ist, res, OBJ(PROP_PROTO))) {
				if (get_proto_attr(ist, res, SYM(SET_), NULL, NULL)) {
					obj_p arr[2]; arr[0] = OBJ(NOKEY); arr[1] = value;
					call_func(ist, res, SYM(SET_), ref_self, 2, arr, NULL);
					return;
				}
				raise_exception(ist, OBJ(TYPE_EXC), "write attempt on a property with no set_ method");
				return;
			}
		}
		set_attr(ist, ref_self, ref_key, value);
	} else if (has_proto(ist, ref_key, OBJ(LOCAL_PROTO))) {
		set_attr(ist, ref_self, ref_key->data.ptr, value);
	} else if (has_proto(ist, ref_key, OBJ(SLICE_PROTO))) {
		obj_p param[4];  
		param[0]=OBJ(NOKEY); param[1]=ref_key;
		param[2]=OBJ(NOKEY); param[3]=value;
        call_func(ist, ref_self, SYM(SETITEM_), NULL, 4, param, 0);
	} else if (has_proto(ist, ref_key, OBJ(OUTER_PROTO))) {
		obj_p scope_obj;
		if_exc_return;
        get_scope_attr(ist, ref_self, ref_key->data.ptr, &scope_obj, ref_self);
		if (!scope_obj) {
			raise_exception(ist, OBJ(INTERPRETER_EXC), "outer.%s not found", as_str(ist, ref_key->data.ptr));
			return;
		}
		if_exc_return;
		set_attr(ist, scope_obj, ref_key->data.ptr, value);
	} else if (has_proto(ist, ref_key, OBJ(INT_PROTO))) {
		obj_p param[4];  
		param[0]=OBJ(NOKEY); param[1]=slice1(ref_key);
		param[2]=OBJ(NOKEY); param[3]=value;
        call_func(ist, ref_self, SYM(SETITEM_), NULL, 4, param, 0);
	} else if (has_proto(ist, ref_key, OBJ(SUPER_PROTO))) {
		obj_p res, proto, cont = get_attr(ist, ref_key, SYM(CONT_));
		if (!cont) {
			raise_exception(ist, OBJ(NAME_EXC), "Container obj for attribute %s not found",
					                             symch(ist, ref_key->data.ptr));
			return;
		}
		if_exc_return;
		res = get_proto_attr(ist, ref_self, ref_key->data.ptr, &proto, cont);
		if (!res) {
			raise_exception(ist, OBJ(NAME_EXC), "Super attribute %s not found",
					                             symch(ist, ref_key->data.ptr));
			return;
		}
		set_attr(ist, proto, ref_key->data.ptr, value);
	} else
		raise_exception(ist, OBJ(INTERNAL_EXC), "Invalid reference type");
}

//********************************* del_item ***********************************
void del_item(isp ist, obj_p ref_self, obj_p ref_key, int prop_flg){
	if (prop_flg && get_proto_attr(ist, ref_self, SYM(DEL_), NULL, NULL)) {
		call_func1(ist, ref_self, SYM(DEL_), ref_key);
		return;
	}
	if (ref_key == SYM(ATTRS_)) {
		raise_exception(ist, OBJ(TYPE_EXC), "attempt to del attrs_");
		return;
	} else if (ref_key == SYM(PROTOS_)) {
		raise_exception(ist, OBJ(TYPE_EXC), "attempt to del protos_");
		return;
	} else if (has_proto(ist, ref_key, OBJ(SYMBOL_PROTO))) {
		if (prop_flg) {
			obj_p res = get_proto_attr(ist, ref_self, ref_key, NULL, NULL);
			if_exc_return;
			if (res && has_proto(ist, res, OBJ(PROP_PROTO))) {
				if (get_proto_attr(ist, res, SYM(DEL_), NULL, NULL)) {
					call_func(ist, res, SYM(DEL_), ref_self, 0, NULL, NULL);
					return;
				}
				raise_exception(ist, OBJ(TYPE_EXC), "delete attempt on a property with no del_ method");
				return;
			}
		}
		if_exc_return;
		del_attr(ist, ref_self, ref_key);
	} else if (has_proto(ist, ref_key, OBJ(LOCAL_PROTO))) {
		del_attr(ist, ref_self, ref_key->data.ptr);
	} else if (has_proto(ist, ref_key, OBJ(SLICE_PROTO))) {
		obj_p param[2];  
		param[0]=OBJ(NOKEY); param[1]=ref_key;
        call_func(ist, ref_self, SYM(DELITEM_), NULL, 2, param, 0);
	} else if (has_proto(ist, ref_key, OBJ(OUTER_PROTO))) {
		obj_p scope_obj;
		if_exc_return;
        get_scope_attr(ist, ref_self, ref_key->data.ptr, &scope_obj, ref_self);
		if (!scope_obj) {
			raise_exception(ist, OBJ(INTERPRETER_EXC), "outer.%s not found", as_str(ist, ref_key->data.ptr));
			return;
		}
		if_exc_return;
		del_attr(ist, scope_obj, ref_key->data.ptr);
	} else if (has_proto(ist, ref_key, OBJ(INT_PROTO))) {
		obj_p param[2];  
		param[0]=OBJ(NOKEY); param[1]=slice1(ref_key);
        call_func(ist, ref_self, SYM(DELITEM_), NULL, 2, param, 0);
	} else if (has_proto(ist, ref_key, OBJ(SUPER_PROTO))) {
		obj_p res, proto, cont = get_attr(ist, ref_key, SYM(CONT_));
		if (!cont) {
			raise_exception(ist, OBJ(NAME_EXC), "Container obj for attribute %s not found",
					                             symch(ist, ref_key->data.ptr));
			return;
		}
		if_exc_return;
		res = get_proto_attr(ist, ref_self, ref_key->data.ptr, &proto, cont);
		if (!res) {
			raise_exception(ist, OBJ(NAME_EXC), "Super attribute %s not found",
					                             symch(ist, ref_key->data.ptr));
			return;
		}
		del_attr(ist, proto, ref_key->data.ptr);
	} else
		raise_exception(ist, OBJ(INTERNAL_EXC), "Invalid reference type");
}

//******************************** add_pos_param ******************************
int add_pos_param(isp ist, fparam_proc_state_t* state, obj_p param){
	if (state->pos_ptr == clist_len(state->lbl_val_list)/2){
		if (!state->free_pos_list_obj)
			return ERR;
		list_append(ist, state->free_pos_list_obj, param);
		return OK;
	}
	clist_item(state->lbl_val_list, (state->pos_ptr++)*2+1) = param;
	return OK;
}

//******************************** add_label_param ****************************
int add_label_param(isp ist, fparam_proc_state_t* state, obj_p label, obj_p param){
	int i;
	for (i=state->pos_ptr; i < clist_len(state->lbl_val_list)/2; i++)
		if (clist_item(state->lbl_val_list, i*2) == label) {
			clist_item(state->lbl_val_list, i*2+1) = param;
			return OK;
		}
	if (!state->free_label_list_obj)
		return ERR;
	dict_add(ist, state->free_label_list_obj, label, param);
	return OK;
}

//******************************** map_params *********************************
int map_params( isp ist, fparam_proc_state_t* fparam_proc_state, 
			    obj_p func_obj, int parm_cnt, obj_p *act_params ){
	obj_p form_params, label, value;
	memset(fparam_proc_state, 0, sizeof(fparam_proc_state_t));
	if ((form_params = get_attr(ist, func_obj, SYM(FPARAMS_)))) {
		int i, j, pstate = 0, fcnt = (int) list_len(ist, form_params);
		fparam_proc_state->lbl_val_list  = new_clist(fcnt);
		for (i=0, j=0; i < fcnt; i+=2) {
			label = list_item(ist, form_params, i);
			value = list_item(ist, form_params, i+1);
			if (pstate == 3) goto fparmerr;
			if (pstate && value == OBJ(NODEF)) goto fparmerr;
			if (value == OBJ(PARAM_STAR)) {
				if (pstate == 2) goto fparmerr;
				pstate = 2;
				fparam_proc_state->free_pos_list_key = label;
				fparam_proc_state->free_pos_list_obj = NEW_LIST(10);
			} else if (value == OBJ(PARAM_STAR_STAR)) {
				pstate = 3;
				fparam_proc_state->free_label_list_key = label;
				fparam_proc_state->free_label_list_obj = NEW_DICT(10);
			} else {
				if (value != OBJ(NODEF)) pstate = 1;
				clist_append(fparam_proc_state->lbl_val_list, label);
				clist_append(fparam_proc_state->lbl_val_list, value);
			}
		}
	}
	if (fparam_proc_state->lbl_val_list) {
		int i, j;
		for (i=0; i < parm_cnt; ) {
			label = act_params[i++];
			value = act_params[i++];
			if (label == OBJ(NOKEY)) {
				if (add_pos_param(ist, fparam_proc_state, value)) goto paramerr;
			} else if (label == OBJ(PARAM_STAR)) {
				int llen = (int) list_len(ist, value);
				for(j=0; j < llen; j++)
					if (add_pos_param(ist, fparam_proc_state, list_item(ist, value, j))) goto paramerr;
			} else if (label == OBJ(PARAM_STAR_STAR)) {
				obj_p list = dict_keys_values(ist, value, 2 /*list*/);
				int llen = (int) list_len(ist, list);
				for(j=0; j < llen; j+=2)
					if ( add_label_param(ist, fparam_proc_state, list_item(ist, list, j), 
						                      list_item(ist, list, j+1)) ) goto paramerr;
				del_unlock(list);
			} else {
				if (add_label_param(ist, fparam_proc_state, label, value)) goto paramerr;
			}
		}
	} else {
		int i;
		for (i=0; i < parm_cnt; ) {
			label = act_params[i++];
			value = act_params[i++];
			if (label == OBJ(PARAM_STAR)) {
				if (list_len(ist, value)) goto paramerr;
			} else if (label == OBJ(PARAM_STAR_STAR)) {
				if (dict_len(ist, value)) goto paramerr;
			} else 
				goto paramerr;
		}
	}
	return OK;
fparmerr:
	return 1;
paramerr:
	return 2;
}

//******************************** get_func_params ****************************
clist_p get_func_params( isp ist, obj_p func_obj, int parm_cnt, obj_p *actparams) {
	int i, llen;
	fparam_proc_state_t fparam_proc_state;
	switch (map_params(ist, &fparam_proc_state, func_obj, parm_cnt, actparams)) 
		   {case 1: goto fparmerr; case 2: goto paramerr;}
	llen = fparam_proc_state.lbl_val_list ?
		   clist_len(fparam_proc_state.lbl_val_list)/2 : 0;
	for (i=0; i < llen; i++)
		if (clist_item(fparam_proc_state.lbl_val_list, 2*i+1) == OBJ(NODEF)) 
			goto paramerr;
	if (fparam_proc_state.free_pos_list_key){
		clist_append(fparam_proc_state.lbl_val_list, fparam_proc_state.free_pos_list_key);
		clist_append(fparam_proc_state.lbl_val_list, fparam_proc_state.free_pos_list_obj);
	}
	if (fparam_proc_state.free_label_list_key){
		clist_append(fparam_proc_state.lbl_val_list, fparam_proc_state.free_label_list_key);
		clist_append(fparam_proc_state.lbl_val_list, fparam_proc_state.free_label_list_obj);
	}
	return fparam_proc_state.lbl_val_list;
fparmerr:
	free_clist(fparam_proc_state.lbl_val_list);
	raise_exception(ist, OBJ(INTERPRETER_EXC), "Formal parameter sequence error");
	return NULL;
paramerr:
	free_clist(fparam_proc_state.lbl_val_list);
	raise_exception(ist, OBJ(INTERPRETER_EXC), "Parameters passed don't match formal parameters");
	return NULL;
}

//******************************** get_frame_params **************************
frame_p get_frame_params( isp ist, int* idx, fparam_proc_state_t* fparam_proc_state,
			              obj_p* new_locals, obj_p func_obj, int parm_cnt, obj_p *actparams ) {
	int i, llen;
	if ((*idx) == -1) {
		(*new_locals) = NEW_OBJ(NULL);
		switch (map_params(ist, fparam_proc_state, func_obj, parm_cnt, actparams))
				{case 1: goto fparmerr; case 2: goto paramerr;}
	}
	llen = (*fparam_proc_state).lbl_val_list ?
					clist_len((*fparam_proc_state).lbl_val_list)/2 : 0;
	for (i=(*idx)+1; i < llen; i++) {
		obj_p parm = clist_item((*fparam_proc_state).lbl_val_list, 2*i+1);
		if (!parm) goto paramerr;
		if (has_proto(ist, parm, OBJ(PARMPTR_PROTO))) {
			obj_p eval_locals;
			code_p codep = (void*)(((char*)func_obj->data.ptr)+parm->data.i64);
			if (!func_obj) goto evalerr;
			eval_locals = get_attr(ist, func_obj, SYM(PREVSCOPE_));
			if (!eval_locals) goto evalerr;
			(*idx) = i;
			return create_frame( ist, NULL, NULL, (*new_locals), 
				                 eval_locals, NULL, codep, NULL );							
		}
	}
	(*idx) = -1;
	for (i=0; i < llen; i++)
		set_attr(ist, (*new_locals), clist_item((*fparam_proc_state).lbl_val_list, i*2), 
						clist_item((*fparam_proc_state).lbl_val_list,(i*2)+1) );
	if ((*fparam_proc_state).free_pos_list_key) {
		switch_proto(ist, (*fparam_proc_state).free_pos_list_obj, OBJ(TUPLE_PROTO));
		set_attr(ist, (*new_locals), (*fparam_proc_state).free_pos_list_key, 
						(*fparam_proc_state).free_pos_list_obj );
	}
	if ((*fparam_proc_state).free_label_list_key)
		set_attr(ist, (*new_locals), (*fparam_proc_state).free_label_list_key, 
						(*fparam_proc_state).free_label_list_obj );
	goto done;
evalerr:
	raise_exception(ist, OBJ(INTERNAL_EXC), "default value not allowed here");
	goto done;
fparmerr:
	raise_exception(ist, OBJ(INTERPRETER_EXC), "Formal parameter sequence error");
	goto done;
paramerr:
	raise_exception(ist, OBJ(INTERPRETER_EXC), "Parameters passed don't match formal parameters");
done:
	(*idx) = -1;
	free_clist((*fparam_proc_state).lbl_val_list);
	return NULL;
}

//******************************** add_frame_to_exception *********************
void add_frame_to_exception(isp ist, frame_p frame) {
	int i, pc, line=0, col=0, save_access; 
	clist_p srcmap;
	obj_p fstk_obj, orig_excobj, str_obj=NULL;

	su(save_access);

	orig_excobj = intrp_exobj;
	ist->exception_obj = 0;
	fstk_obj = get_attr(ist, orig_excobj, sym(ist, "frame_stack"));
	if (!fstk_obj)
		set_attr(ist, orig_excobj, sym(ist, "frame_stack"), (fstk_obj = NEW_LIST(10)));
	srcmap = frame->code->srcmap;
	for (i = 0; i < clist_len(srcmap); i += 3) {
		pc = clist_num(srcmap, i);
		if (fr_pc < pc) break;
		line = clist_num(srcmap, i+1);
		col  = clist_num(srcmap, i+2);
	}
	if (frame->code->file_path)
		str_obj = NEW_STRING(frame->code->file_path);
	list_append(ist, fstk_obj, str_obj);
	list_append(ist, fstk_obj, NEW_INT(line));
	list_append(ist, fstk_obj, NEW_INT(col));
	ist->exception_obj = orig_excobj;

	un_su(save_access);
}

#ifdef WIN32
#define MODULE_EXTENSION	""
#else
#define MODULE_EXTENSION	".so"
#endif

//******************************** load_dll_module *****************************
int load_dll_module(isp ist, char* full_path, char* module_name, obj_p dest_module, obj_p alias){
	int i;
	obj_p module;
	apr_dso_handle_t *handle;
	apr_dso_handle_sym_t dll_entry;
	apr_status_t aprerr;
	char dll_path[APR_PATH_MAX];

	apr_snprintf(dll_path, sizeof(dll_path), "%s/%s%s", full_path, module_name,
		     MODULE_EXTENSION);

	aprerr = apr_dso_load(&handle, dll_path, get_pr_head_pool());
	if (aprerr != APR_SUCCESS) {
#ifdef DEBUG_MODULES
		printf("Failed finding dll: %s\n", apr_dso_error(handle, dll_path, sizeof(dll_path)));
#endif
		return FALSE;
	}

	apr_snprintf(dll_path, sizeof(dll_path), "%s_dll_entry", module_name);
	aprerr = apr_dso_sym(&dll_entry, handle, dll_path);
	IF_APR_ERR(dll_path) return FALSE;

	((void(*)(pr_services_t*))dll_entry)(&dll_services);

	if ((module = get_attr(ist, OBJ(MODULES), sym(ist, module_name))) && dest_module){
		su(i);
		set_attr(ist, dest_module, alias, module);
		un_su(i);
	}
	return TRUE;
}

//******************************** import_module **********************************
obj_p import_module( isp ist, frame_p frame, int param, 
				     int store_local, obj_p alias, int return_mod ){
	int i, llen, pkg_depth = 1;
	obj_p dest_module, module_symbol, dmod;
	char full_path[APR_PATH_MAX], pkg_path[APR_PATH_MAX];
	char *module_name = NULL;
	obj_p path_list = get_sys_path_list();
	apr_finfo_t finfo;

	module_symbol = fr_data(param - 1);
	module_name = symch(ist, module_symbol);

	if (store_local)
		dest_module = frame->locals;
	else
		dest_module = NULL;

	llen = (int) list_len(ist, path_list);
	pkg_path[0] = 0;

	for (i = 0; i < llen; i++) {
		apr_snprintf(full_path, sizeof(full_path), "%s/%s/init_.pr",
				pr2c_strptr(ist, list_item(ist, path_list, i)),
				symch(ist, fr_data(pkg_depth)));

		if (apr_stat(&finfo, full_path, APR_FINFO_TYPE, get_pr_head_pool()) ==
		    APR_SUCCESS && finfo.filetype == APR_REG) {
			apr_snprintf(pkg_path, sizeof(pkg_path), "%s/%s",
				     pr2c_strptr(ist, list_item(ist, path_list, i)),
				     symch(ist, fr_data(pkg_depth)));

			if (alias && pkg_depth < param-1)
				dmod = NULL;
			else
				dmod = dest_module;

			load_module(ist, fr_data(pkg_depth), frame->locals,
					dmod, alias, full_path, ": a package", NULL, NULL);
			if_exc_return 0;

			if (!alias && dest_module) 
				dest_module = get_attr(ist, OBJ(MODULES), fr_data(pkg_depth));

			pkg_depth++;
			while (pkg_depth < param) {
				apr_snprintf(full_path, sizeof(full_path), "%s/%s/init_.pr",
					     pkg_path, symch(ist, fr_data(pkg_depth)));

				if (apr_stat(&finfo, full_path, APR_FINFO_TYPE,
				    get_pr_head_pool()) == APR_SUCCESS &&
				    finfo.filetype == APR_REG) {
					apr_size_t len = strlen(pkg_path);

					apr_cpystrn(pkg_path + len,
						    symch(ist, fr_data(pkg_depth)),
						    sizeof(pkg_path) - len);

					if (alias && pkg_depth < param - 1)
						dmod = NULL;
					else
						dmod = dest_module;

					load_module(ist, fr_data(pkg_depth), frame->locals,
							dmod, alias, full_path, ": a package",
							NULL, NULL );
					if_exc_return 0;

					if (!alias && dest_module) 
						dest_module = get_attr(ist, OBJ(MODULES),
									fr_data(pkg_depth));
					pkg_depth++;
				} else
					break;
			}
			break;
		}
	}

	if (pkg_depth < param - 1) {
		raise_exception(ist, OBJ(INTERPRETER_EXC), "Package %s not found",
				symch(ist, fr_data(pkg_depth)));
		return NULL;
	}

	if (pkg_depth == param)
		goto success;

	if (pkg_path[0]) {
		apr_cpystrn(full_path, pkg_path, sizeof(full_path));
		if (!alias)
			alias = module_symbol;
		if (load_dll_module(ist, full_path, module_name, dest_module, alias))
			goto success;
		if_exc_return NULL;
	} else {
		if (!alias)
			alias = module_symbol;

		for (i = 0; i < llen; i++) {
			apr_cpystrn(full_path, pr2c_strptr(ist, list_item(ist, path_list, i)),
				    sizeof(full_path));

			if (load_dll_module(ist, full_path, module_name, dest_module, alias))
				goto success;

			if_exc_return NULL;
		}
	}

	if (pkg_path[0]) {
		apr_snprintf(full_path, sizeof(full_path), "%s/%s.pr", pkg_path, module_name);
		if (load_module(ist, module_symbol, frame->locals, dest_module,
				alias, full_path, ": a module", NULL, NULL ))
			goto success;
		if_exc_return NULL;
	} else {
		for(i = 0; i < llen; i++) {
			apr_snprintf(full_path, sizeof(full_path), "%s/%s.pr",
				     pr2c_strptr(ist, list_item(ist, path_list, i)),
				     module_name);
			if (load_module(ist, module_symbol, frame->locals, dest_module,
					alias, full_path, ": a module", NULL, NULL ))
				goto success;
			if_exc_return NULL;
		}
	}
	raise_exception(ist, OBJ(INTERPRETER_EXC), "Import module %s not found",
			module_name);
	return NULL;

success:
	if_exc_return NULL;
	if (return_mod)
		return get_attr(ist, OBJ(MODULES), module_symbol);
	else
		return NULL;
}

//******************************** do_return **********************************
#define DONE_FRAME_FLAG ((frame_p) 1)
obj_p do_return( isp ist, frame_p frame, 
				 frame_p *switch_frame, frame_p *free_frame, obj_p return_value ){
	obj_p res = NULL;
	if (!(fr_pc == fr_code->len && fr_sp == 0) && fr_sp != 1) {
		raise_exception(ist, OBJ(INTERNAL_EXC), "stack != 1 at return call");
		return NULL;
	}
	if (fr_prev) {
		if (fr_ccall) 
			res = fr_tos;
		else {
			if (fr_prev->no_return_flag)
				fr_prev->no_return_flag = FALSE;
			else
				fr_prev->stack[fr_prev->stack_ptr-1] = fr_tos;
		}
		*switch_frame = fr_prev;
		(*switch_frame)->next_frame = NULL;
	} else {
		if (fr_ccall) 
			res = fr_tos;
		*switch_frame = DONE_FRAME_FLAG;
	}
	if (frame->gen_marker)
		raise_exception(ist, OBJ(STOP_ITERATION_EXC), NULL);
	else
		*free_frame = frame;
	if (!res) res = return_value;
	return res;
}

//******************************** fr_exstk_append ****************************
void exstk_push(frame_p frame, exception_entry_t* exc_entry){
	if (!fr_exstk) fr_exstk = new_clist(5);
	clist_append(fr_exstk, exc_entry);
}

#define MIN_OBJ ((obj_p) 10)


//******************************** pre_call_lock ******************************
void pre_call_lock(isp ist, obj_p slf, clist_p plist) {
	int i;
	read_lock(ist, slf);							
	if (!intrp_exobj && plist) {					
		for(i=1; i < clist_len(plist); i+=2) {		
			obj_p p = clist_item(plist, i);			
			if (p > MIN_OBJ && p != (slf))			
				rdlock_brk(p);						
		}											
		if (intrp_exobj) {							
			for (i -= 2; i > 0; i -= 2) {				
				obj_p p = clist_item(plist, i);		
				if (p > MIN_OBJ && p != (slf))		
					read_unlock(ist, p);			
			}										
		}											
	}
}

//******************************** post_call_unlock ***************************
void post_call_unlock(isp ist, obj_p slf, clist_p plist) {
	int i;
	read_unlock(ist, slf);							
	if (plist)										
		for(i=1; i < clist_len(plist); i += 2) {		
			obj_p p = clist_item(plist, i);			
			if (p > MIN_OBJ && p != (slf))			
				read_unlock(ist, p);				
		}
}

#define IF_EXC_BREAK if (intrp_exobj) break

//******************************** exec_loop **********************************
obj_p exec_loop(isp ist){
	obj_p  func_obj;
	opcode_t op;
	int i, param;
	frame_p switch_frame = NULL, free_frame = NULL, frame = ist->frame;
	int have_exstk, exc_try_loc = 0, exc_exc_loc = 0, return_flag = FALSE;
	int exc_final=0, exc_final_return=0; 
	frame_p exc_free_frame = NULL, exc_switch_frame = NULL;
	int stack_lim = frame->code->max_stack_depth, cmd_flag;
	obj_p exc_waiting=NULL, return_value = 0;

#ifdef DUMP_MODULE_CODE
	obj_p dump_code_key = NULL;
#endif
#ifdef TRACE_INTERPRETER
	if (trace_interpreter) {
		if (!tfout) tfout = fopen("!code_trace.txt", "w");
		fprintf(tfout, "--- starting exec_loop at frame %8x, pc: %d, %s\n", (intptr_t) frame, frame->pc, frame->func_name);
		fflush(tfout);
	}
#endif

	have_exstk = (fr_exstk && clist_len(fr_exstk));
	if (have_exstk) {
		exception_entry_t* p = (exception_entry_t*)(clist_tos(fr_exstk));
		exc_final   = p->final;
		exc_try_loc	= p->try_loc;
		exc_exc_loc	= p->exc_loc;
	}
	while(TRUE){
		if (debug_obj && (debug_obj->next_obj == debug_obj))
			raise_exception(ist, OBJ(INTERPRETER_EXC), "DEBUG FAILURE");
		while (have_exstk && (fr_pc <= exc_try_loc || fr_pc >= exc_exc_loc)) {
			if (exc_final && fr_pc != exc_exc_loc) {
				exc_final_return = fr_pc;
				fr_pc = exc_exc_loc;
			}
			pr_free(clist_pop(fr_exstk));
			if (clist_len(fr_exstk)) {
				exception_entry_t* p = (exception_entry_t*)(clist_tos(fr_exstk));
				exc_final   = p->final;
				exc_try_loc	= p->try_loc;
				exc_exc_loc	= p->exc_loc;
			} else have_exstk = FALSE;
		}
		if (fr_pc >= fr_code->len){
#ifdef TRACE_INTERPRETER
			if (trace_interpreter) {
				fprintf(tfout, "--- ran off end of code at pc: %d\n", (intptr_t) frame->pc);
				fflush(tfout);
			}
#endif
			return_value = do_return(ist, frame, &switch_frame, &free_frame, return_value);
			if (intrp_exobj) goto endop;
			return_flag = fr_ccall;
			goto endop;
		}
		op    = fr_op;
		param = fr_param;

		if (fr_sp > stack_lim) {
			printf("Internal fatal error: frame stack overflow\n");
			pr_exit(1);
		}
		switch (op) {
			case OP_NOP: 
				break;
			case OP_CLRSP:
				fr_sp = 0;
				break;
			case OP_PUSH: 
				for(i=1; i < param; i++) 
					fr_push(fr_data(i)); 
				break;
			case OP_DUPLICATE: {
				obj_p tmp = fr_stack[fr_sp+param];
				fr_push(tmp);
				break;
			}
			case OP_CHAIN_CMP:
				fr_sp -= 2;
				if (fr_stack[fr_sp+1] == OBJ(PR_TRUE)) {
					obj_p cmp_obj = NEW_OBJ(OBJ(PR_TRUE));
					set_attr(ist, cmp_obj, SYM(CMPVALOBJ_), fr_stack[fr_sp]);
					fr_push(cmp_obj);
				} else 
					fr_push(OBJ(PR_FALSE));
				break;
			case OP_SWAP: { 
				obj_p tmp;
				exch(fr_stack[fr_sp-1], fr_stack[fr_sp-2]);
			}	break;
			case OP_POP: 
				for(i=0; i < param; i++) 
					return_value = fr_pop; 
				break;
			case OP_PUSH_SELF:
				if (!frame->self) {
					raise_exception(ist, OBJ(INTERPRETER_EXC), "self only valid in function");
					break;
				}
				fr_push(frame->self);
				break;
			case OP_PUSH_CALLER:
				if (!frame->dyn_locals) {
					raise_exception(ist, OBJ(INTERPRETER_EXC), "caller not available here");
					break;
				}
				fr_push(frame->dyn_locals);  
				break;
			case OP_PUSH_SELF_REF: 
				if (!frame->self) {
					raise_exception(ist, OBJ(INTERPRETER_EXC), "self.var only valid in function");
					break;
				}
				fr_push(frame->self);  
				fr_push(fr_data(1)); 
				break;
			case OP_PUSH_SUPER_REF: {
				obj_p cont;
				if ( !frame->self || !frame->func ||
					!(cont = get_attr(ist, frame->func, SYM(CONT_))) ) {
					raise_exception(ist, OBJ(INTERPRETER_EXC), "super.var only valid in function");
					break;
				}
				fr_push(frame->self);
				fr_push(new_super_obj(ist, cont, fr_data(1))); 
				break;
			}
			case OP_PUSH_LOCAL_REF: 
				fr_push(frame->locals);  
				fr_push(new_local_obj(ist, fr_data(1))); 
				break;
			case OP_PUSH_OUTER_REF:
				fr_push(frame->locals);  
				fr_push(new_outer_obj(ist, fr_data(1))); 
				break;	
			case OP_PUSH_CALLER_REF: 
				if (!frame->dyn_locals) {
					raise_exception(ist, OBJ(INTERPRETER_EXC), "caller not available here");
					break;
				}
				fr_push(frame->dyn_locals);  
				fr_push(fr_data(1)); 
				break;
			case OP_DEREF: {
				obj_p att, obj, key;			
				fr_sp -= 2;
				obj = fr_stack[fr_sp];
				key = fr_stack[fr_sp+1];
				if (key == SYM(ATTRS_))
					fr_push(new_attrdict_obj(ist, obj));
				else if (key == SYM(PROTOS_))
					fr_push(new_protolist_obj(ist, obj));
				else {
					att = get_item(ist, obj, key, TRUE); IF_EXC_BREAK;
					if (!att) {
						raise_exception( ist, OBJ(INTERPRETER_EXC), "attribute %s not found", 
																	       as_str(ist, key) );
						break;
					}
					fr_push(att);
				}
			}   break;
			case OP_ASSIGN: {
				int sp1, sp2 = fr_sp - param*2;
				sp1 = sp2 - param;
				fr_sp = sp2;
				for(i=0; i < param; i++, sp1++, sp2+=2)
					set_item(ist, fr_stack[sp2], fr_stack[sp2+1], fr_stack[sp1], TRUE);
			}	break;
			case OP_ASSIGN_EXP: {
				obj_p list;
				fr_sp -= param*2;
				list=fr_tos;
				if (has_proto(ist, list, OBJ(TUPLE_PROTO))){
					if (list_len(ist, list) != param){
						raise_exception(ist, OBJ(INTERPRETER_EXC), "right side sequence length doesn't match left side");
						break;
					}
					for(i=0; i < param; i++) {
						obj_p key = fr_stack[fr_sp+i*2+1];
						if (has_proto(ist, key, OBJ(LOCAL_PROTO)))
							key = key->data.ptr;
						set_attr(ist, fr_stack[fr_sp+i*2], key, list_item(ist, list, i));
					}
				} else {
					obj_p key;
					if (param != 1){
						raise_exception(ist, OBJ(INTERPRETER_EXC), "only one object on right side and multiple on left");
						break;
					}
					key = fr_stack[fr_sp+1];
					if (has_proto(ist, key, OBJ(LOCAL_PROTO)))
						key = key->data.ptr;
					set_attr(ist, fr_stack[fr_sp], key, fr_stack[fr_sp]);
				}
			}	break;
			case OP_SEQ_ASSIGN: {
				obj_p list;
				fr_sp -= param+1;
				if (has_proto(ist, list=fr_stack[fr_sp], OBJ(TUPLE_PROTO))) {
					if (list_len(ist, list) > 1 && param == 1) {
						set_attr(ist, frame->locals, fr_stack[fr_sp+1], list);
						break;
					}
					if (list_len(ist, list) != param){
						raise_exception(ist, OBJ(INTERPRETER_EXC), "For param list length doesn't match sequence length");
						break;
					}
					for(i=0; i < param; i++)
						set_attr(ist, frame->locals, fr_stack[fr_sp+i+1], list_item(ist, list, i));
				} else {
					if (param != 1){
						raise_exception(ist, OBJ(INTERPRETER_EXC), "For param list too long, only one item given");
						break;
					}
					set_attr(ist, frame->locals, fr_stack[fr_sp+1], fr_stack[fr_sp]);
				}
			}	break;
			case OP_NEWTUPLE: {
				obj_p tuple_obj = NEW_TUPLE(param);
				fr_sp -= param;
				listlen(tuple_obj) = param;
				for (i=0; i < param; i++)
					listitem(tuple_obj, i) = fr_stack[fr_sp+i];
				fr_push(tuple_obj);
				set_immutable(tuple_obj);
			}	break;
			case OP_NEWLIST: {
				obj_p list_obj = NEW_LIST(param);
				fr_sp -= param;
				for (i=0; i<param; i++)
					list_append(ist, list_obj, fr_stack[fr_sp+i]);
				fr_push(list_obj);
			}	break;
			case OP_NEWDICT: {
				obj_p dict_obj = NEW_DICT(param);
				fr_sp -= param;
				for (i=0; i < param; i+=2) {
					dict_add(ist, dict_obj, fr_stack[fr_sp+i], fr_stack[fr_sp+i+1]);
					IF_EXC_BREAK;
				}
				fr_push(dict_obj);
			}	break;
			case OP_NEWSLICE: {
				obj_p ret;
				fr_sp -= 3;
				ret = new_slice_obj(ist, fr_stack[fr_sp], fr_stack[fr_sp+1], fr_stack[fr_sp+2]);
				fr_push(ret);
				break;
			}
			case OP_APPEND: {
				obj_p list = fr_stack[fr_sp+param];
				list_append(ist, list, fr_pop);
			}   break;
			case OP_GEN:
			case OP_DEF: {
				int fparams_len = fr_data_int(1)*2;
				obj_p func = copy_object(ist, fr_data(2));
				fr_sp -= fparams_len+2;
				if (fparams_len) {
					obj_p fparams_obj = NEW_LIST(fparams_len);
					memcpy( ((list_p)(obj_dataptr(fparams_obj)))->item, 
							fr_stack+fr_sp+2, fparams_len*sizeof(obj_p) );
					listlen(fparams_obj) = fparams_len;
					set_attr(ist, func, SYM(FPARAMS_), fparams_obj);
				}
				set_attr(ist, func, SYM(PREVSCOPE_),  frame->locals);
				set_attr(ist, func, SYM(CONT_),       fr_stack[fr_sp]);
				set_attr(ist, func, SYM(NAME_),       fr_stack[fr_sp+1]);
#ifdef DUMP_MODULE_CODE
				if (dump_module_code) {
					char dump_name[256];
					static int cntr = 1;
					apr_snprintf(dump_name, sizeof(dump_name),
						     "!def_code_%s_%d.txt",
						     as_str(ist, fr_stack[fr_sp+1]), cntr++);
					dump_code(ist, func, func->data.ptr, dump_name);
					set_attr(ist, func, SYM(NAME_), NEW_STRING(dump_name));
				}
#endif
				if (op == OP_GEN) {
					gen_p genp      = pr_malloc(sizeof(gen_t));
					genp->frame     = NULL;
					genp->codep     = func->data.ptr;
					func->data.ptr  = NULL;
					switch_proto(ist, func, OBJ(GEN_PROTO));
					func->data.ptr  = genp;
				}
				set_item(ist, fr_stack[fr_sp], fr_stack[fr_sp+1], func, FALSE);
			}	break;
			case OP_DEL:
				fr_sp -= 2;
				del_item(ist, fr_stack[fr_sp], fr_stack[fr_sp+1], TRUE);
				break;
			case OP_BR:
#ifdef TRACE_INTERPRETER
				if (trace_interpreter) trace_step(frame);
#endif
				fr_pc += param;
				continue;
			case OP_BEQ:
				if (fr_tos == fr_data(1)) {
#ifdef TRACE_INTERPRETER
					if (trace_interpreter) trace_step(frame);
#endif
					fr_pc += param;
					continue;
				}
				break;
			case OP_TRYFINALLY: {
				exception_entry_t* exc_entry = pr_malloc(sizeof(exception_entry_t));
				exc_final	= exc_entry->final   = TRUE;
				exc_try_loc	= exc_entry->try_loc = fr_pc;
				exc_exc_loc	= exc_entry->exc_loc = fr_pc+param;
				exstk_push(frame, exc_entry);
				have_exstk = TRUE;
			}	break;
			case OP_TRYEXCEPT: {
				exception_entry_t* exc_entry = pr_malloc(sizeof(exception_entry_t));
				exc_final	= exc_entry->final   = FALSE;
				exc_try_loc	= exc_entry->try_loc = fr_pc;
				exc_exc_loc	= exc_entry->exc_loc = fr_pc+param;
				exstk_push(frame, exc_entry);
				have_exstk = TRUE;
			}	break;
			case OP_EXCEPT: {
				obj_p exc_proto, exc_obj_label;
				if (!exc_waiting) {
					raise_exception(ist, OBJ(INTERNAL_EXC), "EXCEPT op-code without exception");
					break;
				}
				exc_obj_label = fr_tos;
				fr_sp--;
				exc_proto = fr_tos;
				fr_sp--;
				if (has_proto(ist, exc_proto, OBJ(TUPLE_PROTO))) {
					int i, llen = (int) list_len(ist, exc_proto);
					for (i=0; i < llen; i++) {
						if (has_proto(ist, exc_waiting, list_item(ist, exc_proto, i))) goto do_exc;
						IF_EXC_BREAK;
					}
					IF_EXC_BREAK;
					goto op_exc_cont;
				} else if (!has_proto(ist, exc_waiting, exc_proto)) goto op_exc_cont;
				IF_EXC_BREAK;
do_exc:
				if (exc_obj_label && exc_obj_label != OBJ(NONE))
					set_attr(ist, frame->locals, exc_obj_label, exc_waiting);
				exc_waiting = 0;
			}	break;
op_exc_cont:
#ifdef TRACE_INTERPRETER
			if (trace_interpreter) trace_step(frame);
#endif
			fr_pc += param;
			continue;
			case OP_RERAISE:
				ist->exception_obj = exc_waiting;
				exc_waiting = 0;
				free_frame       = exc_free_frame;
				switch_frame     = exc_switch_frame;
				exc_free_frame   = NULL;
				exc_switch_frame = NULL;
				if (exc_final_return) {
					fr_pc = exc_final_return;
					fr_sp = 0;
				}
				break;
			case OP_EXEC: {
				parse_state* state;
				code_p code;
				obj_p str_obj;
				frame_p new_frame;
				str_obj = fr_pop;
				if (!has_proto(ist, str_obj, OBJ(STRING_PROTO))){
					raise_exception(ist, OBJ(TYPE_EXC), "Exec parameter must be a string");
					break;
				}
				state = parse_file_or_string(ist, NULL, pr2c_strptr(ist, str_obj));
				if (!state || !(code = (state->parse_results))) break;
				new_frame = create_frame( ist, frame->self, frame->syn_locals, frame->dyn_locals, 
					                           frame->locals, NULL, code, "execString" );
				new_frame->syn_locals = frame->syn_locals;
				new_frame->prev_frame = frame;
				frame->next_frame     = new_frame;
				switch_frame		  = new_frame;
			}	break;
			case OP_OBJBIND: {
				obj_p bindee, bound_meth;
				fr_sp -= 2;
				if (!(bindee = fr_stack[fr_sp+1])) bindee = frame->self;
				bound_meth = copy_object(ist, fr_stack[fr_sp]);
				set_attr(ist, bound_meth, SYM(BINDOBJ_), bindee); IF_EXC_BREAK;
				fr_push(bound_meth);
			}	break;
			case OP_BIND: {
				obj_p obj, bindee, bound_meth;
				fr_sp -= 3;
				if (!(bindee = fr_stack[fr_sp+2])) bindee = fr_stack[fr_sp];
				obj = get_item(ist, fr_stack[fr_sp], fr_stack[fr_sp+1], TRUE); IF_EXC_BREAK;
				bound_meth = copy_object(ist, obj);
				su(i);
				set_attr(ist, bound_meth, SYM(BINDOBJ_), bindee); 
				un_su(i);
				IF_EXC_BREAK;
				fr_push(bound_meth);
			}	break;
			case OP_CMDCALL:
				cmd_flag = 1;
				goto got_cmd_flag;
			case OP_OBJCALL: {
				cmd_flag = 0;
got_cmd_flag:
				if (frame->fparam_proc_state_idx >= 0) {
					clist_item( frame->fparam_proc_state.lbl_val_list,
						       (frame->fparam_proc_state_idx*2)+1 ) = fr_pop;
				}
				fr_sp -= 2+2*param;
				func_obj = fr_stack[fr_sp];
				if (as_str_chk(ist, fr_stack[fr_sp+1])) {
					fr_push(NEW_STRING("..."));
					goto break_loc;
				}
				if (cmd_flag && !get_attr(ist, func_obj, SYM(COMMAND_))) {
					if (param == 0) {
						fr_sp++;
						goto break_loc;
					}
					raise_exception( ist, OBJ(COMMAND_EXC), "Unknown: %s", 
						                            symch(ist, fr_data(1)) );
					break;
				}
				if (func_obj->data_type == DATA_TYPE_EXTPTR) {
					obj_p res, self;
					clist_p plist;
					if (!func_obj->data.ptr) goto no_func_ptr;
					if ( !(self = fr_stack[fr_sp+1]) &&
						 !(self = get_attr(ist, func_obj, SYM(BINDOBJ_))) &&
						 !(self = frame->self) &&
						 !(self = ist->module) ) {
						 raise_exception(ist, OBJ(INTERNAL_EXC), "no self for this function");
						 break;
					}
					plist = get_func_params(ist, func_obj, param*2, fr_stack+fr_sp+2);
					IF_EXC_BREAK;
					pre_call_lock(ist, self, plist); IF_EXC_BREAK;
					res = ((pr_func*)(func_obj->data.ptr))
								(ist, self, 
								(plist ? clist_len(plist) : 0), 
								(plist ? (obj_p*)plist->items : NULL),
								frame->locals);
					post_call_unlock(ist, self, plist);
					free_clist(plist);
					IF_EXC_BREAK;
					fr_push(res);
				} else if ( func_obj != OBJ(FUNC_PROTO) && 
					        has_proto(ist, func_obj, OBJ(FUNC_PROTO)) ) {
					obj_p self;
					frame_p new_frame;
					IF_EXC_BREAK;
					if (!func_obj->data.ptr) goto no_func_ptr;
					new_frame = get_frame_params( ist, &(frame->fparam_proc_state_idx), 
						                               &(frame->fparam_proc_state), 
									                   &(frame->new_locals), func_obj, param*2, fr_stack+fr_sp+2 );
					IF_EXC_BREAK;
					if (new_frame) {
						fr_next = new_frame;
						new_frame->prev_frame = frame;
						switch_frame = new_frame;
						fr_sp += 2+(2*param);
						fr_push(OBJ(NONE));
						goto break_loc;
					}
					if ( !(self = fr_stack[fr_sp+1]) &&
						 !(self = get_attr(ist, func_obj, SYM(BINDOBJ_))) &&
						 !(self = frame->self) &&
						 !(self = ist->module) ) {
						 raise_exception(ist, OBJ(INTERNAL_EXC), "no self for this function");
						 break;
					}
					IF_EXC_BREAK;
					new_frame = create_frame( ist, self, NULL, frame->locals, 
						                      frame->new_locals, func_obj, NULL, NULL );
					fr_next = new_frame;
					new_frame->prev_frame = frame;
					switch_frame = new_frame;
					fr_push(OBJ(NONE));
				} else if (has_proto(ist, func_obj, OBJ(GEN_PROTO))) {
					gen_p  genp  = func_obj->data.ptr;
					code_p codep = genp->codep;
					frame_p new_frame = get_frame_params( ist, &(frame->fparam_proc_state_idx), 
													   &(frame->fparam_proc_state), 
									                   &(frame->new_locals), func_obj, param*2, fr_stack+fr_sp+2 );
					IF_EXC_BREAK;
					if (new_frame) {
						fr_next = new_frame;
						new_frame->prev_frame = frame;
						switch_frame = new_frame;
						fr_sp += 2+(2*param);
						fr_push(OBJ(NONE));
						goto break_loc;
					}
					genp->frame = create_frame( ist, NULL, NULL, frame->locals,
												frame->new_locals, func_obj, codep, NULL );
					genp->frame->gen_marker = func_obj;
					fr_push(func_obj);
				} else {
					memmove(fr_stack+fr_sp+2, fr_stack+fr_sp+1, (param*2+1) * sizeof(obj_p));
					fr_stack[fr_sp+1] = SYM(CALL_);
					goto get_func;
				}
			}	break;
no_func_ptr:	raise_exception(ist, OBJ(INTERNAL_EXC), "attempt to call an uninitialized function");
				break;

			case OP_PROP:
				goto op_prop;

			case OP_OBJ: {
				int i, num_protos;
				obj_p new_obj, key;
				goto op_obj;

op_prop:		num_protos = 0; 
				fr_sp -= 2;
				new_obj = NEW_OBJ(OBJ(PROP_PROTO));
				goto op_obj_end;

op_obj:			num_protos = (int)(intptr_t) fr_data(2);
				fr_sp -= (num_protos+1)*2;
				if (num_protos == 0) new_obj = NEW_OBJ(NULL);
				else {
					obj_p psym = fr_stack[fr_sp+3];
					obj_p proto = get_item(ist, fr_stack[fr_sp+2], psym, TRUE); IF_EXC_BREAK;
					if (!proto) {
						raise_exception(ist, OBJ(NAME_EXC), "prototype %s not found", as_str(ist, psym));
						goto end_op_obj;
					}
					new_obj = NEW_OBJ(proto);
				}
				for (i=1; i < num_protos; i++) {
					obj_p psym = fr_stack[fr_sp+(i+1)*2+1];
					obj_p proto = get_item(ist, fr_stack[fr_sp+(i+1)*2], psym, TRUE);
					if (ist->exception_obj) goto end_op_obj;
					if (!proto) {
						raise_exception(ist, OBJ(NAME_EXC), "prototype %s not found", as_str(ist, psym));
						goto end_op_obj;
					}
					add_proto(ist, new_obj, proto);
					if (ist->exception_obj) goto end_op_obj;
				}
op_obj_end:
				key = fr_stack[fr_sp+1];
#ifdef DUMP_MODULE_CODE
				if (dump_module_code) dump_code_key = key;
#endif			
				if (has_proto(ist, key, OBJ(LOCAL_PROTO))) {
					key = key->data.ptr;
				}
				set_attr(ist, fr_stack[fr_sp], key, new_obj); IF_EXC_BREAK;
				fr_push(new_obj);
				goto op_with;
			}
			case OP_WITH:
#ifdef DUMP_MODULE_CODE
				dump_code_key = NULL;
op_with:		if (dump_module_code) {
					char dump_name[256];
					static int with_num = 1;
					obj_p func = fr_data(1);
					if (dump_code_key)
						apr_snprintf( dump_name, sizeof(dump_name),
									  "!object_code_%s_%d.txt", 
									  as_str(ist, dump_code_key), with_num++ );
					else
						apr_snprintf( dump_name, sizeof(dump_name),
									"!with_code_%d.txt", with_num++ );
					dump_code(ist, func, func->data.ptr, dump_name);
					set_attr(ist, func, SYM(NAME_), NEW_STRING(dump_name));
				}
#else
op_with:		
#endif
				fr_sp--;
				switch_frame = create_frame( ist, frame->self, frame->locals, frame->dyn_locals, 
					                              fr_stack[fr_sp], fr_data(1), NULL, "with" );
				switch_frame->prev_frame = frame;
				frame->next_frame = switch_frame;
				frame->no_return_flag = TRUE;
end_op_obj:		break;
			case OP_CALL: {
				obj_p name_obj;
				if (frame->fparam_proc_state_idx >= 0) {
					clist_item( frame->fparam_proc_state.lbl_val_list,
						       (frame->fparam_proc_state_idx*2)+1 ) = fr_pop;
				}
				fr_sp -= 3+(2*param);
				name_obj = fr_stack[fr_sp+1];
				if (as_str_chk(ist, name_obj)) {
					fr_push(NEW_STRING("..."));
					goto break_loc;
				}
				if (has_proto(ist, name_obj, OBJ(SUPER_PROTO))) {
					obj_p cont = get_attr(ist, name_obj, SYM(CONT_));
					name_obj = name_obj->data.ptr;
					if (!cont) {
						raise_exception( ist, OBJ(INTERNAL_EXC), "container not found for attribute %s", 
							                                      symch(ist, name_obj) );
						break;
					}
			 		func_obj = get_proto_attr(ist, fr_stack[fr_sp], name_obj, NULL, cont);
					goto got_func;
				}
				if (has_proto(ist, name_obj, OBJ(LOCAL_PROTO))) {
					name_obj = name_obj->data.ptr;
			 		func_obj = get_scope_attr(ist, fr_stack[fr_sp], name_obj, NULL, NULL);
					goto got_func;
				}
				if (has_proto(ist, name_obj, OBJ(OUTER_PROTO))) {
					name_obj = name_obj->data.ptr;
			 		func_obj = get_scope_attr(ist, fr_stack[fr_sp], name_obj, NULL, fr_stack[fr_sp]);
					goto got_func;
				}
				if (!has_proto(ist, name_obj, OBJ(SYMBOL_PROTO))) {
					raise_exception(ist, OBJ(INTERNAL_EXC), "Indexed function call not supported yet");
					break;
				}
				if (fr_stack[fr_sp+1] == SYM(NEXT) && has_proto(ist, fr_stack[fr_sp], OBJ(GEN_PROTO))) {
					gen_p genp = fr_stack[fr_sp]->data.ptr;
					frame_p new_frame = genp->frame;
					if (!new_frame) {
						raise_exception(ist, OBJ(TYPE_EXC), "iterator not initialized");
						break;
					}
					new_frame->prev_frame = frame;
					frame->next_frame = new_frame;
					switch_frame = new_frame;
					fr_push(OBJ(NONE));
					break;
				}
				IF_EXC_BREAK;
get_func:
				if (!fr_stack[fr_sp]) {
					raise_exception(ist, OBJ(INTERPRETER_EXC), "reference to non-existant self object");
					break;
				}
				func_obj = get_proto_attr(ist, fr_stack[fr_sp], fr_stack[fr_sp+1], NULL, NULL);
got_func:
				IF_EXC_BREAK;
				if (!func_obj) {
					raise_exception(ist, OBJ(FUNCNOTFOUND_EXC),
							"Function %s not found",
							as_str(ist, fr_stack[fr_sp+1]));
					break;
				}
				if (func_obj->data_type == DATA_TYPE_EXTPTR){
					clist_p plist;
					obj_p res, self;
					if (!func_obj->data.ptr) goto no_func_ptr2;
					plist = get_func_params(ist, func_obj, param*2, fr_stack+fr_sp+3);
					IF_EXC_BREAK;
					if ( !(self = fr_stack[fr_sp+2]) &&
						 !(self = get_attr(ist, func_obj, SYM(BINDOBJ_))) &&
						 !(self = fr_stack[fr_sp]) &&
						 !(self = frame->self) &&
						 !(self = ist->module) ) {
						 raise_exception(ist, OBJ(INTERNAL_EXC), "no self for this function");
						 break;
					}
					IF_EXC_BREAK;
					pre_call_lock(ist, self, plist);  IF_EXC_BREAK;
					pr_assert(sizeof(obj_p) == sizeof(clist_item_t));
					res = ((pr_func*)(func_obj->data.ptr))
								(ist, self, 
								(plist ? clist_len(plist) : 0), 
								(plist ? (obj_p*)plist->items : NULL),
								frame->dyn_locals);
					post_call_unlock(ist, self, plist);
					free_clist(plist);
					IF_EXC_BREAK;
					fr_push(res);
				} else if ( func_obj != OBJ(FUNC_PROTO) && 
					        has_proto(ist, func_obj, OBJ(FUNC_PROTO)) ) {
					obj_p self;
					frame_p new_frame;
					IF_EXC_BREAK;
					if (!func_obj->data.ptr) goto no_func_ptr2;
					new_frame = get_frame_params( ist, &(frame->fparam_proc_state_idx), 
													   &(frame->fparam_proc_state), 
									                   &(frame->new_locals), func_obj, param*2, fr_stack+fr_sp+3);
					IF_EXC_BREAK;
					if (new_frame) {
						fr_next = new_frame;
						new_frame->prev_frame = frame;
						switch_frame = new_frame;
						fr_sp += 3+(2*param);
						fr_push(OBJ(NONE));
						goto break_loc;
					}
					if ( !(self = fr_stack[fr_sp+2]) &&
						 !(self = get_attr(ist, func_obj, SYM(BINDOBJ_))) &&
						 !(self = fr_stack[fr_sp]) &&
						 !(self = frame->self) &&
						 !(self = ist->module) ) {
						 raise_exception(ist, OBJ(INTERNAL_EXC), "no self for this function");
						 break;
					}
					IF_EXC_BREAK;
					new_frame = create_frame( ist, self, NULL, frame->locals, 
						                           frame->new_locals, func_obj, NULL, NULL );
					fr_next = new_frame;
					new_frame->prev_frame = frame;
					switch_frame = new_frame;
					fr_push(OBJ(NONE));
				} else if (has_proto(ist, func_obj, OBJ(GEN_PROTO))) {
					gen_p  genp  = func_obj->data.ptr;
					code_p codep = genp->codep;
					frame_p new_frame;
					IF_EXC_BREAK;
					genp->frame->gen_marker = func_obj;
					frame->next_frame = genp->frame;
					new_frame = get_frame_params( ist, &(frame->fparam_proc_state_idx), 
													   &(frame->fparam_proc_state), 
									                   &(frame->new_locals), func_obj, param*2, fr_stack+fr_sp+3);
					IF_EXC_BREAK;
					if (new_frame) {
						fr_next = new_frame;
						new_frame->prev_frame = frame;
						switch_frame = new_frame;
						fr_sp += 3+(2*param);
						fr_push(OBJ(NONE));
						goto break_loc;
					}
					genp->frame = create_frame( ist, NULL, NULL, frame->locals,
												frame->new_locals, func_obj, codep, NULL );
					fr_push(func_obj);
				} else {
					obj_p value;
					if (fr_stack[fr_sp+1] != SYM(CALL_)) {
						value = get_proto_attr(ist, fr_stack[fr_sp],
								fr_stack[fr_sp+1], NULL, NULL);
						IF_EXC_BREAK;
						if (value) {
							fr_stack[fr_sp]   = value;
							fr_stack[fr_sp+1] = SYM(CALL_);
							name_obj = NULL;
							goto get_func;
						}
					}
					raise_exception(ist, OBJ(FUNCNOTFOUND_EXC),
							"Function not found: \"%s\"",
							as_str(ist, fr_stack[fr_sp+1]));
				}
			}	break;
no_func_ptr2:	raise_exception(ist, OBJ(INTERNAL_EXC), "attempt to call an uninitialized function");
				break;
			case OP_RAISE: {
				obj_p param2  = fr_pop;
				obj_p exc_obj = fr_pop;
				if (param2 != OBJ(NONE)) {
					int i;
					exc_obj = new_object(ist, exc_obj);
					su(i);
					set_attr(ist, exc_obj, SYM(DOC_), call_func0(ist, param2, SYM(STR_)));
					un_su(i);
				}
				ist->exception_obj = exc_obj;
			}	break;
			case OP_YIELD: {
				frame_p fp;
				for (fp=frame; fp && !(fp->gen_marker); fp=fp->prev_frame);
				if (!fp)
					raise_exception(ist, OBJ(INTERPRETER_EXC), "Yield without enclosing gen operator");
				else {
					switch_frame = fp->prev_frame;
					switch_frame->next_frame = NULL;
					switch_frame->stack[switch_frame->stack_ptr-1] = return_value = fr_pop;
				}
				return_flag = fr_ccall;
			}	break;
			case OP_RETURN:
				return_value = do_return(ist, frame, &switch_frame, &free_frame, return_value);
				IF_EXC_BREAK;
				return_flag = fr_ccall;
				break;
			case OP_IMPORT:    
				import_module(ist, frame, param, TRUE, NULL, FALSE);
				break;
			case OP_IMPORT_AS:    
				import_module(ist, frame, param-1, TRUE, fr_data(param-1), FALSE);
				break;
			case OP_IMPORT_PUSH: 
				fr_push(import_module(ist, frame, param, FALSE, NULL, TRUE));
				break;
			case OP_IMPORT_ATTR: {
				obj_p obj;
				if (param > 2) {
					raise_exception( ist, OBJ(INTERPRETER_EXC), 
									 "Attribute paths not allowed here" );
					break;
				}
				obj = get_attr(ist, fr_tos, fr_data(1));
				if (!obj) {
					raise_exception(ist, OBJ(INTERPRETER_EXC),
							"Attribute not found in module %s",
							symch(ist, fr_data(1)));
					break;
				}
				if (!ist->module){
					raise_exception( ist, OBJ(INTERPRETER_EXC),
							              "No main module set for importing" );
					break;
				}
				su(i);
				set_attr(ist, ist->module, fr_data(1), obj);
				un_su(i);
			}	break;
			case OP_IMPORT_ATTR_AS: {
				obj_p obj = fr_tos;
				for(i=1; i < param-1; i++) {
					obj = get_attr(ist, obj, fr_data(i));
					if (!obj) {
						raise_exception(ist, OBJ(INTERPRETER_EXC),
								"Attribute not found in module %s",
								symch(ist, fr_data(i)));
						break;
					}
				}
				if (!ist->module){
					raise_exception( ist, OBJ(INTERPRETER_EXC),
							              "No main module set for importing" );
					break;
				}
				su(i);
				set_attr(ist, ist->module, fr_data(param-1), obj);
				un_su(i);
			}	break;
			default: {
				raise_exception(ist, OBJ(INTERNAL_EXC), "Bad opcode: %d", op);
			}
		}
break_loc:

#ifdef TRACE_INTERPRETER
		if (trace_interpreter) trace_step(frame);
#endif
		if (!intrp_exobj && frame->fparam_proc_state_idx == -1) {
			if (op < OP_PARAM_WORDS_BOUNDARY)
				fr_pc += param;
			else if (op < OP_TWO_WORDS_BOUNDARY)
				fr_pc += 2;
			else
				fr_pc++;
		}
endop:
		if (intrp_exobj) {
			int frame_chg = FALSE;
			int last_called_from_c = FALSE;

#ifdef TRACE_INTERPRETER
			if (trace_interpreter) {
				fprintf(tfout, "---exception---\n");
				fflush(tfout);
			}
#endif
			add_frame_to_exception(ist, frame);
			while (!have_exstk && frame->prev_frame && !last_called_from_c) {
				frame_p tmp_frame = frame;
				last_called_from_c = frame->called_from_c;
				frame = ist->frame = frame->prev_frame;
#ifdef TRACE_INTERPRETER
				if (trace_interpreter) {
					fprintf(tfout, "--- delete top frame %8x\n", (intptr_t) tmp_frame);
					fprintf(tfout, "--- new top frame is %8x, %s\n", (intptr_t) frame, frame->func_name);
					fflush(tfout);
				}
#endif
				stack_lim = frame->code->max_stack_depth;
				// free_frame(tmp_frame);  XXX
				if (tmp_frame->gen_marker) {
					gen_p genp = tmp_frame->gen_marker->data.ptr;
					genp->frame = NULL;
				}
				if (!tmp_frame->do_not_free)
					pr_free(tmp_frame);
				frame->next_frame = NULL;
				have_exstk = (fr_exstk && clist_len(fr_exstk));
				frame_chg = TRUE;
				add_frame_to_exception(ist, frame);
			}
			if (have_exstk) {
				if (frame_chg) {
					exception_entry_t* p = (exception_entry_t*)(clist_tos(fr_exstk));
					exc_exc_loc	= p->exc_loc;
					frame_chg = FALSE;
				}
				fr_pc = exc_exc_loc;
				pr_free(clist_pop(fr_exstk));
				if (clist_len(fr_exstk)) {
					exception_entry_t* p = (exception_entry_t*)(clist_tos(fr_exstk));
					exc_final   = p->final;
					exc_try_loc	= p->try_loc;
					exc_exc_loc	= p->exc_loc;
				} else have_exstk = FALSE;
				exc_waiting = intrp_exobj;
				ist->exception_obj = 0;
				continue;
			}
			return OBJ(NONE);
		}
		if (free_frame && have_exstk) {
			exc_free_frame   = free_frame;
			exc_switch_frame = switch_frame;
			free_frame   = NULL;
			switch_frame = NULL;
			fr_pc = exc_exc_loc;
			pr_free(clist_pop(fr_exstk));
			if (clist_len(fr_exstk)) {
				exception_entry_t* p = (exception_entry_t*)(clist_tos(fr_exstk));
				exc_final   = p->final;
				exc_try_loc	= p->try_loc;
				exc_exc_loc	= p->exc_loc;
			} else have_exstk = FALSE;
		}
		if (free_frame) {
			// free_frame(tmp_frame);  XXX
			if (!free_frame->do_not_free)
				pr_free(free_frame);
		}
		if (switch_frame) {
			str_chk_clr(ist, switch_frame);
			while(switch_frame != DONE_FRAME_FLAG && switch_frame->next_frame) 
				switch_frame = switch_frame->next_frame;
			frame = ist->frame = switch_frame;
#ifdef TRACE_INTERPRETER
			if (trace_interpreter) {
				if (frame != DONE_FRAME_FLAG)
					fprintf(tfout, "--- switch to frame %8x, %s\n", (intptr_t) frame, frame->func_name);
				fflush(tfout);
			}
#endif
			if (frame != DONE_FRAME_FLAG)
				stack_lim = frame->code->max_stack_depth;
			if ((have_exstk = (frame != DONE_FRAME_FLAG && fr_exstk && clist_len(fr_exstk)))) {
				exception_entry_t* p = (exception_entry_t*)(clist_tos(fr_exstk));
				exc_final   = p->final;
				exc_try_loc	= p->try_loc;
				exc_exc_loc	= p->exc_loc;
			}
		}
		if (return_flag || frame == DONE_FRAME_FLAG) {
#ifdef TRACE_INTERPRETER
			if (trace_interpreter) {
				fprintf(tfout, "--- exiting exec_loop, value: %8x\n", (intptr_t) return_value);
				fflush(tfout);
			}
#endif
			if (frame == DONE_FRAME_FLAG) frame = ist->frame = NULL;
			return return_value;
		}
		switch_frame = free_frame = NULL;
	}
}

//******************************** call_func **********************************
obj_p call_func( isp ist, obj_p cont_self, obj_p func_sym, obj_p self,
				 int parm_cnt, obj_p* lbl_val_arr, obj_p super_obj ) {
	obj_p func_obj, res;
	frame_p new_frame;
	if (!self) self = cont_self;
	if (!self) self = NEW_OBJ(NULL);
	if (!cont_self) {
		func_obj = func_sym; 
	} else {
		if (as_str_chk(ist, func_sym))
			return NEW_STRING("...");
		func_obj = get_proto_attr(ist, cont_self, func_sym, NULL, super_obj); if_exc_return NULL;
		if (!func_obj) {
			if (!ist) {
				printf("Internal error with exceptions disabled: Function not found: \"%s\"", 
					                                             as_str(ist, func_sym));
				pr_exit(1);
			}
			if (func_sym == SYM(NEXT) && has_proto(ist, self, OBJ(GEN_PROTO))) {
					gen_p genp = self->data.ptr;
					new_frame = genp->frame;
					if (!new_frame) {
						raise_exception(ist, OBJ(STOP_ITERATION_EXC), NULL);
						str_chk_clr(ist, ist->frame);
						return NULL;
					}
					goto gotnewframe;
			}
			raise_exception(ist, OBJ(FUNCNOTFOUND_EXC), "Function not found: \"%s\"",
					                                     as_str(ist, func_sym));
			str_chk_clr(ist, ist->frame);
			return NULL;
		}
	}
	if (func_obj->data_type == DATA_TYPE_EXTPTR) {
		obj_p res, aself;
		clist_p plist = get_func_params( ist, func_obj, parm_cnt, lbl_val_arr);
		if (! (aself = get_attr(ist, func_obj, SYM(BINDOBJ_))) )
			aself = self;     if_exc_return 0;
		pre_call_lock(ist, aself, plist); if (intrp_exobj) { str_chk_clr(ist, ist->frame); return NULL; }
		res = ((pr_func*)(func_obj->data.ptr)) 
								(ist, aself, (plist ? clist_len(plist) : 0), 
								(plist ? (obj_p*)plist->items : NULL), super_obj);
		post_call_unlock(ist, aself, plist);
		free_clist(plist);
		if (intrp_exobj) { str_chk_clr(ist, ist->frame); return NULL; }
		str_chk_clr(ist, ist->frame);
		return res;
	} else if ( func_obj != OBJ(FUNC_PROTO) && 
		        has_proto(ist, func_obj, OBJ(FUNC_PROTO)) ) {
		frame_p frame;
		obj_p new_locals, aself; 
		int idx = -1;
		fparam_proc_state_t fps;

		if (!ist) {
			raise_exception(ist, OBJ(INTERNAL_EXC), "Illegal function call to Prothon function");
			str_chk_clr(ist, ist->frame);
			return 0;
		}
		if (intrp_exobj) {
			str_chk_clr(ist, ist->frame);
			return 0;
		}
		frame = ist->frame;
		if (intrp_exobj) {
			str_chk_clr(ist, ist->frame);
			return 0;
		}
		if (! (aself = get_attr(ist, func_obj, SYM(BINDOBJ_))) )
			aself = self;   if_exc_return 0;
		if (get_frame_params(ist, &idx, &fps, &new_locals, func_obj, parm_cnt, lbl_val_arr)) {
			raise_exception(ist, OBJ(INTERNAL_EXC), "default values not allowed in call_func");
			str_chk_clr(ist, ist->frame);
			return NULL;
		}
		new_frame = create_frame( ist, aself, NULL, NULL, 
			                      new_locals, func_obj, NULL, NULL );
gotnewframe:
		new_frame->called_from_c = TRUE;
		ist->frame->next_frame = new_frame;
		new_frame->prev_frame = ist->frame;
		ist->frame = new_frame;
		ist->frame->called_from_c = TRUE;
		if (intrp_exobj) {
			str_chk_clr(ist, ist->frame);
			return 0;
		}
		res = exec_loop(ist);
		str_chk_clr(ist, ist->frame);
		return res;
	} else {
		raise_exception(ist, OBJ(FUNCNOTFOUND_EXC), "Function not found");
		str_chk_clr(ist, ist->frame);
		return 0;
	}
}

//******************************** check_exceptions ***************************
obj_p check_exceptions(isp ist)
{
	int i;
	obj_p bl, fstk_obj, orig_excobj = intrp_exobj;

	if (!intrp_exobj)
		return NULL;

	ist->exception_obj = 0;
	bl = proto_list(ist, orig_excobj);

	printf("\nUncaught exception:\n");

	if ((fstk_obj = get_attr(ist, orig_excobj, sym(ist, "frame_stack")))) {
		for (i = (int) list_len(ist, fstk_obj) - 3; i >= 0; i -= 3) {
			printf( "--- File: %s, line: %"APR_INT64_T_FMT", char: %"APR_INT64_T_FMT"\n",
				pr2c_strptr(ist, list_item(ist, fstk_obj, i  )), 
				*((i64_t *)obj_dataptr(list_item(ist, fstk_obj, i+1))), 
				*((i64_t *)obj_dataptr(list_item(ist, fstk_obj, i+2))) );
		}
	}

	if (bl) {
		char *p, buf[256], buf2[80]; 
		buf[0] = 0;
		for (i = (int) list_len(ist, bl) - 1; i >= 0; i--){
			obj_p exc = list_item(ist, bl, i);
			if (exc && exc != OBJ(EXCEPTION) && has_proto(ist, exc, OBJ(EXCEPTION))) {
				obj_p doc = get_attr(ist, exc, SYM(DOC_));
				if (doc && i > 0) {
					apr_snprintf(buf2, sizeof(buf2), "%s, ", pr2c_strptr(ist, doc));
					if (strlen(buf)+strlen(buf2) < sizeof(buf)) strcat(buf, buf2);
				} else if (doc) {
					apr_snprintf(buf2, sizeof(buf2), "%s.", pr2c_strptr(ist, doc));
					if (strlen(buf)+strlen(buf2) < sizeof(buf)) strcat(buf, buf2);
				} else if (i == 0) {
					for (p=buf+strlen(buf)-1; p > buf && *p != ','; p--);
					*p = '.'; *(p+1) = 0;
				}
			}
		}
		printf("%s\n\n",buf);
	}
	return orig_excobj;
}

//******************************** exec_string ********************************
obj_p exec_string(isp ist, char* str, int get_frame, frame_p *framep) {
	code_p code;
	obj_p new_locals;
	frame_p frame, new_frame;
	parse_state* state;

	state = parse_file_or_string(ist, NULL, str);  if_exc_return NULL;
	if (!state || !(code = (state->parse_results))) return NULL;

#ifdef DUMP_MODULE_CODE
	if (dump_module_code) {
		char name[256], buf[65], *p, *d, *e = buf+64;
		for (p = str, d = buf; *p && d < e; p++) {
			if (*p >= 'A' && *p <= 'Z') *(d++) = *p;
			if (*p >= 'a' && *p <= 'z') *(d++) = *p;
		}
		*d = 0;
		apr_snprintf(name, sizeof(name), "!exec_code_%s.txt", buf);
		dump_code(ist, NULL, code, name);
	}
#endif
	if (framep && *framep) {
		frame = *framep;
		new_frame = create_frame( ist, frame->self, frame->syn_locals, frame->dyn_locals, 
									   frame->locals, NULL, code, "execString" );
		pr_free(*framep);
		frame = NULL;
	} else {
		frame = ist->frame;
		if (frame)
			new_frame = create_frame( ist, frame->self, frame->syn_locals, frame->dyn_locals, 
			                               frame->locals, NULL, code, "execString" );
		else {
			obj_p new_globals = NEW_OBJ(NULL);
			new_locals = NEW_OBJ(NULL);
			new_frame = create_frame( ist, new_globals, new_locals, new_locals, 
				                           new_locals, NULL, code, "execString" );
		}
	}
	new_frame->called_from_c = TRUE;
	new_frame->do_not_free   = get_frame;
	if (frame) ist->frame->next_frame = new_frame;
	new_frame->prev_frame = ist->frame;
	ist->frame = new_frame;
	if (get_frame) *framep = new_frame;
	return exec_loop(ist);
}

//******************************** load_module ********************************
int load_module( isp ist, obj_p module_name, obj_p dyn_locals, 
				 obj_p dest_module, obj_p module_alias, char* filename, char* comment,
				 obj_p module, char* exec_string ) {
	obj_p thread;
	frame_p frame, new_frame;
	parse_state* state;
	char full_doc[1024], *name, *frame_name;
	code_p code;
	int i;

	if (!module)
		module = get_attr(ist, OBJ(MODULES), module_name);
	if (!module)
		module = NEW_OBJ(NULL);
	ist->module = module;
	su(i);
	set_attr(ist, OBJ(MODULES), module_name, module);
	un_su(i);
	if (exec_string)
		state = parse_file_or_string(ist, NULL, exec_string);
	else
		state = parse_file_or_string(ist, filename, NULL);
	catch_exception(ist, OBJ(FILENOTFOUND_EXC), NULL);  if_exc_return FALSE;

	if (!state || !(code = (state->parse_results)))
		return FALSE;

	name = symch(ist, module_name);

#ifdef DUMP_MODULE_CODE
	if (dump_module_code) {
		char dump_name[256];
		apr_snprintf(dump_name, sizeof(dump_name), "!module_code_%s.txt", name);
		dump_code(ist, NULL, code, dump_name);
	}
#endif
	apr_snprintf(full_doc, sizeof(full_doc), "%s%s", name, comment);
	set_obj_doc(module, full_doc);
	frame = ist->frame;
	if (filename)
		frame_name = filename;
	else
		frame_name = "execString";
	new_frame = create_frame( ist, NULL, NULL, NULL, module, NULL, code, frame_name );
	new_frame->called_from_c = TRUE;
	if (frame) ist->frame->next_frame = new_frame;
	new_frame->prev_frame = ist->frame;
	ist->frame = new_frame;
	if (module && (thread = get_attr(ist, module, sym(ist, "thread"))) && thread_saveframe(thread)) {
		new_frame->do_not_free = TRUE;
		thread_frame(thread) = new_frame;
	}
	if_exc_return FALSE;
	exec_loop(ist); if_exc_return FALSE;
	if (dest_module){
		if (!module_alias) module_alias = module_name;
		su(i);
		set_attr(ist, dest_module, module_alias, module);
		un_su(i);
	}
	return TRUE;
}

//******************************** main_thread ***********************************
void *main_thread(apr_thread_t *handle, void *argv){
	isp ist = new_ist(ACC_USER2);
	int i, main_index=0;
	obj_p main_sym, module, thread_obj;
	pr_thread_p thread_p;
	char* filename = pr2c_strptr(ist, list_item(ist, (obj_p)argv, 0));

#ifdef DEBUG_THREADS
	if (debug_threads) printf("Starting a main thread\n");
#endif

	thread_obj = register_thread(handle);
	thread_p = thread_obj->data.ptr;
	ist = thread_p->ist;

	pr_lock(&start_main_lock);
	
	main_threads_started++;
	main_threads_running++;

	while (TRUE) {
		char name[16];
		if (main_index)
			apr_snprintf(name, sizeof(name), "Main%d", main_index);
		else
			strcpy(name, "Main");
		main_sym = sym(ist, name);
		if (!get_attr(ist, OBJ(MODULES), main_sym)) {
			module = NEW_OBJ(OBJ(ROOT_GLOBALS));
			su(i);
			set_attr(ist, OBJ(MODULES), main_sym, module);
			set_attr(ist, OBJ(OBJECT), main_sym, module);
			set_attr(ist, thread_obj, sym(ist, "name"), NEW_STRING(name));
			set_attr(ist, module, sym(ist, "thread"), thread_obj);
			un_su(i);
			thread_p->main_id = main_index;
			break;
		}
		main_index++;
	}
	pr_unlock(&start_main_lock);

#ifdef DEBUG_THREADS
	if (debug_threads) {
		printf("%s:%d thread has started\n", symch(ist, main_sym), main_index);
		SetThreadName(symch(ist, main_sym));
	}
#endif

	set_attr(ist, module, sym(ist, "argv_"), (obj_p) argv);
	if (!load_module(ist, main_sym, NULL, NULL, NULL, filename, 
									":a top-level module", module, NULL)) {
		if (!intrp_exobj) {
			printf("File %s not found, execution aborted\n", filename);
			pr_unlock(&start_main_lock);
			return NULL;
		}
	}
	check_exceptions(ist);

	main_threads_running--;
	thread_p->running = FALSE;

#ifdef DEBUG_THREADS
	if (debug_threads) printf("%s:%d thread has terminated\n", symch(ist, main_sym), main_index);
#endif

	return NULL;
}

// ***************************** User Thread **********************************
#define MAX_NUM_PARAMS	64
void* user_thread(apr_thread_t *handle, void *argv) {
	user_thread_p uthread = (user_thread_p) argv;
	obj_p thread_module, func_obj = uthread->func;
	frame_p new_frame;
	obj_p new_locals, lbl_val_arr[MAX_NUM_PARAMS]; 
	int i, parm_cnt;
	obj_p thread_obj = register_thread(handle);
	pr_thread_p thread_p = thread_obj->data.ptr;
	isp ist = thread_p->ist;
	int idx = -1;
	fparam_proc_state_t fps;

	parm_cnt = (int) list_len(ist, uthread->params) * 2;
	if (parm_cnt > MAX_NUM_PARAMS) {
		printf("User thread error: too many user thread parameters");
		thread_p->running = FALSE;
		return NULL;
	}
	for (i=0; i < parm_cnt; i += 2) {
		lbl_val_arr[i]   = OBJ(NOKEY);
		lbl_val_arr[i+1] = list_item(ist, uthread->params, i/2);
	}
	thread_module = NEW_OBJ(NULL);
	while ((new_frame = get_frame_params( ist, &idx, &fps, &new_locals, func_obj, parm_cnt, lbl_val_arr))) {
		new_frame->called_from_c = TRUE;
		new_frame->prev_frame = NULL;
		ist->frame = new_frame;
		clist_item(fps.lbl_val_list, (idx*2)+1) = exec_loop(ist);
		if (ist->exception_obj) goto done;
	}
	new_frame = create_frame(ist, thread_module, thread_module, NULL, new_locals, func_obj, NULL , NULL);
	new_frame->called_from_c = TRUE;
	new_frame->prev_frame = NULL;
	if (intrp_exobj) {
		printf("User thread exception");
		check_exceptions(ist);
		thread_p->running = FALSE;
		return NULL;
	}
	ist->frame = new_frame;
	exec_loop(ist);
done:
	check_exceptions(ist);
	thread_p->running = FALSE;
	return NULL;
}

#define CODE_DATA_STR_SIZE	1024

//******************************** codedata ***********************************
static char* codedata(char* str, code_p code, int op, int param, int pc)
{
	int i;

	*str = 0;

	if (op < OP_PARAM_WORDS_BOUNDARY) {
		for (i = 0; i < param - 1; i++)
			apr_snprintf(str, CODE_DATA_STR_SIZE, "%s %x", str,
				     (unsigned)(uintptr_t)code->code_data[pc+1+i].data);
	} else if (op < OP_TWO_WORDS_BOUNDARY) {
		apr_snprintf(str, CODE_DATA_STR_SIZE, " %x",
			     (unsigned)(uintptr_t)code->code_data[pc+1].data);
	}

	return str;
}

//******************************** prt_code_line ******************************
static int prt_code_line(frame_p frame, code_p code, int pc, FILE* fout)
{
	char str[CODE_DATA_STR_SIZE];
	opcode_t op;
	int param;

	op    = code->code_data[pc].bytecode.opcode;
	param = code->code_data[pc].bytecode.param;

	if (param < -1000000)
		param = 0;
	if (frame)
		fprintf(fout, "%x ", (unsigned)(uintptr_t)frame);

	switch (op) {
	case OP_NOP: 
		pr_assert(param);
		fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_NOP", param, codedata(str, code, op, param, pc));
		break;
	case OP_CLRSP: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_CLRSP", param, codedata(str, code, op, param, pc));
		break;
	case OP_IMPORT: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_IMPORT", param, codedata(str, code, op, param, pc));
		break;
	case OP_IMPORT_AS: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_IMPORT_AS", param, codedata(str, code, op, param, pc));
		break;
	case OP_IMPORT_PUSH: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_IMPORT_PUSH", param, codedata(str, code, op, param, pc));
		break;
	case OP_IMPORT_ATTR: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_IMPORT_ATTR", param, codedata(str, code, op, param, pc));
		break;
	case OP_IMPORT_ATTR_AS: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_IMPORT_ATTR_AS", param, codedata(str, code, op, param, pc));
		break;
	case OP_PUSH: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_PUSH", param, codedata(str, code, op, param, pc));
		break;
	case OP_CHAIN_CMP: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_CHAIN_CMP", param, codedata(str, code, op, param, pc));
		break;
	case OP_SWAP: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_SWAP", param, codedata(str, code, op, param, pc));
		break;
	case OP_DEF: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_DEF", param, codedata(str, code, op, param, pc));
		break;
	case OP_PUSH_SELF: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_PUSH_SELF", param, codedata(str, code, op, param, pc));
		break;
	case OP_PUSH_CALLER: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_PUSH_CALLER", param, codedata(str, code, op, param, pc));
		break;
	case OP_PUSH_LOCAL_REF: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_PUSH_LOCAL_REF", param, codedata(str, code, op, param, pc));
		break;
	case OP_PUSH_SELF_REF: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_PUSH_SELF_REF", param, codedata(str, code, op, param, pc));
		break;
	case OP_PUSH_OUTER_REF: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_PUSH_OUTER_REF", param, codedata(str, code, op, param, pc));
		break;
	case OP_PUSH_CALLER_REF: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_PUSH_CALLER_REF", param, codedata(str, code, op, param, pc));
		break;
	case OP_BEQ: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_BEQ", param, codedata(str, code, op, param, pc));
		break;
	case OP_BREAK: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_BREAK", param, codedata(str, code, op, param, pc));
		break;
	case OP_CONTINUE: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_CONTINUE", param, codedata(str, code, op, param, pc));
		break;
	case OP_POP: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_POP", param, codedata(str, code, op, param, pc));
		break;
	case OP_DUPLICATE: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_DUPLICATE", param, codedata(str, code, op, param, pc));
		break;
	case OP_GEN: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_GEN", param, codedata(str, code, op, param, pc));
		break;
	case OP_OBJ: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_OBJ", param, codedata(str, code, op, param, pc));
		break;
	case OP_PROP: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_PROP", param, codedata(str, code, op, param, pc));
		break;
	case OP_WITH: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_WITH", param, codedata(str, code, op, param, pc));
		break;
	case OP_OBJCALL: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_OBJCALL", param, codedata(str, code, op, param, pc));
		break;
	case OP_CMDCALL: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_CMDCALL", param, codedata(str, code, op, param, pc));
		break;
	case OP_CALL: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_CALL", param, codedata(str, code, op, param, pc));
		break;
	case OP_OBJBIND: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_OBJBIND", param, codedata(str, code, op, param, pc));
		break;
	case OP_BIND: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_BIND", param, codedata(str, code, op, param, pc));
		break;
	case OP_ASSIGN: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_ASSIGN", param, codedata(str, code, op, param, pc));
		break;
	case OP_ASSIGN_EXP: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_ASSIGN_EXP", param, codedata(str, code, op, param, pc));
		break;
	case OP_SEQ_ASSIGN: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_SEQ_ASSIGN", param, codedata(str, code, op, param, pc));
		break;
	case OP_BR: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_BR", param, codedata(str, code, op, param, pc));
		break;
	case OP_NEWTUPLE: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_NEWTUPLE", param, codedata(str, code, op, param, pc));
		break;
	case OP_NEWLIST: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_NEWLIST", param, codedata(str, code, op, param, pc));
		break;
	case OP_NEWDICT: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_NEWDICT", param, codedata(str, code, op, param, pc));
		break;
	case OP_NEWSLICE: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_NEWSLICE", param, codedata(str, code, op, param, pc));
		break;
	case OP_APPEND: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_APPEND", param, codedata(str, code, op, param, pc));
		break;
	case OP_TRYEXCEPT: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_TRYEXCEPT", param, codedata(str, code, op, param, pc));
		break;
	case OP_EXCEPT: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_EXCEPT", param, codedata(str, code, op, param, pc));
		break;
	case OP_RERAISE: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_RERAISE", param, codedata(str, code, op, param, pc));
		break;
	case OP_TRYFINALLY: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_TRYFINALLY", param, codedata(str, code, op, param, pc));
		break;
	case OP_EXEC: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_EXEC", param, codedata(str, code, op, param, pc));
		break;
	case OP_DEREF: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_DEREF", param, codedata(str, code, op, param, pc));
		break;
	case OP_RAISE: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_RAISE", param, codedata(str, code, op, param, pc));
		break;
	case OP_DEL: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_DEL", param, codedata(str, code, op, param, pc));
		break;
	case OP_YIELD: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_YIELD", param, codedata(str, code, op, param, pc));
		break;
	case OP_RETURN: fprintf(fout,"%4d %20s(%4d) %s", pc, "OP_RETURN", param, codedata(str, code, op, param, pc));
		break;
	default:
		break;
	}
	fflush(fout);
	if (op < OP_PARAM_WORDS_BOUNDARY)
		return pc + param;
	else if (op < OP_TWO_WORDS_BOUNDARY)
		return pc + 2;
	else
		return pc + 1;
}
//******************************** dump_code **********************************
void dump_code(isp ist, obj_p func, code_p code, char* filename){
	int i, pc=0;
	obj_p lbl_value_list;
	FILE* fout;
	fout = fopen(filename, "w");

	fprintf(fout,"\nCode dump\n");
	fprintf(fout,  "---------\n");
	fprintf(fout,"len:             %4d\n", code->len);
	fprintf(fout,"stack_depth:     %4d\n", code->stack_depth);
	fprintf(fout,"max_stack_depth: %4d\n", code->max_stack_depth);
	fprintf(fout,"assign_parm_cnt:      %4d\n", code->assign_parm_cnt);
	if (code->file_path)
		fprintf(fout,"file_path: %s\n\n",  code->file_path);
	if (func && (lbl_value_list = get_attr(ist, func, SYM(FPARAMS_)))) {
		int llen = (int) list_len(ist, lbl_value_list);
		if (llen) fprintf(fout,"\nFormal Params:\n");
		for (i=0; i < llen; i += 2) {
			char *lbl_str = NULL, *val_str = NULL, buf[10];
			obj_p lbl = list_item(ist, lbl_value_list, i);
			obj_p val = list_item(ist, lbl_value_list, i+1);
			if (lbl) {
				if      ((intptr_t) lbl == 1) lbl_str = "*";
				else if ((intptr_t) lbl == 2) lbl_str = "**";
				else if ((intptr_t) lbl < 10) {
					apr_snprintf(buf, sizeof(buf), "<lbl:%d>", (intptr_t) lbl);
					lbl_str = buf;
				} else
					lbl_str = as_str(ist, lbl);
			}
			if (val) val_str = as_str(ist, val);
			fprintf(fout,"    %s = %s\n", lbl_str, val_str);
		}
		if (llen) fprintf(fout,"\n");
	}
	if (code->srcmap && code->srcmap->items)
		for (i=0; i < clist_len(code->srcmap); i+=3){
			fprintf(fout, "  srcmap pc:%3d, lin:%3d, col:%3d\n",  
							clist_num(code->srcmap,i),
							clist_num(code->srcmap,i+1),
							clist_num(code->srcmap,i+2) );
		}
	fprintf(fout,"\n");
	while(pc < code->len) {pc = prt_code_line(NULL, code, pc, fout); fprintf(fout,"\n");}
	fclose(fout);
}
