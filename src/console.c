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

// console.c

#include <stdlib.h>

#include <apr_strings.h>

#include "object.h"
#include "getline.h"
#include "interp.h"

#define INDENT_WIDTH  3

static obj_p sys;
static char *src, *line;

static int pr_tab(char *buf, int offset, int *loc);
int (*Gl_tab_hook)(char *buf, int prompt_width, int *loc) = pr_tab;
int (*Gl_spc_hook)(char *buf, int prompt_width, int *loc) = pr_tab;

static int pr_bsp(char *buf, int offset, int *loc);
int (*Gl_bsp_hook)(char *buf, int prompt_width, int *loc) = pr_bsp;

void pr_exit(int n);

int indent_cnt(char* buf) {
	int n = 0;
	char* p;
	for (p = buf; !strncmp(p, "                   ", INDENT_WIDTH); p += INDENT_WIDTH, n++);
	return n;
}

int in_indent(char* buf, int len) {
	return (len == indent_cnt(buf)*INDENT_WIDTH);
}

char* ps1(isp ist){
	obj_p prompt;
	if (!sys) sys = get_attr(ist, OBJ(OBJECT), sym(ist, "sys"));
	prompt = get_attr(ist, sys, sym(ist, "cons"));
	prompt = get_attr(ist, prompt, sym(ist, "ps1"));
	return pr2c_strptr(ist, prompt);
}

char* ps2(isp ist){
	obj_p prompt;
	if (!sys) sys = get_attr(ist, OBJ(OBJECT), sym(ist, "sys"));
	prompt = get_attr(ist, sys, sym(ist, "cons"));
	prompt = get_attr(ist, prompt, sym(ist, "ps2"));
	return pr2c_strptr(ist, prompt);
}

int get_src(isp ist) {
	int i,ind_cnt;
	char *d, *s, *lim;
	line = Getline(ps1(ist));
	if (!(*line)) return TRUE;
    Gl_histadd(line);
	ind_cnt = indent_cnt(line);
	lim = line+strlen(line)+1;
	for (d=line, i=0;  i < ind_cnt; d++, i++) *d = '\t';
	for (s=line+ind_cnt*INDENT_WIDTH; s < lim; d++, s++) *d = *s;
	src = pr_realloc(src, strlen(line)+1);
	strcpy(src, line);
	return FALSE;
}

int add_src(isp ist) {
	int i,ind_cnt;
	char *d, *s, *lim;
	line = Getline(ps2(ist));
	if (!(*line)) return TRUE;
    Gl_histadd(line);
	ind_cnt = indent_cnt(line);
	lim = line+strlen(line)+1;
	for (d=line, i=0;  i < ind_cnt; d++, i++) *d = '\t';
	for (s=line+ind_cnt*INDENT_WIDTH; s < lim; d++, s++) *d = *s;
	src = pr_realloc(src, strlen(src)+strlen(line)+1);
	strcat(src, line);
	return FALSE;
}

int tab_cnt(char* s) {
	int tab_count=0, spc_count=0;
	char* p;
	for (p=s; *p == '\t' || *p == ' '; p++) {
		if (*p == '\t') tab_count++;
		if (*p == ' ')  spc_count++;
	}
	if (spc_count) printf("Warning: spaces ignored in indentation\n");
	return tab_count;
}

static int pr_tab(char *buf, int offset, int *loc) {
     int i, count, len;
	 int ind_flg = in_indent(buf, *loc);

     len = (int) strlen(buf);
	 if (ind_flg) count = INDENT_WIDTH;
     else         count = 1;
     for (i=len; i >= *loc; i--)
         buf[i+count] = buf[i];
	 for (i=0; i < count; i++)
         buf[*loc+i] = ' ';
     i = *loc;
     *loc = i + count;
     return i;
}
 
static int pr_bsp(char *buf, int offset, int *loc) {
	int i = *loc, len;
	int ind_flg = in_indent(buf, *loc);
	len = (int) strlen(buf);
	if (*loc) {
		if (ind_flg) {
			for (i=*loc; i <= len; i++)
				buf[i-INDENT_WIDTH] = buf[i];
			i = *loc;
			*loc = i - INDENT_WIDTH;
		} else {
			for (i=*loc; i <= len; i++)
				buf[i-1] = buf[i];
			i = *loc;
			*loc = i - 1;
		}
	 }
     return *loc;
}


#define PROTHON_HISTORY  ".prothon_history"

void console(isp ist) {
	obj_p res, new_self, new_locals;
	frame_p frame = NULL;
    char buf[16];
	int ind_level = 0, colon_flg = FALSE;
	char histfile[512], *home;
	
	ist->in_console = TRUE;
	new_self = new_object(ist, NULL);
	if (!ist->frame) {
		if (ist->module)
			new_locals  = ist->module;
		else
			new_locals  = NEW_OBJ(NULL);
		frame = create_frame(ist, new_self, NULL, NULL, new_locals, NULL, NULL, "console");
	}
	if ((home = getenv("HOME")) != NULL)
		apr_snprintf(histfile, sizeof(histfile), "%s/%s", home, PROTHON_HISTORY);
	else
		apr_cpystrn(histfile, PROTHON_HISTORY, sizeof(histfile));

	Gl_histinit(histfile);

	printf( "\nProthon %s Interactive Console, Build %s, %s (Ctrl-D to exit)\n", 
		    PROTHON_VERSION_NUMBER, numonly(PROTHON_VERSION_BUILD, buf), PROTHON_VERSION_DATE ); 
	src = pr_malloc(1);
    while (TRUE) {
		if (get_src(ist)) goto done;
		if (tab_cnt(line)) {
			printf ("Error: outer line should not be indented\n");
			continue;
		}
		while (src[strlen(line)-2] == '\\')
			if (add_src(ist)) goto done;
		colon_flg = (strlen(line) >= 2 && src[strlen(line)-2] == ':');
		while (colon_flg || ind_level) {
			int ind_chg;
			if (add_src(ist)) goto done;
			ind_chg = tab_cnt(line) - ind_level;
			if (ind_chg < 2) {
				if (colon_flg && ind_chg != 1) {
					printf ("Error: incorrect indentation for new block\n");
					ind_level = 0; 
					colon_flg = FALSE;
					pr_realloc(src, 1); src[0]=0;
					break;
				}
				ind_level += ind_chg;
			}
			colon_flg = (strlen(line) >= 2 && line[strlen(line)-2] == ':');
		}
		reset_as_str_stack(ist);
		res = exec_string(ist, src, TRUE, &frame);
		if (check_exceptions(ist)) {
			ist->frame = NULL;
		} else {
			if (res && res != OBJ(NONE)) {
				char* res_str = as_str(ist, res);
				if (res_str) {
					if (res_str[strlen(res_str)-1] == '\n')
						printf("%s", res_str);
					else
						printf("%s\n",res_str);
				}
			}
		}
    }
done:
	pr_free(src);
	if (frame) pr_free(frame);
}

