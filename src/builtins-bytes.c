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
 * derivative works, dibytibute, and otherwise use Prothon alone or in any
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
 * RESULT OF MODIFYING, DIBYTIBUTING, OR OTHERWISE USING PROTHON, OR ANY
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


// builtins-bytes.c

#include <prothon/prothon.h>
#include "object.h"
#include <prothon/prothon_dll.h>

#include "pr_snprintf.h"

#define is_Bytes(objid) (has_proto(ist, objid, Bytes_OBJ))

MODULE_DECLARE(Bytes);
MODULE_DECLARE(BytesGen);

//********************************* new_bytes_obj ****************************
obj_p new_bytes_obj(isp ist, const u8_t* byt, size_t len) {
	obj_p obj;
	byt_p obj_byt;
	if(!byt) return NULL;
	obj = NEW_OBJ(OBJ(BYTES_PROTO));
	if (len <= IMM_DATA_LEN) {
		SET_TYPE_IF_EXC(OBJ(BYTES_PROTO), obj, DATA_TYPE_IMMDATA) return NULL;
		obj->imm_data_len = (int) len;
		if (len)
			memcpy(obj->data.bytes, byt, len);
	} else {
		obj_byt = obj_malloc(ist, OBJ(BYTES_PROTO), obj, sizeof(byt_t)+len); if_exc_return NULL;
		obj_byt->size = (data_size_t) (sizeof(byt_t)+len);
		memcpy(&(obj_byt->byt[0]), byt, len);
	}
	obj->immutable = TRUE;
	return obj;
}

//********************************* set_bytes_data ***************************
void set_bytes_data(isp ist, obj_p obj, void* byt, size_t len) {
	byt_p obj_byt;
	if (len <= IMM_DATA_LEN) {
		SET_TYPE_IF_EXC(OBJ(BYTES_PROTO), obj, DATA_TYPE_IMMDATA) return;
		obj->imm_data_len = (int) len;
		memcpy(obj->data.bytes, byt, len);
	} else {
		obj_byt = obj_malloc(ist, OBJ(BYTES_PROTO), obj, sizeof(byt_t)+len); if_exc_return;
		obj_byt->size = (data_size_t) sizeof(byt_t)+len;
		memcpy(&(obj_byt->byt[0]), byt, len);
	}
	obj->immutable = TRUE;
	return;
}

//********************************* bytes_ptr *********************************
u8_t* bytes_ptr(obj_p obj) {
	if (obj->data_type == DATA_TYPE_IMMDATA) 
		return obj_dataptr(obj);
	else
		return ((u8_t*) obj_dataptr(obj)) + sizeof(data_size_t);
}

//********************************* bytes_len *********************************
size_t bytes_len(obj_p obj) {
	if (obj->data_type == DATA_TYPE_IMMDATA) 
		return obj->imm_data_len;
	else
		return DATA_SIZE(obj)-sizeof(data_size_t);
}

//********************************* BYTES MODULE ******************************

MODULE_START(Bytes)
{
	Bytes_OBJ = OBJ(BYTES_PROTO);
	MODULE_SET_DOC(Bytes, "bytes object prototype");
	set_obj_id(Bytes_OBJ, *, Bytes);
	set_attr(ist, OBJ(OBJECT), sym(ist, "Bytes"), Bytes_OBJ);
	SET_TYPE_IF_EXC(Bytes_OBJ, Bytes_OBJ, DATA_TYPE_IMMDATA) return;
	Bytes_OBJ->imm_data_len = 0;
}

