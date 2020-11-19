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


// argproc.c

#include "object.h"
#include "memory_mgr.h"
#include "parser.h"
#include "interp.h"
#include <stdio.h>
#include <stdlib.h>

int num(char* p) {
	int value = 0;
	while (*p && (*p < '0' || *p > '9')) p++;
	if (!(*p)) {
		printf("Error: Numeric value missing from command line parameter\n");
		return -1;
	}
	while (*p >= '0' && *p <= '9') {
		value *= 10;
		value += *p++ - '0';
	}
	return value;
}

obj_p parse_argv(isp ist, char* p) {
	obj_p argv = NEW_LIST(4);
	char* p1;
	while(*p) {
		if (*p == '\'') {
			for (p1=p+1; *p1 && *p1 != '\''; p1++);
			if (!(*p1)) {
				printf ("Unterminated quote in command-line parameter\n");
				return NULL;
			}
			list_append(ist, argv, NEW_STRINGN(p+1, p1-p-1));
		} else {
			for (p1=p; *p1 && *p1 != ' ' && *p1 != '\t'; p1++);
			list_append(ist, argv, NEW_STRINGN(p, p1-p));
		}
		for (p = p1; *p && (*p == ' ' || *p == '\t'); p++);
	}
	return argv;
}

obj_p arginit(isp ist, int argc, char* argv[]) {
	int i, past_c_args = FALSE;
	obj_p thread_argv, argv_obj = NULL;
	obj_p thread_list = NEW_LIST(4), res = NEW_OBJ(NULL);
	char *p, buf[10];
	for (i=1; i < argc; i++) {
		if (!past_c_args && argv[i][0] != '-') {
			argv_obj = NEW_LIST(argc - i);
			set_attr(ist, res, sym(ist, "argv"), argv_obj);
			past_c_args = TRUE;
		}
		if (past_c_args)
			list_append(ist, argv_obj, NEW_STRING(argv[i]));
		else {
			for (p=argv[i]; *p == '-'; p++);
			switch (*p) {
			case 'd':
				for (p++; *p; p++)
					switch (*p) {
					case 'p': trace_parser = 1;			break;
					case 't': debug_threads = 1;		break;
					case 'b': dump_module_code = 1;		break;
					case 'c': trace_interpreter = 1;	break;
					case 'o': dump_objects_at_end = 1;	break;
					case 'G': disable_mem_mgr = 1;		break;
					}
				break;
			case 'm': 
				if (++i == argc) goto err;
				p=argv[i]; 
				if ((hi_mem_boundary = num(p)) < 0) goto err;		
				break;
			case 'l': 
				if (++i == argc) goto err;
				p=argv[i]; 
				if ((lo_mem_boundary = num(p)) < 0) goto err;		
				break;
			case 't': 
				if (++i == argc) goto err;
				if (!(thread_argv = parse_argv(ist, argv[i]))) goto err;
				list_append(ist, thread_list, thread_argv);
				set_attr(ist, thread_argv, SYM(ACCLEVEL_), NEW_INT(ACC_USER2));
				break;
			case 's': 
				if (++i == argc) goto err;
				if (!(thread_argv = parse_argv(ist, argv[i]))) goto err;
				list_append(ist, thread_list, thread_argv);
				set_attr(ist, thread_argv, SYM(ACCLEVEL_), NEW_INT(ACC_SYSTEM));
				break;
			case 'i': 
				set_attr(ist, res, sym(ist, "i"), OBJ(PR_TRUE));							
				break;
			case 'c': 
				if (++i == argc) goto err;
				set_attr(ist, res, sym(ist, "code"), NEW_STRING(argv[i]));	
				break;
			case 'v': 
			case 'V': 
				printf( "Prothon %s, Build %s, %s\n", PROTHON_VERSION_NUMBER,	
					     numonly(PROTHON_VERSION_BUILD, buf), PROTHON_VERSION_DATE ); 
				pr_exit(0);
			default:
err:
				printf("\n");
				printf("Usage: Prothon <- options> main-prothon-file cmd-line-params\n");
				printf("\n");
				printf("      Starting Prothon with no arguments runs interactive console.\n");
				printf("\n");
				printf(" - options:\n");
				printf("\n");
				printf("   -m --maxmem mem-size-in-megs\n");
				printf("      (Not implemented yet)\n");
				printf("      Maximum memory usage desired.  Garbage collector thread runs \n");
				printf("      in high priority when memory reaches this level.  Memory value\n");
				printf("      is a number indicating megabytes.\n");
				printf("\n");
				printf("   -l --lowmem mem-size-in-megs\n");
				printf("      (Not implemented yet)\n");
				printf("      Memory usage defined as low.  Garbage collector runs at low priority\n");
				printf("      (effectively off) when memory usage is below this level. Allows \n");
				printf("      maximum speed for app. Memory value is a number indicating megabytes.\n");
				printf("\n");
				printf("   -t --thread filename\n");
				printf("      filename.pr will run as a seperate thread simultaneously with any other\n");
				printf("      Prothon code.  More than one of these options may be specified in one\n");
				printf("      command.  If the prothon file needs command line parameters, then\n");
				printf("      surround the file name and parameters in quotes.  To nest quotes in the\n");
				printf("      command use single quote characters.\n");
				printf("\n");
				printf("   -s --system filename\n");
				printf("      Same as the -t (thread) option, except that the thread runs at system\n");
				printf("      security level.  This should only be used for trusted code.\n");
				printf("\n");
				printf("   -i --interact\n");
				printf("      Enter interactive session after the main-thread code terminates, leaving\n");
				printf("      variables from that code intact. Other threads may still be running\n");
				printf("      that you may interact with.\n");
				printf("\n");
				printf("   -c --code \"code to execute\"\n");
				printf("      Execute code in quotes before any code in files are run.  This option\n");
				printf("      may co-exist with the main file execution.\n");
				printf("\n");
#ifdef CMDLINE_FLAGS
				printf("   -d<flags>\n");
				printf("      Single letter flags must immediately follow the d to indicate what debug\n");
				printf("      options you wish turned on for this run of Prothon.  Choose from:\n");
#ifdef TRACE_PARSER
				printf("         p: Parser trace dumps\n");
#endif
#ifdef DUMP_MODULE_CODE
				printf("         b: Bytecode disassembly dumps\n");
#endif
#ifdef TRACE_INTERPRETER
				printf("         c: code trace dumps\n");
#endif
#ifdef DUMP_OBJECTS_AT_END
				printf("         o: Dump all objects at end of execution\n");
#endif
#ifdef DEBUG_THREADS
				printf("         t: Thread status messages\n");
#endif
#ifdef DEBUG_MEM_MGR
				printf("         g: Garbage collector status messages\n");
#endif
				printf("         G: Disable garbage collector\n");
				printf("\n");
#endif
				printf("   -V -- version\n");
				printf("      Print version and quit\n");
				printf("\n");
				printf("   -h --help\n");
				printf("      Print this message and quit.\n");
				printf("\n");
				pr_exit(0);
			}
		} 
	}
	if (list_len(ist,thread_list))
		set_attr(ist, res, sym(ist, "threads"), thread_list);
	return res;
}
