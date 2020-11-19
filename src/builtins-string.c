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

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <apr_strings.h>

#include <prothon/prothon.h>
#include "dict.h"
#include "parser.h"
#include "object.h"
#include "interp.h"
#include <prothon/prothon_dll.h>

#include "pr_snprintf.h"

#define IS_STRING(objid) (has_proto(ist, objid, String_OBJ))

#define IS_SPACE(ch3p)								       \
	( ( (((ch3p)->b0)== ' ') || (((ch3p)->b0)=='\t') ||    \
        (((ch3p)->b0)=='\r') || (((ch3p)->b0)=='\n') )  && \
	  (((ch3p)->b1)==0)                                 && \
	  (((ch3p)->b2)==0)  )

#define ORD(ch3p) ((((((u32_t)(ch3p)->b2)<<8)|((ch3p)->b1))<<8)|((ch3p)->b0))

#define CH3EQ(ch3p1, ch3p2)					\
	(	(((ch3p1)->b0)==((ch3p2)->b0)) &&	\
		(((ch3p1)->b1)==((ch3p2)->b1)) &&	\
		(((ch3p1)->b2)==((ch3p2)->b2))   )

// only valid for Unicode 3.0 and prior versions
#define IF_NO_CASE(ch)							\
	if ( ((0x2500 <= ch) && (ch <= 0xFEFF)) ||	\
		 ((0x0600 <= ch) && (ch <= 0x1DFF)) ||	\
		 ((0x2000 <= ch) && (ch <= 0x20FF)) ||	\
		 ((0x2200 <= ch) && (ch <= 0x23FF)) ) 

MODULE_DECLARE(String);
MODULE_DECLARE(StringGen);

//********************************* new_string_obj ****************************
// this cannot be used with binary data, for C strings only
// for binary data with embedded nulls use NEW_STRINGN()
obj_p new_string_obj(isp ist, char* str) {
	obj_p obj;
	if(!str) return NULL;
	obj = NEW_OBJ(OBJ(STRING_PROTO));
	c2pr_strcpy(ist, obj, str, strlen(str));
	return obj;
}

//********************************* new_string_n_obj **************************
obj_p new_string_n_obj(isp ist, char* str, size_t len) {
	obj_p obj;
	if(!str) return NULL;
	obj = NEW_OBJ(OBJ(STRING_PROTO));
	c2pr_strcpy(ist, obj, str, len);
	return obj;
}

//********************************* new_string_ch3_obj **************************
obj_p new_string_ch3_obj(isp ist, ch3_p str, size_t len) {
	obj_p obj;
	if(!str) return NULL;
	obj = NEW_OBJ(OBJ(STRING_PROTO));
	pr2pr_strcpy(ist, obj, str, (data_size_t) len);
	return obj;
}

//********************************* c2pr_strcpy *******************************
void c2pr_strcpy(isp ist, obj_p obj, char* str, data_size_t len) {
	str_p obj_str;
	ch3_p ch3p;
	data_size_t i, size = len*3;

	if (size <= IMM_DATA_LEN) {
		SET_TYPE_IF_EXC(OBJ(STRING_PROTO), obj, DATA_TYPE_IMMDATA) return;
		obj->imm_data_len = (int) size;
		ch3p = obj->data.str;
	} else {
		obj_str = obj_malloc(ist, OBJ(STRING_PROTO), obj, sizeof(str_t)+size); if_exc_return;
		obj_str->size = sizeof(str_t)+size;
		ch3p = obj_str->str;
	}
	for (i=0; i < len; i++, ch3p++, str++) {
		ch3p->b0 = *str;
		ch3p->b1 = 0;
		ch3p->b2 = 0;
	}
	obj->immutable = TRUE;
}

//********************************* pr2pr_strcpy *******************************
void pr2pr_strcpy(isp ist, obj_p obj, ch3_p str, data_size_t len) {
	str_p obj_str;
	ch3_p ch3p;
	data_size_t i, size = len*3;

	if (size <= IMM_DATA_LEN) {
		SET_TYPE_IF_EXC(OBJ(STRING_PROTO), obj, DATA_TYPE_IMMDATA) return;
		obj->imm_data_len = (int) size;
		ch3p = obj->data.str;
	} else {
		obj_str = obj_malloc(ist, OBJ(STRING_PROTO), obj, sizeof(str_t)+size); if_exc_return;
		obj_str->size = sizeof(str_t)+size;
		ch3p = obj_str->str;
	}
	for (i=0; i < len; i++, ch3p++, str++) {
		ch3p->b0 = str->b0;
		ch3p->b1 = str->b1;
		ch3p->b2 = str->b2;
	}
	obj->immutable = TRUE;
}

//********************************* c2pr_bytcpy *******************************
void c2pr_bytcpy(isp ist, obj_p obj, u8_t* byt, data_size_t len, int latin) {
	str_p obj_str;
	ch3_p ch3p;
	data_size_t i, size = len*3;

	if (size <= IMM_DATA_LEN) {
		SET_TYPE_IF_EXC(OBJ(STRING_PROTO), obj, DATA_TYPE_IMMDATA) return;
		obj->imm_data_len = (int) size;
		ch3p = obj->data.str;
	} else {
		obj_str = obj_malloc(ist, OBJ(STRING_PROTO), obj, sizeof(str_t)+size); if_exc_return;
		obj_str->size = sizeof(str_t)+size;
		ch3p = obj_str->str;
	}
	for (i=0; i < len; i++, byt++, ch3p++) {
		if (!latin && *byt > 127) {
			raise_exception(ist, OBJ(VALUE_EXC), "bad ASCII value (0x%02x), use latin-1 encoding(?)", *byt);
			return;
		}
		ch3p->b0 = *byt;
		ch3p->b1 = 0;
		ch3p->b2 = 0;
	}
	obj->immutable = TRUE;
}

