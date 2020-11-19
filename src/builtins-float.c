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


// builtins.c

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <apr_strings.h>

#include <prothon/prothon.h>
#include "dict.h"
#include "parser.h"
#include "object.h"
#include <prothon/prothon_dll.h>


#define	FLOAT_DATA_SIZE		8

#define is_Float(objid)		(has_proto(ist, objid, Float_OBJ))
#define Float_value(objid)	(objid->data.f64)
MODULE_DECLARE(Float);

//********************************* new_float_obj *****************************
obj_p new_float_obj(isp ist, double num){
	obj_p obj = NEW_OBJ(Float_OBJ);
	SET_TYPE_IF_EXC(Float_OBJ, obj, DATA_TYPE_IMMDATA) return NULL;
	obj->imm_data_len = 8;
	obj->data.f64 = num;
	obj->immutable = TRUE;
	return obj;
}

MODULE_START(Float)
{
	Float_OBJ = OBJ(FLOAT_PROTO) = NEW_OBJ(NULL);
//	Imag_OBJ  = OBJ(IMAG_PROTO)  = NEW_OBJ(NULL);
                OBJ(IMAG_PROTO)  = NEW_OBJ(NULL);

	set_obj_doc(OBJ(FLOAT_PROTO),   "Float number object prototype");
	set_obj_id(Float_OBJ, *, Float);
	set_obj_doc(OBJ(IMAG_PROTO),    "Imaginary number object prototype");
	set_obj_id(OBJ(IMAG_PROTO), *, Imaginary);

	set_attr(ist, OBJ(OBJECT), sym(ist, "Float"),     OBJ(FLOAT_PROTO));
	set_attr(ist, OBJ(OBJECT), sym(ist, "Imaginary"), OBJ(IMAG_PROTO));

	SET_TYPE_IF_EXC(Float_OBJ, Float_OBJ, DATA_TYPE_IMMDATA) return;
	Float_OBJ->imm_data_len = IMM_DATA_LEN;
	Float_OBJ->data.f64     = 0.0;

	//Imag_OBJ->data_type    = DATA_TYPE_IMMDATA;
	//Imag_OBJ->imm_data_len = IMM_DATA_LEN;
	//Imag_OBJ->data.f64     = 0.0;
}

DEF(Float, init_, FORM_STAR_PARAM) { 
	double f = 0.0;
	BIN_EMPTY_CHK();
	if (list_len(ist, parms[1]))
		FLOAT__PARAM(0, f);
	SET_TYPE_IF_EXC(Float_OBJ, self, DATA_TYPE_IMMDATA) return NULL;
	self->imm_data_len = IMM_DATA_LEN;
	self->data.f64     = f;
	return OBJ(NONE);
}

DEF(Float, bool__QUES, NULL) { 
	BIN_CONTENT_CHK(Float);
	if (Float_value(self) != 0.0 )
		return OBJ(PR_TRUE);
	else
		return OBJ(PR_FALSE);
}

DEF(Float, hash_, NULL) { 
	double flt = (double) Float_value(self);
	BIN_CONTENT_CHK(Float);
	if (flt < 0) flt = -flt;
	if (flt == 0.0) flt = 1.0;
	while (flt < 100.0) flt *= 10.0;
	while (flt > 1e9) flt /= 10.0;
	return new_hash_obj(ist, (i32_t)flt);
}

DEF(Float, abs_, NULL) {
	double num = Float_value(self);
	BIN_CONTENT_CHK(Float);
	return NEW_FLOAT(num < 0 ? -num : num);
}

DEF(Float, neg_, NULL){
	BIN_CONTENT_CHK(Float);
	return NEW_FLOAT( - Float_value(self) );
}

DEF(Float, pos_, NULL){
	BIN_CONTENT_CHK(Float);
	return self;
}

DEF(Float, add_, FORM_RPARAM){
	double float_other;
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Float);
	if (is_Float(other)) float_other = Float_value(other);
	else {
		if (has_proto(ist, other, OBJ(INT_PROTO)))
			float_other = (double)other->data.i64;
		else {
			if (covers(other, self))
				return call_func1(ist, other, SYM(ADD_), self);
			raise_exception(ist, OBJ(TYPE_EXC), "Float cannot be added to this object");
			return OBJ(NONE);
		}
	}
	return NEW_FLOAT(Float_value(self) + float_other);
}

