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


// parser_routines.c

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include "interp.h"
#include "parser.h"
#include "bytecodes.h"

int trace_parser = 0;

#ifdef TRACE_PARSER

FILE* parse_debug_fout;

code_p debug_retrn(int line_no, code_p res){
	if (trace_parser)
		fprintf(parse_debug_fout, "******> lin:%4d, len:%3d, dep:%3d, max:%3d\n", line_no, res->len, res->stack_depth, res->max_stack_depth);
	return res;
}
#endif

#ifndef TRACE_PARSER
#define debug_retrn(line_no,res)  res
#endif

#define IST (((parse_state*) param)->ist)

//********************************* new_parse_state ***************************
parse_state* new_parse_state(isp ist, FILE* stream, char* in_buf) {
	parse_state* state = pr_malloc(sizeof(parse_state));
	memset(state, 0, sizeof(parse_state));
	if (!state){raise_exception(ist, OBJ(OUTOFMEMORY_EXC), NULL); return NULL;}
	state->ist = ist;
	state->line = 1;
	state->column = 1;
	state->old_column = 1;
	state->stream = stream;
	state->in_buf = in_buf;
	state->num_errors = -1;
	state->assert_enbld = TRUE;
	state->indent_stack = new_clist(10);
	state->const_objs   = new_clist(10);
	return state;
}

//********************************* parse_file_or_string **********************
parse_state* parse_file_or_string(isp ist, char* filename, char* string) {
	FILE*         stream;
	parse_state*  parse_state;
	if (filename) {
		if ( !(stream = fopen(filename, "rb")) ) {
			raise_exception(ist, OBJ(FILENOTFOUND_EXC), "File: %s", filename);
			return NULL;
		}
		if ( !(parse_state = new_parse_state(ist, stream, NULL)) ) return NULL;
	} else {
		char *d, *s;
		filename = pr_malloc(39);
		for (s=string, d=filename; *s && d < filename+30; s++) 
			if ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || 
				(*s >= '0' && *s <= '9') || *s == '_')
				*d++ = *s;
		*d = 0;
		strcat(filename, "-src.txt");
		if ( !(parse_state = new_parse_state(ist, NULL, string)) ) return NULL;
	}
#ifdef TRACE_PARSER
	if (trace_parser){
		char *d, *s, *dfname = pr_malloc(35);
		for (s=filename, d=dfname; *s && d < dfname+30; s++) 
			if ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || 
				(*s >= '0' && *s <= '9') || *s == '_')
				*d++ = *s;
		*d = 0;
		strcat(dfname, ".txt");
		{	size_t len = strlen(dfname);
			char* dbg_filename = pr_malloc(len+15);
			strcpy(dbg_filename, "!parse_trace-");
			strcat(dbg_filename, dfname);
			parse_debug_fout = fopen(dbg_filename, "w");
			parse_state->debug_stream = parse_debug_fout;
		}
	}
#endif

	yyparse(parse_state);

#ifdef TRACE_PARSER
	if (trace_parser)
		fclose(parse_debug_fout);
#endif

	if (parse_state->parse_results)
		parse_state->parse_results->file_path = filename;
	if (parse_state->num_errors) {
		int line=0, col=0, ch, tab_count=0;
		if (parse_state->stream) {
			fclose(parse_state->stream);
			printf( "--- File: %s, line: %d, col: %d\n", filename, 
					parse_state->line, parse_state->column );
			stream = fopen(filename, "rb");
			while(line < parse_state->line-1) {
				if ((ch = fgetc(stream)) == EOF) break;
				if (ch == '\n') line++;
			}
			while ((ch = fgetc(stream)) != EOF) {
				if (ch == '\t') { printf("    "); tab_count++; }
				else              printf("%c",ch);
				if (ch == '\n') break;
			}
			if (ch == EOF) printf("\n");
			for(col=-(tab_count*3); col < parse_state->column-1; col++) printf(" ");
			printf("^\n");
		} else 
			printf( "--- col: %d\n", parse_state->column );
		raise_exception(ist, OBJ(PARSEERROR_EXC), NULL);
		return NULL;
	}
	return parse_state;
}

//********************************* add_to_const_list ***************************
#define add_to_const_list(param, obj)  \
	clist_append(((parse_state*)param)->const_objs, obj);

//********************************* new_code **********************************
code_p new_code(void* param, int size, int stk, int max_stk, opcode_t opcode) {
	data_size_t data_size = sizeof(code)+size*sizeof(code_t);
	code_p p = pr_malloc(data_size);
	if (p == (void*) 0xa189c8)
		p->size = 0;
	memset(p, 0, data_size);
	p->size            = data_size;
	p->len             = size;
	p->stack_depth     = stk;
	p->max_stack_depth = max_stk;
	if(size) p->code_data[0].bytecode.opcode = opcode;
	p->srcmap = new_clist(16);
	clist_append_num(p->srcmap, 0);
	clist_append_num(p->srcmap, ((parse_state*) param)->line);
	clist_append_num(p->srcmap, ((parse_state*) param)->column);
	return debug_retrn(__LINE__, p);
}

//********************************* add_all_srcmap ********************************
void add_all_srcmap(code_p dst, code_p src, int ofs, int free_flg){
	int i;
	clist_p dstmap = dst->srcmap;
	clist_p srcmap = src->srcmap;
	for (i=0; i < clist_len(srcmap); i+=3) {
		int pc = ofs + clist_num(srcmap, i);
		int dlen = clist_len(dstmap);
		if (dlen > 2 && clist_num(dstmap, dlen-3) == pc) {
			clist_num(dstmap, dlen-2) = clist_num(srcmap, i+1);
			clist_num(dstmap, dlen-1) = clist_num(srcmap, i+2);
		} else {
			clist_append_num(dstmap, pc);
			clist_append_num(dstmap, clist_num(srcmap, i+1));
			clist_append_num(dstmap, clist_num(srcmap, i+2));
		}
	}
	if (free_flg) pr_free(srcmap);
}

//********************************* add_code_flg *******************************
void add_code_flg(code_p newcode, code_p res, int *k, int free_flg) {
	memcpy(res->code_data + *k, newcode->code_data, newcode->len * sizeof(code_t));
	add_all_srcmap(res, newcode, *k, free_flg);
	*k += newcode->len;
	if (free_flg) {
		pr_free(newcode);
	}
}

//********************************* add_code *******************************
void add_code(code_p newcode, code_p res, int *k){
	add_code_flg(newcode, res, k, TRUE);
}

//********************************* add_code_no_free *******************************
void add_code_no_free(code_p newcode, code_p res, int *k){
	add_code_flg(newcode, res, k, FALSE);
}

//********************************* parser called routines ********************

void finished(void* param, code_p code, int nerrs){
	((parse_state*) param)->parse_results = code;
	((parse_state*) param)->num_errors    = nerrs;
}

code_p stack_check(void* param, code_p stmt){
	if (stmt->stack_depth < 0 || stmt->stack_depth > 1){
		printf("Parser error, simulated stack value error: %d\n",stmt->stack_depth);
		pr_exit(1);
	}
	if (((parse_state*) param)->defdoc_flag){
		((parse_state*) param)->defdoc_flag = 0;
		if ( stmt->stack_depth && stmt->len == 2 && 
			 stmt->code_data[0].bytecode.opcode == OP_PUSH &&
			 stmt->code_data[0].bytecode.param  == 2 ) {
		    stmt->stack_depth = 0;
			stmt->max_stack_depth = 0;
		    stmt->code_data[0].bytecode.opcode = OP_NOP;
			stmt->doc_flg = TRUE;
		}
	}
	if (stmt->stack_depth) {
		code_p res = new_code(param, 1, -1, 0, OP_POP);
		res->code_data[0].bytecode.param = 1;
		return debug_retrn(__LINE__, append_code(param, stmt, res, 0));
	}
	return debug_retrn(__LINE__, stmt);
}

