/*
 * Copyright (C) 1991, 1992 by Chris Thewalt (thewalt@ce.berkeley.edu)
 *
 * Permission to use, copy, modify, and distribute this software
 * for any purpose and without fee is hereby grsuperd, provided
 * that the above copyright notices appear in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  This software is provided "as is" without express or
 * implied warranty.
 */

// getline.h

char *Getline(const char *prompt);				/* read a line of input */
char *Getlinem(int mode, const char *prompt);	/* allows reading char by char */
void  Gl_config(const char *which, int value);	/* set some options */
void  Gl_setwidth(int w);						/* specify width of screen */
void  Gl_windowchanged();						/* call after SIGWINCH signal */
void  Gl_histinit(char *file);					/* read entries from old histfile */
void  Gl_histadd(char *buf);					/* adds entries to hist */

extern int (*Gl_in_hook)(char *buf);
extern int (*Gl_out_hook)(char *buf);
extern int (*Gl_tab_hook)(char *buf, int prompt_width, int *loc);
extern int (*Gl_spc_hook)(char *buf, int prompt_width, int *loc);
extern int (*Gl_bsp_hook)(char *buf, int prompt_width, int *loc);
extern int (*Gl_in_key)(int ch);