DEF(Float, div_, FORM_RPARAM){
	double float_other;
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Float);
	if (is_Float(other)) float_other = Float_value(other);
	else {
		if (has_proto(ist, other, OBJ(INT_PROTO)))
			float_other = (double)other->data.i64;
		else {
			if (covers(other, self))
				return call_func1(ist, other, SYM(RDIV_), self);
			raise_exception(ist, OBJ(TYPE_EXC), "Float cannot be divided by this object");
			return NULL;
		}
	}
	if (float_other == 0.0) {
		raise_exception(ist, OBJ(DIVIDEZERO_EXC), NULL);
		return NULL;
	}
	return NEW_FLOAT(Float_value(self) / float_other);
}

DEF(Float, rDiv_, FORM_RPARAM){
	double float_other;
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Float);
	if (is_Float(other)) float_other = Float_value(other);
	else {
		if (has_proto(ist, other, OBJ(INT_PROTO)))
			float_other = (double)other->data.i64;
		else {
			raise_exception(ist, OBJ(TYPE_EXC), "Float cannot be divided into this object");
			return NULL;
		}
	}
	if (Float_value(self) == 0.0) {
		raise_exception(ist, OBJ(DIVIDEZERO_EXC), NULL);
		return NULL;
	}
	return NEW_FLOAT(float_other / Float_value(self));
}

DEF(Float, mul_, FORM_RPARAM){
	double float_other;
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Float);
	if (is_Float(other)) float_other = Float_value(other);
	else {
		if (has_proto(ist, other, OBJ(INT_PROTO)))
			float_other = (double)other->data.i64;
		else {
			if (covers(other, self))
				return call_func1(ist, other, SYM(MUL_), self);
			raise_exception(ist, OBJ(TYPE_EXC), "float cannot be multiplied by this object");
			return NULL;
		}
	}
	return NEW_FLOAT(Float_value(self) * float_other);
}

DEF(Float, sub_, FORM_RPARAM){
	double float_other;
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Float);
	if (is_Float(other)) float_other = Float_value(other);
	else {
		if (has_proto(ist, other, OBJ(INT_PROTO)))
			float_other = (double)other->data.i64;
		else {
			if (covers(other, self))
				return call_func1(ist, other, SYM(RSUB_), self);
			raise_exception(ist, OBJ(TYPE_EXC), "this object cannot be subtracted from a float");
			return NULL;
		}
	}
	return NEW_FLOAT(Float_value(self) - float_other);
}

DEF(Float, rSub_, FORM_RPARAM){
	double float_other;
	obj_p other = parms[1];
	if (is_Float(other)) float_other = Float_value(other);
	else {
		if (has_proto(ist, other, OBJ(INT_PROTO)))
			float_other = (double)other->data.i64;
		else {
			raise_exception(ist, OBJ(TYPE_EXC), "this object cannot be subtracted from a float");
			return NULL;
		}
	}
	return NEW_FLOAT(float_other - Float_value(self));
}

DEF(Float, pow_, FORM_RPARAM){
	double float_other;
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Float);
	if (is_Float(other)) float_other = Float_value(other);
	else {
		if (has_proto(ist, other, OBJ(INT_PROTO)))
			float_other = (double)other->data.i64;
		else {
			if (covers(other, self))
				return call_func1(ist, other, SYM(RPOW_), self);
			raise_exception(ist, OBJ(TYPE_EXC), "this object cannot be in a pow function with a float");
			return NULL;
		}
	}
	return NEW_FLOAT(pow(Float_value(self), float_other));
}

DEF(Float, rPow_, FORM_RPARAM){
	double float_other;
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Float);
	if (is_Float(other)) float_other = Float_value(other);
	else {
		if (has_proto(ist, other, OBJ(INT_PROTO)))
			float_other = (double)other->data.i64;
		else {
			raise_exception(ist, OBJ(TYPE_EXC), "this object cannot be in a pow function with a float");
			return NULL;
		}
	}
	return NEW_FLOAT(pow(float_other, Float_value(self)));
}


DEF(Float, cmp_, FORM_RPARAM){
	double float_other;
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Float);
	if (is_Float(other)) float_other = Float_value(other);
	else {
		if (has_proto(ist, other, OBJ(INT_PROTO)))
			float_other = (double)other->data.i64;
		else {
			if (covers(other, self))
				return call_func1(ist, other, SYM(RCMP_), self);
			raise_exception(ist, OBJ(TYPE_EXC), "this object cannot compared to a float");
			return NULL;
		}
	}
	if (Float_value(self) == float_other)
		return NEW_INT(0);
	else if (Float_value(self) > float_other)
		return NEW_INT(1);
	else
		return NEW_INT(-1);
}