code_p none(void* param){
	return debug_retrn(__LINE__, new_code(param,0,0,0,0));
}
code_p push_null(void* param) {
	code_p res = new_code(param, 2, 1, 1, OP_PUSH);
	res->code_data[0].bytecode.param = 2;
	res->code_data[1].data = NULL;
	return debug_retrn(__LINE__, res);
}
clist_p empty_list(void* param){
	return new_clist(4);
}
clist_p new_list(void* param, code_p item){
	return new_clist_1(item);
}
clist_p new_list_list(void* param, clist_p params){
	return new_clist_1(params);
}
clist_p new_list2(void* param, code_p item1, code_p item2){
	return new_clist_2(item1, item2);
}
clist_p append_list(void* param, clist_p list, code_p item){
	return clist_append(list, item);
}
clist_p append_list_list(void* param, clist_p list, clist_p item){
	return clist_append(list, item);
}
clist_p append_list_label(void* param, clist_p list, obj_p label){
	return clist_append(list, label);
}
void calc_code(code *p, int *len, int *stack_depth, int *max_stack_depth){
	(*len) += p->len;
	(*max_stack_depth) = max(*max_stack_depth, (*stack_depth) + p->max_stack_depth);
	(*stack_depth) += p->stack_depth;
}
code_p append_code(void* param, code_p p1, code_p p3, int size) {
	if (p3) {
		if (p1) {
			data_size_t data_size = sizeof(code) + (p1->len + p3->len + size)*sizeof(code_t);
			p1 = pr_realloc(p1, data_size);

			p1->size = data_size;
			memcpy(p1->code_data + p1->len, p3->code_data, p3->len * sizeof(code_t));
			add_all_srcmap(p1, p3, p1->len, TRUE);
			p1->len += p3->len + size;
			p1->max_stack_depth = max(p1->max_stack_depth, p1->stack_depth + p3->max_stack_depth);
			p1->stack_depth = p1->stack_depth + p3->stack_depth;
			p1->doc_flg = p1->doc_flg || p3->doc_flg;
			pr_free(p3);
			return debug_retrn(__LINE__, p1);
		} else if (size) {
			data_size_t data_size;
			p3->len += size;
			data_size = sizeof(code) + p3->len*sizeof(code_t);
			p3 = pr_realloc(p3, data_size);
			p3->size = data_size;
		}
		return debug_retrn(__LINE__, p3);
	} else if (p1 && size) {
		data_size_t data_size;
		p1->len += size;
		data_size = sizeof(code) + p1->len*sizeof(code_t);
		p1 = pr_realloc(p1, data_size);
		p1->size = data_size;
	}
	return debug_retrn(__LINE__, p1);
}
clist_p label_to_import_path(void* param, obj_p label){
	return new_clist_1(sym(IST, pr2c_strptr(IST, label)));
}
clist_p append_label_to_import_path(void* param, clist_p list, obj_p label){
	return clist_append(list, sym(IST, pr2c_strptr(IST, label)));
}
clist_p import_path_to_import_param(void* param, clist_p list){
	return ins_clist(new_clist_1(list), NULL, 1);	
}
clist_p import_path_as_label_to_import_param(void* param, clist_p list, obj_p label){
	return ins_clist(new_clist_1(list), sym(IST, pr2c_strptr(IST, label)), 1);	
}
clist_p new_import_param(void* param, clist_p list){
	return new_clist_1(list);
}
clist_p append_import_param(void* param, clist_p list, clist_p iparam){
	return clist_append(list, iparam);
}
code_p import_params(void* param, clist_p iparms){
	code_p p;
	int i, size=0, j, k=0;
	for(i=0; i < clist_len(iparms); i++){
		clist_p   iparm = clist_item(iparms, i);
		clist_p    path = clist_item( iparm, 0);
		obj_p label_key = clist_item(iparm, 1);
		size += 1 + clist_len(path) + (label_key?1:0);
	}
	p = new_code(param, size, 0, 0, OP_IMPORT);
	for(i=0; i < clist_len(iparms); i++){
		clist_p   iparm = clist_item(iparms, i);
		clist_p    path = clist_item( iparm, 0);
		obj_p label_key = clist_item(iparm, 1);
		p->code_data[k].bytecode.opcode = 
					(label_key ? OP_IMPORT_AS : OP_IMPORT);
		p->code_data[k++].bytecode.param = 
					1 + clist_len(path) + (label_key?1:0);
		for (j=0; j < clist_len(path); j++)
			p->code_data[k++].data = clist_item(path, j);
		if (label_key)
			p->code_data[k++].data = label_key;
	}
	return debug_retrn(__LINE__, p);
}
code_p	from_path_import_params(void* param, clist_p path, clist_p iparms){
	code_p p;
	int i, size, j, k=0;
	size = 1 + clist_len(path);
	for (i=0; i < clist_len(iparms); i++){
		clist_p   iparm = clist_item(iparms, i);
		clist_p   path2 = clist_item( iparm, 0);
		obj_p label_key = clist_item(iparm, 1);
		size += 1 + clist_len(path2) + (label_key?1:0);
	}
	p = new_code(param, size+1, 0, 1, OP_IMPORT_PUSH);
	p->code_data[k++].bytecode.param = 1 + clist_len(path);
	for (i=0; i < clist_len(path); i++)
		p->code_data[k++].data = clist_item(path, i);
	for (i=0; i < clist_len(iparms); i++){
		clist_p   iparm = clist_item(iparms, i);
		clist_p   path2 = clist_item( iparm, 0);
		obj_p label_key = clist_item(iparm, 1);
		p->code_data[k].bytecode.opcode   = 
			(label_key ? OP_IMPORT_ATTR_AS : OP_IMPORT_ATTR);
		p->code_data[k++].bytecode.param = 
					1 + clist_len(path2)+ (label_key?1:0);
		for (j=0; j < clist_len(path2); j++)
			p->code_data[k++].data = clist_item(path2, j);
		if (label_key)
			p->code_data[k++].data = label_key;
	}
	p->code_data[k++].bytecode.opcode = OP_POP;
	return debug_retrn(__LINE__, p);
}
code_p label_to_attrref(void* param, obj_p label){
	code_p p;
	p = new_code(param, 2, 2, 2, OP_PUSH_LOCAL_REF);
	p->code_data[1].data = sym(IST, (pr2c_strptr(IST, label)));
	return debug_retrn(__LINE__, p);
}
code_p self_label_to_attrref(void* param, obj_p label){
	code_p p;
	p = new_code(param, 2, 2, 2, OP_PUSH_SELF_REF);
	p->code_data[1].data = sym(IST, pr2c_strptr(IST, label));
	return debug_retrn(__LINE__, p);
}
code_p super_label_to_attrref(void* param, obj_p label) {
	code_p p;
	p = new_code(param, 2, 2, 2, OP_PUSH_SUPER_REF);
	p->code_data[1].data = sym(IST, pr2c_strptr(IST, label));
	return debug_retrn(__LINE__, p);
}
code_p outer_label_to_attrref(void* param, obj_p label){
	code_p p;
	p = new_code(param, 2, 2, 2, OP_PUSH_OUTER_REF);
	p->code_data[1].data = sym(IST, pr2c_strptr(IST, label));
	return debug_retrn(__LINE__, p);
}
code_p caller_label_to_attrref(void* param, obj_p label){
	code_p p;
	p = new_code(param, 2, 2, 2, OP_PUSH_CALLER_REF);
	p->code_data[1].data = sym(IST, pr2c_strptr(IST, label));
	return debug_retrn(__LINE__, p);
}
code_p obj_label_to_attrref(void* param, code_p obj, obj_p label){
	data_size_t data_size;
	obj->len += 2;
	data_size = sizeof(code)+(obj->len)*sizeof(code_t);
	obj = pr_realloc(obj, data_size);
	obj->size = data_size;
	obj->code_data[obj->len-2].bytecode.opcode = OP_PUSH;
	obj->code_data[obj->len-2].bytecode.param = 2;
	obj->code_data[obj->len-1].data = sym(IST, pr2c_strptr(IST, label));
	obj->stack_depth += 1;
	obj->max_stack_depth = max(obj->max_stack_depth, obj->stack_depth);
	return debug_retrn(__LINE__, obj);
}
code_p attrref_to_tgtparm(void* param, code_p p1){
	return debug_retrn(__LINE__, p1);
}
code_p obj_func_params(void* param, code_p obj, clist_p parms){
	code_p res;
	int i, k=0;
	int len = obj->len+3;
	int stack_depth = obj->stack_depth+1;
	int max_stack_depth = obj->max_stack_depth+1;
	int llen = clist_len(parms);
	for(i=0; i < llen; i++)
		calc_code(clist_item(parms,i), &len, &stack_depth, &max_stack_depth);
	res = new_code(param, len, stack_depth, max_stack_depth, 0);
	add_code(obj, res, &k);
	res->code_data[k  ].bytecode.opcode = OP_PUSH;
	res->code_data[k++].bytecode.param  = 2;
	res->code_data[k++].data = NULL;
	for(i=0; i < llen; i++)
		add_code(clist_item(parms,i), res, &k);
	res->code_data[k  ].bytecode.opcode  = OP_OBJCALL;
	res->code_data[k++].bytecode.param = clist_len(parms);
	res->stack_depth -= 1+(2*llen);
	pr_assert(k==res->len);
	return debug_retrn(__LINE__, res);
}
code_p bound_bind_params(void* param, code_p attr_ref, code_p obj){
	code_p res;
	int k=0, len = 0, stack_depth = 0, max_stack_depth = 0;
	calc_code(attr_ref, &len, &stack_depth, &max_stack_depth);
	calc_code(obj, &len, &stack_depth, &max_stack_depth);
	res = new_code(param, len+1, stack_depth, max_stack_depth, 0);
	add_code(attr_ref, res, &k);
	add_code(obj, res, &k);
	res->code_data[k++].bytecode.opcode  = OP_BIND;
	res->stack_depth -= 2;
	pr_assert(k == res->len);
	return debug_retrn(__LINE__, res);
}
code_p unbound_bind_params(void* param, code_p attr_ref, code_p obj){
	code_p res;
	int k=0, len = 2, stack_depth = 0, max_stack_depth = 0;
	calc_code(attr_ref, &len, &stack_depth, &max_stack_depth);
	calc_code(obj, &len, &stack_depth, &max_stack_depth);
	res = new_code(param, len, stack_depth, max_stack_depth, 0);
	add_code(attr_ref, res, &k);
	res->code_data[k++].bytecode.opcode = OP_DEREF;
	add_code(obj, res, &k);
	res->code_data[k++].bytecode.opcode  = OP_OBJBIND;
	res->stack_depth -= 2;
	pr_assert(k == res->len);
	return debug_retrn(__LINE__, res);
}
code_p bound_func_params(void* param, code_p attr_ref, clist_p parms) {
	code_p res, nul = push_null(param);
	int i, k=0, len = 0, stack_depth = 0, max_stack_depth = 5;
	int llen = clist_len(parms);
	calc_code(attr_ref, &len, &stack_depth, &max_stack_depth);
	calc_code(nul, &len, &stack_depth, &max_stack_depth);
	for(i=0; i < llen; i++)
		calc_code(clist_item(parms,i), &len, &stack_depth, &max_stack_depth);
	res = new_code(param, len+1, stack_depth, max_stack_depth, 0);
	add_code(attr_ref, res, &k);
	add_code(nul, res, &k);
	for(i=0; i < llen; i++)
		add_code(clist_item(parms,i), res, &k);
	res->code_data[k].bytecode.opcode  = OP_CALL;
	res->code_data[k++].bytecode.param = clist_len(parms);
	res->stack_depth -= 2+(2*llen);
	pr_assert(k==res->len);
	return debug_retrn(__LINE__, res);
}
code_p unbound_func_params(void* param, code_p attr_ref, clist_p parms, int cmd_flag) {
	code_p res, nul = push_null(param);
	int i, k=0, len = 0, stack_depth = 0, max_stack_depth = 0;
	int llen = clist_len(parms);
	calc_code(attr_ref, &len, &stack_depth, &max_stack_depth);
	calc_code(nul, &len, &stack_depth, &max_stack_depth);
	for(i=0; i < llen; i++)
		calc_code(clist_item(parms,i), &len, &stack_depth, &max_stack_depth);
	res = new_code(param, len+2+cmd_flag, stack_depth, max_stack_depth, 0);
	add_code(attr_ref, res, &k);
	res->code_data[k++].bytecode.opcode = OP_DEREF;
	add_code(nul, res, &k);
	for(i=0; i < llen; i++)
		add_code(clist_item(parms,i), res, &k);
	if (cmd_flag == 1) {
		res->code_data[k  ].bytecode.opcode = OP_CMDCALL;
		res->code_data[k++].bytecode.param  = clist_len(parms);
		res->code_data[k++].data = attr_ref->code_data[1].data;
	} else {
		res->code_data[k  ].bytecode.opcode = OP_OBJCALL;
		res->code_data[k++].bytecode.param  = clist_len(parms);
	}
	res->stack_depth -= 2+(2*llen);
	pr_assert(k==res->len);
	return debug_retrn(__LINE__, res);
}
code_p bound_bind_func_params(void* param, code_p attr_ref, code_p obj, clist_p parms) {
	code_p res;
	int i, k=0, len = 0, stack_depth = 0, max_stack_depth = 0;
	int llen = clist_len(parms);
	calc_code(attr_ref, &len, &stack_depth, &max_stack_depth);
	calc_code(obj, &len, &stack_depth, &max_stack_depth);
	for(i=0; i < llen; i++)
		calc_code(clist_item(parms,i), &len, &stack_depth, &max_stack_depth);
	res = new_code(param, len+2, stack_depth, max_stack_depth, 0);
	add_code(attr_ref, res, &k);
	res->code_data[k++].bytecode.opcode = OP_DEREF;
	add_code(obj, res, &k);
	for(i=0; i < llen; i++)
		add_code(clist_item(parms,i), res, &k);
	res->code_data[k  ].bytecode.opcode = OP_OBJCALL;
	res->code_data[k++].bytecode.param  = clist_len(parms);
	res->stack_depth -= 2+(2*llen);
	pr_assert(k==res->len);
	return debug_retrn(__LINE__, res);
}
code_p unbound_bind_func_params(void* param, code_p attr_ref, code_p obj, clist_p parms){
	code_p res;
	int i, k=0, len = 0, stack_depth = 0, max_stack_depth = 0;
	int llen = clist_len(parms);
	calc_code(attr_ref, &len, &stack_depth, &max_stack_depth);
	calc_code(obj, &len, &stack_depth, &max_stack_depth);
	for(i=0; i < llen; i++)
		calc_code(clist_item(parms,i), &len, &stack_depth, &max_stack_depth);
	res = new_code(param, len+2, stack_depth, max_stack_depth, 0);
	add_code(attr_ref, res, &k);
	res->code_data[k++].bytecode.opcode = OP_DEREF;
	add_code(obj, res, &k);
	for(i=0; i < llen; i++)
		add_code(clist_item(parms,i), res, &k);
	res->code_data[k  ].bytecode.opcode = OP_OBJCALL;
	res->code_data[k++].bytecode.param  = clist_len(parms);
	res->stack_depth -= 2+(2*llen);
	pr_assert(k==res->len);
	return debug_retrn(__LINE__, res);
}
code_p label_to_formparm(void* param, obj_p label){
	code_p res = new_code(param, 3, 2, 2, OP_PUSH);
	res->code_data[0].bytecode.param = 3;
	res->code_data[1].data = sym(IST, pr2c_strptr(IST, label));
	res->code_data[2].data = OBJ(NODEF);
	return debug_retrn(__LINE__, res);
}
code_p label_eq_expr_to_formparm(void* param, obj_p label, code_p expr) {
	data_size_t data_size;
	expr->len += 5;
	expr->stack_depth++;
	expr->max_stack_depth++;
	data_size = sizeof(code)+(expr->len)*sizeof(code_t);
	expr = pr_realloc(expr, data_size);
	expr->size = data_size;
	memmove(expr->code_data+2, expr->code_data, ((expr->len)-2)*sizeof(code_t));
	expr->code_data[0].bytecode.opcode = OP_PUSH;
	expr->code_data[0].bytecode.param  = 2;
	expr->code_data[1].data = sym(IST, pr2c_strptr(IST, label));
	expr->code_data[expr->len-3].bytecode.opcode = OP_SWAP;
	expr->code_data[expr->len-2].bytecode.opcode = OP_POP;
	expr->code_data[expr->len-2].bytecode.param  = 1;
	expr->code_data[expr->len-1].bytecode.opcode = OP_RETURN;
	return debug_retrn(__LINE__, expr);
}
code_p lbl_proto_body_to_obj(void* param, obj_p label, code_p ref, clist_p protos, code_p body) {
	obj_p func;
	code_p res;
	int code_len, llen = 0;
	int i, k=0, len=0, stack_depth=0, max_stack_depth=0;
	if (label)
		{ len += 2; stack_depth += 2; max_stack_depth += 2; }
	if (protos)
		llen = clist_len(protos);
	if (ref)
		calc_code(ref, &len, &stack_depth, &max_stack_depth);
	for (i=0; i < llen; i++)
		calc_code(clist_item(protos, i),&len, &stack_depth, &max_stack_depth);
	len += (label ? 2 : 3);
	res = new_code(param, len, stack_depth, max_stack_depth, 0);
	if (label) {
		res->code_data[k++].bytecode.opcode = OP_PUSH_LOCAL_REF;
		res->code_data[k++].data = sym(IST, pr2c_strptr(IST, label));
	}
	if (ref)
		add_code(ref, res, &k);
	for (i=0; i < llen; i++)
		add_code(clist_item(protos, i), res, &k);
	if (label)
		res->code_data[k++].bytecode.opcode = OP_PROP;
	else {
		res->code_data[k  ].bytecode.opcode = OP_OBJ;
		res->code_data[k++].bytecode.param = 3;
	}
	func = new_object(IST, OBJ(FUNC_PROTO));
	code_len = sizeof(code) + body->len * sizeof(code_t);
	memcpy(obj_malloc(IST, OBJ(FUNC_PROTO), func, code_len), body, code_len);
	res->code_data[k++].data = func;
	if (!label)
		res->code_data[k++].num  = llen;
	pr_assert(k = len);
	res->stack_depth = 0;
	return res;
}
code_p star_ref_to_formparm(void* param, obj_p label){
	code_p res = new_code(param, 3, 2, 2, OP_PUSH);
	res->code_data[0].bytecode.param = 3;
	res->code_data[1].data = sym(IST, pr2c_strptr(IST, label));
	res->code_data[2].data = OBJ(PARAM_STAR);
	return debug_retrn(__LINE__, res);
}
code_p star_star_ref_to_formparm(void* param, obj_p label){
	code_p res = new_code(param, 3, 2, 2, OP_PUSH);
	res->code_data[0].bytecode.param = 3;
	res->code_data[1].data = sym(IST, pr2c_strptr(IST, label));
	res->code_data[2].data = OBJ(PARAM_STAR_STAR);
	return debug_retrn(__LINE__, res);
}
code_p expr_to_sparm(void* param, code_p p1){
	return debug_retrn(__LINE__, p1);
}
void add_slice_count(code_p p, int *len, int *stack_depth, int *max_stack_depth){
	if (p && p->len)
		calc_code(p, len, stack_depth, max_stack_depth);
	else {
		(*len) += 2;
		(*max_stack_depth) = max(*max_stack_depth, (*stack_depth)+1);
		(*stack_depth)++;
	}
}
void add_slice(code_p res, code_p p, int *k, int parm_cnt, int count){
	if (p && p->len)
		add_code(p, res, k);
	else {
		res->code_data[(*k)  ].bytecode.opcode = OP_PUSH;
		res->code_data[(*k)++].bytecode.param  = 2;
		if (parm_cnt < count)
			res->code_data[(*k)++].data = SLICEPARAM_MISSING;
		else
			res->code_data[(*k)++].data = SLICEPARAM_EMPTY;
	}
}
code_p sparms_to_slice(void* param, code_p p1, code_p p2, code_p p3, int parm_cnt){
	code *res;
	int k=0, len=1, stack_depth=0, max_stack_depth=0;
	add_slice_count(p1, &len, &stack_depth, &max_stack_depth);
	add_slice_count(p2, &len, &stack_depth, &max_stack_depth);
	add_slice_count(p3, &len, &stack_depth, &max_stack_depth);
	res = new_code(param, len, stack_depth, max_stack_depth, 0);
	add_slice(res, p1, &k, parm_cnt, 1);
	add_slice(res, p2, &k, parm_cnt, 2);
	add_slice(res, p3, &k, parm_cnt, 3);
	res->code_data[k++].bytecode.opcode = OP_NEWSLICE;
	res->stack_depth -= 2;
	return debug_retrn(__LINE__, res);
}
code_p expr_to_slice(void* param, code_p expr) {
	data_size_t data_size;
	expr->len += 4;
	data_size = sizeof(code)+(expr->len)*sizeof(code_t);
	expr->max_stack_depth = max(expr->max_stack_depth, expr->stack_depth + 2);
	expr = pr_realloc(expr, data_size);
	expr->size = data_size;
	expr->code_data[expr->len-4].bytecode.opcode = OP_PUSH;
	expr->code_data[expr->len-4].bytecode.param = 3;
	expr->code_data[expr->len-3].data = SLICEPARAM_MISSING;
	expr->code_data[expr->len-2].data = SLICEPARAM_MISSING;
	expr->code_data[expr->len-1].bytecode.opcode = OP_NEWSLICE;
	return debug_retrn(__LINE__, expr);
}
code_p unary(void* param, int op, code_p parm){
	data_size_t data_size;
	parm->len += 4;
	data_size = sizeof(code)+(parm->len)*sizeof(code_t);
	parm->max_stack_depth = max(parm->max_stack_depth, parm->stack_depth + 5);
	parm = pr_realloc(parm, data_size);
	parm->size = data_size;
	parm->code_data[parm->len-4].bytecode.opcode = OP_PUSH;
	parm->code_data[parm->len-4].bytecode.param = 3;
	parm->code_data[parm->len-3].data = sym_id_table[op].id;
	parm->code_data[parm->len-2].data = NULL;
	parm->code_data[parm->len-1].bytecode.opcode = OP_CALL;
	parm->code_data[parm->len-1].bytecode.param  = 0;
	return debug_retrn(__LINE__, parm);
}
code_p binary(void* param, code_p lparm, int op, code_p rparm){
	code_p res;
	int have_lparm = FALSE;
	int k=0, len=0, stack_depth=7, max_stack_depth = 7;
	int have_cmp_op = (op >= LT__QUES && op <= EQ__QUES);
	if (lparm->cmp_expr && have_cmp_op) {
		lparm->cmp_expr = FALSE;
		calc_code(lparm, &len, &stack_depth, &max_stack_depth);
		len += 6;
		calc_code(rparm, &len, &stack_depth, &max_stack_depth);
		len += 8;
		res = new_code(param, len, stack_depth, max_stack_depth, 0);
		res->cmp_expr = TRUE;
		add_code(lparm, res, &k);
		res->code_data[k  ].bytecode.opcode = OP_BEQ;
		res->code_data[k  ].bytecode.param  = res->len-k;
		k++;
		res->code_data[k++].data            = OBJ(PR_FALSE);
		res->code_data[k  ].bytecode.opcode = OP_PUSH;
		res->code_data[k++].bytecode.param  = 3;
		res->code_data[k++].data = SYM(GETCMP_);
		res->code_data[k++].data = NULL;
		res->code_data[k  ].bytecode.opcode = OP_CALL;
		res->code_data[k++].bytecode.param  = 0;
		have_lparm = TRUE;
		goto got_lparm;
	} else {
		calc_code(lparm, &len, &stack_depth, &max_stack_depth);
		if (have_cmp_op) {
			calc_code(rparm, &len, &stack_depth, &max_stack_depth);
			len++;
		}
		len += 4;
		if (have_cmp_op) len++;
		else calc_code(rparm, &len, &stack_depth, &max_stack_depth);
		len++;
		if (have_cmp_op) len++;
		res = new_code(param, len, stack_depth, max_stack_depth, 0);
got_lparm:
		if (!have_lparm)
			add_code(lparm, res, &k);			
		if (have_cmp_op) {
			res->cmp_expr = TRUE;
			add_code(rparm, res, &k);
			res->code_data[k++].bytecode.opcode = OP_SWAP;
		}
		res->code_data[k  ].bytecode.opcode = OP_PUSH;
		res->code_data[k++].bytecode.param  = 4;
		res->code_data[k++].data = sym_id_table[op].id;
		res->code_data[k++].data = NULL;
		res->code_data[k++].data = OBJ(NOKEY);
		if (have_cmp_op) {
			res->code_data[k  ].bytecode.opcode = OP_DUPLICATE;
			res->code_data[k++].bytecode.param  = -5;
		} else
			add_code(rparm, res, &k);
		res->code_data[k  ].bytecode.opcode = OP_CALL;
		res->code_data[k++].bytecode.param  = 1;
		if (have_cmp_op)
			res->code_data[k++].bytecode.opcode = OP_CHAIN_CMP;
		res->stack_depth = 1;
		pr_assert(k==res->len);
	}
	return debug_retrn(__LINE__, res);
}
code_p expr_andor_expr(void* param, code_p lparm, code_p rparm, int andor_flg){
	code_p res;
	int k=0, len=0, stack_depth=0, max_stack_depth=0;
	calc_code(lparm, &len, &stack_depth, &max_stack_depth);
	len += 8;
	calc_code(rparm, &len, &stack_depth, &max_stack_depth);
	len += 2;
	res = new_code(param, len, stack_depth, max_stack_depth, 0);
	add_code(lparm, res, &k);
	res->code_data[k  ].bytecode.opcode = OP_DUPLICATE;
	res->code_data[k++].bytecode.param  = -1;
	res->code_data[k  ].bytecode.opcode = OP_PUSH;
	res->code_data[k++].bytecode.param  = 3;
	res->code_data[k++].data = SYM(BOOL__QUES);
	res->code_data[k++].data = NULL;
	res->code_data[k  ].bytecode.opcode = OP_CALL;
	res->code_data[k++].bytecode.param  = 0;
	res->code_data[k  ].bytecode.opcode = OP_BEQ;
	res->code_data[k++].bytecode.param  = rparm->len+4;
	res->code_data[k++].data = 
		         (andor_flg == OR_FLAG? OBJ(PR_TRUE) : OBJ(PR_FALSE));
	res->code_data[k  ].bytecode.opcode = OP_POP;
	res->code_data[k++].bytecode.param  = 2;
	add_code(rparm, res, &k);
	res->code_data[k  ].bytecode.opcode = OP_BR;
	res->code_data[k++].bytecode.param  = 2;
	res->code_data[k  ].bytecode.opcode = OP_POP;
	res->code_data[k++].bytecode.param  = 1;
	pr_assert(k == res->len);
	res->stack_depth = 1;
	return debug_retrn(__LINE__, res);
}
#define SKIP_CODE1_LEN 7
void add_skip_code1(code_p body, code_p res, int* k){
	res->code_data[(*k)  ].bytecode.opcode = OP_PUSH;
	res->code_data[(*k)++].bytecode.param  = 3;
	res->code_data[(*k)++].data = SYM(BOOL__QUES);
	res->code_data[(*k)++].data = NULL;
	res->code_data[(*k)  ].bytecode.opcode = OP_CALL;
	res->code_data[(*k)++].bytecode.param  = 0;
	res->code_data[(*k)  ].bytecode.opcode = OP_BEQ;
	res->code_data[(*k)++].bytecode.param  = body->len + 4;
	res->code_data[(*k)++].data = OBJ(PR_FALSE);
	res->code_data[(*k)  ].bytecode.opcode = OP_POP;
	res->code_data[(*k)++].bytecode.param  = 1;
}
#define SKIP_CODE2_LEN 2
void add_skip_code2(int dist, code_p res, int* k){
	res->code_data[(*k)  ].bytecode.opcode = OP_BR;
	res->code_data[(*k)++].bytecode.param  = dist;
	res->code_data[(*k)  ].bytecode.opcode = OP_POP;
	res->code_data[(*k)++].bytecode.param  = 1;
}

