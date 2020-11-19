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


// parser.h

#ifndef PARSER_H
#define PARSER_H

#include "object.h"
#include "clist.h"
#include "bytecodes.h"
#include <stdio.h>

typedef struct {
	long			param : 24;
	opcode_t		opcode:  8;
} bytecode_t;

typedef union {
	char*		pstr;
	long		num;
	bytecode_t	bytecode;
	obj_p		data;
} code_t;

typedef struct code_s* code_p;

typedef struct code_s {
	data_size_t	size;
	int     	len;
	int 		stack_depth;
	int 		max_stack_depth;
	int			assign_parm_cnt;
	int			cmp_expr;
	char*		file_path;
	clist_p		srcmap;
	int			doc_flg;
	code_t		code_data[];
} code;

int yyparse(void* yylex_param);
int yyerror (char* s);

extern int trace_parser;

typedef struct {
	isp			ist;
	int			line;
	int			column;
	int			old_column;
	int			old_tab_count;
	int			indent_char;
	int			indents_needed;
	int			last_line_colon;
	clist_p     indent_stack;
	FILE*		stream;
	int			char_ptr;
	char		old_chars[10];
	char*		in_buf;
	code_p   	parse_results;
	int	    	num_errors;
	int			assert_enbld;
	int			defdoc_flag;
	int			eof_flag;
	clist_p		const_objs;
	FILE*		debug_stream;
} parse_state;

parse_state* parse_file_or_string(isp ist, char* filename, char* string);

#define OR_FLAG		0
#define AND_FLAG	1

#define NEW_LIST_	0
#define NEW_TUPLE_	1

#define NEW_REAL	0
#define NEW_IMAG	1

#define NEW_STRN	0
#define NEW_LONG	1

#define DEF_FLG		0
#define GEN_FLG		1
#define WTH_FLG		2

#define CMD_FLAG    1

clist_p append_import_param(void* param, clist_p list, clist_p iparam);
clist_p append_label_to_import_path(void* param, clist_p list, obj_p label);
clist_p append_list(void* param, clist_p list, code_p item);
clist_p append_list_label(void* param, clist_p list, obj_p label);
clist_p append_list_list(void* param, clist_p list, clist_p item);
clist_p empty_list(void* param);
clist_p import_path_as_label_to_import_param(void* param, clist_p list, obj_p label);
clist_p import_path_to_import_param(void* param, clist_p list);
clist_p label_to_forrefs(void* param, obj_p label1, obj_p label2);
clist_p label_to_import_path(void* param, obj_p label);
clist_p new_import_param(void* param, clist_p list);
clist_p new_list(void* param, code_p item);
clist_p new_list2(void* param, code_p item1, code_p item2);
clist_p new_list_list(void* param, clist_p params);
code_p append_code(void* param, code_p p1, code_p p3, int size);
code_p assign_cleanup(void* param, code_p ass_code);
code_p assign_right(void* param, clist_p right);
code_p attrref_to_tgtparm(void* param, code_p p1);
code_p binary(void* param, code_p lparm, int op, code_p rparm);
code_p body_except_else_to_tryexcept(void* param, code_p body, clist_p except_list, code_p els);
code_p break_stmt(void* param, obj_p label);
code_p cond_exc_to_assert(void* param, code_p cond, code_p exc);
code_p continue_stmt(void* param, obj_p label);
code_p del_ref(void* param, code_p expr);
code_p self_label_to_attrref(void* param, obj_p label);
code_p super_label_to_attrref(void* param, obj_p label);
code_p outer_label_to_attrref(void* param, obj_p label);
code_p caller_label_to_attrref(void* param, obj_p label);
code_p self_to_obj(void* param);
code_p caller_to_obj(void* param);
code_p exceptall(void* param);
code_p exec_expr(void* param, code_p expr);
code_p expr_andor_expr(void* param, code_p lparm, code_p rparm, int andor_flg);
code_p expr_label_body_to_except(void* param, code_p expr, obj_p label, code_p body);
code_p expr_to_param(void* param, code_p expr);
code_p expr_to_slice(void* param, code_p expr);
code_p expr_to_sparm(void* param, code_p p1);
code_p float_to_obj(void* param, double num, obj_p int_obj, int imag_flag);
code_p expr_for_lcc_to_listcomp(void* param, code_p expr, clist_p forlst, clist_p lcc);
code_p for_lcc_body_else(void* param, clist_p locals, code_p body, code_p els);
code_p from_path_import_params(void* param, clist_p path, clist_p iparms);
code_p if_expr_body_elif_else(void* param, clist_p ifex_list, code_p body, clist_p elif, code_p els);
code_p import_params(void* param, clist_p iparms);
code_p int_to_obj(void* param, obj_p num);
code_p label_eq_expr_to_formparm(void* param, obj_p label, code_p expr);
code_p label_eq_expr_to_param(void* param, obj_p label, code_p expr);
code_p label_to_attrref(void* param, obj_p label);
code_p label_to_formparm(void* param, obj_p label);
code_p lbl_proto_body_to_obj(void* param, obj_p label, code_p ref, clist_p protos, code_p body);
code_p new_dict(void* param, clist_p lst);
code_p new_tuple_list(void* param, clist_p lst, int tuple_flag);
code_p none(void* param);
code_p obj_func_params(void* param, code_p obj, clist_p parms);
code_p obj_label_to_attrref(void* param, code_p obj, obj_p label);
code_p op_expr(int op, code_p expr, int size);
code_p push_null(void* param);
code_p raise_expr(void* param, code_p expr1,  code_p expr2);
code_p ref_expr_to_ass_op(void* param, code_p ref, code_p rparm, int iop, int op);
code_p ref_parm_body_to_def(void* param, code_p name, clist_p params, code_p self, code_p body, int flg) ;
code_p return_expr(void* param, code_p expr);
code_p rquote_lbl_to_obj(void* param, obj_p label);
code_p bound_bind_params(void* param, code_p attr_ref, code_p obj);
code_p unbound_bind_params(void* param, code_p attr_ref, code_p obj);
code_p bound_func_params(void* param, code_p attr_ref, clist_p parms);
code_p unbound_func_params(void* param, code_p attr_ref, clist_p parms, int cmd_flag);
code_p bound_bind_func_params(void* param, code_p attr_ref, code_p obj, clist_p parms);
code_p unbound_bind_func_params(void* param, code_p attr_ref, code_p obj, clist_p parms);
code_p sparms_to_slice(void* param, code_p p1, code_p p2, code_p p3, int parm_cnt);
code_p stack_check(void* param, code_p stmt);
code_p star_ref_to_formparm(void* param, obj_p label);
code_p star_seq_to_param(void* param, code_p expr);
code_p star_star_dict_to_param(void* param, code_p expr);
code_p star_star_ref_to_formparm(void* param, obj_p label);
code_p string_to_obj(void* param, obj_p str);
code_p tgt_eq_expr_to_stmt(void* param, clist_p left, clist_p right);
code_p tgt_eq_stmt_to_stmt(void* param, clist_p left, code_p right);
code_p tgtparm_to_obj(void* param, code_p expr);
code_p tryfinally_body(void* param, code_p body, code_p final);
code_p unary(void* param, int op, code_p parm);
code_p while_expr_body_else(void* param, obj_p label, code_p expr, code_p body, code_p els);
code_p word_expr_to_param(void* param, obj_p word, code_p expr);
code_p yield_expr(void* param, code_p expr);
void calc_code(code *p, int *len, int *stack_depth, int *max_stack_depth);
void finished(void* param, code_p code, int nerrs);

#endif // #define PARSER_H
