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

// pr_arg.h

// this is used to pass a variable arg list from Prothon to apr_pr_vformatter
// routine in apr_snprintf.c.  apr_pr_vformatter is a Prothon custom version
// of apr_vformatter that uses a pr_arg_t list instead of a va_list.
// a pr_arg_t list includes type information so that a type mismatch will
// cause an exception instead of a crash.

#ifndef PR_ARG_H
#define PR_ARG_H

typedef enum {
	ARG_TYPE_END = 0,
	ARG_TYPE_INT,
	ARG_TYPE_FLOAT,
	ARG_TYPE_STRING,
	ARG_TYPE_ERROR,
} arg_type_t;

typedef union {
	apr_int64_t	i64;
	double		f64;
	char*		str;
} arg_arg_t;

typedef struct {
	arg_arg_t		arg;
	arg_type_t		type;
} pr_arg_t;

typedef pr_arg_t* pr_arg_p;

#define pr_arg_err       (ap[0].type  = ARG_TYPE_ERROR)
#define if_prerr_rtrn  if(ap[0].type == ARG_TYPE_ERROR) return -99

#define pr_arg_start(ap)	int _pr_arg_ind = 0;

#define pr_arg_int(ap)    \
	((ap)[_pr_arg_ind].type == ARG_TYPE_INT    ? ap[_pr_arg_ind++].arg.i64 : (pr_arg_err,   0) )
#define pr_arg_float(ap)  \
	((ap)[_pr_arg_ind].type == ARG_TYPE_FLOAT  ? ap[_pr_arg_ind++].arg.f64 : (pr_arg_err, 0.0) )
#define pr_arg_string(ap) \
	((ap)[_pr_arg_ind].type == ARG_TYPE_STRING ? ap[_pr_arg_ind++].arg.str : (pr_arg_err,  "") )

#define pr_arg_end(ap)		((ap)[_pr_arg_ind].type == ARG_TYPE_END)

APR_DECLARE(int) apr_pr_vsnprintf(char *buf, apr_size_t len, const char *format, pr_arg_p ap);

#endif // #ifndef PR_ARG_H