code_p if_expr_body_elif_else(void* param, clist_p ifex_list, code_p body, clist_p elif, code_p els) {
	int i, k=0, len=0, stack_depth=0, max_stack_depth=0;
	code *res;
	code_p ifex = clist_item(ifex_list, 0);
	calc_code(ifex, &len, &stack_depth, &max_stack_depth);
	len += SKIP_CODE1_LEN;
	calc_code(body, &len, &stack_depth, &max_stack_depth);
	len += SKIP_CODE2_LEN;
	max_stack_depth += max(max_stack_depth, stack_depth+3);
	for(i=0; i < clist_len(elif); i++){
		clist_p eil = clist_item(elif, i);
		calc_code(clist_item(eil,0), &len, &stack_depth, &max_stack_depth);
		len += SKIP_CODE1_LEN;
		calc_code(clist_item(eil,1), &len, &stack_depth, &max_stack_depth);
		len += SKIP_CODE2_LEN;
		max_stack_depth += max(max_stack_depth, stack_depth+3);
	}
	if (els->len)
		calc_code(els, &len, &stack_depth, &max_stack_depth);
	res = new_code(param, len, stack_depth, max_stack_depth, 0);
	add_code(ifex, res, &k);
	add_skip_code1(body, res, &k);
	add_code(body, res, &k);
	add_skip_code2(res->len - k, res, &k);
	for(i=0; i < clist_len(elif); i++){
		clist_p eil = clist_item(elif,i);
		code_p pexpr = clist_item(eil,0);
		code_p pbody = clist_item(eil,1);
		add_code(pexpr, res, &k);
		add_skip_code1(pbody, res, &k);
		add_code(pbody, res, &k);
		add_skip_code2(res->len - k, res, &k);
	}
	if (els->len)
		add_code(els, res, &k);
	pr_assert(k == res->len);
	return debug_retrn(__LINE__, res);
}

