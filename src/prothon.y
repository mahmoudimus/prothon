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


// prothon.y

/* prothon parser source file */
 
%{

#define YYERROR_VERBOSE 1

#define alloca			pr_malloc
#define YYLEX_PARAM		yylex_param
#define YYPARSE_PARAM	yylex_param

#define INITIAL_STRING_ALLOC 1024

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "clist.h"
#include <apr_strings.h>

#define state ((parse_state*) yylex_param)
#define IST  (((parse_state*) yylex_param)->ist)

#define TD new_string_obj(state->ist, "~")

#define YYDEBUG 0
#ifdef TRACE_PARSER
#undef YYDEBUG
#define YYDEBUG 1
#define fout   state->debug_stream
#endif
%}

/* BISON Declarations */

%pure_parser 

%union {
	obj_p		int_type;
	double		float_type;
	obj_p		str_type;
	code_p		code_type;
	clist_p		list_type;
}

%{
int yylex (YYSTYPE *lvalp, void* yylex_param);
%}

%nonassoc AS 
%nonassoc ASSERT    
%nonassoc BREAK     
%nonassoc CALLER  
%nonassoc CONTINUE  
%nonassoc DEF_       
%nonassoc DEL       
%nonassoc ELIF      
%nonassoc ELSE      
%nonassoc EXCEPT_    
%nonassoc EXEC      
%nonassoc FINALLY   
%nonassoc FOR       
%nonassoc FROM      
%nonassoc GEN      
%nonassoc IF        
%nonassoc IMPORT    
%nonassoc OBJ_      
%nonassoc OUTER      
%nonassoc PASS      
%nonassoc PROP      
%nonassoc RAISE     
%nonassoc RETURN    
%nonassoc SELF    
%nonassoc SUPER 
%nonassoc TRY       
%nonassoc WHILE     
%nonassoc WITH    
%nonassoc YIELD     

%nonassoc ADD_EQ 
%nonassoc SUB_EQ 
%nonassoc MUL_EQ 
%nonassoc DIV_EQ 
%nonassoc TDV_EQ 
%nonassoc REM_EQ 
%nonassoc BAND_EQ
%nonassoc BOR_EQ 
%nonassoc BXOR_EQ
%nonassoc RSH_EQ 
%nonassoc LSH_EQ 
%nonassoc EXP_EQ 

%nonassoc ADD_BANG 
%nonassoc SUB_BANG 
%nonassoc MUL_BANG 
%nonassoc DIV_BANG 
%nonassoc TDV_BANG 
%nonassoc REM_BANG 
%nonassoc BAND_BANG
%nonassoc BOR_BANG 
%nonassoc BXOR_BANG
%nonassoc RSH_BANG 
%nonassoc LSH_BANG 
%nonassoc EXP_BANG 

%right    '=' 
 
%nonassoc '['	   
%nonassoc ']'	   
%nonassoc '{'	   
%nonassoc '}'	   
%nonassoc '('   
%nonassoc ')' 
  
%left	  ','	   

%left	  OR
%left	  AND
%nonassoc IN_
%nonassoc NOT_IN
%left     IS
%right    NOT

%left	  '<'	 
%left	  LE	 
%left	  '>' 	 
%left	  GE	
%left	  NE	 
%left	  EQ	 

%left	  '|'	   
%left	  '^'	   
%left	  '&'	   
%left	  LSH	 
%left	  RSH	 
%left	  '+'	   
%left	  '-'  
%left	  '*'	   
%left	  '/'	   
%left	  TDV	 
%left	  '%'	   
%left     UPL     
%left     UMI    
%nonassoc '~'	   
%nonassoc '$'	   
%right	  EXP	 
%left	  '.'	   
%left	  ':'	   
%nonassoc ELIPS	 
%nonassoc UDOT
%nonassoc PAREN
%nonassoc TUPLE
%nonassoc FUNC
%nonassoc INDENT
%nonassoc DEDENT
%left     EOL
%nonassoc ERR
%nonassoc DEFSTMT

%nonassoc <float_type>  FLOAT_IMAG		/*  (?:\d+\.[\deE+-]*|\d*\.\d+[\deE+-]*)[jJ]\b	*/
%nonassoc <float_type>  FLOAT_			/*  (?:\d+\.[\deE+-]*|\d*\.\d+[\deE+-]*)		*/
%nonassoc <int_type>	INT_IMAG		/*  [1-9]\d*[jJ]\b								*/
%nonassoc <int_type>	INT_			/*  [1-9]\d*, 0[xX][0-9a-fA-F]+, 0[0-7]+		*/
%nonassoc <str_type>	LONG_			/*  STRING version of int						*/
%nonassoc <str_type>	STRING			/*  python strs & \b[xX]['"][0-9a-fA-F]+['"]	*/
%nonassoc <str_type>	LABEL			/*  \b\w+\b										*/

%type <list_type> import_path import_param import_params target
			target_params func_params brace_params
			brace_params2 brace_param mul_except ass_expr_list
			formal_params elif mul_elif list_params
			list_params2 for_refs for_refs1 for_refs2
			tuple_params tuple_params2 for_lc_clause
			lc_clauses if_lc_clause protos proto

%type <code_type>	statement_block statement mul_statement
			mul_statements compound_statement expr obj obj_or_empty
			assignment_statement assignment_statement2
			def_statement import_statement small_statement
			if_statement while_statement for_statement
			try_except_statement except else_except
			try_finally_statement list_comp	 attr_ref def_ref gen_ref obj_ref
			assert_statement target_param unbound_ref bound_ref
			ass_add_statement ass_sub_statement
			ass_mul_statement ass_div_statement
			ass_tdv_statement ass_rem_statement
			ass_band_statement ass_bor_statement
			ass_bxor_statement ass_shr_statement
			ass_shl_statement ass_exp_statement compound_body
			else slice slice_param formal_param is_not_check
			function_param break_statement return_statement
			yield_statement with_statement continue_statement
			del_statement exec_statement command_statement 
			raise_statement prop_statement obj_statement				
					
/* Grammar */
%%

goal:  statement_block  { finished(yylex_param, $1, yynerrs); };

statement_block:    
		/* empty string */											{ $$ = none(yylex_param); }
	|	statement_block statement									{ $$ = append_code(yylex_param, $1, $2, 0); }
	;

statement:
		mul_statement
	|	compound_statement
	;
mul_statement:
		EOL															{ $$ = none(yylex_param); }
	|	mul_statements EOL
	;
mul_statements:
		small_statement												{ $$ = stack_check(yylex_param, $1); }
	|	mul_statements ';' small_statement							{ $$ = append_code(yylex_param, $1, stack_check(yylex_param, $3), 0); }
	;
compound_statement: 
		if_statement 		 | while_statement       |  def_statement    | with_statement |
		try_except_statement | try_finally_statement |  for_statement	 | obj_statement  |
		prop_statement
	;
