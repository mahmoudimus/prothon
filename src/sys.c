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


// sys.c

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#ifndef WIN32
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#else
#include <io.h>
#include <direct.h>
#endif

#include <prothon/prothon.h>
#include "object.h"
#include "interp.h"
#include "clist.h"

#include <apr_pools.h>
#include <apr_file_info.h>
#include <apr_fnmatch.h>
#include <apr_file_io.h>
#include <apr_portable.h>
#include <apr_strings.h>

static obj_p sys_exit(isp ist, obj_p self, int pcnt, obj_p* parms, obj_p dlocals) {
	apr_thread_t* apr_thread;
	if (!has_proto(ist, parms[1], OBJ(INT_PROTO))) {
		raise_exception(ist, OBJ(TYPE_EXC), "sys.exit parameter must be an integer");
		return NULL;
	}
	main_threads_running--;
	apr_thread = os_thread_2_apr(apr_os_thread_current());
	if (!apr_thread) {
		raise_exception(ist, OBJ(INTERNAL_EXC), "sys_exit: no thread to exit");
	} else
		apr_thread_exit(apr_thread, (int)parms[1]->data.i64);
	return NULL;
}

char* numonly(char* s, char* buf){
	char *d = buf;
	for(; *s; s++)
		if (*s >= '0' && *s <= '9') *(d++) = *s;
	*d = 0;
	return buf;
}

static void add_path(isp ist, obj_p list, char* path) {
	int i, llen = (int) list_len(ist, list);
	char full_path[1024];

#ifdef WIN32
	if(!_fullpath( full_path, path, sizeof(full_path))) {
		raise_exception(ist, OBJ(IO_EXC), "Bad path in PROTHONPATH: %s", path);
		return;
	}
#else
	/* XXX The use of _fullpath() above is to make relative paths
	 * absolute. I don't think this is needed in Unix, but we may need
	 * to validate the location (_fullpath() checks to make sure the
	 * path exists). */
	strncpy(full_path, path, sizeof(full_path));
#endif

	for(i=0; i < llen; i++) {
		char* item = pr2c_strptr(ist, list_item(ist, list, i));
		if (!strcmp(full_path, item))
			return;
	}
	list_append(ist, list, NEW_STRING(full_path));
}