//********************************* pr2c_strptr *******************************
char* pr2c_strptr(isp ist, obj_p str_obj) {
	str_p strp;
	ch3_p ch3p;
	data_size_t i, data_size, len;
	char* res;

	if (!str_obj)
		return "<null>";
	if (str_obj->data_type == DATA_TYPE_IMMDATA) {
		data_size = str_obj->imm_data_len;
		ch3p      = str_obj->data.str;
	} else if (str_obj->data_type == DATA_TYPE_DATAPTR) {
		strp      = str_obj->data.ptr;
		data_size = strp->size - sizeof(str_t);
		ch3p      = strp->str;
	} else {
		raise_exception(ist, OBJ(VALUE_EXC), "string object uninitialized");
		return "";
	}
	len = data_size/3;
	res = pr_malloc(len+1);
	for (i=0; i < len; i++) {
		res[i] = (char) ch3p[i].b0;
		if (ch3p[i].b1 || ch3p[i].b2) {
			raise_exception(ist, OBJ(VALUE_EXC), "bad ASCII value (0x%x), need unicode encoding", ORD(ch3p+i));
			return "";
		}
		if (ch3p[i].b0 > 127) {
			raise_exception(ist, OBJ(VALUE_EXC), "bad ASCII value (0x%02x), try 'latin-1' encoding", ch3p[i].b0);
			return "";
		}
	}
	res[i] = 0;
	return res;
}
//********************************* pr_str2bytes *******************************
void pr_str2bytes(isp ist, u8_t* buf, obj_p str_obj, int latin) {
	str_p strp;
	ch3_p ch3p;
	data_size_t i, data_size, len;

	if (!str_obj) {
		raise_exception(ist, OBJ(INTERNAL_EXC), "string object missing in pr_str2bytes");
		return;
	}
	if (str_obj->data_type == DATA_TYPE_IMMDATA) {
		data_size = str_obj->imm_data_len;
		ch3p      = str_obj->data.str;
	} else if (str_obj->data_type == DATA_TYPE_DATAPTR) {
		strp      = str_obj->data.ptr;
		data_size = strp->size - sizeof(str_t);
		ch3p      = strp->str;
	} else {
		raise_exception(ist, OBJ(VALUE_EXC), "string object uninitialized");
		return;
	}
	len = data_size/3;
	for (i=0; i < len; i++) {
		buf[i] = ch3p[i].b0;
		if (ch3p[i].b1 || ch3p[i].b2) {
			raise_exception(ist, OBJ(VALUE_EXC), "bad char value (0x%x), need different encoding", ORD(ch3p+i));
			return;
		}
		if (!latin && ch3p[i].b0 > 127) {
			raise_exception(ist, OBJ(VALUE_EXC), "bad ASCII value (0x%02x), use 'latin-1' encoding(?)", ch3p[i].b0);
			return;
		}
	}
}

//********************************* pr_tolower ********************************
void pr_tolower(isp ist, ch3_p ch3p) {
	if (ch3p->b1 || ch3p->b2) {
		u32_t ch = ORD(ch3p);
		IF_NO_CASE(ch) return;
		raise_exception(ist, OBJ(VALUE_EXC), "lower case unknown for this character value: 0x%x", ch);
		return;
	}
	ch3p->b0 = (u8_t) tolower((char)(ch3p->b0));
}

//********************************* pr_toupper ********************************
void pr_toupper(isp ist, ch3_p ch3p) {
	if (ch3p->b1 || ch3p->b2) {
		u32_t ch = ORD(ch3p);
		IF_NO_CASE(ch) return;
		raise_exception(ist, OBJ(VALUE_EXC), "upper case unknown for this character value: 0x%x", ch);
		return;
	}
	ch3p->b0 = (u8_t) toupper((char)(ch3p->b0));
}

//********************************* changeCase ********************************
// Generic case-changer function.
#define AT_WORD_START -1
static obj_p changeCase(isp ist, obj_p self, void (*changer)(isp, ch3_p), size_t howMany) {
	size_t str_len = pr_strlen(self), i;
	ch3_p src_ptr;
	int atStart = 1;
	obj_p res = copy_object(ist, self);
	src_ptr  = pr_ch3ptr(res);
	for (i=0; i < str_len; ++i) {
		if (howMany == AT_WORD_START) {
			if (atStart) (*changer)(ist, src_ptr+i); if_exc_return NULL;
			if (IS_SPACE(src_ptr+i)) atStart = 1; else atStart = 0;
		} else {
			if (i<howMany) (*changer)(ist, src_ptr+i); if_exc_return NULL;
		}
	}
	return res;
}

//********************************* ch3_strstr ********************************
// strstr()-analog that works for ch3 strings
static ch3_p ch3_strstr(ch3_p haystack, size_t lhaystack, ch3_p needle, size_t lneedle) {
	ch3_p lookfor = needle;
	ch3_p looking_at = haystack;
	ch3_p hs_end = haystack + lhaystack;
	ch3_p ndl_end = needle + lneedle;
	while (looking_at < hs_end) {
		if (CH3EQ(lookfor, looking_at)) {
			// Matched a character.
			++lookfor;
			if (CH3EQ(lookfor, ndl_end)) {
				// Matched the whole string. It started lneedle-1
				// characters before the character we're looking at
				// now.
				return looking_at + 1 - lneedle;
			}
		} else {
			// No match.
			if (lookfor > needle) {
				// We had recognized the beginning of a match. We must
				// return to the character after the location at which
				// the non-match began.
				looking_at -= lookfor-needle;
				lookfor = needle;
			}
		}
		++looking_at;
	}
	return NULL;
}