small_statement: 
		assignment_statement | assert_statement  | import_statement   |	
		ass_add_statement    | ass_sub_statement | ass_mul_statement  |
		ass_div_statement    | ass_tdv_statement | ass_rem_statement  |
		ass_band_statement   | ass_bor_statement | ass_bxor_statement |
		ass_shr_statement    | ass_shl_statement | ass_exp_statement  |
		break_statement      | continue_statement| del_statement      |
   		exec_statement	     | raise_statement   | return_statement   |
   	    yield_statement      | command_statement | expr               |
   	    PASS														{ $$ = none(yylex_param); }
	;
if_statement: 
		if_lc_clause compound_body mul_elif else					{ $$ = if_expr_body_elif_else(yylex_param, $1, $2, $3, $4); }
	;
mul_elif:
		/* empty string */											{ $$ = empty_list(yylex_param); }
	| 	mul_elif elif												{ $$ = append_list_list(yylex_param, $1, $2); }
	;
elif:
		ELIF expr compound_body 									{ $$ = new_list2(yylex_param, $2, $3); }
	;
else:
		/* empty string */											{ $$ = none(yylex_param); }
	|	ELSE compound_body 											{ $$ = $2; }
	;
while_statement: 
		WHILE expr compound_body else
		{ $$ = while_expr_body_else(yylex_param, TD, $2, $3, $4); }
	|	LABEL ':' WHILE expr compound_body
		{ $$ = while_expr_body_else(yylex_param, $1, $4, $5, NULL); }
	;
for_statement: 
		for_lc_clause compound_body else							{ $$ = for_lcc_body_else(yylex_param, $1, $2, $3); }
	;
list_comp:
		'[' expr for_lc_clause lc_clauses ']'						{ $$ = expr_for_lcc_to_listcomp(yylex_param, $2, $3, $4); }
	;
lc_clauses:
		/* empty string */											{ $$ = empty_list(yylex_param); }
	|   lc_clauses if_lc_clause										{ $$ = append_list_list(yylex_param, $1, $2); }
	|   lc_clauses for_lc_clause									{ $$ = append_list_list(yylex_param, $1, $2); }
	;
if_lc_clause:
		IF expr														{ $$ = new_list(yylex_param, $2); }
	;
for_lc_clause:
		for_refs IN_ expr											{ $$ = append_list(yylex_param, $1, $3); }
	;
for_refs:
		for_refs1
	|	for_refs2 ')'
	;
for_refs1:
		FOR LABEL													{ $$ = label_to_forrefs(yylex_param, TD, $2); }
	|	LABEL ':' FOR LABEL											{ $$ = label_to_forrefs(yylex_param, $1, $4); }
	|	for_refs1 ',' LABEL											{ $$ = append_list_label(yylex_param, $1, $3); }
	;																
for_refs2:
		FOR '(' LABEL												{ $$ = label_to_forrefs(yylex_param, TD, $3); }
	|	LABEL ':' FOR '(' LABEL										{ $$ = label_to_forrefs(yylex_param, $1, $5); }
	|	for_refs2 ',' LABEL											{ $$ = append_list_label(yylex_param, $1, $3); }
	;	
break_statement: 
		BREAK 														{ $$ = break_stmt(yylex_param, TD); }
	|	BREAK LABEL													{ $$ = break_stmt(yylex_param, $2); }
	;
continue_statement: 
		CONTINUE   													{ $$ = continue_stmt(yylex_param, TD); }
	|	CONTINUE LABEL 												{ $$ = continue_stmt(yylex_param, $2); }
	;
del_statement:
		DEL target_param											{ $$ = del_ref(yylex_param, $2); }
	|	del_statement ',' target_param								{ $$ = append_code(yylex_param, $1, del_ref(yylex_param, $3), 0); }
	;
exec_statement: 
		EXEC expr													{ $$ = exec_expr(yylex_param, $2); }
	;
assert_statement:
		ASSERT expr ',' expr										{ $$ = cond_exc_to_assert(yylex_param, $2, $4); }
	;
raise_statement: 
		RAISE expr													{ $$ = raise_expr(yylex_param, $2, NULL); }
	|	RAISE expr ',' expr											{ $$ = raise_expr(yylex_param, $2, $4); }
	; 
return_statement: 
		RETURN expr													{ $$ = return_expr(yylex_param, $2); }
	;
yield_statement: 
		YIELD expr													{ $$ = yield_expr(yylex_param, $2); }
	;
try_except_statement: 
		TRY compound_body mul_except else_except					{ $$ = body_except_else_to_tryexcept(yylex_param, $2, $3, $4); }
	;
mul_except:
		except														{ $$ = new_list(yylex_param, $1); }
	|	mul_except except											{ $$ = append_list(yylex_param, $1, $2); }
	;
except:
		EXCEPT_ expr compound_body									{ $$ = expr_label_body_to_except(yylex_param, $2, NULL, $3); }
	|	EXCEPT_ expr AS LABEL compound_body						    { $$ = expr_label_body_to_except(yylex_param, $2, $4, $5); }
	;
else_except:
		/* empty string */											{ $$ = none(yylex_param); }
	|	ELSE compound_body											{ $$ = $2; }
	;
try_finally_statement: 
		TRY compound_body FINALLY compound_body						{ $$ = tryfinally_body(yylex_param, $2, $4); }
	;
with_statement:
		WITH obj compound_body										{ $$ = ref_parm_body_to_def(yylex_param, NULL, NULL, $2, $3, WTH_FLG); }						
	;
prop_statement:
		PROP LABEL compound_body									{ $$ = lbl_proto_body_to_obj(yylex_param, $2, NULL, NULL, $3); }
	;
obj_statement:	
		obj_ref  compound_body										{ $$ = lbl_proto_body_to_obj(yylex_param, NULL, $1, NULL, $2); }
	|	obj_ref '('  protos ')' compound_body						{ $$ = lbl_proto_body_to_obj(yylex_param, NULL, $1,   $3, $5); }
	;
obj_ref:
		OBJ_ LABEL													{ $$ = label_to_attrref(yylex_param, $2);		state->defdoc_flag=1; }
	|	OBJ_ SELF '.' LABEL											{ $$ = self_label_to_attrref(yylex_param, $4);	state->defdoc_flag=1; }
	|	OBJ_ OUTER '.' LABEL										{ $$ = outer_label_to_attrref(yylex_param, $4);	state->defdoc_flag=1; }
	|	OBJ_ CALLER '.' LABEL										{ $$ = caller_label_to_attrref(yylex_param, $4);state->defdoc_flag=1; }
	|	OBJ_ obj '.' LABEL											{ $$ = obj_label_to_attrref(yylex_param, $2, $4);state->defdoc_flag=1; }
	;										
