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

// interp.h

#ifndef INTERP_H
#define INTERP_H

#include "object.h"
#include "parser.h"
#include "thread.h"

#include <apr_thread_proc.h>

isp new_ist(access_t access);

extern int debug_threads;
extern int dump_module_code;
extern int trace_interpreter; 
extern int dump_objects_at_end;
extern int disable_mem_mgr;

extern i32_t main_threads_started;
extern i32_t main_threads_running;

#define INITIAL_EXCEPTION_STACK_SIZE	10
#define EXCEPTION_STACK_GROWTH_FACTOR	2

typedef struct {
	int			pos_ptr;
	clist_p		lbl_val_list;
	int*		occupied_list;
	obj_p		free_pos_list_key;
	obj_p		free_pos_list_obj;
	obj_p		free_label_list_key;
	obj_p		free_label_list_obj;
} fparam_proc_state_t;

typedef struct frame_t {
	frame_p				next_frame;
	frame_p				prev_frame;
	int					called_from_c;
	int					do_not_free;
	obj_p				gen_marker;
	obj_p				func;
	obj_p				syn_self;
	obj_p				self;
	obj_p				syn_locals;
	obj_p				dyn_locals;
	obj_p				locals;
	int					pc;
	code_p				code;
	clist_p				exc_stack;
	int					stack_ptr;
	char*				func_name;
	fparam_proc_state_t fparam_proc_state;
	int					fparam_proc_state_idx;
	obj_p				new_locals;
	int					no_return_flag;
	obj_p				stack[];
} frame_t;

typedef struct {
	int				final;
	int				try_loc;
	int				exc_loc;
} exception_entry_t;

#define intrp_exobj		(ist? ist->exception_obj : NULL)

typedef struct {
	code_p		codep;
	frame_p		frame;
} gen_t;

typedef gen_t* gen_p;

frame_p create_frame( isp ist, obj_p self, obj_p syn_locals, obj_p dyn_locals, 
					           obj_p locals, obj_p func_obj, code_p code_in, char* name );

void *main_thread(apr_thread_t *handle, void *filename);
void *user_thread(apr_thread_t *handle, void *argv); 

obj_p exec_string(isp ist, char* str, int get_frame, frame_p *framep);

int load_module( isp ist, obj_p module_name, obj_p dyn_locals, 
				 obj_p dest_module, obj_p module_alias, char* filename, char* comment,
				 obj_p module_in, char* exec_string );
obj_p exec_loop(isp ist);


obj_p new_parmptr_obj(isp ist, int ofs);
void reset_as_str_stack(isp ist);

#define fr_next			(frame->next_frame)
#define fr_prev			(frame->prev_frame)
#define fr_ccall		(frame->called_from_c)
#define fr_pc			(frame->pc)
#define fr_code			(frame->code)
#define fr_op			(fr_code->code_data[fr_pc].bytecode.opcode)
#define fr_param		(fr_code->code_data[fr_pc].bytecode.param)
#define fr_data(i)		(fr_code->code_data[fr_pc+(i)].data)
#define fr_data_int(i)	((unsigned)(int)(uintptr_t)(fr_code->code_data[fr_pc+(i)].data))
#define fr_exstk		(frame->exc_stack)
#define fr_sp			(frame->stack_ptr)
#define fr_stack		(frame->stack)
#define fr_tos			(frame->stack[frame->stack_ptr-1])
#define fr_push(w)		 frame->stack[frame->stack_ptr++] = w
#define fr_pop			(frame->stack[--(frame->stack_ptr)])

void dump_code(isp ist, obj_p func, code_p code, char* filename);

#endif // #define INTERP_H
