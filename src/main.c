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


// main.c

// #define TIMEIT

#include <stdio.h>
#include <stdlib.h>

#include "object.h"
#include <prothon/prothon_dll.h>
#include "memory_mgr.h"
#include "parser.h"
#include "thread.h"
#include "interp.h"

#include <apr_time.h>
#include <apr_pools.h>
#include <apr_thread_proc.h>
#include <apr_portable.h>

#ifdef TRACE_PARSER
extern int yydebug;
#endif

static apr_pool_t *apr_root_pool = NULL;

//**************************** get_pr_head_pool *******************************
apr_pool_t* get_pr_head_pool(void) {
	if (!apr_root_pool) {
		printf("get_pr_head_pool error: called with no pool\n");
		pr_exit(1);
	}
	return apr_root_pool;
}

void sig_handler(int signum) {
	switch (signum) {
	case SIGTERM:
		signum = 0; // for breakpoint
	}
}

//********************************* main **************************************
int main(int argc, char *argv[]) {
	obj_p sys_argv_obj, main_argv_obj, threads, code_str_obj, main1_thread_obj;
	pr_thread_p main1_thread_ptr = NULL;
	isp ist;
	apr_status_t aprerr;
	char buf[1024];
	int cmd_line_option_i_flg;

	if ((aprerr = apr_app_initialize(&argc, (char const * const **)&argv, NULL))) {
		char buf[1024];
		printf("APR init error: %s\n", apr_strerror(aprerr, buf, 1024));
		pr_exit(1);
	}

	atexit(&apr_terminate);
	atexit(finalize_all);

	if ((aprerr = apr_pool_create(&apr_root_pool, NULL)))  {
		printf("Main error: apr_pool_create: %s\n",
		       apr_strerror(aprerr, buf, sizeof(buf)));
		pr_exit(1);
	}
	// these don't seem to work in win32 at all
	signal(SIGTERM, sig_handler); 
	signal(SIGABRT, sig_handler);
	signal(SIGFPE,  sig_handler);
	signal(SIGILL,  sig_handler); 
	signal(SIGINT,  sig_handler); 
	signal(SIGSEGV, sig_handler);
	signal(SIGTERM, sig_handler); 

	/* Now that apr is ready, on to our stuff */
	ist = new_ist(ACC_SYSTEM);

	object_store_init(ist, 20);

	thread_init(ist);

	sys_argv_obj = arginit(ist, argc, argv);
	sysinit(ist, argv[0], sys_argv_obj);

#ifdef TRACE_PARSER
	yydebug = trace_parser;
#endif

	if ((code_str_obj = get_attr(ist, sys_argv_obj, sym(ist, "code")))) {
		exec_string(ist, pr2c_strptr(ist, code_str_obj), FALSE, NULL);
		check_exceptions(ist);
	}
	cmd_line_option_i_flg = (int)(intptr_t) get_attr(ist, sys_argv_obj, sym(ist, "i"));

	main_argv_obj = get_attr(ist, sys_argv_obj, sym(ist, "argv"));
	if (main_argv_obj && list_len(ist, main_argv_obj)) {
		main1_thread_obj = new_thread_obj(ist, (apr_thread_start_t)main_thread, main_argv_obj, NULL, ACC_USER2);
		main1_thread_ptr = main1_thread_obj->data.ptr;
		if (cmd_line_option_i_flg) 
			main1_thread_ptr->save_frame = TRUE;
	}

	threads = get_attr(ist, sys_argv_obj, sym(ist, "threads"));
	if (threads) {
		int i;
		for (i = 0; i < (int) list_len(ist, threads); i++) {
			obj_p thrd_argv = list_item(ist, threads, i);
			obj_p acc_level = get_attr(ist, thrd_argv, SYM(ACCLEVEL_));  if_exc_return 0;
			new_thread_obj( ist, (apr_thread_start_t) main_thread, thrd_argv, 
				            NULL, (int) acc_level->data.i64 );
		}
	}
	if (!disable_mem_mgr)
		new_thread_obj(ist, (apr_thread_start_t)mem_mgr_thread, NULL, NULL, ACC_SYSTEM);

#ifdef DEBUG_THREADS
	if (debug_threads) SetThreadName("main");
#endif

#ifdef DEBUG_THREADS
	if (debug_threads) printf("All threads started, now waiting for mains to finish...\n");
#endif

	/* Grab this lock, so we know all threads are started */
	pr_lock(&thread_registry_lock); pr_unlock(&thread_registry_lock);

	if (!main_threads_started)  {
		ist->access = ACC_USER2;
		if (!ist->module)
			ist->module = NEW_OBJ(NULL);
		console(ist);
		ist->access = ACC_SYSTEM;
	} else {
		while(main_threads_running) {
			apr_sleep(PAUSE_MS*1000);
			if (cmd_line_option_i_flg && (!main1_thread_ptr || !main1_thread_ptr->running)) {
				ist->access = ACC_USER2;
				if (main1_thread_ptr)
					console(main1_thread_ptr->ist);
				else
					console(ist);
				ist->access = ACC_SYSTEM;
				cmd_line_option_i_flg = FALSE;
			}
		}
		if (cmd_line_option_i_flg) {
			ist->access = ACC_USER2;
			if (main1_thread_ptr)
				console(main1_thread_ptr->ist);
			else
				console(ist);
			ist->access = ACC_SYSTEM;
		}
	}

#ifdef DEBUG_THREADS
	if (debug_threads && !disable_mem_mgr) 
		printf("All main threads have terminated, now stopping memory manager ...\n");
#endif

	stop_mem_mgr();

#ifdef DEBUG_THREADS
	if (debug_threads && !disable_mem_mgr) 
		printf("Memory manager terminated\n");
	if (debug_threads)
		printf("Shutting down ...\n");
#endif

#ifdef DUMP_OBJECTS_AT_END
	if (dump_objects_at_end)
		dump(ist, "!object-dump.txt", OBJ(ROOT_GLOBALS));
#endif

	return 0;
}

// common exit point for debug breakpoints
void pr_exit(int n){
	exit(n);
}