code_p break_stmt(void* param, obj_p label){
	code_p res = new_code(param, 2, 0, 0, OP_BREAK);
	res->code_data[1].pstr = pr2c_strptr(IST, label);
	return debug_retrn(__LINE__, res);
}
code_p continue_stmt(void* param, obj_p label){
	code_p res = new_code(param, 2, 0, 0, OP_CONTINUE);
	res->code_data[1].pstr = pr2c_strptr(IST, label);
	return debug_retrn(__LINE__, res);
}
void patch_break_continues(void* param, obj_p label, code_p body) {
	int p=0;
	while(p < body->len-1){
		int opcode = body->code_data[p].bytecode.opcode;
		int parm   = body->code_data[p].bytecode.param;
		char* pstr = body->code_data[p+1].pstr;
		if ( (opcode == OP_BREAK || opcode == OP_CONTINUE) &&
			 (!strcmp(pstr, "~") || !strcmp(pstr, pr2c_strptr(IST, label))) ){
			body->code_data[p].bytecode.opcode = OP_BR;
			body->code_data[p].bytecode.param  = (body->len - p) +
									(opcode == OP_CONTINUE ?  0 : 1);
//			pr_free(body->code_data[p+1].pstr);  XXX
			body->code_data[p+1].bytecode.opcode = OP_NOP;
			body->code_data[p+1].bytecode.param  = 1;
		}
		if (opcode < OP_PARAM_WORDS_BOUNDARY)
			p += parm;
		else if (opcode < OP_TWO_WORDS_BOUNDARY)
			p += 2;
		else
			p++;
	}
}
clist_p label_to_forrefs(void* param, obj_p label1, obj_p label2){
	return new_clist_2(label1, label2);
}