//********************************* STRING MODULE *****************************

MODULE_START(String) {
	String_OBJ = OBJ(STRING_PROTO);
	MODULE_SET_DOC(String, "string object prototype");
	set_obj_id(String_OBJ, *, String);
	set_attr(ist, OBJ(OBJECT), sym(ist, "String"), String_OBJ);
	SET_TYPE_IF_EXC(String_OBJ, String_OBJ, DATA_TYPE_IMMDATA) return;
	String_OBJ->imm_data_len = 0;
}

DEF(String, init_, FORM_STAR_PARAM) {
	char *s;
	obj_p res = NULL;
	int llen;
	BIN_EMPTY_CHK();
	llen = (int) list_len(ist, parms[1]);
	if (llen > 2) {
		raise_exception(ist, OBJ(TYPE_EXC), "incorrect number of parameters");
		return NULL;
	}
	if (llen > 0) {
		obj_p obj2, obj = list_item(ist, parms[1], 0);
		if (llen == 1 && has_proto(ist, obj, OBJ(SYMBOL_PROTO))) {
			s = symch(ist, obj);
		} else if ( llen == 2 && has_proto(ist, obj, OBJ(INT_PROTO)) && 
			        has_proto(ist, obj2 = list_item(ist, parms[1], 1), OBJ(INT_PROTO)) ) {
			if (obj->data_type == DATA_TYPE_IMMDATA) {
				res = long_format(new_longp(obj->data.i64), int2i32t(ist, obj2), FALSE);
				s = pr2c_strptr(ist, res);
			} else {
				res = long_format(obj->data.ptr, int2i32t(ist, obj2), FALSE);
				s = pr2c_strptr(ist, res);
			}
		} else if (has_proto(ist, obj, OBJ(BYTES_PROTO))) {
			char* encoding_str;
			int latin = FALSE;
			if (llen == 1) 
				encoding_str = "ASCII";
			else if (has_proto(ist, obj2 = list_item(ist, parms[1], 1), OBJ(STRING_PROTO)))
				encoding_str = pr2c_strptr(ist, obj2);
			else {
				raise_exception(ist, OBJ(TYPE_EXC), "second param must be an encoding spec (string)");
				return NULL;
			}
			if (!strcmp(encoding_str, "latin-1")) latin = TRUE;
			else if (strcmp(encoding_str, "ASCII")) {
				raise_exception(ist, OBJ(TYPE_EXC), "unknown encoding format: \"%s\"", encoding_str);
				return NULL;
			}
			c2pr_bytcpy(ist, self, bytes_ptr(obj), (int) bytes_len(obj), latin); if_exc_return NULL;
			return OBJ(NONE);
		} else if (llen == 1) {
			s = as_str(ist, obj);
		} else {
			raise_exception(ist, OBJ(TYPE_EXC), "invalid parameters");
			return NULL;
		}
	} else
		s = "";
	c2pr_strcpy(ist, self, s, strlen(s));
	if (res) del_unlock(res);
	return OBJ(NONE);
}

DEF(String, str_, FPARM1(spec, NEW_STRING(""))) {
	obj_p res;
	char *spec, *wid_p = NULL, *prec_p = NULL, *p;
	int left_align = FALSE;
	int zero_pad   = FALSE;
	int width = 0, precision = 0, i, self_len, res_len;
	ch3_p self_ptr, res_ptr, ptr;

	BIN_STR_CHK(String);
	STRING_PARAM(1, spec);
	if (*spec == 0) return self;
	for(p=spec; *p; p++) {
		if (!wid_p && !prec_p && *p == '-') left_align = TRUE;
		else if (!wid_p && !prec_p && *p == '0') zero_pad   = TRUE;
		else if (!wid_p && !prec_p && *p > '0' && *p <= '9') wid_p = p;
		else if (!prec_p && *p == '.') { prec_p = p+1; *p = 0; }
		else if (*p == 's' || *p == 'S') {
			if (*(p+1) != 0) {
				raise_exception(ist, OBJ(TYPE_EXC), "character found after s in string fmt spec");
				return NULL;
			}
			*p = 0; 
			break; 
		} else if (*p < '0' || *p > '9') {
			raise_exception(ist, OBJ(VALUE_EXC), "invalid fmt spec in str_ parameter: \"%s\"", spec);
			return NULL;
		}
	}
	if (!wid_p && !prec_p) return self;
	if (wid_p)  width     = atoi(wid_p);
	if (prec_p) precision = atoi(prec_p);
	self_len = pr_strlen(self);
	self_ptr = pr_ch3ptr(self);
	if (prec_p)
		self_len = min(precision, self_len);
	if (wid_p)
		res_len = max(self_len, width);
	else
		res_len = self_len;
	ptr = res_ptr = pr_malloc(res_len*3);
	if (!left_align)
		for (i=0; i < res_len-self_len; i++, ptr++) {
			ptr->b0 = (u8_t) (zero_pad ? '0' : ' ');
			ptr->b1 = 0;  ptr->b2 = 0;
		}
	for (i=0; i < self_len; i++, ptr++, self_ptr++) {
		ptr->b0 = self_ptr->b0;
		ptr->b1 = self_ptr->b1;
		ptr->b2 = self_ptr->b2;
	}
	if (left_align)
		for (i=0; i < res_len-self_len; i++, ptr++) {
			ptr->b0 = (u8_t) ' ';
			ptr->b1 = 0;  ptr->b2 = 0;
		}
	res = new_string_ch3_obj(ist, res_ptr, res_len);
	pr_free(res_ptr);
	return res;
}