DEF(Bytes, init_, FPARM2(value, OBJ(NODEF), encoding, NEW_STRING("ASCII"))) {
	size_t len;
	byt_p obj_byt;

	BIN_EMPTY_CHK();
	if (has_proto(ist, parms[1], OBJ(STRING_PROTO))) {
		char* encoding_str;
		int latin = FALSE;
		u8_t* buf;
		STRING_PARAM(2, encoding_str);
		if (!strcmp(encoding_str, "latin-1"))
			latin = TRUE;
		else if (strcmp(encoding_str, "ASCII")) {
			raise_exception(ist, OBJ(VALUE_EXC), "unknown encoding parameter: '%s'", encoding_str);
			return NULL;
		}
		len = pr_strlen(parms[1]);
		if (len <= IMM_DATA_LEN) {
			SET_TYPE_IF_EXC(OBJ(BYTES_PROTO), self, DATA_TYPE_IMMDATA) return NULL;
			self->imm_data_len = (int) len;
			buf = self->data.bytes;
		} else {
			obj_byt = obj_malloc(ist, OBJ(BYTES_PROTO), self, sizeof(byt_t)+len); if_exc_return NULL;
			obj_byt->size = (data_size_t) (sizeof(byt_t)+len);
			buf = &(obj_byt->byt[0]);
		}
		pr_str2bytes(ist, buf, parms[1], latin);
		self->immutable = TRUE;
		return OBJ(NONE);
	} else if (has_proto(ist, parms[1], OBJ(LIST_PROTO))) {
		obj_p list = parms[1];
		size_t i, len = list_len(ist, list);
		u8_t* bufp;
		if (len <= IMM_DATA_LEN) {
			SET_TYPE_IF_EXC(OBJ(BYTES_PROTO), self, DATA_TYPE_IMMDATA) return NULL;
			self->imm_data_len = (int) len;
			bufp = self->data.bytes;
		} else {
			obj_byt = obj_malloc(ist, OBJ(BYTES_PROTO), self, sizeof(byt_t)+len); if_exc_return NULL;
			obj_byt->size = (data_size_t) (sizeof(byt_t)+len);
			bufp = &(obj_byt->byt[0]);
		}
		for(i=0; i < len; i++) {
			i32_t val;
			obj_p item = list_item(ist, list, (int) i);
			if (!has_proto(ist, item, OBJ(INT_PROTO))) {
				raise_exception(ist, OBJ(VALUE_EXC), "list item must be an integer");
				return NULL;
			}
			val = int2i32t(ist, item); if_exc_return NULL;
			if (val < 0 || val > 255) {
				raise_exception(ist, OBJ(VALUE_EXC), "invalid byte value(%d)", val);
				return NULL;
			}
			*(bufp++) = (u8_t) val;
		}
		self->immutable = TRUE;
		return OBJ(NONE);
	} else if (has_proto(ist, parms[1], OBJ(BYTES_PROTO))) {
		return copy_object(ist, parms[1]);
	} else {
		raise_exception(ist, OBJ(TYPE_EXC), "invalid parameters");
		return NULL;
	}
}

DEF(Bytes, str_, NULL) {
	u8_t* self_ptr;
	char *buf;
	size_t i, self_len;
	obj_p res;

	BIN_STR_CHK(Bytes);
	self_ptr = bytes_ptr(self);
	self_len = bytes_len(self);
	buf = pr_malloc(self_len*3+3);
	strcpy(buf, "x\"");
	for (i=0; i < self_len; i++) {
		char str[8];
		apr_snprintf(str, sizeof(str), "%02x ", *(self_ptr++));
		strcat(buf, str);
	}
	*(buf+strlen(buf)-1) = '\"';
	res = NEW_STRING(buf);
	pr_free(buf);
	return res;
}

DEF(Bytes, hash_, NULL) {
	i32_t res = 0x45ced34;
	u8_t *p, *lastp;
	BIN_CONTENT_CHK(Bytes);
	p = bytes_ptr(self);
	lastp = p + bytes_len(self);
	for(; p < lastp; p++) res += 131 * (*p);
	return new_hash_obj(ist, res);
}

DEF(Bytes, getItem_, FORM_RPARAM) {
	BIN_CONTENT_CHK(Bytes);
	return get_sequence_item(ist, self, parms[1], SEQ_TYPE_BYTES);
}

DEF(Bytes, setItem_, FORM_RPARAM) {
	raise_exception(ist, OBJ(MUTABLE_EXC), "bytes may not be modifed");
	return NULL;
}

DEF(Bytes, delItem_, NULL) {
	raise_exception(ist, OBJ(MUTABLE_EXC), "bytes may not be modifed");
	return NULL;
}

DEF(Bytes, cmp_, FORM_RPARAM){
	obj_p other = parms[1];
	size_t i, len, len1, len2;
	u8_t *p1, *p2, str_flg = FALSE;

	BIN_CONTENT_CHK(Bytes);
	if (self == other)
		return NEW_INT(0);
	if (!is_Bytes(other)) {
		if (has_proto(ist, other, OBJ(STRING_PROTO))) {
			char* s;
			other = new_object(ist, OBJ(BYTES_PROTO));
			s = pr2c_strptr(ist, parms[1]); if_exc_return NULL;
			set_bytes_data(ist, other, s, strlen(s));
			pr_free(s);
			str_flg = TRUE;
		} else {
			raise_exception(ist, OBJ(TYPE_EXC), "Cannot compare a bytes object with this object");
			return NULL;
		}
	}
	p1 = bytes_ptr(self);
	p2 = bytes_ptr(other);
	len1 = bytes_len(self);
	len2 = bytes_len(other);
	len = min(len1,len2);
	for (i=0; i < len; i++) {
		if (*p1 < *p2) goto neg;
		if (*p1 > *p2) goto pos;
	}
	if (len1 < len2) goto neg;
	if (len1 > len2) goto pos;
	if (str_flg) del_unlock(other);
	return NEW_INT(0);
neg:
	if (str_flg) del_unlock(other);
	return NEW_INT(-1);
pos:
	if (str_flg) del_unlock(other);
	return NEW_INT(+1);
}

