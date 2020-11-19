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

// builtins-time.c

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <apr_time.h>
#include <apr_strings.h>

#include <prothon/prothon.h>
#include "object.h"
#include <prothon/prothon_dll.h>

MODULE_DECLARE(DateTime);

//********************************* new_time_obj *****************************
obj_p new_time_obj(isp ist, i64_t usecs){
	obj_p obj = NEW_OBJ(DateTime_OBJ);
	SET_TYPE_IF_EXC(DateTime_OBJ, obj, DATA_TYPE_IMMDATA) return NULL;
	obj->imm_data_len = 8;
	obj->data.i64 = usecs;
	obj->immutable = TRUE;
	return obj;
}

MODULE_START(DateTime)
{
	DateTime_OBJ = OBJ(DATETIME_PROTO);

	set_obj_doc(DateTime_OBJ, "DateTime object prototype");
	set_obj_id(DateTime_OBJ, *, DateTime);

	set_attr(ist, OBJ(OBJECT), sym(ist, "DateTime"), DateTime_OBJ);

	SET_TYPE_IF_EXC(DateTime_OBJ, DateTime_OBJ, DATA_TYPE_IMMDATA) return;
	DateTime_OBJ->imm_data_len = 8;
	DateTime_OBJ->data.i64     = apr_time_now();
}

DEF(DateTime, init_, FPARM8( year,        OBJ(NONE), month,  OBJ(NONE), 
	                         day,         OBJ(NONE), hour,   OBJ(NONE),      
						     minute,      OBJ(NONE), second, OBJ(NONE), 
						     microsecond, OBJ(NONE), tzinfo, OBJ(NONE) ) ) {
	apr_status_t aprerr;
	apr_time_t result;
	apr_time_exp_t aprxt;
	i32_t month=0, day=0, hour=0, minute=0, second=0;
	i64_t year=0, microsecond=0; 
	obj_p tzinfo;
	int have_yr = FALSE, have_mo = FALSE, have_dy = FALSE;
	int have_hr = FALSE, have_mi = FALSE, have_sc = FALSE, have_us = FALSE;

	BIN_EMPTY_CHK();
	if (parms[1]  != OBJ(NONE)) { have_yr = TRUE; INT_64_PARAM(1, year); }
	if (parms[3]  != OBJ(NONE)) { have_mo = TRUE; INT_32_PARAM(2, month); }
	if (parms[5]  != OBJ(NONE)) { have_dy = TRUE; INT_32_PARAM(3, day); }
	if (parms[7]  != OBJ(NONE)) { have_hr = TRUE; INT_32_PARAM(4, hour); }
	if (parms[9]  != OBJ(NONE)) { have_mi = TRUE; INT_32_PARAM(5, minute); }
	if (parms[11] != OBJ(NONE)) { have_sc = TRUE; INT_32_PARAM(6, second); }
	if (parms[13] != OBJ(NONE)) { have_us = TRUE; INT_64_PARAM(7, microsecond); }
	tzinfo = parms[15];

	SET_TYPE_IF_EXC(DateTime_OBJ, self, DATA_TYPE_IMMDATA) return NULL;
	self->imm_data_len = 8;
	if ( !have_yr && !have_mo && !have_dy &&
		 !have_hr && !have_mi && !have_sc && !have_us ) {
		self->data.i64 = apr_time_now();
		return OBJ(NONE);
	}
	if (  have_yr && !have_mo && !have_dy &&
		 !have_hr && !have_mi && !have_sc && !have_us ) {
		self->data.i64 = year;
		return OBJ(NONE);
	}
	if (!have_yr && !have_mo) {
		self->data.i64 = 
			(((i64_t)day)*24*60*60*1000000) + 
		    (((i64_t)hour)*60*60*1000000) + 
		    (((i64_t)minute)*60*1000000) + 
		    (((i64_t)second)*1000000) + microsecond;
		return OBJ(NONE);
	}
	memset(&aprxt, 0, sizeof(aprxt));
	aprxt.tm_year = ((i32_t)year)-1900;
	aprxt.tm_mon  = month-1;
	aprxt.tm_mday = day;
	aprxt.tm_hour = hour;
	aprxt.tm_min  = minute;
	aprxt.tm_sec  = second;
	aprxt.tm_usec = (apr_int32_t) microsecond;
	aprerr = apr_time_exp_gmt_get(&result, &aprxt);
	IF_APR_ERR("converting time params") return NULL;
	self->data.i64 = result;
	return OBJ(NONE);
}

DEF(DateTime, hash_, NULL) { 
	BIN_CONTENT_CHK(DateTime);
	return new_hash_obj(ist, self->data.i32[0]+self->data.i32[1]);
}

DEF(DateTime, cmp_, FORM_RPARAM){
	BIN_CONTENT_CHK(DateTime);
	CHECK_TYPE_EXC(parms[1], DateTime_OBJ, DateTime);
	if (self->data.i64 == parms[1]->data.i64)
		return NEW_INT(0);
	else if (self->data.i64 > parms[1]->data.i64)
		return NEW_INT(1);
	else
		return NEW_INT(-1);
}

DEF(DateTime, eq__QUES, FORM_RPARAM){
	BIN_CONTENT_CHK(DateTime);
	CHECK_TYPE_EXC(parms[1], DateTime_OBJ, DateTime);
	if (self->data.i64 == parms[1]->data.i64) return OBJ(PR_TRUE);
	else                                      return OBJ(PR_FALSE);
}

DEF(DateTime, str_, FPARM1(spec, NEW_STRING(""))) {
	char buf[APR_RFC822_DATE_LEN], *spec;
	BIN_STR_CHK(DateTime);
	STRING_PARAM(1, spec);
	if (strlen(spec) > 0) {
		raise_exception(ist, OBJ(OVERFLOW_EXC), "time str_ spec not supported yet");
		return NULL;
	}
	apr_rfc822_date(buf, self->data.i64);
	return NEW_STRING(buf);
}

MAIN_MODULE_INIT(DateTime)
{
	MODULE_SUB_INIT(DateTime);
	MODULE_ADD_SYM(DateTime, init_);
	MODULE_ADD_SYM(DateTime, str_);
	MODULE_ADD_SYM(DateTime, hash_);
	MODULE_ADD_SYM(DateTime, eq__QUES);
	MODULE_ADD_SYM(DateTime, cmp_);

	check_exceptions(ist);
}