DEF(String, hash_, NULL) {
	i32_t res = 0x9a7c5e3;
	ch3_p p, p2;
	BIN_CONTENT_CHK(String);
	p = pr_ch3ptr(self);
	p2 = p + pr_strlen(self);
	for(; p < p2; p++) res += 131 * ORD(p);
	return new_hash_obj(ist, res);
}

DEF(String, getItem_, FORM_RPARAM) {
	BIN_CONTENT_CHK(String);
	return get_sequence_item(ist, self, parms[1], SEQ_TYPE_STRING);
}

DEF(String, setItem_, FORM_RPARAM) {
	raise_exception(ist, OBJ(MUTABLE_EXC), "strings may not be modifed");
	return NULL;
}

DEF(String, delItem_, NULL) {
	raise_exception(ist, OBJ(MUTABLE_EXC), "strings may not be modifed");
	return NULL;
}

DEF(String, cmp_, FORM_RPARAM){
	obj_p other = parms[1];
	data_size_t i, slen, olen, len;
	ch3_p sptr, optr;

	BIN_CONTENT_CHK(String);
	if (self == other)
		return NEW_INT(0);
	if (has_proto(ist, other, OBJ(INT_PROTO)) && pr_strlen(self) == 1) {
		i32_t ival = int2i32t(ist, other); if_exc_return NULL;
		if (ival < 0 || ival > MAX_UNICODE_VALUE) {
			raise_exception( ist, OBJ(VALUE_EXC), 
			                 "integer value outside of unicode character range" );
		    return NULL;
		}
		sptr = pr_ch3ptr(self);
		if (ORD(sptr) < (u32_t) ival) return NEW_INT(-1);
		if (ORD(sptr) > (u32_t) ival) return NEW_INT(+1);
		return NEW_INT(0);
	}
	if (!IS_STRING(other)) {
		raise_exception(ist, OBJ(TYPE_EXC), "Cannot compare a string and non-string");
		return NULL;
	}
	slen = pr_strlen(self);
	olen = pr_strlen(other);
	sptr = pr_ch3ptr(self);
	optr = pr_ch3ptr(other);
	len = min(slen, olen);
	for (i=0; i < len; i++) {
		if (ORD(sptr+i) < ORD(optr+i)) return NEW_INT(-1);
		if (ORD(sptr+i) > ORD(optr+i)) return NEW_INT(+1);
	}
	if (slen == olen) return NEW_INT(0);
	if (slen < olen)  return NEW_INT(-1);
	return NEW_INT(+1);
}

DEF(String, add_, FORM_RPARAM){
	obj_p res, other = parms[1];
	size_t slen, olen, tlen;
	ch3_p buf;
	BIN_CONTENT_CHK(String);
	if (!IS_STRING(other)) other = call_func0(ist, other, SYM(STR_));
	slen = pr_strlen(self); olen = pr_strlen(other);
	tlen = slen + olen;
	buf = pr_malloc(tlen*3);
	memcpy(buf,      pr_ch3ptr(self),  slen*3);
	memcpy(buf+slen, pr_ch3ptr(other), olen*3);
	res = NEW_STRING_CH3(buf, (int) tlen);
	pr_free(buf);
	return res;
}

DEF(String, mul_, FORM_RPARAM){
	obj_p res;
	data_size_t tlen, len;
	i32_t i, times;
	ch3_p buf, self_strptr;

	BIN_CONTENT_CHK(String);
	len = pr_strlen(self);
	if (!has_proto(ist, parms[1], OBJ(INT_PROTO))) {
		raise_exception(ist, OBJ(TYPE_EXC), "multiply times parameter must be an integer");
		return NULL;
	}
	times = int2i32t(ist, parms[1]);
	if(times < 0) {
		raise_exception(ist, OBJ(VALUE_EXC), "times parameter must be >= zero");
		return NULL;
	}
	if(times == 0) return NEW_STRING("");
	if(times == 1) return self;
	tlen = len*times;
	buf = pr_malloc(tlen*3);
	self_strptr = pr_ch3ptr(self);
	for(i=0; i < times; i++)
		memcpy(buf+(i*len), self_strptr, len*3);
	res = NEW_STRING_CH3(buf, tlen);
	pr_free(buf);
	return res;
}

DEF(String, mod_, FORM_RPARAM) {
	obj_p			res, argv_obj = parms[1];
	size_t			i, argc;
	pr_arg_p		argv;
	char			*fmt_str = pr2c_strptr(ist, self), *buf;
	apr_status_t	aprerr;

	BIN_CONTENT_CHK(String);
	if ( has_proto(ist, argv_obj, OBJ(STRING_PROTO)) ||
		!has_proto(ist, argv_obj,    OBJ(SEQ_PROTO)) ) {
		argv_obj = NEW_LIST(1);
		list_append(ist, argv_obj, parms[1]);
	}
	argc = list_len(ist, argv_obj);
	argv = pr_malloc((argc+1) * sizeof(pr_arg_t));
	for (i=0; i < argc; i++) {
		obj_p arg_obj = list_item(ist, argv_obj, (int) i);
		if (has_proto(ist, arg_obj, OBJ(INT_PROTO))) {
			argv[i].arg.i64 = arg_obj->data.i64;
			argv[i].type    = ARG_TYPE_INT;
		} else if (has_proto(ist, arg_obj, OBJ(FLOAT_PROTO))) {
			argv[i].arg.f64 = arg_obj->data.f64;
			argv[i].type    = ARG_TYPE_FLOAT;
		} else if (has_proto(ist, arg_obj, OBJ(STRING_PROTO))) {
			argv[i].arg.str = pr2c_strptr(ist, arg_obj);
			argv[i].type    = ARG_TYPE_STRING;
		} else {
			raise_exception(ist, OBJ(TYPE_EXC), "invalid type for string (%%) format");
			return NULL;
		}
	}
	argv[i].type = ARG_TYPE_END;
	buf = pr_malloc(16384);
	aprerr = apr_pr_vsnprintf(buf, 16384, fmt_str, argv);
	if (aprerr == -99) {
		raise_exception(ist, OBJ(INTERPRETER_EXC), "invalid or wrong number of %% params");
		return NULL;
	}
	res = NEW_STRING(buf);
	pr_free(buf);
	return res;
}