protos:
		/*none*/													{ $$ = empty_list(yylex_param); }
	|	proto														
	;																
proto:
		attr_ref													{ $$ = new_list(yylex_param, $1); }
	|	proto ',' attr_ref											{ $$ = append_list(yylex_param, $1, $3); }
	;
def_statement:	
		def_ref '('  formal_params ')' compound_body				{ $$ = ref_parm_body_to_def(yylex_param, $1, $3, NULL, $5, DEF_FLG); }
	|	gen_ref '('  formal_params ')' compound_body				{ $$ = ref_parm_body_to_def(yylex_param, $1, $3, NULL, $5, GEN_FLG); }
	;
def_ref:
		DEF_ LABEL													{ $$ = label_to_attrref(yylex_param, $2);		state->defdoc_flag=1; }
	|	DEF_ SELF '.' LABEL											{ $$ = self_label_to_attrref(yylex_param, $4);	state->defdoc_flag=1; }
	|	DEF_ OUTER '.' LABEL										{ $$ = outer_label_to_attrref(yylex_param, $4);	state->defdoc_flag=1; }
	|	DEF_ CALLER '.' LABEL										{ $$ = caller_label_to_attrref(yylex_param, $4);state->defdoc_flag=1; }
	|	DEF_ obj '.' LABEL											{ $$ = obj_label_to_attrref(yylex_param, $2, $4);state->defdoc_flag=1; }
	;										
gen_ref:
		GEN LABEL													{ $$ = label_to_attrref(yylex_param, $2);		state->defdoc_flag=1; }
	|	GEN SELF '.' LABEL											{ $$ = self_label_to_attrref(yylex_param, $4);	state->defdoc_flag=1; }
	|	GEN OUTER '.' LABEL											{ $$ = outer_label_to_attrref(yylex_param, $4);	state->defdoc_flag=1; }
	|	GEN CALLER '.' LABEL										{ $$ = caller_label_to_attrref(yylex_param, $4);	state->defdoc_flag=1; }
	|	GEN obj '.' LABEL											{ $$ = obj_label_to_attrref(yylex_param, $2, $4);state->defdoc_flag=1; }
	;										
import_statement:													
		IMPORT import_params										{ $$ = import_params(yylex_param, $2); }
	|   FROM import_path IMPORT import_params						{ $$ = from_path_import_params(yylex_param, $2, $4); }
	;
import_params:
		import_param												{ $$ = new_import_param(yylex_param, $1); }
	|	import_params ',' import_param								{ $$ = append_import_param(yylex_param, $1, $3); }
	;
import_param:
		import_path													{ $$ = import_path_to_import_param(yylex_param, $1); }
	|   import_path AS LABEL										{ $$ = import_path_as_label_to_import_param(yylex_param, $1,$3); }
	;	
import_path:
		LABEL														{ $$ = label_to_import_path(yylex_param, $1); }
	|	import_path '.' LABEL										{ $$ = append_label_to_import_path(yylex_param, $1, $3); }
	;
compound_body:
		':' EOL INDENT statement_block DEDENT						{ $$ = $4; }
	|	':' mul_statement											{ $$ = $2; }
	;
assignment_statement:
		assignment_statement2										{ $$ = assign_cleanup(yylex_param, $1); }
	;
assignment_statement2:
		target '=' ass_expr_list									{ $$ = tgt_eq_expr_to_stmt(yylex_param, $1, $3); }
	|	target '=' assignment_statement2							{ $$ = tgt_eq_stmt_to_stmt(yylex_param, $1, $3); }
	;
ass_expr_list:
		expr														{ $$ = new_list(yylex_param, $1); }
	|	ass_expr_list ',' expr										{ $$ = append_list(yylex_param, $1, $3); }
	; 
ass_add_statement:
		target_param ADD_EQ expr									{ $$ = ref_expr_to_ass_op(yylex_param, $1, $3, IADD_, ADD_); }
	;
ass_sub_statement:
		target_param SUB_EQ expr									{ $$ = ref_expr_to_ass_op(yylex_param, $1, $3, ISUB_, SUB_); }
	;
ass_mul_statement:
		target_param MUL_EQ expr									{ $$ = ref_expr_to_ass_op(yylex_param, $1, $3, IMUL_, MUL_); }
	;
ass_div_statement:
		target_param DIV_EQ expr									{ $$ = ref_expr_to_ass_op(yylex_param, $1, $3, IDIV_, DIV_); }
	;
ass_tdv_statement:
		target_param TDV_EQ expr									{ $$ = ref_expr_to_ass_op(yylex_param, $1, $3, IFLOORDIV_, FLOORDIV_); }
	;
ass_rem_statement:
		target_param REM_EQ expr									{ $$ = ref_expr_to_ass_op(yylex_param, $1, $3, IMOD_, MOD_); }
	;
ass_band_statement:
		target_param BAND_EQ expr									{ $$ = ref_expr_to_ass_op(yylex_param, $1, $3, IAND_, AND_); }
	;
ass_bor_statement:
		target_param BOR_EQ expr									{ $$ = ref_expr_to_ass_op(yylex_param, $1, $3, IOR_, OR_); }
	;
ass_bxor_statement:
		target_param BXOR_EQ expr									{ $$ = ref_expr_to_ass_op(yylex_param, $1, $3, IXOR_, XOR_); }
	;
ass_shr_statement:
		target_param RSH_EQ expr									{ $$ = ref_expr_to_ass_op(yylex_param, $1, $3, IRSHIFT_, RSHIFT_); }
	;
ass_shl_statement:
		target_param LSH_EQ expr									{ $$ = ref_expr_to_ass_op(yylex_param, $1, $3, ILSHIFT_, LSHIFT_); }
	;
ass_exp_statement:
		target_param EXP_EQ expr									{ $$ = ref_expr_to_ass_op(yylex_param, $1, $3, IPOW_, POW_); }
	;
is_not_check:
		/* empty string */											{ $$ = (void *)(unsigned long)IS__QUES; }
	|	NOT															{ $$ = (void *)(unsigned long)ISNOT__QUES; }
	;