void calc_for_clause( void* param, code_p itemexpr, clist_p forlst, int lccp, clist_p lcc,
				      int *len, int *stack_depth, int *max_stack_depth, clist_p loc_list  );

void add_for_clause( void* param, code_p res, int *k, code_p itemexpr, clist_p forlst, 
				     int lccp, clist_p lcc, int stk_ofs, clist_p loc_list );

void calc_if_clause( void* param, code_p itemexpr, code_p ifexpr, int lccp, clist_p lcc,
				     int *len, int *stack_depth, int *max_stack_depth, clist_p loc_list  ) {
	int len_lcc	= clist_len(lcc);
	int end_loc;
	
	*stack_depth += 2;
	*max_stack_depth = max(*max_stack_depth, *stack_depth);

	calc_code(ifexpr, len, stack_depth, max_stack_depth);
	*len += 6;
	if (lccp < len_lcc) {
		if (clist_len(clist_item(lcc, lccp)) == 1)
			calc_if_clause( param, itemexpr, clist_item(clist_item(lcc, lccp), 0), lccp+1, lcc,
				            len, stack_depth, max_stack_depth, loc_list );
		else 
			calc_for_clause( param, itemexpr, clist_item(lcc, lccp), lccp+1, lcc,
				             len, stack_depth, max_stack_depth, loc_list );
	} else {
		calc_code(itemexpr, len, stack_depth, max_stack_depth);
		(*len)++;
	}
	end_loc = *len;
	(*len)++;
	clist_push_num(loc_list,  end_loc);
}

void add_if_clause( void* param, code_p res, int *k, code_p itemexpr, code_p ifexpr, 
				    int lccp, clist_p lcc, int stk_ofs, clist_p loc_list  ) {
	int len_lcc	= clist_len(lcc);
	int end_loc;

	pr_assert(clist_len(loc_list) >= 1);
	end_loc  = clist_pop_num(loc_list);

	add_code(ifexpr, res, k);
	res->code_data[(*k)  ].bytecode.opcode = OP_PUSH;
	res->code_data[(*k)++].bytecode.param  = 3;
	res->code_data[(*k)++].data = SYM(BOOL__QUES);
	res->code_data[(*k)++].data = NULL;
	res->code_data[(*k)  ].bytecode.opcode = OP_CALL;
	res->code_data[(*k)++].bytecode.param  = 0;
	res->code_data[(*k)  ].bytecode.opcode = OP_BEQ;
	res->code_data[(*k)  ].bytecode.param  = end_loc - *k;
	(*k)++;
	res->code_data[(*k)++].data = OBJ(PR_FALSE);
	if (lccp < len_lcc) {
		if (clist_len(clist_item(lcc, lccp)) == 1)
			add_if_clause( param, res, k, itemexpr, clist_item(clist_item(lcc, lccp), 0), 
			               lccp+1, lcc, stk_ofs-1, loc_list );
		else 
			add_for_clause( param, res, k, itemexpr, clist_item(lcc, lccp),
			                lccp+1, lcc, stk_ofs-1, loc_list );
	} else {
		add_code(itemexpr, res, k);
		res->code_data[*k    ].bytecode.opcode = OP_APPEND;
		res->code_data[(*k)++].bytecode.param  = stk_ofs-2;
		pr_assert(clist_len(loc_list) == 0);
		free_clist(loc_list);
	}
	pr_assert(end_loc == *k);
	res->code_data[*k    ].bytecode.opcode = OP_POP;
	res->code_data[(*k)++].bytecode.param  = 1;
	res->stack_depth -=3;
}

void calc_for_clause( void* param, code_p itemexpr, clist_p forlst, int lccp, clist_p lcc,
				      int *len, int *stack_depth, int *max_stack_depth, clist_p loc_list  ) {
	int len_lcc	 = clist_len(lcc);
	int	llen	 = clist_len(forlst)-1;
	code_p expr	 = clist_item(forlst, llen);
	int loop_loc, exc_loc, end_loc;

	*stack_depth += 3;
	*max_stack_depth = max(*max_stack_depth, *stack_depth);
	calc_code(expr, len, stack_depth, max_stack_depth);
	*len += 5;
	loop_loc = *len;
	*len += llen-1+7;
	if (lccp < len_lcc) {
		if (clist_len(clist_item(lcc, lccp)) == 1)
			calc_if_clause( param, itemexpr, clist_item(clist_item(lcc, lccp), 0), lccp+1, lcc,
				            len, stack_depth, max_stack_depth, loc_list );
		else 
			calc_for_clause( param, itemexpr, clist_item(lcc, lccp), lccp+1, lcc,
				             len, stack_depth, max_stack_depth, loc_list );
	} else {
		calc_code(itemexpr, len, stack_depth, max_stack_depth);
		(*len)++;
	}
	(*len)++;
	exc_loc = *len;
	*len += 6;
	end_loc = *len;
	clist_push_num(loc_list,  end_loc);
	clist_push_num(loc_list,  exc_loc);
	clist_push_num(loc_list, loop_loc);
}

void add_for_clause( void* param, code_p res, int *k, code_p itemexpr, clist_p forlst, 
				     int lccp, clist_p lcc, int stk_ofs, clist_p loc_list ) {
	int i, len_lcc	= clist_len(lcc);
	int	llen		= clist_len(forlst)-1;
	code_p expr		= clist_item(forlst, llen);
	int loop_loc, exc_loc, end_loc;

	pr_assert(clist_len(loc_list) >= 3);
	loop_loc = clist_pop_num(loc_list);
	exc_loc	 = clist_pop_num(loc_list);
	end_loc  = clist_pop_num(loc_list);

	for (i=1; i < llen; i++){
		obj_p s = clist_item(forlst,i);
		if ( !(*(pr2c_strptr(IST, s)) >= 'a' && *(pr2c_strptr(IST, s)) <= 'z') && ! *(pr2c_strptr(IST, s)) == '_'){
			printf("For variable not local: %s\n", pr2c_strptr(IST, s));
			pr_exit(1);
		}
		clist_item(forlst,i) = sym(IST, pr2c_strptr(IST, s));
	}
	add_code(expr, res, k);
	res->code_data[*k    ].bytecode.opcode = OP_PUSH;
	res->code_data[(*k)++].bytecode.param  = 3;
	res->code_data[(*k)++].data = SYM(ITER_);
	res->code_data[(*k)++].data = NULL;
	res->code_data[*k    ].bytecode.opcode = OP_CALL;
	res->code_data[(*k)++].bytecode.param  = 0;
	res->code_data[*k    ].bytecode.opcode = OP_TRYEXCEPT;
	res->code_data[*k    ].bytecode.param  = exc_loc - *k;
	(*k)++;
	pr_assert(loop_loc == *k);
	res->code_data[*k    ].bytecode.opcode = OP_DUPLICATE;
	res->code_data[(*k)++].bytecode.param  = -1;
	res->code_data[*k    ].bytecode.opcode = OP_PUSH;
	res->code_data[(*k)++].bytecode.param  = 3;
	res->code_data[(*k)++].data = SYM(NEXT);
	res->code_data[(*k)++].data = NULL;
	res->code_data[*k    ].bytecode.opcode = OP_CALL;
	res->code_data[(*k)++].bytecode.param  = 0;
	res->code_data[*k    ].bytecode.opcode = OP_PUSH;
	res->code_data[(*k)++].bytecode.param  = llen;
	for (i=1; i < llen; i++)
		res->code_data[(*k)++].data = clist_item(forlst,i);
	res->code_data[*k    ].bytecode.opcode = OP_SEQ_ASSIGN;
	res->code_data[(*k)++].bytecode.param  = llen-1;
	if (lccp < len_lcc) {
		if (clist_len(clist_item(lcc, lccp)) == 1)
			add_if_clause( param, res, k, itemexpr, clist_item(clist_item(lcc, lccp), 0), 
			               lccp+1, lcc, stk_ofs-1, loc_list );
		else 
			add_for_clause( param, res, k, itemexpr, clist_item(lcc, lccp),
			                lccp+1, lcc, stk_ofs-1, loc_list );
	} else {
		add_code(itemexpr, res, k);
		res->code_data[*k    ].bytecode.opcode = OP_APPEND;
		res->code_data[(*k)++].bytecode.param  = stk_ofs-2;
		pr_assert(clist_len(loc_list) == 0);
		free_clist(loc_list);
	}
	res->code_data[*k    ].bytecode.opcode = OP_BR;
	res->code_data[*k    ].bytecode.param  = loop_loc - *k;
	(*k)++;
	pr_assert(exc_loc == *k);
	res->code_data[*k    ].bytecode.opcode = OP_PUSH;
	res->code_data[(*k)++].bytecode.param  = 3;
	res->code_data[(*k)++].data = OBJ(STOP_ITERATION_EXC);
	res->code_data[(*k)++].data = NULL;
	res->code_data[*k    ].bytecode.opcode = OP_EXCEPT;
	res->code_data[(*k)++].bytecode.param  = 1;
	res->code_data[(*k)++].bytecode.opcode = OP_RERAISE;
	res->code_data[*k    ].bytecode.opcode = OP_POP;
	res->code_data[(*k)++].bytecode.param  = 1;
	res->stack_depth -=4;
	pr_assert(end_loc == *k);
}