#define BRACE_LEFT	0x7B
#define BRACE_RIGHT 0x7D

#define CH3MOVE(src)						\
do {										\
	if (dstp == resp+res_len) {				\
		resp = pr_realloc(resp, res_len*6);	\
		dstp = resp+res_len;				\
		res_len *= 2;						\
	}										\
	dstp->b0 = src->b0;						\
	dstp->b1 = src->b1;						\
	dstp->b2 = src->b2;						\
	dstp++;									\
	src++;									\
} while (FALSE)

obj_p de_spec(isp ist, ch3_p* srcp, ch3_p endp, obj_p list, obj_p dict) {
	u32_t ch = 0;
	char buf[256], *p, *pend, *var, *spec;
	obj_p value, ins_text;
	int i, ins_len;
	ch3_p insp;
	for ( p = buf, pend = buf + sizeof(buf); 
		  p < pend && *srcp < endp;
		  p++, (*srcp)++ ) {

		if ((ch = ORD(*srcp)) == BRACE_RIGHT)
			break;

		if (ch < 0x20 || ch > 0x7f) {
			raise_exception(ist, OBJ(VALUE_EXC), "invalid character in formatting spec");
			return NULL;
		}
		if (ch == BRACE_LEFT) {
			(*srcp)++;
			if (*srcp < endp && (ch = ORD(*srcp)) == BRACE_LEFT)
				goto savech;
			ins_text = de_spec(ist, srcp, endp, list, dict); if_exc_return NULL;
			ins_len = pr_strlen(ins_text);
			insp    = pr_ch3ptr(ins_text);
			if (p+ins_len > pend) {
				raise_exception(ist, OBJ(VALUE_EXC), "unterminated formatting spec");
				return NULL;
			}
			for(i=0; i < ins_len; i++, p++, insp++) {
				ch = ORD(insp);
				if (ch < 0x20 || ch > 0x7f) {
					raise_exception(ist, OBJ(VALUE_EXC), "invalid character in formatting spec");
					return NULL;
				}
				*p = (char) ch;
			}
			del_unlock(ins_text);
			if (p < pend && *srcp < endp && (ch = ORD(*srcp)) != BRACE_RIGHT)
				goto savech;
			else
				break;
		}
savech:
		*p = (char) ch;
	}
	*p = 0;
	if (ch != BRACE_RIGHT) {
		raise_exception(ist, OBJ(VALUE_EXC), "unterminated formatting spec");
		return NULL;
	}
	(*srcp)++;
	var = spec = buf;
	for(; *spec; spec++)
		if (*spec == ':') {
			*spec = 0; 
			spec++;
			break;
		}
	if (*var >= '0' && *var <= '9') {
		size_t index;
		for (p=var+1; *p; p++)
			if (*p < '0' || *p > '9') {
				raise_exception(ist, OBJ(VALUE_EXC), "invalid character in fmt spec list index (%s)", var);
				return NULL;
			}
		index = atoi(var);
		if (index >= list_len(ist, list)) {
			raise_exception(ist, OBJ(VALUE_EXC), "fmt spec list index too big (%s)", var);
			return NULL;
		}
		value = list_item(ist, list, (int) index);
	} else {
		value = dict_item(ist, dict, NEW_STRING(var));
		if (!value)
			value = get_scope_attr(ist, ist->frame->locals, sym(ist, var), NULL, NULL);
	}
	if_exc_return NULL;
	if (!value) {
		raise_exception(ist, OBJ(VALUE_EXC), "no match in dictionary for fmt spec variable \"%s\"", var);
		return NULL;
	}
	if (*spec) {
		ins_text = call_func1(ist, value, SYM(STR_), NEW_STRING(spec)); if_exc_return NULL;
	} else {
		ins_text = call_func0(ist, value, SYM(STR_)); if_exc_return NULL;
	}
	return ins_text;
}

DEF(String, fmt, FORM_STAR3_PARAM) {
	obj_p res, list = parms[1], dict = parms[3];
	ch3_p srcp = pr_ch3ptr(self);
	data_size_t src_len = pr_strlen(self), i, res_len = src_len*2, ins_len;
	ch3_p endp = srcp+src_len, resp = pr_malloc(res_len*3), dstp=resp, insp;
	while (srcp < endp) {
		obj_p ins_text;
		while (srcp < endp && ORD(srcp) != BRACE_LEFT) CH3MOVE(srcp);
		if (srcp == endp) break;
		srcp++;
		if (srcp < endp && ORD(srcp) == BRACE_LEFT) {
			CH3MOVE(srcp);
			continue;
		}
		ins_text = de_spec(ist, &srcp, endp, list, dict); if_exc_return NULL;
		ins_len = pr_strlen(ins_text);
		insp    = pr_ch3ptr(ins_text);
		for(i=0; i < ins_len; i++) CH3MOVE(insp);
		del_unlock(ins_text);
	}
	res = new_string_ch3_obj(ist, resp, dstp-resp);
	pr_free(resp);
	return res;
}