obj_p load_sys_module(isp ist, char* prog_path, obj_p argv_obj) {
	int have_home = FALSE;
	char *s, *d, path[1024], home[1024], *pp;
	obj_p sys_path, vers_tuple;
	obj_p sys_module, cons_obj;
	apr_dir_t *search_dir = NULL;
	apr_finfo_t file_info;
	apr_file_t *file_pth;
	apr_pool_t *mod_pool = NULL;

	sys_path = NEW_LIST(10);
	sys_module = OBJ(SYS);
	set_attr(ist, OBJ(MODULES), sym(ist, "sys"), sys_module);
	set_attr(ist, OBJ(OBJECT),  sym(ist, "sys"), sys_module);
	set_obj_doc(sys_module, "sys: a built-in module of system attributes");
	add_path(ist, sys_path, "."); if_exc_return NULL;
#ifdef PROTHON_DEFAULT_MODDIR
	add_path(ist, sys_path, PROTHON_DEFAULT_MODDIR); if_exc_return NULL;
#endif

	if ((pp = getenv("PROTHONPATH"))) {
		s=pp;
		while(TRUE) {
			for (d=path; *s && *s!=';'; s++, d++) *d = *s;
			*d = 0;
			add_path(ist, sys_path, path); if_exc_return NULL;
			if (!*s++) break;
		}	
	}
	set_obj_doc(sys_path, "path: list of paths to search for modules");
	set_attr(ist, sys_module, sym(ist, "path"), sys_path);
	set_obj_wracc(sys_path, ACC_USER2);
	if ((s = getenv("PROTHONHOME"))) {
		for (d=path; *s && *s!=';'; s++, d++) *d = *s;
		*d = 0;
		if (strlen(path)) {
			size_t len = strlen(path);
			if (path[len-1] == '\\')
				path[len-1] = 0;
			strcpy(home, path);
			have_home = TRUE;
			set_attr(ist, sys_module, sym(ist, "home"), NEW_STRING(home));
		}
	}
	if (!have_home) {
#ifdef WIN32
		_getcwd(path, sizeof(path)); 
#else
		getcwd(path, sizeof(path));
#endif
		strcpy(home, path);
		have_home = TRUE;
		set_attr(ist, sys_module, sym(ist, "prothonhome"), NEW_STRING(home));
	}
	vers_tuple = NEW_TUPLE(3);
	list_append(ist, vers_tuple, NEW_STRING(PROTHON_VERSION_NUMBER));
	list_append(ist, vers_tuple, NEW_STRING(numonly(PROTHON_VERSION_BUILD, path)));
	list_append(ist, vers_tuple, NEW_STRING(PROTHON_VERSION_DATE));
	vers_tuple->immutable = TRUE;
	set_attr(ist, sys_module, sym(ist, "version"), vers_tuple);
	set_attr(ist, sys_module, sym(ist, "modules"), OBJ(MODULES));
	set_attr( ist, sys_module, sym(ist, "exit"),
		      new_C_func_obj(ist, sys_exit, list2(ist, sym(ist,"num"),NEW_INT(1))) );

#if defined(WIN32)
	set_attr(ist, sys_module, sym(ist, "platform"), NEW_STRING("win32"));
#elif defined(HAVE_SYS_UTSNAME_H)
	{
		struct utsname uts;

		if (uname(&uts)) {
			set_attr(ist, sys_module, sym(ist, "platform"), NEW_STRING("unknown"));
		} else {
			char plt_buf[1024];
			apr_snprintf(plt_buf, sizeof(plt_buf), "%s %s (%s)",
				     uts.sysname, uts.release, uts.machine);
			set_attr(ist, sys_module, sym(ist, "platform"), NEW_STRING(plt_buf));
		}
	}
#else
	set_attr(ist, sys_module, sym(ist, "platform"), NEW_STRING("unknown"));
#endif
	cons_obj = new_object(ist, NULL);
	set_obj_wracc(cons_obj, ACC_USER1);
	set_attr(ist, sys_module, sym(ist, "cons"), cons_obj);
	set_attr(ist, cons_obj, sym(ist, "ps1"), NEW_STRING("O>> "));
	set_attr(ist, cons_obj, sym(ist, "ps2"), NEW_STRING("... "));
	set_attr(ist, sys_module, sym(ist, "execpath"), NEW_STRING(prog_path));
	set_attr(ist, sys_module, sym(ist, "argv"),     argv_obj);

	if (!APR_STATUS_IS_SUCCESS(apr_pool_create(&mod_pool, NULL)))
		; // XXX Set exception?

	strcpy(path, home);
	if (apr_dir_open(&search_dir, path, mod_pool) == APR_SUCCESS) {
		while (apr_dir_read(&file_info, APR_FINFO_TYPE | APR_FINFO_NAME, search_dir) ==
			APR_SUCCESS) {
			int modname_cnt=0;
			char *p, *e, line[1024], modname[10];

			if (apr_fnmatch("*.pth", file_info.name, APR_FNM_PERIOD | APR_FNM_CASE_BLIND) !=
			    APR_SUCCESS)
				continue;

			apr_snprintf(path, 1024, "%s/%s", home, file_info.name);

			if (apr_file_open(&file_pth, path, APR_READ, 0, mod_pool) != APR_SUCCESS)
				continue;

			while (apr_file_gets(line, sizeof(line), file_pth) == APR_SUCCESS) {
				p = line;
				while (*p == ' ' || *p == '\t') p++;
				if (!(*p) || *p == '#') continue;
				if (!strncmp(p, "import ", 7)) {
					char doc[300];
					isp ist = new_ist(ACC_USER2);
					apr_snprintf(modname, sizeof(modname), "PthMod%d", ++modname_cnt);
					apr_snprintf(doc, sizeof(doc), ": a module loaded by %s",
							file_info.name);
					load_module( ist, sym(ist, modname), NULL, OBJ(OBJECT), 
						                              NULL, NULL, doc, NULL, p );
					if (ist->exception_obj) check_exceptions(ist);
					continue;
				}
				for (e=p; *e > ' '; e++);
				if (p == e) continue;
				*e = 0;
				list_append(ist, sys_path, NEW_STRING(p));
			}
			apr_file_close(file_pth);
		}

		apr_dir_close(search_dir);
	}

	apr_pool_destroy(mod_pool);

	return sys_module;
}

void sysinit(isp ist, char* prog_path, obj_p argv_obj) {
	load_sys_module(ist, prog_path, argv_obj);	
	if (check_exceptions(ist)) 
		pr_exit(1);
}