expr:	
		expr OR expr												{ $$ = expr_andor_expr(yylex_param, $1, $3, OR_FLAG); }
	| 	expr AND expr												{ $$ = expr_andor_expr(yylex_param, $1, $3, AND_FLAG); }
	| 	expr IN_ expr												{ $$ = binary(yylex_param, $1, IN__QUES, $3); }
	| 	expr NOT IN_ expr		%prec NOT_IN						{ $$ = binary(yylex_param, $1, NOTIN__QUES, $4); }
	| 	expr IS is_not_check expr									{ $$ = binary(yylex_param, $1, (int)(intptr_t)$3, $4); }
	|	NOT expr													{ $$ = unary(yylex_param, NOT__QUES, $2); }
	| 	expr '<' expr			%prec EQ							{ $$ = binary(yylex_param, $1, LT__QUES, $3); }
	| 	expr LE expr			%prec EQ							{ $$ = binary(yylex_param, $1, LE__QUES, $3); }
	| 	expr '>'  expr			%prec EQ							{ $$ = binary(yylex_param, $1, GT__QUES , $3); }
	| 	expr GE expr			%prec EQ							{ $$ = binary(yylex_param, $1, GE__QUES, $3); }
	| 	expr NE expr			%prec EQ							{ $$ = binary(yylex_param, $1, NE__QUES, $3); }
	| 	expr EQ expr			%prec EQ							{ $$ = binary(yylex_param, $1, EQ__QUES, $3); }
	| 	expr '|' expr												{ $$ = binary(yylex_param, $1, OR_, $3); }
	| 	expr '^' expr												{ $$ = binary(yylex_param, $1, XOR_, $3); }
	| 	expr '&' expr												{ $$ = binary(yylex_param, $1, AND_, $3); }
	| 	expr LSH expr      											{ $$ = binary(yylex_param, $1, LSHIFT_, $3); }
	| 	expr RSH expr      											{ $$ = binary(yylex_param, $1, RSHIFT_, $3); }
	| 	expr '+' expr      											{ $$ = binary(yylex_param, $1, ADD_, $3); }
	| 	expr '-' expr      											{ $$ = binary(yylex_param, $1, SUB_, $3); }
	| 	expr '*' expr      											{ $$ = binary(yylex_param, $1, MUL_, $3); }
	| 	expr '/' expr      											{ $$ = binary(yylex_param, $1, DIV_, $3); }
	| 	expr TDV expr      											{ $$ = binary(yylex_param, $1, FLOORDIV_, $3); }
	| 	expr '%' expr      											{ $$ = binary(yylex_param, $1, MOD_, $3); }
	| 	expr ADD_BANG expr      									{ $$ = binary(yylex_param, $1, IADD_, $3); }
	| 	expr SUB_BANG expr      									{ $$ = binary(yylex_param, $1, ISUB_, $3); }
	| 	expr MUL_BANG expr      									{ $$ = binary(yylex_param, $1, IMUL_, $3); }
	| 	expr DIV_BANG expr      									{ $$ = binary(yylex_param, $1, IDIV_, $3); }
	| 	expr TDV_BANG expr      									{ $$ = binary(yylex_param, $1, IFLOORDIV_, $3); }
	| 	expr REM_BANG expr      									{ $$ = binary(yylex_param, $1, IMOD_, $3); }
	| 	expr BAND_BANG expr      									{ $$ = binary(yylex_param, $1, IAND_, $3); }
	| 	expr BOR_BANG expr      									{ $$ = binary(yylex_param, $1, IOR_, $3); }
	| 	expr BXOR_BANG expr      									{ $$ = binary(yylex_param, $1, IXOR_, $3); }
	| 	expr RSH_BANG expr      									{ $$ = binary(yylex_param, $1, IRSHIFT_, $3); }
	| 	expr LSH_BANG expr      									{ $$ = binary(yylex_param, $1, ILSHIFT_, $3); }
	| 	expr EXP_BANG expr      									{ $$ = binary(yylex_param, $1, IPOW_, $3); }
	| 	'+' expr 							%prec UPL      			{ $$ = unary(yylex_param, POS_, $2); }
	| 	'-' expr 							%prec UMI				{ $$ = unary(yylex_param, NEG_, $2); }
	| 	'~' expr      												{ $$ = unary(yylex_param, INV_, $2); }
	| 	expr EXP expr												{ $$ = binary(yylex_param, $1, POW_, $3); }
	|   list_comp
	|	obj
	;
obj:
		SELF														{ $$ = self_to_obj(yylex_param); }
	|	CALLER														{ $$ = caller_to_obj(yylex_param); }
	|	INT_														{ $$ = int_to_obj(yylex_param, $1); }
	| 	FLOAT_														{ $$ = float_to_obj(yylex_param,  $1, NULL, NEW_REAL); }
	| 	INT_IMAG													{ $$ = float_to_obj(yylex_param, 0.0,   $1, NEW_IMAG); }
	|	FLOAT_IMAG													{ $$ = float_to_obj(yylex_param,  $1, NULL, NEW_IMAG); }
	| 	STRING														{ $$ = string_to_obj(yylex_param, $1); }
	|   '$' LABEL							%prec '$'				{ $$ = rquote_lbl_to_obj(yylex_param, $2); }
	|	target_param												{ $$ = tgtparm_to_obj(yylex_param, $1); }
	|	'{' brace_params '}'										{ $$ = new_dict(yylex_param, $2); }
	|	'[' list_params ']'  	 									{ $$ = new_tuple_list(yylex_param, $2, NEW_LIST_); }
	| 	'(' expr ')'      			 								{ $$ = $2; }
	| 	'(' tuple_params ')'         								{ $$ = new_tuple_list(yylex_param, $2, NEW_TUPLE_); }
	|	obj '(' func_params ')'  									{ $$ = obj_func_params(yylex_param, $1, $3); }
	| 	bound_ref '{' obj_or_empty '}'  							{ $$ = bound_bind_params(yylex_param, $1, $3); }
	| 	unbound_ref '{' obj_or_empty '}'  							{ $$ = unbound_bind_params(yylex_param, $1, $3); }
	| 	bound_ref '{' obj_or_empty '}' '(' func_params ')'  		{ $$ = bound_bind_func_params(yylex_param, $1, $3, $6); }
	| 	unbound_ref '{' obj_or_empty '}' '(' func_params ')'  		{ $$ = unbound_bind_func_params(yylex_param, $1, $3, $6); }
	| 	bound_ref '(' func_params ')'  								{ $$ = bound_func_params(yylex_param, $1, $3); }
	| 	unbound_ref '(' func_params ')'  							{ $$ = unbound_func_params(yylex_param, $1, $3, 0); }
	;
obj_or_empty:
		/* empty string */											{ $$ = push_null(yylex_param); }
	|	obj
	;
target:
		target_params
	|	'[' target_params ']'  				 						{ $$ = $2; }
	|	'[' target_params ',' ']'  			 						{ $$ = $2; }
	| 	'(' target_params ')'										{ $$ = $2; }
	| 	'(' target_params ',' ')'									{ $$ = $2; }
	;
target_params:
		target_param												{ $$ = new_list(yylex_param, $1); }
	|	target_params ',' target_param								{ $$ = append_list(yylex_param, $1, $3); }
	;
target_param:
		attr_ref
	|	obj '[' slice ']'											{ $$ = append_code(yylex_param, $1, $3, 0); }
	;
attr_ref:
		unbound_ref
	|	bound_ref
	;
unbound_ref:
		LABEL														{ $$ = label_to_attrref(yylex_param, $1); }
	;