DEF(String, ord_, NULL) {
	BIN_CONTENT_CHK(String);
	if (!pr_strlen(self)) {
		raise_exception(ist, OBJ(VALUE_EXC), "ord_ not valid on empty string");
		return NULL;
	}
	return NEW_INT(ORD(pr_ch3ptr(self)));
}

DEF(String, join, FORM_RPARAM) {
	obj_p obj, list, str_list;
	str_p obj_str;
	ch3_p self_str, dest_ptr;
	int i, llen;
	u32_t j;
	size_t self_len, tlen;

	BIN_CONTENT_CHK(String);
	if (!has_proto(ist, parms[1], OBJ(LIST_PROTO))) {
		raise_exception(ist, OBJ(TYPE_EXC), "join function parameter must be a list");
		return NULL;
	}
	list = parms[1];
	llen = (int) list_len(ist, parms[1]);
	if (!llen) return NEW_STRING("");
	str_list = NEW_LIST(llen);
	self_len = pr_strlen(self);
	self_str = pr_ch3ptr(self);
	tlen = 0;
	for (i=0; i < llen; i++) {
		obj_p str_obj = call_func0(ist, list_item(ist, list, i), SYM(STR_)); if_exc_return NULL;
		list_append(ist, str_list, str_obj);
		tlen += pr_strlen(str_obj); 
		if (i != llen-1) tlen += self_len;
	}
	obj = NEW_OBJ(String_OBJ);
	if (tlen*3 <= IMM_DATA_LEN) {
		SET_TYPE_IF_EXC(String_OBJ, obj, DATA_TYPE_IMMDATA) return NULL;
		obj->imm_data_len = (int) tlen*3;
		dest_ptr = obj->data.str;
	} else {
		obj_str = obj_malloc(ist, OBJ(STRING_PROTO), obj, sizeof(str_t)+tlen*3);
		if(!obj_str) {
			raise_exception(ist, OBJ(OUTOFMEMORY_EXC), "memory allocation failed for string join");
			return NULL;
		}
		obj_str->size = (data_size_t)(sizeof(str_t)+tlen*3);
		dest_ptr = obj_str->str;
	}
	for(i=0; i < llen; i++) {
		obj_p  str_obj = list_item(ist, str_list, i);
		size_t str_len = pr_strlen(str_obj);
		ch3_p src_ptr  = pr_ch3ptr(str_obj);
		for (j=0; j < str_len; j++, dest_ptr++, src_ptr++) {
			dest_ptr->b0 = src_ptr->b0;
			dest_ptr->b1 = src_ptr->b1;
			dest_ptr->b2 = src_ptr->b2;
		}
		if (i != llen-1) {
			src_ptr = self_str;
			for (j=0; j < self_len; j++, dest_ptr++, src_ptr++) {
				dest_ptr->b0 = src_ptr->b0;
				dest_ptr->b1 = src_ptr->b1;
				dest_ptr->b2 = src_ptr->b2;
			}
		}
	}
	set_immutable(obj);
	del_unlock(str_list);
	return obj;
}

DEF(String, in__QUES, FORM_RPARAM) {
	BIN_CONTENT_CHK(String);
	if (!has_proto(ist, parms[1], String_OBJ))
		return call_func1(ist, parms[1], SYM(RIN__QUES), self);
	if ( ch3_strstr( pr_ch3ptr(parms[1]), pr_strlen(parms[1]), 
		             pr_ch3ptr(self),     pr_strlen(self) ) ) return OBJ(PR_TRUE);
	else						                              return OBJ(PR_FALSE);
}

DEF(String, notIn__QUES, FORM_RPARAM) {
	BIN_CONTENT_CHK(String);
	if (!has_proto(ist, parms[1], String_OBJ))
		return call_func1(ist, parms[1], SYM(RNOTIN__QUES), self);
	if ( ch3_strstr( pr_ch3ptr(parms[1]), pr_strlen(parms[1]), 
		             pr_ch3ptr(self),     pr_strlen(self) ) ) return OBJ(PR_FALSE);
	else						                              return OBJ(PR_TRUE);
}

DEF(String, len_, NULL) {
	BIN_CONTENT_CHK(String);
	return NEW_INT(pr_strlen(self));
}

DEF(String, bool__QUES, NULL) {
	BIN_CONTENT_CHK(String);
	if (pr_strlen(self)) return OBJ(PR_TRUE);
	else                 return OBJ(PR_FALSE);
}

DEF(String, lower, NULL) {
	BIN_CONTENT_CHK(String);
	return changeCase(ist,self,&pr_tolower,pr_strlen(self));
}

DEF(String, upper, NULL) {
	BIN_CONTENT_CHK(String);
	return changeCase(ist,self,&pr_toupper,pr_strlen(self));
}

DEF(String, capitalize, NULL) {
	BIN_CONTENT_CHK(String);
	return changeCase(ist,self,&pr_toupper,1);
}

DEF(String, capWords, NULL) {
	BIN_CONTENT_CHK(String);
	return changeCase(ist,self,&pr_toupper,AT_WORD_START);
}

DEF(String, find, FPARM2( stringToFind, OBJ(NODEF), indexToStartLooking, NEW_INT(0) )) {
	size_t str_len;
	size_t find_len = 0;
	ch3_p found, find_ptr = NULL, self_str;
	size_t start = 0;
	obj_p result = 0;

	BIN_CONTENT_CHK(String);
	str_len = pr_strlen(self);
	if (!has_proto(ist, parms[1], String_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "find function parameter 1 must be a string");
		return NULL;
	}
	if (!has_proto(ist, parms[3], OBJ(INT_PROTO))) {
		raise_exception(ist, OBJ(TYPE_EXC), "find function parameter 2 must be an integer");
		return NULL;
	} else {
		start = (size_t)int2i32t(ist, parms[3]);
	}

	find_ptr = pr_ch3ptr(parms[1]);
	find_len = pr_strlen(parms[1]);
	if (find_len < 1) {
		return new_int_obj(ist,0);
	}
	self_str = pr_ch3ptr(self);
	found = ch3_strstr(self_str + start, str_len, find_ptr, find_len);
	if (found) {
		result = new_int_obj(ist, found - self_str);
	} else {
		result = new_int_obj(ist, 0);
	}
	pr_free(find_ptr);
	pr_free(self_str);
	return result;
}