// lcc is list of lists
// lcc list with one entry is if expr
// lcc list with >= 3 entries is forlst: 
//     "tag: FOR tgts in expr", first is tag, last is expr, rest are tgts
// forlst param is the same forlst format: "tag: FOR tgts in expr"
// expr param is list comprehension expr
code_p expr_for_lcc_to_listcomp(void* param, code_p expr, clist_p forlst, clist_p lcc) {
	int k=0, len=1, stack_depth=1, max_stack_depth=1;
	code_p res;
	clist_p loc_list = new_clist(9);

	calc_for_clause(param, expr, forlst, 0, lcc, &len, &stack_depth, &max_stack_depth, loc_list); 
	res = new_code(param, len, stack_depth, max_stack_depth, 0);
	res->code_data[k  ].bytecode.opcode = OP_NEWLIST;
	res->code_data[k++].bytecode.param  = 0;
	add_for_clause(param, res, &k, expr, forlst, 0, lcc, -1, loc_list);
	res->stack_depth -=1;
	return debug_retrn(__LINE__, res);
}

code_p for_lcc_body_else(void* param, clist_p targets, code_p body, code_p els){
	code_p res;
	int i, k=0, len=0, stack_depth=0, max_stack_depth=0;
	int llen = clist_len(targets)-1, loop_loc, body_loc, els_loc, end_els_loc, end_loc;
	code_p expr = clist_item(targets, llen);
	patch_break_continues(param, clist_item(targets,0), body);
	for (i=1; i < llen; i++)
		clist_item(targets, i) = sym(IST, pr2c_strptr(IST, (obj_p)clist_item(targets, i)));
	calc_code(expr, &len, &stack_depth, &max_stack_depth); 
	len += 5;			
	loop_loc = len;  
	len += llen + 6;	
	body_loc = len;  
	calc_code(body, &len, &stack_depth, &max_stack_depth); 
	len += 3;			
	els_loc  = len;  
	len += 4;
	if (els->len)
		calc_code(els, &len, &stack_depth, &max_stack_depth);
	end_els_loc = len;
	len += 2;
	end_loc  = len;
	res = new_code(param, len, stack_depth, max_stack_depth+2, 0);
	add_code(expr, res, &k);
	res->code_data[k  ].bytecode.opcode = OP_PUSH;
	res->code_data[k++].bytecode.param  = 3;
	res->code_data[k++].data = SYM(ITER_);
	res->code_data[k++].data = NULL;
	res->code_data[k  ].bytecode.opcode = OP_CALL;
	res->code_data[k++].bytecode.param  = 0;
	res->code_data[k  ].bytecode.opcode = OP_TRYEXCEPT;
	res->code_data[k ].bytecode.param  = els_loc - k;
	k++;
	pr_assert(loop_loc == k);
	res->code_data[k  ].bytecode.opcode = OP_DUPLICATE;
	res->code_data[k++].bytecode.param  = -1;
	res->code_data[k  ].bytecode.opcode = OP_PUSH;
	res->code_data[k++].bytecode.param  = 3;
	res->code_data[k++].data = SYM(NEXT);
	res->code_data[k++].data = NULL;
	res->code_data[k  ].bytecode.opcode = OP_CALL;
	res->code_data[k++].bytecode.param  = 0;
	res->code_data[k  ].bytecode.opcode = OP_PUSH;
	res->code_data[k++].bytecode.param  = llen;
	for (i=1; i < llen; i++)
		res->code_data[k++].data = clist_item(targets,i);
	res->code_data[k  ].bytecode.opcode = OP_SEQ_ASSIGN;
	res->code_data[k++].bytecode.param  = llen-1;
	pr_assert(body_loc == k);
	add_code(body, res, &k);
	res->code_data[k  ].bytecode.opcode = OP_BR;
	res->code_data[k  ].bytecode.param  = loop_loc - k;
	k++;
	res->code_data[k  ].bytecode.opcode = OP_POP;
	res->code_data[k++].bytecode.param  = 1;
	res->code_data[k  ].bytecode.opcode = OP_BR;
	res->code_data[k  ].bytecode.param  = end_loc - k;
	k++;
	pr_assert(els_loc == k);
	res->code_data[k  ].bytecode.opcode = OP_PUSH;
	res->code_data[k++].bytecode.param  = 3;
	res->code_data[k++].data = OBJ(STOP_ITERATION_EXC);
	res->code_data[k++].data = NULL;
	res->code_data[k  ].bytecode.opcode = OP_EXCEPT;
	res->code_data[k  ].bytecode.param  = end_els_loc - k;
	k++;
	if (els->len)
		add_code(els, res, &k);
	pr_assert(end_els_loc == k);
	res->code_data[k  ].bytecode.opcode = OP_POP;
	res->code_data[k++].bytecode.param  = 1;
	res->code_data[k++].bytecode.opcode = OP_RERAISE;
	pr_assert(end_loc == k);
	return debug_retrn(__LINE__, res);
}
code_p ref_expr_to_ass_op(void* param, code_p ref, code_p rparm, int iop, int op){
	code *res;
	int k=0, len=0, stack_depth=0, max_stack_depth = 0;
	calc_code(ref, &len, &stack_depth, &max_stack_depth); 
	len += 5; stack_depth += 3;  max_stack_depth += 3;  
	calc_code(rparm, &len, &stack_depth, &max_stack_depth); 
	len++; stack_depth -= 3; 
	calc_code(ref, &len, &stack_depth, &max_stack_depth); 
	len += 2; stack_depth -= 3; 
	res = new_code(param, len, stack_depth, max_stack_depth, 0);
	add_code_no_free(ref, res, &k);
	res->code_data[k++].bytecode.opcode = OP_DEREF;
	res->code_data[k  ].bytecode.opcode = OP_PUSH;
	res->code_data[k++].bytecode.param  = 4;
	res->code_data[k++].data = sym_id_table[op].id;
	res->code_data[k++].data = NULL;
	res->code_data[k++].data = OBJ(NOKEY);
	add_code(rparm, res, &k);
	res->code_data[k  ].bytecode.opcode = OP_CALL;
	res->code_data[k++].bytecode.param  = 1;
	add_code(ref, res, &k);
	res->code_data[k  ].bytecode.opcode = OP_ASSIGN;
	res->code_data[k++].bytecode.param  = 1;
	res->code_data[k  ].bytecode.opcode = OP_POP;
	res->code_data[k++].bytecode.param  = 1;
	pr_assert(len = k);
	res->stack_depth = 0;
	return debug_retrn(__LINE__, res);
}
code_p while_expr_body_else(void* param, obj_p label, code_p expr, code_p body, code_p els){
	code *res;
	int k=0, len=0, stack_depth=0, max_stack_depth = 6;
	int loop_loc, body_loc, els_loc, end_loc;
	patch_break_continues(param, label, body);
	loop_loc = len;
	calc_code(expr, &len, &stack_depth, &max_stack_depth); 
	len += 7;
	body_loc = len;  
	calc_code(body, &len, &stack_depth, &max_stack_depth); 
	len += 2;			
	els_loc  = len;  
	len++; 
	if (els->len)
		calc_code(els, &len, &stack_depth, &max_stack_depth);
	end_loc  = len;
	res = new_code(param, len, stack_depth, max_stack_depth, 0);
	pr_assert(loop_loc == k);
	add_code(expr, res, &k);
	res->code_data[k  ].bytecode.opcode = OP_PUSH;
	res->code_data[k++].bytecode.param  = 3;
	res->code_data[k++].data = SYM(BOOL__QUES);
	res->code_data[k++].data = NULL;
	res->code_data[k  ].bytecode.opcode = OP_CALL;
	res->code_data[k++].bytecode.param  = 0;
	res->code_data[k  ].bytecode.opcode = OP_BEQ;
	res->code_data[k  ].bytecode.param  = els_loc - k;
	k++;
	res->code_data[k++].data = OBJ(PR_FALSE);
	res->code_data[k  ].bytecode.opcode = OP_POP;
	res->code_data[k++].bytecode.param  = 1;
	pr_assert(body_loc == k);
	add_code(body, res, &k);
	res->code_data[k  ].bytecode.opcode = OP_BR;
	res->code_data[k  ].bytecode.param  = loop_loc - k;
	k++;
	res->code_data[k  ].bytecode.opcode = OP_BR;
	res->code_data[k  ].bytecode.param  = end_loc - k;
	k++;
	pr_assert(els_loc == k);
	res->code_data[k  ].bytecode.opcode = OP_POP;
	res->code_data[k++].bytecode.param  = 1;
	if (els->len)
		add_code(els, res, &k);
	pr_assert(end_loc == k);
	return debug_retrn(__LINE__, res);
}
code_p cond_exc_to_assert(void* param, code_p cond, code_p exc){
	int k=0, len=0, stack_depth=0, max_stack_depth=7, pop_loc;
	code *res;
	if(!((parse_state*) param)->assert_enbld)
		return debug_retrn(__LINE__, none(param));
	calc_code(cond, &len, &stack_depth, &max_stack_depth);
	len += 7;
	calc_code(exc, &len, &stack_depth, &max_stack_depth);
	len++;
	pop_loc = len;
	res = new_code(param, len+1, stack_depth, max_stack_depth, 0);
	add_code(cond, res, &k);
	res->code_data[k  ].bytecode.opcode = OP_PUSH;
	res->code_data[k++].bytecode.param  = 3;
	res->code_data[k++].data = SYM(BOOL__QUES);
	res->code_data[k++].data = NULL;
	res->code_data[k  ].bytecode.opcode = OP_CALL;
	res->code_data[k++].bytecode.param  = 0;
	res->code_data[k  ].bytecode.opcode = OP_BEQ;
	res->code_data[k  ].bytecode.param  = pop_loc - k;
	k++;
	res->code_data[k++].data = OBJ(PR_TRUE);
	res->code_data[k  ].bytecode.opcode = OP_POP;
	res->code_data[k++].bytecode.param  = 1;
	add_code(exc, res, &k);
	res->code_data[k++].bytecode.opcode = OP_RAISE;
	pr_assert(pop_loc == k);
	res->code_data[k  ].bytecode.opcode = OP_POP;
	res->code_data[k++].bytecode.param  = 1;
	res->stack_depth -= 1;
	pr_assert(res->len == k);
	return debug_retrn(__LINE__, res);
}