DEF(Float, rCmp_, FORM_RPARAM){
	double float_other;
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Float);
	if (is_Float(other)) float_other = Float_value(other);
	else {
		if (has_proto(ist, other, OBJ(INT_PROTO)))
			float_other = (double)other->data.i64;
		else {
			raise_exception(ist, OBJ(TYPE_EXC), "this object cannot compared to a float");
			return NULL;
		}
	}
	if (Float_value(self) == float_other)
		return NEW_FLOAT(0);
	else if (Float_value(self) < float_other)
		return NEW_INT(1);
	else
		return NEW_INT(-1);
}

DEF(Float, eq__QUES, FORM_RPARAM){
	double float_other;
	obj_p other = parms[1];
	BIN_CONTENT_CHK(Float);
	if (is_Float(other)) float_other = Float_value(other);
	else {
		if (has_proto(ist, other, OBJ(INT_PROTO)))
			float_other = (double)other->data.i64;
		else {
			if (covers(other, self))
				return call_func1(ist, other, SYM(EQ__QUES), self);
			return OBJ(PR_FALSE);
		}
	}
	if (Float_value(self) == float_other) return OBJ(PR_TRUE);
	else                                  return OBJ(PR_FALSE);
}

DEF(Float, str_, FPARM1(spec, NEW_STRING(""))) {
	char str[256], *spec, spec_buf[80];
	BIN_STR_CHK(Float);
	STRING_PARAM(1, spec);
	if (strlen(spec) >= sizeof(spec_buf)) {
		raise_exception(ist, OBJ(OVERFLOW_EXC), "fmt spec too large");
		return NULL;
	}
	if (strlen(spec)) {
		char lastch = spec[strlen(spec)-1];
		if ( lastch == 'd' || lastch == 'D' || lastch == 'i' || lastch == 'I' || 
			 lastch == 'c' || lastch == 'C' || lastch == 'o' || lastch == 'O' || 
			 lastch == 'x' || lastch == 'X' ) {
			obj_p res_obj, int_obj;
			apr_snprintf(str, sizeof(str), "%1.0f", Float_value(self));
			int_obj = NEW_INT(apr_atoi64(str));
			res_obj = call_func1(ist, int_obj, SYM(STR_), parms[1]);
			del_unlock(int_obj);
			return res_obj;
		}
		strcpy(spec_buf, "%");
		strcat(spec_buf, spec);
		if (lastch != 'f' && lastch != 'F' && lastch != 'e' && lastch != 'E')
			strcat(spec_buf, "f");
	} else
		strcpy(spec_buf, "%g");
	apr_snprintf(str, sizeof(str), spec_buf, Float_value(self));
	return NEW_STRING(str);
}

DEF(Float, covers__QUES, FORM_RPARAM) {
	BIN_CONTENT_CHK(Float);
	if (has_proto(ist, parms[1], OBJ(INT_PROTO))) return OBJ(PR_TRUE);
	else										   return OBJ(PR_FALSE);
}

DEF(Float, coerce_, FORM_RPARAM) {
	BIN_CONTENT_CHK(Float);
	if (has_proto(ist, parms[1], OBJ(INT_PROTO)))
		return NEW_FLOAT((double)(parms[1]->data.i64));
	else if (has_proto(ist, parms[1], Float_OBJ))
		return NEW_FLOAT(parms[1]->data.f64);
	else
		raise_exception(ist, OBJ(TYPE_EXC), "This object cannot be coerced to a Float");
	return NULL;
}



MAIN_MODULE_INIT(Float)
{
	MODULE_SUB_INIT(Float);
	MODULE_ADD_SYM(Float, init_);
	MODULE_ADD_SYM(Float, bool__QUES);
	MODULE_ADD_SYM(Float, str_);
	MODULE_ADD_SYM(Float, hash_);
	MODULE_ADD_SYM(Float, eq__QUES);
	MODULE_ADD_SYM(Float, cmp_);
	MODULE_ADD_SYM(Float, rCmp_);
	MODULE_ADD_SYM(Float, abs_);
	MODULE_ADD_SYM(Float, neg_);
	MODULE_ADD_SYM(Float, pos_);
	MODULE_ADD_SYM(Float, add_);
	MODULE_ADD_SYM(Float, sub_);
	MODULE_ADD_SYM(Float, rSub_);
	MODULE_ADD_SYM(Float, mul_);
	MODULE_ADD_SYM(Float, div_);
	MODULE_ADD_SYM(Float, rDiv_);
	MODULE_ADD_SYM(Float, pow_);
	MODULE_ADD_SYM(Float, rPow_);
	MODULE_ADD_SYM(Float, covers__QUES);
	MODULE_ADD_SYM(Float, coerce_);

	check_exceptions(ist);
}