DEF(String, replace, FORM_PARAM2) {
	size_t str_len = pr_strlen(self);
	size_t find_len = 0;
	size_t repl_len = 0;
	size_t result_len = str_len;
	ch3_p buf, dest_ptr = NULL;
	ch3_p src_ptr = 0;
	ch3_p find_ptr = 0;
	ch3_p repl_ptr = 0;
	size_t temp_len = 0;
	ch3_p a_match = 0;
	ch3_p where_ptr = 0;

	BIN_CONTENT_CHK(String);

	if (!has_proto(ist, parms[1], String_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "replace function parameter 1 must be a string");
		return NULL;
	}
	if (!has_proto(ist, parms[3], String_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "replace function parameter 2 must be a string");
		return NULL;
	}

	find_len = pr_strlen(parms[1]);
	if (find_len>str_len) return self;
	if (find_len<1) {
		raise_exception(ist, OBJ(TYPE_EXC), "replace: empty search string");
		return NULL;
	}
	repl_len = pr_strlen(parms[3]);
	src_ptr  = pr_ch3ptr(self);
	find_ptr = pr_ch3ptr(parms[1]);
	repl_ptr = pr_ch3ptr(parms[3]);

	// This is perhaps not as efficient as it could be.

	// Find the length of the result string.
	temp_len = str_len;
	a_match = ch3_strstr(src_ptr, temp_len, find_ptr, find_len);
	while (a_match) {
		result_len -= find_len;
		result_len += repl_len;
		a_match += find_len; // Skip the matching text.
		temp_len = str_len - (a_match - src_ptr);
		a_match = ch3_strstr(a_match, temp_len, find_ptr, find_len);
	}

	// Create the copy.
	dest_ptr = buf = pr_malloc(sizeof(str_t) + result_len*3);

	// Fill the replaced copy.
	temp_len = str_len;
	where_ptr = src_ptr;
	a_match = ch3_strstr(where_ptr, temp_len, find_ptr, find_len);
	while (a_match) {
		// Copy everything from where we were to the beginning of the
		// match.
		int frag_len = (int) (a_match - where_ptr);
		memcpy(dest_ptr, where_ptr, frag_len*3);
		dest_ptr += frag_len;

		// Copy the replacement text.
		memcpy(dest_ptr, repl_ptr, repl_len*3);
		dest_ptr += repl_len;

		a_match += find_len; // Skip the matched text.
		where_ptr = a_match;
		temp_len = str_len - (a_match - where_ptr);
		a_match = ch3_strstr(a_match, temp_len, find_ptr, find_len);
	}

	// Copy any trailing text after the last match.
	if (where_ptr < src_ptr + str_len) {
		memcpy(dest_ptr, where_ptr, ((src_ptr+str_len)-where_ptr)*3);
	}
	return NEW_STRING_CH3(buf, result_len);
}

DEF(String, expandTabs, FPARM1( nSpaces, NEW_INT(8) )) {
	obj_p result;	
	BIN_CONTENT_CHK(String);
	{
		obj_p tab = new_string_obj(ist, "\t");
		obj_p tspace = new_string_obj(ist, " ");
		obj_p mul_args[]= {NULL, parms[1]};
		obj_p spcs = Stringmul_(ist, tspace,2, mul_args, 0);
		obj_p repl_args[] = {NULL, tab, NULL, spcs};
		result = Stringreplace(ist, self, 4, repl_args, 0);
		del_unlock(tab);
		del_unlock(spcs);
		del_unlock(tspace);
	}
	return result;
}

DEF(String, lStrip, NULL) {
	ch3_p ch3p, endp;
	data_size_t len;
	BIN_CONTENT_CHK(String);
	ch3p = pr_ch3ptr(self);
	len  = pr_strlen(self);
	endp = ch3p + len;
	while (ch3p < endp && IS_SPACE(ch3p)) ch3p++;
	return new_string_ch3_obj(ist, ch3p, endp-ch3p);
}

DEF(String, rStrip, NULL) {
	ch3_p ch3p, endp;
	data_size_t len;
	BIN_CONTENT_CHK(String);
	ch3p = pr_ch3ptr(self);
	len  = pr_strlen(self);
	endp = ch3p + len;
	while (endp > ch3p && IS_SPACE(endp-1)) endp--;
	return new_string_ch3_obj(ist, ch3p, endp-ch3p);
}

DEF(String, strip, NULL) {
	ch3_p ch3p, endp;
	data_size_t len;
	BIN_CONTENT_CHK(String);
	ch3p = pr_ch3ptr(self);
	len  = pr_strlen(self);
	endp = ch3p + len;
	while (ch3p < endp && IS_SPACE(ch3p)) ch3p++;
	while (endp > ch3p && IS_SPACE(endp-1)) endp--;
	return new_string_ch3_obj(ist, ch3p, endp-ch3p);
}