code_p ref_parm_body_to_def(void* param, code_p name, clist_p params, code_p self, code_p body, int flg) { 
	code *res;
	obj_p func;
	int i, k=0, len=2, stack_depth=0, max_stack_depth=0;
	int blen, llen=0, opcode = OP_NOP, pcnt = 0, param_funcs_len = 0;
	int hdr_in_codet = sizeof(code)/sizeof(code_t);
	code_t *baseptr, *ptr;

	if (params) llen = clist_len(params);
	if (flg != OP_WITH) {
		if(!name && body->stack_depth)
			res = new_code(param, 1, -1, 0, OP_RETURN);
		else {
			res = new_code(param, 3, 0, 1, OP_PUSH);
			res->code_data[k++].bytecode.param = 2;
			res->code_data[k++].data = OBJ(NONE);
			res->code_data[k++].bytecode.opcode = OP_RETURN;
		}
		body = append_code(param, body, res, 0);
	}
	k=0;
	if (name)
		calc_code(name, &len, &stack_depth, &max_stack_depth); 
	if (params) {
		for(i=0; i < llen; i++) {
			code_p param = (code_p)clist_item(params,i); 
			if (param->len > 3) param_funcs_len += hdr_in_codet+param->len;
		}
	}
	blen = hdr_in_codet + body->len;
	func = new_object(IST, OBJ(FUNC_PROTO));
	baseptr = ptr = obj_malloc(IST, OBJ(FUNC_PROTO), func, (blen+param_funcs_len)*sizeof(code_t));
	memcpy(ptr, body, blen * sizeof(code_t));
	if (params) {
		ptr += blen;
		len++;
		for(i=0; i < llen; i++) {
			code_p param = (code_p)clist_item(params,i); 
			if (param->len == 3) {
				calc_code( (code_p)clist_item(params,i), 
						    &len, &stack_depth, &max_stack_depth ); 
			} else {
				len += 3;
				max_stack_depth = max(max_stack_depth, stack_depth+2);
				stack_depth += 2;
				memcpy(ptr, param, (hdr_in_codet+param->len)*sizeof(code_t));
				ptr += hdr_in_codet+param->len;
			}
		}
	}
	if (self) {
		calc_code(self, &len, &stack_depth, &max_stack_depth); 
		len++;
	}
	if(flg == WTH_FLG) 
		len--;
	res = new_code(param, len, stack_depth, max_stack_depth, 0);
	if (name)
		add_code(name, res, &k);
	if (params) {
		ptr = baseptr + blen;
		for(i=0; i < llen; i++) {
			code_p parm = (code_p)clist_item(params,i); 
			if (parm->len == 3) {
				add_code(parm, res, &k);
			} else {
				res->code_data[k  ].bytecode.opcode = OP_PUSH;
				res->code_data[k++].bytecode.param  = 3;
				res->code_data[k++].data = parm->code_data[1].data;
				res->code_data[k++].data = new_parmptr_obj(IST, (int)(ptr-baseptr)*sizeof(code_t));
				ptr += hdr_in_codet+parm->len;
			}
		}
	}			
	if (self)
		add_code(self, res, &k);
	switch (flg) {
		case DEF_FLG: opcode = OP_DEF;	  pcnt = 3; break;
		case GEN_FLG: opcode = OP_GEN;	  pcnt = 3; break;
		case WTH_FLG: opcode = OP_WITH;	  pcnt = 2; break;
	}
	res->code_data[k  ].bytecode.opcode = opcode;
	res->code_data[k++].bytecode.param  = pcnt;
	if (pcnt == 3) res->code_data[k++].num = llen;
	res->code_data[k++].data = (void*) func;
	add_to_const_list(param, func);
	res->stack_depth -= llen*2 + 2;
	pr_assert(res->len == k);
	if (body->doc_flg)
		set_attr(IST, func, SYM(DOC_), (obj_p)(body->code_data[1].data));
	return debug_retrn(__LINE__, res);
}
code_p new_tuple_list(void* param, clist_p lst, int tuple_flag){
	int i, k=0, len=1, stack_depth=1, max_stack_depth=1;
	code_p res;
	for(i=0; i < clist_len(lst); i++)
		calc_code((code_p)clist_item(lst,i), &len, &stack_depth, &max_stack_depth);
	res = new_code(param, len, stack_depth, max_stack_depth, 0);
	for(i=0; i < clist_len(lst); i++)
		add_code(clist_item(lst,i), res, &k);
	res->code_data[k  ].bytecode.opcode = 
					(tuple_flag == NEW_TUPLE_ ? OP_NEWTUPLE : OP_NEWLIST);
	res->code_data[k++].bytecode.param  = clist_len(lst);
	pr_free(lst);
	res->stack_depth -= clist_len(lst);
	pr_assert(res->len == k);
	return debug_retrn(__LINE__, res);
}
code_p new_dict(void* param, clist_p lst){
	int i, k=0, len=1, stack_depth=1, max_stack_depth=1;
	code_p res;
	for(i=0; i < clist_len(lst); i++) {
		clist_p pl = (clist_p)clist_item(lst,i);
		calc_code((code_p)clist_item(pl,0), &len, &stack_depth, &max_stack_depth);
		calc_code((code_p)clist_item(pl,1), &len, &stack_depth, &max_stack_depth);
	}
	res = new_code(param, len, stack_depth, max_stack_depth, 0);
	for(i=0; i < clist_len(lst); i++) {
		clist_p pl = (clist_p)clist_item(lst,i);
		code_p key = (code_p)clist_item(pl,0);
		code_p val = (code_p)clist_item(pl,1);
		add_code(key, res, &k);
		add_code(val, res, &k);
		pr_free(pl);
	}
	res->code_data[k  ].bytecode.opcode = OP_NEWDICT;
	res->code_data[k++].bytecode.param  = clist_len(lst)*2;
	pr_free(lst);
	res->stack_depth -= clist_len(lst)*2;
	pr_assert(res->len == k);
	return debug_retrn(__LINE__, res);
}
code_p raise_expr(void* param, code_p expr1,  code_p expr2) {
	int k=0, len=0, stack_depth=0, max_stack_depth=0;
	code_p res;
	calc_code(expr1, &len, &stack_depth, &max_stack_depth);
	if (expr2)
		calc_code(expr2, &len, &stack_depth, &max_stack_depth);
	else { len += 2; stack_depth += 1; }
	res = new_code(param, len+1, stack_depth, max_stack_depth, 0);
	add_code(expr1, res, &k);
	if (expr2)
		add_code(expr2, res, &k);
	else {
		res->code_data[k  ].bytecode.opcode = OP_PUSH;
		res->code_data[k++].bytecode.param  = 2;
		res->code_data[k++].data            = OBJ(NONE);
	}
	res->code_data[k++].bytecode.opcode = OP_RAISE;
	res->stack_depth = 0;
	pr_assert(res->len == k);
	return debug_retrn(__LINE__, res);
}
code_p word_expr_to_param(void* param, obj_p word, code_p expr){
	int k=0, len=2, stack_depth=1, max_stack_depth=1;
	code_p res;
	calc_code(expr, &len, &stack_depth, &max_stack_depth);
	res = new_code(param, len, stack_depth, max_stack_depth, OP_PUSH);
	res->code_data[k++].bytecode.param = 2;
	res->code_data[k++].data = word;
	add_code(expr, res, &k);
	pr_assert(res->len == k);
	return debug_retrn(__LINE__, res);
}
code_p expr_to_param(void* param, code_p expr){
	return debug_retrn(__LINE__, word_expr_to_param(param, OBJ(NOKEY), expr));
}
code_p label_eq_expr_to_param(void* param, obj_p label, code_p expr){
	return debug_retrn(__LINE__, word_expr_to_param(param, sym(IST, pr2c_strptr(IST, label)), expr));
}
code_p star_seq_to_param(void* param, code_p expr){
	return debug_retrn(__LINE__, word_expr_to_param(param, OBJ(PARAM_STAR), expr));
}
code_p star_star_dict_to_param(void* param, code_p expr){
	return debug_retrn(__LINE__, word_expr_to_param(param, OBJ(PARAM_STAR_STAR), expr));
}
code_p int_to_obj(void* param, obj_p num) {
	code_p res = new_code(param, 2, 1, 1, OP_PUSH);
	res->code_data[0].bytecode.param = 2;
	res->code_data[1].data = num;
	return debug_retrn(__LINE__, res);
}
code_p float_to_obj(void* param, double num, obj_p int_obj, int imag_flag) {
	code_p res;
	obj_p proto = (imag_flag == NEW_IMAG? OBJ(IMAG_PROTO) : OBJ(FLOAT_PROTO));
	obj_p obj = new_object(IST, proto);
	if (int_obj)
		*((double*) obj_malloc(IST, proto, obj, sizeof(double))) = (double) int2i64t(IST, int_obj);
	else
		*((double*) obj_malloc(IST, proto, obj, sizeof(double))) = num;
	res = new_code(param, 2, 1, 1, OP_PUSH);
	res->code_data[0].bytecode.param = 2;
	res->code_data[1].data = obj;
	add_to_const_list(param, obj);
	return debug_retrn(__LINE__, res);	
}
code_p string_to_obj(void* param, obj_p str){
	code_p p = new_code(param, 2, 1, 1, OP_PUSH);
	obj_p obj = copy_object(IST, str);
	p->code_data[0].bytecode.param = 2;
	p->code_data[1].data = obj;
	add_to_const_list(param, obj);
	return debug_retrn(__LINE__, p);
}
code_p rquote_lbl_to_obj(void* param, obj_p label) {
	code_p p = new_code(param, 2, 1, 1, OP_PUSH);
	obj_p symbol = sym(IST, pr2c_strptr(IST, label));
	p->code_data[0].bytecode.param = 2;
	p->code_data[1].data = symbol;
	add_to_const_list(param, symbol);
	return debug_retrn(__LINE__, p);
}
code_p self_to_obj(void* param){
	code_p p = new_code(param, 1, 1, 1, OP_PUSH_SELF);
	return debug_retrn(__LINE__, p);
}
code_p caller_to_obj(void* param){
	code_p p = new_code(param, 1, 1, 1, OP_PUSH_CALLER);
	return debug_retrn(__LINE__, p);
}
code_p assign_right(void* param, clist_p right){
	code_p res;
	int i, k=0, len=0, stack_depth=0, max_stack_depth = 0;
	int rlen = clist_len(right);
	for(i=0; i < rlen; i++)
		calc_code(clist_item(right,i), &len, &stack_depth, &max_stack_depth);
	res = new_code(param, len, stack_depth, max_stack_depth, 0);
	res->assign_parm_cnt = rlen;
	for(i=0; i < rlen; i++)
		add_code(clist_item(right,i), res, &k);
	pr_assert(k==res->len);
	return debug_retrn(__LINE__, res);
}
code_p tgt_eq_stmt_to_stmt(void* param, clist_p left, code_p right){
	code_p res;
	int i, k=0, len=0, stack_depth=0, max_stack_depth = 0;
	int llen = clist_len(left);
	int rlen = right->assign_parm_cnt;
	int seq_expand = (rlen == 1 && llen > 1);
	if(llen != rlen && !seq_expand){
		printf("Assignment target count not equal to right side expression count\n");
		pr_exit(1);
	}
	for(i=0; i < llen; i++)
		calc_code(clist_item(left,i), &len, &stack_depth, &max_stack_depth);
	len += 1;
	res = new_code(param, len, stack_depth, max_stack_depth, 0);
	res->assign_parm_cnt = rlen;
	for(i=0; i < llen; i++)
		add_code(clist_item(left,i), res, &k);
	res->code_data[k  ].bytecode.opcode = (seq_expand ? OP_ASSIGN_EXP : OP_ASSIGN);
	res->code_data[k++].bytecode.param  = llen;
	res->stack_depth -= llen*2;
	pr_assert(k==res->len);
	return debug_retrn(__LINE__, append_code(param, right, res, 0));
}
code_p tgt_eq_expr_to_stmt(void* param, clist_p left, clist_p right){
	return debug_retrn(__LINE__, tgt_eq_stmt_to_stmt(param, left, assign_right(param, right)));
}
code_p assign_cleanup(void* param, code_p ass_code){
	code_p res = new_code(param, 1, -(ass_code->assign_parm_cnt), 0, OP_POP);
	res->code_data[0].bytecode.param = ass_code->assign_parm_cnt;
	return debug_retrn(__LINE__, append_code(param, ass_code, res, 0));
}
code_p exceptall(void* param){
	code_p res = new_code(param, 2, 1, 1, OP_PUSH);
	res->code_data[0].bytecode.param  = 2;
	res->code_data[1].data = OBJ(EXCEPTION);
	return debug_retrn(__LINE__, res);
}
code_p expr_label_body_to_except(void* param, code_p expr, obj_p label, code_p body) {
	code *res;
	int k=0, len=3, stack_depth=1, max_stack_depth = 1;
	calc_code(expr, &len, &stack_depth, &max_stack_depth);
	res = new_code(param, len, stack_depth, max_stack_depth, 0);
	add_code(expr, res, &k);
	res->code_data[k  ].bytecode.opcode = OP_PUSH;
	res->code_data[k++].bytecode.param  = 2;
	if (label)
		res->code_data[k++].data = sym(IST, pr2c_strptr(IST, label));
	else
		res->code_data[k++].data = OBJ(NONE);
	res->code_data[k  ].bytecode.opcode = OP_EXCEPT;
	res->code_data[k++].bytecode.param  = body->len+1;
	pr_assert(res->len == k);
	return debug_retrn(__LINE__, append_code(param, res, body, 0));
}
code_p body_except_else_to_tryexcept(void* param, code_p body, clist_p except_list, code_p els){
	code *res;
	int i, k=0, len=0, stack_depth=0, max_stack_depth = 0;
	int llen = clist_len(except_list);
	int except_loc, els_loc, end_loc;
	calc_code(body, &len, &stack_depth, &max_stack_depth); 
	len += 2;
	except_loc = len;  
	len += 1;
	for(i=0; i < llen; i++)
		calc_code(clist_item(except_list,i), &len, &stack_depth, &max_stack_depth);
	len += llen + 1;
	els_loc = len;
	if (els) 
		calc_code(els, &len, &stack_depth, &max_stack_depth);
	end_loc  = len;
	res = new_code(param, len, stack_depth, max_stack_depth, 0);
	res->code_data[k  ].bytecode.opcode = OP_TRYEXCEPT;
	res->code_data[k  ].bytecode.param  = except_loc - k;
	k++;
	add_code(body, res, &k);
	res->code_data[k  ].bytecode.opcode = OP_BR;
	res->code_data[k  ].bytecode.param  = els_loc - k;
	k++;
	pr_assert(except_loc == k);
	res->code_data[k++].bytecode.opcode = OP_CLRSP;
	for (i = 0; i < llen; i++, k++) {
		add_code(clist_item(except_list,i), res, &k);
		res->code_data[k  ].bytecode.opcode = OP_BR;
		res->code_data[k  ].bytecode.param  = end_loc - k;
	}
	res->code_data[k++].bytecode.opcode = OP_RERAISE;
	pr_assert(els_loc == k);
	if (els)
		add_code(els, res, &k);
	pr_assert(end_loc == k);
	return debug_retrn(__LINE__, res);
}
code_p tryfinally_body(void* param, code_p body, code_p final){
	code *res;
	int k=0, len=3, stack_depth=0, max_stack_depth = 0;
	calc_code(body, &len, &stack_depth, &max_stack_depth);
	calc_code(final, &len, &stack_depth, &max_stack_depth);
	res = new_code(param, len, stack_depth, max_stack_depth, OP_TRYFINALLY);
	res->code_data[k++].bytecode.param = body->len+1;
	add_code(body, res, &k);
	res->code_data[k++].bytecode.opcode = OP_CLRSP;
	add_code(final, res, &k);
	res->code_data[k++].bytecode.opcode = OP_RERAISE;
	pr_assert(res->len == k);
	return debug_retrn(__LINE__, res);
}
code_p op_expr(int op, code_p expr, int size){
	data_size_t data_size;
	expr->len += 1;
	data_size = sizeof(code)+(expr->len)*sizeof(code_t);
	expr = pr_realloc(expr, data_size);
	expr->size = data_size;
	expr->code_data[expr->len-1].bytecode.opcode = op;
	expr->stack_depth -= size;
	return debug_retrn(__LINE__, expr);
}
code_p exec_expr(void* param, code_p expr){
	return debug_retrn(__LINE__, op_expr(OP_EXEC, expr, 1));
}
code_p tgtparm_to_obj(void* param, code_p expr){
	return debug_retrn(__LINE__, op_expr(OP_DEREF, expr, 1));
}
code_p del_ref(void* param, code_p expr){
	return debug_retrn(__LINE__, op_expr(OP_DEL, expr, 2));
}
code_p yield_expr(void* param, code_p expr){
	return debug_retrn(__LINE__, op_expr(OP_YIELD, expr, 1));
}
code_p return_expr(void* param, code_p expr){
	return debug_retrn(__LINE__, op_expr(OP_RETURN, expr, 1));
}