bound_ref:
		SELF '.' LABEL												{ $$ = self_label_to_attrref(yylex_param, $3); }
	|	SUPER '.' LABEL												{ $$ = super_label_to_attrref(yylex_param, $3); }
	|	OUTER '.' LABEL												{ $$ = outer_label_to_attrref(yylex_param, $3); }
	|	CALLER '.' LABEL											{ $$ = caller_label_to_attrref(yylex_param, $3); }
	|	obj '.' LABEL												{ $$ = obj_label_to_attrref(yylex_param, $1, $3); }
	;
slice:
		expr														{ $$ = expr_to_slice(yylex_param, $1); }
	|	slice_param ':' slice_param									{ $$ = sparms_to_slice(yylex_param, $1, $3, NULL, 2); }
	|	slice_param ':' slice_param ':' slice_param					{ $$ = sparms_to_slice(yylex_param, $1, $3,   $5, 3); }
	;
slice_param:
		/* empty string */											{ $$ = none(yylex_param); }
	|	expr														{ $$ = expr_to_sparm(yylex_param, $1); }
	;
func_params:
		/* empty string */											{ $$ = empty_list(yylex_param); }
	|	function_param												{ $$ = new_list(yylex_param, $1); }
	|	func_params ',' function_param								{ $$ = append_list(yylex_param, $1, $3); }
	;
function_param:
		expr														{ $$ = expr_to_param(yylex_param, $1); }
	|	LABEL '=' expr												{ $$ = label_eq_expr_to_param(yylex_param, $1,$3); }
	|   '*' expr													{ $$ = star_seq_to_param(yylex_param, $2); }
	|   EXP expr													{ $$ = star_star_dict_to_param(yylex_param, $2); }
	;
formal_params:
		/* empty string */											{ $$ = empty_list(yylex_param); }
	|	formal_param												{ $$ = new_list(yylex_param, $1); }
	|	formal_params ',' formal_param								{ $$ = append_list(yylex_param, $1, $3); }
	;
formal_param:
		LABEL														{ $$ = label_to_formparm(yylex_param, $1); }
	|	LABEL '=' expr												{ $$ = label_eq_expr_to_formparm(yylex_param, $1, $3); }
	|   '*' LABEL													{ $$ = star_ref_to_formparm(yylex_param, $2); }
	|   EXP LABEL													{ $$ = star_star_ref_to_formparm(yylex_param, $2); }
	;
brace_params:
		/* empty string */											{ $$ = empty_list(yylex_param); }
	|	brace_params2
	|	brace_params2 ','
	;
brace_params2:
		brace_param													{ $$ = new_clist_1($1); }
	|	brace_params2 ',' brace_param								{ $$ = append_list_list(yylex_param, $1, $3); }
	;
brace_param:
		expr ':' expr												{ $$ = new_list2(yylex_param, $1, $3); }
	;
tuple_params:
		/* empty string */											{ $$ = new_clist(0); }
	|	expr ','													{ $$ = new_list(yylex_param, $1); }
	|	tuple_params2
	|	tuple_params2 ','
	;
tuple_params2:
		expr ',' expr												{ $$ = new_list2(yylex_param, $1, $3); }
	|	tuple_params2 ',' expr										{ $$ = append_list(yylex_param, $1, $3); }
	;
list_params:
		/* empty string */											{ $$ = empty_list(yylex_param); }
	|	list_params2
	|	list_params2 ','
	;
list_params2:
		expr														{ $$ = new_list(yylex_param, $1); }
	|	list_params2 ',' expr										{ $$ = append_list(yylex_param, $1, $3); }
	;
command_statement:
		LABEL func_params                                           { $$ = unbound_func_params(yylex_param, label_to_attrref(yylex_param, $1), $2, CMD_FLAG); }
	;
%%

// **************************************************************************************************
// Lexical analyzer places a value on the stack and
//   returns the token.  Skips all blanks and tabs, 
//   returns 0 for EOF.
// **************************************************************************************************

#include <ctype.h>

// special get character function
// automatically handle different format line ends (\r, \n, & \r\n -> \n)
// increment lines and columns for YYLTYPE in parser
char getch(parse_state* stat){
	char c;
	if (stat->char_ptr) {
		c = stat->old_chars[--stat->char_ptr];
		if (c == '\n') {
			stat->line++;
			stat->old_column = stat->column;
			stat->column = 1;
			return '\n';
		}
	} else if (stat->stream) {
		c = fgetc(stat->stream);
		if (c == '\r' || c == '\n'){
			if (c == '\r' && (c = fgetc(stat->stream)) != '\n')
				stat->old_chars[stat->char_ptr++] = c;
			stat->line++;
			stat->old_column = stat->column;
			stat->column = 1;
			return '\n';
		}
	} else if (stat->in_buf && *(stat->in_buf)) {
		c = *((stat->in_buf)++);
		if (c == '\r' || c == '\n'){
			if (c == '\r' && (c = *((stat->in_buf)++)) != '\n')
				stat->old_chars[stat->char_ptr++] = c;
			stat->line++;
			stat->old_column = stat->column;
			stat->column = 1;
			return '\n';
		}
	} else c = EOF;
	stat->column++;
	return c;
}

// special put-back character function
// decrement lines and columns for YYLTYPE in parser
void ungetch(char c, parse_state* stat){
	stat->old_chars[stat->char_ptr++] = c;
	if (c == '\n'){
		stat->line--;
		stat->column = stat->old_column;
	}
	else
		stat->column--;
}

void add_string(char c, char **str_ptr, int *str_index, int *cur_size){
	if (*str_index == *cur_size-1){
		char *oldbuf = *str_ptr;
		*cur_size *= 2;
		*str_ptr = pr_malloc(*cur_size);
		oldbuf[*str_index] = 0;
		strcpy(*str_ptr, oldbuf);
	}
	(*str_ptr)[(*str_index)++] = c;
}