DEF(Bytes, add_, FORM_RPARAM){
	obj_p obj, other = parms[1];
	size_t slen, olen, tlen;
	u8_t* buf;

	BIN_CONTENT_CHK(Bytes);
	if (!is_Bytes(other)) {
		raise_exception(ist, OBJ(TYPE_EXC), "Cannot concatenate a bytes object with this object");
		return NULL;
	}
	slen = bytes_len(self); olen = bytes_len(other);
	tlen = slen + olen;
	buf = pr_malloc(tlen);
	memcpy(buf,      bytes_ptr(self),  slen);
	memcpy(buf+slen, bytes_ptr(other), olen);
	obj = NEW_BYTES(buf, tlen);
	pr_free(buf);
	return obj;
}

DEF(Bytes, mul_, FORM_RPARAM){
	obj_p obj;
	size_t i, times, tlen, len = bytes_len(self);
	u8_t *buf, *self_ptr;

	BIN_CONTENT_CHK(Bytes);
	INT_32_PARAM(1, times);
	if(times == 0) return NEW_BYTES(NULL, 0);
	if(times == 1) return self;
	tlen = len*times;
	buf = pr_malloc(tlen);
	self_ptr = bytes_ptr(self);
	for (i=0; i < times; i++)
		memcpy(buf+(i*len), self_ptr, len);
	obj = NEW_BYTES(buf, tlen);
	pr_free(buf);
	return obj;
}

DEF(Bytes, len_, NULL) {
	BIN_CONTENT_CHK(Bytes);
	return NEW_INT(bytes_len(self));
}

DEF(Bytes, bool__QUES, NULL) {
	BIN_CONTENT_CHK(Bytes);
	if (bytes_len(self)) return OBJ(PR_TRUE);
	else                 return OBJ(PR_FALSE);
}

DEF(Bytes, objList_, FORM_RPARAM) {
	return parms[1];
}

//********************************* BYTESGEN MODULE **************************

typedef struct {
	data_size_t size;
	u8_t*		ptr;
} bytgen_t;

typedef bytgen_t* bytgen_p;

DEF(Bytes, iter_, NULL) {
	obj_p gen_obj;
	bytgen_p bytgenp;

	BIN_CONTENT_CHK(Bytes);
	gen_obj = NEW_OBJ(BytesGen_OBJ);
	SET_TYPE_IF_EXC(BytesGen_OBJ, gen_obj, DATA_TYPE_DATAPTR) return NULL;
	bytgenp = gen_obj->data.ptr = pr_malloc(sizeof(bytgen_t) + bytes_len(self));
	bytgenp->size = DATA_SIZE(self);
	bytgenp->ptr  = ((u8_t*)bytgenp) + sizeof(bytgen_t);
	memcpy(bytgenp->ptr, bytes_ptr(self), bytes_len(self));
	return gen_obj;
}

MODULE_START(BytesGen)
{
	BytesGen_OBJ = NEW_OBJ(NULL);
	set_attr(ist, OBJ(OBJECT), sym(ist, "BytesGen"), BytesGen_OBJ);
	set_obj_id(BytesGen_OBJ, *, BytesGen);
}

DEF(BytesGen, next, NULL) {
        obj_p res = NULL;
        bytgen_p bytgenp;
		u8_t* limit;

		BIN_CONTENT_CHK(BytesGen);
        def_write_lock(self);
        bytgenp = (bytgen_p)(self->data.ptr);
		limit = ((u8_t*)(self->data.ptr)) + (bytgenp->size);
		if (bytgenp->ptr == limit) {
			raise_exception(ist, OBJ(STOP_ITERATION_EXC), NULL);
			return NULL;
		}
		res = NEW_INT(*((bytgenp->ptr)++));
        def_write_unlock(self);
        return res;
}

DEF(BytesGen, objList_, FORM_RPARAM) {
	return parms[1];
}

//********************************* MODULES INIT ******************************

MAIN_MODULE_INIT(Bytes)
{
	MODULE_SUB_INIT(Bytes);
	MODULE_ADD_SYM(Bytes, init_);
	MODULE_ADD_SYM(Bytes, bool__QUES);
	MODULE_ADD_SYM(Bytes, str_);
	MODULE_ADD_SYM(Bytes, hash_);
	MODULE_ADD_SYM(Bytes, cmp_);
	MODULE_ADD_SYM(Bytes, len_);
	MODULE_ADD_SYM(Bytes, getItem_);
	MODULE_ADD_SYM(Bytes, add_);
	MODULE_ADD_SYM(Bytes, mul_);
	MODULE_ADD_SYM(Bytes, iter_);
	MODULE_ADD_SYM(Bytes, setItem_);
	MODULE_ADD_SYM(Bytes, delItem_);
	MODULE_ADD_SYM(Bytes, objList_);

	MODULE_SUB_INIT(BytesGen);
	MODULE_ADD_SYM(BytesGen, next);
	MODULE_ADD_SYM(BytesGen, objList_);
	
	set_immutable(Bytes_OBJ);

	check_exceptions(ist);
}