DEF(String, split, FORM_RPARAM) {
	size_t str_len = pr_strlen(self);
	size_t find_len = 0;
	ch3_p src_ptr = 0;
	ch3_p find_ptr = 0;
	size_t temp_len = 0;
	ch3_p a_match = 0;
	ch3_p where_ptr = 0;
	obj_p next_frag, list_obj = new_list_obj(ist,0);

	BIN_CONTENT_CHK(String);
	if (!has_proto(ist, parms[1], String_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "split function parameter 1 must be a string");
		return NULL;
	}

	find_len = pr_strlen(parms[1]);
	if (find_len < 1) {
		raise_exception(ist, OBJ(TYPE_EXC), "split: empty separator");
		return NULL;
	}
	src_ptr = pr_ch3ptr(self);
	where_ptr = src_ptr;
	find_ptr = pr_ch3ptr(parms[1]);

	temp_len = str_len;
	a_match = ch3_strstr(src_ptr, temp_len, find_ptr, find_len);
	while (a_match) {
		// Make new string from where_ptr to a_match.
		obj_p next_frag = new_string_ch3_obj(ist, where_ptr, a_match-where_ptr);
		list_append(ist, list_obj, next_frag);

		a_match += find_len; // Skip the matching text.
		where_ptr = a_match;
 		temp_len = str_len - (a_match - src_ptr);
		a_match = ch3_strstr(a_match, temp_len, find_ptr, find_len);
	}

	// Copy any trailing text after the last match.
	next_frag = new_string_ch3_obj(ist, where_ptr, str_len-(where_ptr-src_ptr));
	list_append(ist, list_obj, next_frag);
	
	return list_obj;
}

DEF(String, objList_, FORM_RPARAM) {
	return parms[1];
}

//********************************* STRINGGEN MODULE **************************

typedef struct {
	data_size_t size;
	ch3_p		ptr;
	ch3_t		str[];
} str_gen_t;

typedef str_gen_t* str_gen_p;

#define str_gen_len(strgenp) (((strgenp->size)-sizeof(str_gen_t))/sizeof(ch3_t))

MODULE_START(StringGen)
{
	StringGen_OBJ = NEW_OBJ(OBJ(ITER_PROTO));
	set_attr(ist, OBJ(OBJECT), sym(ist, "StringGen"), StringGen_OBJ);
	set_obj_id(StringGen_OBJ, *, StringGen);
}

DEF(String, iter_, NULL) {
	obj_p gen_obj;
	data_size_t i, len;
	str_gen_p strgenp;
	ch3_p srcp, dstp;
	BIN_CONTENT_CHK(String);
	gen_obj = NEW_OBJ(StringGen_OBJ);
	SET_TYPE_IF_EXC(StringGen_OBJ, gen_obj, DATA_TYPE_DATAPTR) return NULL;
	len  = pr_strlen(self);
	srcp = pr_ch3ptr(self);
	gen_obj->data.ptr = strgenp = pr_malloc(sizeof(str_gen_t) + len*sizeof(ch3_t));
	strgenp->size = sizeof(str_gen_t) + len*sizeof(ch3_t);
	strgenp->ptr  = dstp = strgenp->str;
	for(i=0; i < len; i++, dstp++, srcp++) {
		dstp->b0 = srcp->b0;
		dstp->b1 = srcp->b1;
		dstp->b2 = srcp->b2;
	}
	return gen_obj;
}

DEF(StringGen, next, NULL) {
        obj_p res = NULL;
        str_gen_p strgenp;
		BIN_CONTENT_CHK(StringGen);
        def_write_lock(self);
		strgenp = self->data.ptr;
		if (strgenp->ptr == (strgenp->str)+str_gen_len(strgenp)) {
                raise_exception(ist, OBJ(STOP_ITERATION_EXC), NULL);
				return NULL;
		}
		res = new_string_ch3_obj(ist, strgenp->ptr++, 1);
        def_write_unlock(self);
        return res;
}

DEF(StringGen, objList_, FORM_RPARAM) {
	return parms[1];
}

//********************************* MODULES INIT ******************************

MAIN_MODULE_INIT(String)
{
	MODULE_SUB_INIT(String);
	MODULE_ADD_SYM(String, init_);
	MODULE_ADD_SYM(String, bool__QUES);
	MODULE_ADD_SYM(String, str_);
	MODULE_ADD_SYM(String, hash_);
	MODULE_ADD_SYM(String, cmp_);
	MODULE_ADD_SYM(String, ord_);
	MODULE_ADD_SYM(String, len_);
	MODULE_ADD_SYM(String, getItem_);
	MODULE_ADD_SYM(String, add_);
	MODULE_ADD_SYM(String, mul_);
	MODULE_ADD_SYM(String, mod_);
	MODULE_ADD_SYM(String, fmt);
	MODULE_ADD_SYM(String, iter_);
	MODULE_ADD_SYM(String, join);
	MODULE_ADD_SYM(String, in__QUES);
	MODULE_ADD_SYM(String, notIn__QUES);
	MODULE_ADD_SYM(String, lower);
	MODULE_ADD_SYM(String, upper);
	MODULE_ADD_SYM(String, replace);
	MODULE_ADD_SYM(String, capitalize);
	MODULE_ADD_SYM(String, capWords);
	MODULE_ADD_SYM(String, expandTabs);
	MODULE_ADD_SYM(String, lStrip);
	MODULE_ADD_SYM(String, rStrip);
	MODULE_ADD_SYM(String, strip);
	MODULE_ADD_SYM(String, split);
	MODULE_ADD_SYM(String, find);
	MODULE_ADD_SYM(String, setItem_);
	MODULE_ADD_SYM(String, delItem_);
	MODULE_ADD_SYM(String, objList_);

	MODULE_SUB_INIT(StringGen);
	MODULE_ADD_SYM(StringGen, next);
	MODULE_ADD_SYM(StringGen, objList_);
	
	set_immutable(String_OBJ);

	check_exceptions(ist);
}