int yylex (YYSTYPE *lvalp, void* yylex_param){
	int c, indent_cnt=0, unknown_indent_flag=0, mixed_indents, level_mismatch;

	if (state->indents_needed > 0){
		state->indents_needed--;
		return INDENT;
	}
	if (state->indents_needed < 0){ 
		state->indents_needed++;
		return DEDENT;
	}

	/* process white space and comments */
	while(1){
		c=getch(state);
check_comment:
		while (c == ' ' || c == '\t') c = getch(state);
		if (c == '#')
			while ((c=getch(state)) != '\n' && c != EOF ) ;
		if (c == '/'){
			int c2;
			if ((c2=getch(state)) == '*' )	{
				while(1){
					while((c2=getch(state)) != '*' && c2 != EOF ) ;
					if (c2 == EOF || (c2=getch(state)) == '/')
						break;
					ungetch(c2, state);
				}
				continue;
			} else {
				ungetch(c2, state);
				break;
			}
		}
		if (c == '\\'){
			int c2;
			if ((c2=getch(state)) == '\n' )
				continue;
			else
				ungetch(c2, state);
		}
		if (c == '\n'){
			int llen = clist_len(state->indent_stack);
			level_mismatch = 0;
			unknown_indent_flag = 0; 
			// if a ( { or [ is on top of indent stack then this is a continuation line
			if (clist_len(state->indent_stack) && clist_tos_num(state->indent_stack) > 0)
				continue;
			if (!state->indent_char) {
				if ((c=getch(state)) == '\t' || c == ' ') {
					int tmp_indent_char = c, new_chr_cnt = 1;
					while ((c=getch(state)) == tmp_indent_char) new_chr_cnt++;
					if (!state->last_line_colon) goto check_comment;
					state->indent_char = tmp_indent_char;
					clist_append_num(state->indent_stack, -new_chr_cnt);
					indent_cnt = 1;
				}
			} else {
				int chr_cnt;
				for(chr_cnt=0; (c=getch(state)) == state->indent_char; chr_cnt++) ;
				for (indent_cnt=0; chr_cnt > 0 && indent_cnt < llen; indent_cnt++)
					chr_cnt -= -clist_num(state->indent_stack, indent_cnt);
				if (chr_cnt < 0)
					level_mismatch = TRUE;
				else if (chr_cnt)  {
					if (!state->last_line_colon) goto check_comment;
					indent_cnt++;
					clist_append_num(state->indent_stack, -chr_cnt);
				}
			}
			mixed_indents = ( (state->indent_char == '\t' && c == ' ') ||
						      (state->indent_char == ' '  && c == '\t')  );
			while (c == ' ' || c == '\t') c = getch(state);
			if (c == '\n' || c == '#')
				goto check_comment;
			if (c == '/') {
				int c2=getch(state);
				ungetch(c2, state);
				if (c2 == '*'){
					unknown_indent_flag = 1;
					goto check_comment;	
				}
			}
			ungetch(c, state);
			if (level_mismatch) {
				printf("Lexical error: indentation level does not match any previous level\n");
				return ERR;
			}
			if (mixed_indents){
				printf("Lexical error: mixed tabs and spaces in indentation\n");
				return ERR;
			}
			state->indents_needed = indent_cnt - state->old_tab_count;
			state->old_tab_count = indent_cnt;
			while (clist_len(state->indent_stack) > indent_cnt) clist_pop(state->indent_stack);
			if (!indent_cnt) state->indent_char = 0;
			return EOL;			
		}
		break;
	}
	if (c == EOF){
		if (state->old_tab_count) {
			state->indents_needed = 0 - state->old_tab_count;
			state->old_tab_count  = 0;
			return EOL;
		} else if (!state->eof_flag) {
			state->eof_flag = 1;
			return EOL;
		} else
			return 0;   // EOF
	}
	if (unknown_indent_flag) {
		printf("Lexical error: Inline quote not allowed at beginning of indented line\n");
		return ERR;
	}
	state->last_line_colon = FALSE;
	/* process quotes */
	if (c == 'r' || c == 'R' || c == 'x' || c == 'X' || c == '\'' || c == '\"') {
		int r_flag=0, x_flag=0, triple_flag=0, q_char=c;
		int str_index=0, cur_size=INITIAL_STRING_ALLOC;
		char* str_ptr = pr_malloc(cur_size);
		if (c == 'r' || c == 'R') {
			if ((q_char=getch(state)) == '\'' || q_char == '\"' )
				r_flag = 1;
			else {
				ungetch(q_char, state);
				goto not_quote;
			}
		}
		if (c == 'x' || c == 'X') {
			if ((q_char=getch(state)) == '\'' || q_char == '\"' )
				x_flag = 1;
			else {
				ungetch(q_char, state);
				goto not_quote;
			}
		}
		if ((c=getch(state)) == q_char ) {
			int c2;
			if ((c2=getch(state)) == q_char )
				triple_flag = 1;
			else {
				ungetch(c2, state);
				ungetch(c, state);
			}
		} else
			ungetch(c, state);
		while(1){
			int c2, c3;
			c = getch(state);
			if ((c == EOF || c == '\n') && !triple_flag)
				goto runon_quote;
			if (c == q_char){
				if (!triple_flag)
					break;
				if ((c2=getch(state)) == q_char ) {
					if ((c3=getch(state)) == q_char )
						break;
					else
						ungetch(c3, state);
				} else
					ungetch(c2, state);
			}
			if (x_flag){
				int val = 0;
				if (c >= '0' && c <= '9') 
					val = c - '0';
				else if (c >= 'a' && c <= 'f') 
					val = c - 'a' + 10;
				else if (c >= 'A' && c <= 'F') 
					val = c - 'A' + 10;
				else
					continue;
				c = getch(state);
				if (c >= '0' && c <= '9') 
					val = val*16 + c - '0';
				else if (c >= 'a' && c <= 'f') 
					val = val*16 + c - 'a' + 10;
				else if (c >= 'A' && c <= 'F') 
					val = val*16 + c - 'A' + 10;
				else {
					printf("Lexical error: hex digits in hex string must appear in pairs\n");
					return ERR;
				}
				add_string(val, &str_ptr, &str_index, &cur_size);
				continue;
			}
			if (!r_flag && c == '\\') {
				c2=getch(state);
				if (c2 == '\n')
					continue;
				switch (c2){
				case '\\': c3 = '\\'; break;
				case '\'': c3 = '\''; break;
				case '\"': c3 = '\"'; break;
				case 'a':  c3 = '\a'; break;
				case 'b':  c3 = '\b'; break;
				case 'f':  c3 = '\f'; break;
				case 'n':  c3 = '\n'; break;
				case 'r':  c3 = '\r'; break;
				case 't':  c3 = '\t'; break;
				case 'v':  c3 = '\v'; break;
				case 'o':
					if ((c2=getch(state)) < '0' || c2 > '3' ){
						printf("Lexical error: bad octal constant \\ooo\n");
						return ERR;
					}
					c3 = c2-'0';
					if ((c2=getch(state)) < '0' || c2 > '7' ){
						printf("Lexical error: bad octal constant \\ooo\n");
						return ERR;
					}
					c3 = c3*8+(c2-'0');
					if ((c2=getch(state)) < '0' || c2 > '7' ){
						printf("Lexical error: bad octal constant \\ooo\n");
						return ERR;
					}
					c3 = c3*8+(c2-'0');
					break;
				case 'x':
				case 'X':  
					if ((c2=getch(state)) >= '0' && c2 <= '9' )
						c3 = (c2-'0');
					else if (c2 >= 'a' && c2 <= 'f' )
						c3 = (c2-'a'+10);
					else if (c2 >= 'A' && c2 <= 'F' )
						c3 = (c2-'A'+10);
					else {
						printf("Lexical error: bad hex constant \\xXX\n");
						return ERR;
					}
					if ((c2=getch(state)) >= '0' && c2 <= '9' )
						c3 = c3*16+(c2-'0');
					else if (c2 >= 'a' && c2 <= 'f' )
						c3 = c3*16+(c2-'a'+10);
					else if (c2 >= 'A' && c2 <= 'F' )
						c3 = c3*16+(c2-'A'+10);
					else {
						printf("Lexical error: bad hex constant \\xXX\n");
						return ERR;
					}
					break;
				default:
					printf("Lexical error: bad escape sequence \\%c\n",c2);
					return ERR;
				}
				c = c3;
			}
			add_string(c, &str_ptr, &str_index, &cur_size);
		}
		str_ptr[str_index++]=0;		
		str_ptr = pr_realloc(str_ptr, str_index);
		if (x_flag)
			lvalp->str_type = new_bytes_obj(IST, str_ptr, str_index-1);
		else
			lvalp->str_type = new_string_n_obj(IST, str_ptr, str_index-1); 
#ifdef TRACE_PARSER
	if (trace_parser){
		fprintf(((parse_state*) yylex_param)->debug_stream, "\"%s\"\n",str_ptr);
	}
#endif
		return STRING;
	}
	goto not_quote;
runon_quote:
	printf("Lexical error: unterminated quote string\n");
	return ERR;
not_quote:

	/* process SYMBOL tokens */
	if (c == '<'){
		int c2 = getch(state);
		if (c2 == '=')
			return LE;
		if (c2 == '<') {
			int c3;
			if ((c3=getch(state)) == '=')
				return LSH_EQ;
			else if (c3 == '!')
				return LSH_BANG;
			ungetch(c3, state);
			return LSH;
		}
		ungetch(c2, state);
	}
	if (c == '>'){
		int c2;
		if ((c2=getch(state)) == '=')
			return GE;
		if (c2 == '>'){
			int c3;
			if ((c3=getch(state)) == '=')
				return RSH_EQ;
			else if (c3 == '!')
				return RSH_BANG;
			ungetch(c3, state);
			return RSH;
		}
		ungetch(c2, state);
	}
	if (c == '/'){
		int c2 = getch(state);
		if (c2 == '=')
			return DIV_EQ;
		else if (c2 == '!')
			return DIV_BANG;
		if (c2 == '/') {
			int c3 = getch(state);
			if (c3 == '=')
				return TDV_EQ;
			else if (c3 == '!')
				return TDV_BANG;
			ungetch(c3, state);
			return TDV;
		}
		ungetch(c2, state);
	}
	if (c == '*'){
		int c2 = getch(state);
		if (c2 == '=')
			return MUL_EQ;
		else if (c2 == '!')
			return MUL_BANG;
		if (c2 == '*') {
			int c3 = getch(state);
			if (c3 == '=')
				return EXP_EQ;
			else if (c3 == '!')
				return EXP_BANG;
			ungetch(c3, state);
			return EXP;
		}
		ungetch(c2, state);
	}
	if (c == '!'){
		int c2;
		if ((c2=getch(state)) == '=')
			return NE;
		ungetch(c2, state);
	}
	if (c == '='){
		int c2;
		if ((c2=getch(state)) == '=')
			return EQ;
		ungetch(c2, state);
	}
	if (c == '+'){
		int c2;
		if ((c2=getch(state)) == '=')
			return ADD_EQ;
		else if (c2 == '!')
			return ADD_BANG;
		ungetch(c2, state);
	}
	if (c == '-'){
		int c2;
		if ((c2=getch(state)) == '=')
			return SUB_EQ;
		else if (c2 == '!')
			return SUB_BANG;
		ungetch(c2, state);
	}
	if (c == '%'){
		int c2;
		if ((c2=getch(state)) == '=')
			return REM_EQ;
		else if (c2 == '!')
			return REM_BANG;
		ungetch(c2, state);
	}
	if (c == '&'){
		int c2;
		if ((c2=getch(state)) == '=')
			return BAND_EQ;
		else if (c2 == '!')
			return BAND_BANG;
		ungetch(c2, state);
	}
	if (c == '|'){
		int c2;
		if ((c2=getch(state)) == '=')
			return BOR_EQ;
		else if (c2 == '!')
			return BOR_BANG;
		ungetch(c2, state);
	}
	if (c == '^'){
		int c2;
		if ((c2=getch(state)) == '=')
			return BXOR_EQ;
		else if (c2 == '!')
			return BXOR_BANG;
		ungetch(c2, state);
	}
	/* process LABEL or english keyword*/
	if (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')  ) {
		int str_index=0, cur_size=INITIAL_STRING_ALLOC;
		char* str_ptr = pr_malloc(cur_size);
		add_string(c, &str_ptr, &str_index, &cur_size);
		while ((c=getch(state)) == '_' || c == '!' || c == '?' || 
			    (c >= '0' && c <= '9')  || 
	    		(c >= 'a' && c <= 'z')  || 
	    		(c >= 'A' && c <= 'Z')    ) {
			add_string(c, &str_ptr, &str_index, &cur_size);
			if (c == '!' || c == '?') break;
		}
		if (c != '!' && c != '?')
			ungetch(c, state);
		str_ptr[str_index++]=0;	
		if (!strcmp(str_ptr,"or")) 		return OR;
		if (!strcmp(str_ptr,"and")) 	return AND;
		if (!strcmp(str_ptr,"not"))  	return NOT;
		if (!strcmp(str_ptr,"in")) 		return IN_;
		if (!strcmp(str_ptr,"is")) 		return IS;
		if (!strcmp(str_ptr,"as")) 		return AS;
		if (!strcmp(str_ptr,"assert")) 	return ASSERT;
		if (!strcmp(str_ptr,"break")) 	return BREAK;
		if (!strcmp(str_ptr,"caller")) 	return CALLER;
		if (!strcmp(str_ptr,"continue"))return CONTINUE;
		if (!strcmp(str_ptr,"def")) 	return DEF_;
		if (!strcmp(str_ptr,"del")) 	return DEL;
		if (!strcmp(str_ptr,"elif")) 	return ELIF;
		if (!strcmp(str_ptr,"else")) 	return ELSE;
		if (!strcmp(str_ptr,"except")) 	return EXCEPT_;
		if (!strcmp(str_ptr,"exec")) 	return EXEC;
		if (!strcmp(str_ptr,"finally")) return FINALLY;
		if (!strcmp(str_ptr,"for")) 	return FOR;
		if (!strcmp(str_ptr,"from")) 	return FROM;
		if (!strcmp(str_ptr,"gen")) 	return GEN;		
		if (!strcmp(str_ptr,"if")) 		return IF;
		if (!strcmp(str_ptr,"import")) 	return IMPORT;
		if (!strcmp(str_ptr,"object")) 	return OBJ_;
		if (!strcmp(str_ptr,"outer")) 	return OUTER;
		if (!strcmp(str_ptr,"pass")) 	return PASS;
		if (!strcmp(str_ptr,"prop")) 	return PROP;
		if (!strcmp(str_ptr,"raise")) 	return RAISE;
		if (!strcmp(str_ptr,"self")) 	return SELF;
		if (!strcmp(str_ptr,"super")) 	return SUPER;
		if (!strcmp(str_ptr,"return")) 	return RETURN;
		if (!strcmp(str_ptr,"try")) 	return TRY;
		if (!strcmp(str_ptr,"while")) 	return WHILE;
		if (!strcmp(str_ptr,"with")) 	return WITH;
		if (!strcmp(str_ptr,"yield")) 	return YIELD;

		str_ptr = pr_realloc(str_ptr, str_index);
		lvalp->str_type = new_string_n_obj(IST, str_ptr, str_index-1); 
#ifdef TRACE_PARSER
	if (trace_parser){
		fprintf(((parse_state*) yylex_param)->debug_stream, "\"%s\"\n",str_ptr);
	}
#endif
		return LABEL;
	}
	/* process dot, int, or float */
	if (c == '.'  || (c >= '0' && c <= '9') ) {
		int imag_flag=0, nonzero_flag=0, past_e_flag=0;
		int octal_flag=0, hex_flag=0, int_dot_flag=0;
		int str_index=0, cur_size=INITIAL_STRING_ALLOC;
		char* str_ptr = pr_malloc(cur_size);
		
		add_string(c, &str_ptr, &str_index, &cur_size);
		if (c == '.') {
			int c2 = getch(state);
			if (c2 == '.') {
				int c3 = getch(state);
				if (c3 == '.')
					return ELIPS;
				else {
					printf("Lexical error: two periods (..) not allowed\n");
					return ERR;
				}
			}
			ungetch(c2, state);
			if (!(c2 >= '0' && c2 <= '9') )
				return '.';
		} else {
			while ( ((c=getch(state)) >= '0' && c <= '9')				             ||
				    (hex_flag && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) ||
			        (str_index == 1 && str_ptr[0] == '0' && (c == 'x' || c == 'X'))	 ||
			        (str_index == 1 && str_ptr[0] == '0' &&  c == 'o')	             ||
			         c == 'j' || c == 'J' || c == '_'                                  ) {
			    if (c != '_')
					add_string(c, &str_ptr, &str_index, &cur_size);
				if (c == 'o')
					octal_flag = 1;
				else if (c == 'x' || c == 'X')
					hex_flag = 1;
				else if (c != '0')
					nonzero_flag = 1;
				if ((imag_flag = (c == 'j' || c == 'J')))
					break;
			}
			if (c == '.') {
				if ((c=getch(state)) < '0' || c > '9')
					int_dot_flag = 1;
				ungetch(c, state);
				c = '.';
			}
			if (octal_flag || hex_flag || int_dot_flag || (c != '.' && c != 'e' && c != 'E')){
				if (imag_flag)
					str_index--;
				else
					ungetch(c, state);
				str_ptr[str_index++] = 0;
				if (hex_flag) {
					long_p longp = cstr2longp(IST, str_ptr+2, NULL, 16); 
					if (IST->exception_obj) {
						IST->exception_obj = NULL;
						printf("Lexical error: invalid hex number\n");
						return ERR;
					}
					lvalp->int_type = new_int_long_obj(IST, longp);
				} else if (octal_flag) {
					long_p longp = cstr2longp(IST, str_ptr+2, NULL, 8); 
					if (IST->exception_obj) {
						IST->exception_obj = NULL;
						printf("Lexical error: invalid octal number\n");
						return ERR;
					}
					lvalp->int_type = new_int_long_obj(IST, longp);
				} else {
					if (strlen(str_ptr) < 16)
						lvalp->int_type = new_int_obj(IST, apr_atoi64(str_ptr));
					else {
						long_p longp = cstr2longp(IST, str_ptr, NULL, 10); 
						if (IST->exception_obj) {
							IST->exception_obj = NULL;
							printf("Lexical error: invalid integer number\n");
							return ERR;
						}
						lvalp->int_type = new_int_long_obj(IST, longp);
					}
				}
				if (nonzero_flag && int_is_zero(lvalp->int_type)){
					printf("Lexical error: invalid integer number format\n");
					return ERR;
				}
				if (imag_flag)
					return INT_IMAG;
				else
					return INT_;
			}
		}
		if (imag_flag)
			str_index--;
		else {
			while ((c >= '0' && c <= '9') || c == '.' || 
			        c == 'e' || c == 'E' || c == 'j' || c == 'J' || c == '+' || c == '-' ) {
				if ((imag_flag = (c == 'j' || c == 'J')))
					goto end_input;
				add_string(c, &str_ptr, &str_index, &cur_size);
				if (c == 'e' || c == 'E')
					past_e_flag = 1;
				else if (!past_e_flag && (c > '0' && c <= '9') )
					nonzero_flag = 1;
				c=getch(state);
			}
			ungetch(c, state);
		}
end_input:
		str_ptr[str_index++]=0;	
		lvalp->float_type = atof(str_ptr);
		if (nonzero_flag && lvalp->float_type == 0.0){
			printf("Lexical error: invalid floating number format\n");
			return ERR;
		}
		if (imag_flag)
			return FLOAT_IMAG;
		else
			return FLOAT_;
	}
	/* return other chars */
	if (c == '(') clist_push_num(state->indent_stack, c);
	if (c == '{') clist_push_num(state->indent_stack, c);
	if (c == '[') clist_push_num(state->indent_stack, c);
	if (c == ')') {
		if (!clist_len(state->indent_stack) || clist_tos_num(state->indent_stack) != '(') {
			printf("Lexical error: mismatched parens ()\n");
			return ERR;
		}
		clist_pop(state->indent_stack);
	}
	if (c == '}') {
		if (!clist_len(state->indent_stack) || clist_tos_num(state->indent_stack) != '{') {
			printf("Lexical error: mismatched braces {}\n");
			return ERR;
		}
		clist_pop(state->indent_stack);
	}
	if (c == ']') {
		if (!clist_len(state->indent_stack) || clist_tos_num(state->indent_stack) != '[') {
			printf("Lexical error: mismatched brackets []\n");
			return ERR;
		}
		clist_pop(state->indent_stack);
	}
	if (c == ':') state->last_line_colon = TRUE;
	return c;                                
}

/* Called by yyparse on error */
int yyerror (char* s){  
  // parse error, => Parse Error:
  if (s[0] == 'p' && s[6] == 'e' && s[11] == ','){
	 s[0]='P'; s[6]='E'; s[11]=':';
  }
  printf ("\n%s\n", s);
  return 0;
}


