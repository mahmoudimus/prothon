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


// Prosist.c - peristant Prothon db

#include <stdlib.h>

#include <prothon/prothon_dll.h>

#include <apr_strings.h>
#include <apr_dbm.h>

#define INITIAL_OBJ_COUNT	32

typedef apr_dbm_t* apr_dbm_p;

typedef i32_t psid_t;

typedef struct {
	apr_dbm_p	db;
	apr_pool_t*	pool;
	obj_p		root;
	psid_t		nextid;
	obj_p		id_to_idobj;
	obj_p		idobj_to_id;
	obj_p		id_to_obj;
	obj_p		obj_to_id;
	obj_p		obj_to_proxy;
} psrec_t;

typedef psrec_t* psrec_p;

MODULE_DECLARE(Prosist);
MODULE_DECLARE(ProsistExc);
MODULE_DECLARE(MutWrap);

MODULE_CONSTANT_DECLARE(Prosist, READONLY ); 
MODULE_CONSTANT_DECLARE(Prosist, READWRITE); 
MODULE_CONSTANT_DECLARE(Prosist, RWCREATE ); 
MODULE_CONSTANT_DECLARE(Prosist, RWTRUNCATE  ); 

obj_p PSNONE;


#define IF_APR_DBM_ERR(db, msg1, msg2)													\
	if (aprerr != APR_SUCCESS){															\
		char *msg3, buf[256], errbuf[256];												\
		msg3 = apr_dbm_geterror((db), NULL,  errbuf,  sizeof(errbuf));					\
		if (msg2)																		\
			apr_snprintf(buf, sizeof(buf), "dbm error %s%s: %s", (msg1), (msg2), msg3);	\
		else																			\
			apr_snprintf(buf, sizeof(buf), "dbm error %s: %s", (msg1), msg3);			\
	}if (aprerr != APR_SUCCESS)	

//******************************** ADD_BUF macros *****************************

#define BUF_BLOCK_SIZE 1024

#define ADD_BUF_DECLARE			\
	int _idx, _buf_size;		\
	char* _buf = pr_malloc(1)

#define ADD_BUF_START()						\
	_idx=0; _buf_size = BUF_BLOCK_SIZE;		\
    _buf = pr_realloc(_buf, BUF_BLOCK_SIZE)

#define ADD_CHR(c)											\
{if (_idx == _buf_size) {									\
		_buf_size += BUF_BLOCK_SIZE;						\
		_buf = pr_realloc(_buf, _buf_size);					\
} (_buf[_idx++] = (c)); }

#define ADD_MEM(ptr, len)							\
	if (_idx+len > _buf_size) {						\
			_buf_size = _idx+len+BUF_BLOCK_SIZE;	\
			_buf = pr_realloc(_buf, _buf_size);		\
	} memcpy(_buf+_idx, ptr, len); _idx += len

#define ADD_STR(str) { int slen = (int) strlen(str); ADD_MEM(str, slen); }	

#define ADD_NUM(num)  {									\
	char numbuf[80];									\
	apr_snprintf(numbuf, sizeof(numbuf), "%d", (num));	\
	ADD_STR(numbuf);									\
}

#define ADD_REF(obj) ADD_NUM(obj_to_id(ist, psrecp, (obj)))

#define ADD_BUF_TO_DATUM(datum)	{	\
	char* buf = pr_malloc(_idx+1);	\
	memcpy(buf, _buf, _idx);		\
	buf[_idx] = 0;					\
	datum.dptr  = buf;				\
	datum.dsize = _idx;				\
	_buf = pr_realloc(_buf, 1);		\
}

#define ADD_BUF_END()   pr_free(_buf)

//******************************** new_mutwrap_obj ****************************
obj_p new_mutwrap_obj(isp ist, obj_p obj) {
	obj_p mutwrap_obj = new_object(ist, MutWrap_OBJ);
	SET_TYPE_IF_EXC(MutWrap_OBJ, mutwrap_obj, DATA_TYPE_DATAPTR) return NULL;
	mutwrap_obj->data.ptr = obj;
	set_immutable(mutwrap_obj);
	return mutwrap_obj;
}
#define NEW_MW(obj)  new_mutwrap_obj(ist, obj)

//******************************** load_dbm_int *******************************
i32_t load_dbm_int(psrec_p psrecp, char* key) {
	apr_status_t aprerr;
	apr_datum_t key_d, value;
	ADD_BUF_DECLARE;

	ADD_BUF_START();
	ADD_STR(key);
	ADD_BUF_TO_DATUM(key_d);

	aprerr = apr_dbm_fetch(psrecp->db, key_d, &value);
	IF_APR_DBM_ERR(psrecp->db, "loading int value ", key_d.dptr);
	ADD_BUF_END();

#ifdef PROSIST_DEBUG
	{	char *valbuf, *keybuf = pr_malloc(key_d.dsize+1);
		memcpy(keybuf, key_d.dptr, key_d.dsize);
		keybuf[key_d.dsize] = 0;
		valbuf = pr_malloc(value.dsize+1);
		memcpy(valbuf, value.dptr, value.dsize);
		valbuf[value.dsize] = 0;
		printf("load_dbm_int, key: %s, val: %s\n", keybuf, valbuf); }
#endif

	return atoi(value.dptr);
}

//******************************** store_dbm_int ******************************
void store_dbm_int(psrec_p psrecp, char* key, i32_t val) {
	apr_status_t aprerr;
	apr_datum_t key_d, value;
	ADD_BUF_DECLARE;

	ADD_BUF_START();
	ADD_STR(key);
	ADD_BUF_TO_DATUM(key_d);
	ADD_BUF_START();
	ADD_NUM(val);
	ADD_BUF_TO_DATUM(value);

#ifdef PROSIST_DEBUG
	{	char *valbuf, *keybuf = pr_malloc(key_d.dsize+1);
		memcpy(keybuf, key_d.dptr, key_d.dsize);
		keybuf[key_d.dsize] = 0;
		valbuf = pr_malloc(value.dsize+1);
		memcpy(valbuf, value.dptr, value.dsize);
		valbuf[value.dsize] = 0;
		printf("store_dbm_int, key: %s, val: %s\n", keybuf, valbuf); }
#endif

	aprerr = apr_dbm_store(psrecp->db, key_d, value);
	IF_APR_DBM_ERR(psrecp->db, "storing int value ", key_d.dptr);
	ADD_BUF_END();
}

//******************************** load_obj_by_id *************************
apr_datum_t load_obj_by_id(psrec_p psrecp, psid_t id) {
	apr_status_t aprerr;
	apr_datum_t key, value;
	ADD_BUF_DECLARE;

	ADD_BUF_START();
	ADD_NUM(id);
	ADD_BUF_TO_DATUM(key);

	aprerr = apr_dbm_fetch(psrecp->db, key, &value);
	IF_APR_DBM_ERR(psrecp->db, "loading object ", key.dptr);
	ADD_BUF_END();

#ifdef PROSIST_DEBUG
	{	char *valbuf, *keybuf = pr_malloc(key.dsize+1);
		memcpy(keybuf, key.dptr, key.dsize);
		keybuf[key.dsize] = 0;
		valbuf = pr_malloc(value.dsize+1);
		memcpy(valbuf, value.dptr, value.dsize);
		valbuf[value.dsize] = 0;
		printf("load_obj_by_id, key: %s, val: %s\n", keybuf, valbuf); }
#endif

	return value;
}

//******************************** obj_to_id ********************************
psid_t obj_to_id(isp ist, psrec_p psrecp, obj_p obj) {
	psid_t res = 0;
	obj_p id_obj, mw_obj;
	mw_obj = NEW_MW(obj);
	id_obj = dict_item(ist, psrecp->obj_to_id, mw_obj); if_exc_return -1;
	if (!id_obj) {
		//dump(ist, "obj-to-id.txt", obj);
		raise_exception(ist, ProsistExc_OBJ, "obj_to_id called on obj with no id");
	} else {
		del_unlock(id_obj);
		res = (psid_t) id_obj->data.i64;
	}
	del_unlock(mw_obj);
	return res;
}

//******************************** apr_str ************************************
apr_datum_t apr_str(char* str){
	apr_datum_t apr_str;
	apr_str.dptr  = str;
	apr_str.dsize = strlen(str);
	return apr_str;
}

//******************************** empty_obj **********************************
void empty_obj(isp ist, psrec_p psrecp, obj_p id_obj, int *new_flg) {
	if (*new_flg) {
		obj_p obj = NEW_OBJ(NULL);
		obj_p mw_obj = NEW_MW(obj);
		dict_add(ist, psrecp->obj_to_id, mw_obj, id_obj); if_exc_return;
		dict_add(ist, psrecp->id_to_obj, id_obj, obj); 
	}
	*new_flg = FALSE;
}

//******************************** store_obj **********************************
void store_obj(isp ist, psrec_p psrecp, obj_p obj) {
	apr_status_t aprerr;
	apr_datum_t key, value;
	attr_key_t akey;
	obj_p mw_obj, proxy_obj;
	psid_t id;
	int i, plen, added_attr = FALSE;
	ADD_BUF_DECLARE;

	if (obj == PSNONE || is_archived(obj)) return;
	
	set_archived(obj);

	id = obj_to_id(ist, psrecp, obj); if_exc_return;
	ADD_BUF_START();
	ADD_NUM(id); 
	ADD_BUF_TO_DATUM(key);

	mw_obj = NEW_MW(obj);
	proxy_obj = dict_item(ist, psrecp->obj_to_proxy, mw_obj);
	del_unlock(mw_obj);
	if (!proxy_obj) {
		raise_exception(ist, ProsistExc_OBJ, "store_obj called with unknown obj");
		return;
	}
	ADD_BUF_START();
	plen = proto_len(ist, proxy_obj);
	if (plen) ADD_CHR('p');
	for(i=0; i < plen; i++) {
		psid_t id = obj_to_id(ist, psrecp, proto_item(ist, proxy_obj, i)); if_exc_return;
		ADD_NUM(id);
		if (i < plen-1) ADD_CHR(',') 
		else            ADD_CHR(';'); 
	}
	akey = attr_first_key(ist, proxy_obj);
	if (akey) ADD_CHR('a');
	while(akey) {
		ADD_STR(symch(ist, key_to_sym(ist, akey)));
		ADD_CHR(':');
		ADD_REF(attr_value_by_key(ist, proxy_obj, akey));
		ADD_CHR(',');
		added_attr = TRUE;
		akey = attr_next_key(ist, proxy_obj, akey);
	}
	if (added_attr) { _idx--; ADD_CHR(';'); }
	if (proxy_obj->data_type == DATA_TYPE_IMMDATA) {
		ADD_CHR('i');
		ADD_NUM(proxy_obj->imm_data_len);
		ADD_CHR(':');
		ADD_MEM(&(proxy_obj->data), 8);
	}
	if (has_proto(ist, proxy_obj, OBJ(STRING_PROTO))) {
		ADD_CHR('s');
		ADD_MEM(pr2c_strptr(ist, proxy_obj), (int) pr_strlen(proxy_obj));
	} else if (has_proto(ist, proxy_obj, OBJ(TUPLE_PROTO))) {
		int llen = (int) list_len(ist, proxy_obj);
		ADD_CHR('l');
		for (i=0; i < llen; i++) {
			ADD_REF(list_item(ist, proxy_obj, i));
			if (i < llen-1) ADD_CHR(','); 
		}
	}
	ADD_BUF_TO_DATUM(value);

#ifdef PROSIST_DEBUG
	{	char *valbuf, *keybuf = pr_malloc(key.dsize+1);
		memcpy(keybuf, key.dptr, key.dsize);
		keybuf[key.dsize] = 0;
		valbuf = pr_malloc(value.dsize+1);
		memcpy(valbuf, value.dptr, value.dsize);
		valbuf[value.dsize] = 0;
		printf("store_obj, key: %s, val: %s\n", keybuf, valbuf); }
#endif

	aprerr = apr_dbm_store(psrecp->db, key, value);
	IF_APR_DBM_ERR(psrecp->db, "storing proxy object ", key.dptr);
	ADD_BUF_END();
}

//******************************** unlock_objects *****************************
void unlock_objects(isp ist, psrec_p psrecp) {
	obj_p olst;
	int i, llen;
	olst = dict_keys_values(ist, psrecp->obj_to_proxy, 1 /*keys*/);
	if_exc_return;
	llen = (int) list_len(ist, olst);
	for(i=0; i < llen; i++) {					
		obj_p item = list_item(ist, olst, i);
		if (item != PSNONE) {
			if (has_proto(ist, item, MutWrap_OBJ))
				item = (obj_p) item->data.ptr;
			read_unlock(ist, item);				
		}
	}											
	del_unlock(olst);
}

//******************************** unlock_list ********************************
void unlock_list(isp ist, obj_p olst) {
	int i, llen = (int) list_len(ist, olst);
	for(i=0; i < llen; i++) {					
		obj_p item = list_item(ist, olst, i);
		if (item != PSNONE) {
			if (has_proto(ist, item, MutWrap_OBJ))
				item = (obj_p) item->data.ptr;
			read_unlock(ist, item);				
		}
	}											
	del_unlock(olst);
}

//******************************** append_refs ********************************
void append_refs( isp ist, psrec_p psrecp, obj_p obj, 
					    int objidx, obj_p store_refs, int append_all, int rback_flg ) {
	psid_t id=-1;
	attr_key_t key;
	obj_p mw_obj, id_obj=NULL, id_tuple, proxy_obj = NULL;
	int i, plen, tuple_type=FALSE, obj_known = FALSE, reuse_proxy, repair_obj;

	mw_obj = NEW_MW(obj);
	id_obj = dict_item(ist, psrecp->obj_to_id, mw_obj);
	if (id_obj) {
		id = (psid_t) id_obj->data.i64;
		obj_known = TRUE;
	}
	reuse_proxy = obj_known && rback_flg && is_archived(obj);
	repair_obj  = obj_known && rback_flg && !is_archived(obj);

	if (obj == PSNONE || (!append_all && obj_known)) {
		list_item_set(ist, store_refs, objidx, PSNONE);
		del_unlock(mw_obj);
		return;
	}
	rdlock_rtrn(obj);

	if (!obj_known) {
		id = psrecp->nextid++;
		id_obj = NEW_INT(id);
	}

	if ((id_tuple = get_attr(ist, obj, SYM(ID_)))) {
		apr_status_t aprerr;
		size_t slen;
		char *s1, *s2;
		apr_datum_t key, value;
		ADD_BUF_DECLARE;
		if (append_all & obj_known) 
			goto endidobj;
		dict_add(ist, psrecp->obj_to_id,   mw_obj,   id_obj);
		if (ist->exception_obj) 
			{ del_unlock(mw_obj); return; }
		dict_add(ist, psrecp->id_to_obj,   id_obj,   obj);
		if (ist->exception_obj) 
			{ del_unlock(mw_obj); return; }
		dict_add(ist, psrecp->id_to_idobj, id_tuple, obj);
		if (ist->exception_obj) 
			{ del_unlock(mw_obj); return; }
		dict_add(ist, psrecp->idobj_to_id, mw_obj,   id_tuple);
		if (ist->exception_obj) 
			{ del_unlock(mw_obj); return; }
		ADD_BUF_START();
		ADD_NUM(id); 
		ADD_BUF_TO_DATUM(key);
		s1 = pr2c_strptr(ist, list_item(ist, id_tuple, 0));
		s2 = pr2c_strptr(ist, list_item(ist, id_tuple, 1));
		slen = strlen(s1)+strlen(s2)+2;
		ADD_BUF_START();
		ADD_CHR('`');
		ADD_STR(s1);
		ADD_CHR('.');
		ADD_STR(s2);
		ADD_BUF_TO_DATUM(value);
#ifdef PROSIST_DEBUG
	{	char *valbuf, *keybuf = pr_malloc(key.dsize+1);
		memcpy(keybuf, key.dptr, key.dsize);
		keybuf[key.dsize] = 0;
		valbuf = pr_malloc(value.dsize+1);
		memcpy(valbuf, value.dptr, value.dsize);
		valbuf[value.dsize] = 0;
		printf("store id obj, key: %s, val: %s\n", keybuf, valbuf); }
#endif
		aprerr = apr_dbm_store(psrecp->db, key, value);
		IF_APR_DBM_ERR(psrecp->db, "storing id object ", value.dptr);
		ADD_BUF_END();
endidobj:
		list_item_set(ist, store_refs, objidx, PSNONE);
		read_unlock(ist, obj);
		del_unlock(mw_obj);
		return;
	}
	if (repair_obj || reuse_proxy) {
		proxy_obj = dict_item(ist, psrecp->obj_to_proxy, mw_obj);
		if (ist->exception_obj) {
			del_unlock(mw_obj); 
			del_unlock(id_obj);
			return;
		}
		if (!proxy_obj) {
			raise_exception(ist, ProsistExc_OBJ, "known rollback object not found");
			del_unlock(mw_obj); 
			del_unlock(id_obj);
			return;
		}
	}
	if (repair_obj) {
		obj_p res = call_func0(ist, proxy_obj, SYM(FROMSTORPROXY_));
		if (!res) {
			raise_exception(ist, ProsistExc_OBJ, "fromStorProxy_ not found on proxy obj");
			del_unlock(mw_obj); 
			del_unlock(id_obj);
			return;
		}
		if (obj != proxy_obj) {
			if (obj->has_attrs && obj->attr_proto.attrs)
				pr_free(obj->attr_proto.attrs);
			if (obj->data_type == DATA_TYPE_DATAPTR && obj->data.ptr) {
				pr_free(obj->data.ptr);
			}
			memcpy(obj, res, sizeof(*obj)-2*sizeof(obj->next_obj));
			obj->data = res->data;
			read_lock(ist, obj);
		}
	}
	if (!proxy_obj) {
		proxy_obj = call_func0(ist, obj, SYM(TOSTORPROXY_));
		if (ist->exception_obj) {
			del_unlock(mw_obj); 
			del_unlock(id_obj);
			return;
		}
		if (proxy_obj == PSNONE || is_unclonable(proxy_obj)) { 
			list_item_set(ist, store_refs, objidx, PSNONE);
			read_unlock(ist, obj);
			del_unlock(mw_obj); 
			del_unlock(id_obj);
			del_unlock(proxy_obj); 
			return;
		}
		if (proxy_obj->data_type == DATA_TYPE_DATAPTR) {
			tuple_type = has_proto(ist, proxy_obj, OBJ(TUPLE_PROTO));
			if (!tuple_type && !has_proto(ist, proxy_obj, OBJ(STRING_PROTO))) {
				raise_exception(ist, ProsistExc_OBJ, "unstorable object (data ptr) found in tree");
				del_unlock(mw_obj); 
				del_unlock(id_obj);
				del_unlock(proxy_obj); 
				return;
			}
		}
		dict_add(ist, psrecp->obj_to_proxy, mw_obj,    proxy_obj);
		if (ist->exception_obj) 
			{ del_unlock(mw_obj); return; }
		dict_add(ist, psrecp->obj_to_id,    mw_obj,    id_obj);
		if (ist->exception_obj) 
			{ del_unlock(mw_obj); return; }
		dict_add(ist, psrecp->id_to_obj,    id_obj, obj);
		if (ist->exception_obj) 
			{ del_unlock(mw_obj); return; }
	}
	del_unlock(mw_obj);

	plen = proto_len(ist, proxy_obj);
	for(i=0; i < plen; i++) { // Tuple proto 992060
		obj_p proto = proto_item(ist, proxy_obj, i);
		mw_obj = NEW_MW(proto);
		if ( !is_unclonable(proto) && 
			 (append_all || !dict_item(ist, psrecp->obj_to_id, mw_obj)) )
			list_append(ist, store_refs, proto);
		del_unlock(mw_obj);
		if_exc_return;
	}
	key = attr_first_key(ist, proxy_obj);
	while(key) {
		obj_p attr = attr_value_by_key(ist, proxy_obj, key);
		mw_obj = NEW_MW(attr);
		if ( !is_unclonable(attr) && 
			 (append_all || !dict_item(ist, psrecp->obj_to_id, mw_obj)) )
			list_append(ist, store_refs, attr);
		del_unlock(mw_obj);
		if_exc_return;
		key = attr_next_key(ist, proxy_obj, key);
	}
	if (tuple_type) {
		int llen = (int) list_len(ist, proxy_obj);
		for (i=0; i < llen; i++) {
			obj_p item = list_item(ist, proxy_obj, i);
			mw_obj = NEW_MW(item);
			if ( !is_unclonable(item) && 
			     (append_all || !dict_item(ist, psrecp->obj_to_id, mw_obj)) )
				list_append(ist, store_refs, item);
			del_unlock(mw_obj);
			if_exc_return;
		}
	}
}

//******************************** load_obj ***********************************
obj_p load_obj(isp ist, psrec_p psrecp, obj_p id_obj) {
	apr_datum_t value;
	psid_t n, id;
	char *p, *end;
	obj_p res, obj;

	id = (psid_t) id_obj->data.i64;
	value = load_obj_by_id(psrecp, id);  if_exc_return NULL;
	p = value.dptr;
	end = p + value.dsize;
	
	obj = dict_item(ist, psrecp->id_to_obj, id_obj);  if_exc_return NULL;

	while (p < end) {
		switch (*p) {
			case 'p': {
				int first_proto = TRUE;
				while(*p++ != ';') {
					obj_p ref_idobj, proto;
					for(n=0; *p != ',' && *p != ';'; p++)
						n = n*10 + *p - '0';
					ref_idobj = NEW_INT(n);
					proto = dict_item( ist, psrecp->id_to_obj, ref_idobj);  
					del_unlock(ref_idobj);  if_exc_return NULL;
					if (first_proto)
						switch_proto(ist, obj, proto);
					else
						add_proto(ist, obj, proto);
					first_proto = FALSE;
				}
				break;
			}
			case 'a': {   // 0x009b87e0
				while(*p++ != ';') {
					obj_p ref_id_obj, aval;
					char *q, *aname = pr_malloc(value.dsize);
					for (q=aname; *p != ':';) *q++ = *p++;
					*q = 0;
					for(n=0, p++; *p != ',' && *p != ';'; p++)
						n = n*10 + *p - '0';
					ref_id_obj = NEW_INT(n);
					aval = dict_item( ist, psrecp->id_to_obj, ref_id_obj);  
					if_exc_return NULL;
					set_attr(ist, obj, sym(ist, aname), aval);  if_exc_return NULL;
					del_unlock(ref_id_obj);
				}
				break;
			}
			case 'l': {
				init_list_data(ist, obj, 0);
				while(++p < end) {
					obj_p ref_id_obj, item;
					for(n=0; *p != ',' && p < end; p++)
						n = n*10 + *p - '0';
					ref_id_obj = NEW_INT(n);
					item = dict_item( ist, psrecp->id_to_obj, ref_id_obj);  
					if_exc_return NULL;
					list_append(ist, obj, item);
					del_unlock(ref_id_obj);
				}
				if (has_proto(ist, obj, OBJ(TUPLE_PROTO)))
					set_immutable(obj);
				goto done;
			}
			case 'i':
				SET_TYPE_IF_EXC(OBJ(INT_PROTO), obj, DATA_TYPE_IMMDATA) return NULL;
				for(n=0,p++; *p != ':'; p++)
					n = n*10 + *p - '0';
				obj->imm_data_len = n;
				memcpy(&(obj->data), p+1, sizeof(obj->data));
				set_immutable(obj);
				goto done;
			case 's':
				c2pr_strcpy(ist, obj, p+1, end-(p+1));
				set_immutable(obj);
				goto done;
			case '`':
				del_unlock(obj);
				return NULL;
			default:
				raise_exception(ist, ProsistExc_OBJ, "invalid record from db: %d", id);
				return NULL;
		}
	}
done:
	res = call_func0(ist, obj, SYM(FROMSTORPROXY_));
	if (obj != res) {
		obj_p mw_res = NEW_MW(res); if_exc_return NULL;
		dict_add(ist, psrecp->obj_to_id, mw_res, id_obj);
		if (ist->exception_obj) 
			{ del_unlock(mw_res); return NULL; }
		dict_add(ist, psrecp->id_to_obj, id_obj, res);
		if (ist->exception_obj) 
			{ del_unlock(mw_res); return NULL; }
		del_unlock(mw_res);
		del_unlock(obj);
	}
	set_archived(res);
	return res;
}

//******************************** append_load_ids *********************************
void append_load_ids(isp ist, psrec_p psrecp, obj_p idlist, psid_t id) {
	apr_datum_t value;
	obj_p id_obj;
	psid_t n;
	int new_flg = TRUE;
	char *p, *end;

	id_obj = NEW_INT(id);
	if (dict_item(ist, psrecp->id_to_obj, id_obj)) {
		del_unlock(id_obj);
		return;
	}
	value = load_obj_by_id(psrecp, id); if_exc_return;
	p = value.dptr;
	end = p + value.dsize;

	while (p < end) {
		switch (*p) {
			case '`': {
				int i=0, mlen, module_flg = TRUE;
				char *module, *name;
				obj_p module_obj, mw_obj, obj, id_tuple;
				mlen = max(8, (int) value.dsize);
				module = pr_malloc(mlen);
				for(p++; *p != '.'; p++) module[i++] = *p;
				module[i] = 0;
				if (module[0] == '*')
					module_flg = FALSE;
				name = pr_malloc(value.dsize - i);
				for(i=0,p++; p < end; p++) name[i++] = *p;
				name[i] = 0;
				// do auto-import here XXX
				if (module_flg)	
					module_obj = get_attr(ist, OBJ(MODULES), sym(ist, module));
				else
					module_obj = OBJ(OBJECT);
				if_exc_return;
				obj = get_attr(ist, module_obj, sym(ist, name)); if_exc_return;
				if (!obj) {
					raise_exception(ist, ProsistExc_OBJ, "identity object %s not found in module %s", name, module);
					return;
				}
				id_tuple = get_attr(ist, obj, SYM(ID_)); if_exc_return;
				if (!id_tuple) {
					raise_exception(ist, ProsistExc_OBJ, "identity object is missing id tuple");
					return;
				}
				mw_obj = NEW_MW(obj);
				dict_add(ist, psrecp->id_to_obj, id_obj, obj);     if_exc_return;
				dict_add(ist, psrecp->obj_to_id, mw_obj, id_obj);     if_exc_return;
				dict_add(ist, psrecp->id_to_idobj, id_tuple, obj); if_exc_return;
				dict_add(ist, psrecp->idobj_to_id, mw_obj, id_tuple);
				del_unlock(mw_obj);
				break;
			}
			case 'p': {
				empty_obj(ist, psrecp, id_obj, &new_flg);
				while(*p++ != ';') {
					obj_p ref_id_obj;
					for(n=0; *p != ',' && *p != ';'; p++)
						n = n*10 + *p - '0';
					ref_id_obj = NEW_INT(n);
					if (!dict_item(ist, psrecp->id_to_obj, ref_id_obj))
						list_append(ist, idlist, ref_id_obj);
					else
						del_unlock(ref_id_obj);
				}
				break;
			}
			case 'a': {
				empty_obj(ist, psrecp, id_obj, &new_flg);
				while(*p++ != ';') {
					obj_p ref_id_obj;
					while(*p++ != ':');
					for(n=0; *p != ',' && *p != ';'; p++)
						n = n*10 + *p - '0';
					ref_id_obj = NEW_INT(n);
					if (!dict_item(ist, psrecp->id_to_obj, ref_id_obj))
						list_append(ist, idlist, ref_id_obj);
					else
						del_unlock(ref_id_obj);
				}
				break;
			}
			case 'l': {
				empty_obj(ist, psrecp, id_obj, &new_flg);
				while(++p < end) {
					obj_p ref_id_obj;
					for(n=0; *p != ',' && p < end; p++)
						n = n*10 + *p - '0';
					ref_id_obj = NEW_INT(n);
					if (!dict_item(ist, psrecp->id_to_obj, ref_id_obj))
						list_append(ist, idlist, ref_id_obj);
					else
						del_unlock(ref_id_obj);
				}
				return;
			}
			case 'i':
			case 's':
				empty_obj(ist, psrecp, id_obj, &new_flg);
				return;
			default:
				raise_exception(ist, ProsistExc_OBJ, "invalid record from db: %d", id);
				return;
		}
	}
}

#define IF_EXC_UNLKOBJS_RTRN()			\
	if (ist->exception_obj) {			\
		unlock_objects(ist, psrecp);	\
		return;							\
	}

//******************************** store_new_db *******************************
void store_new_db(isp ist, psrec_p psrecp) {
	obj_p store_refs;
	int i, llen;
#ifdef PROSIST_DEBUG
	printf ("\nStarting store_new_db\n\n");
#endif
	if (apr_dbm_exists(psrecp->db, apr_str("0"))) {
		raise_exception(ist, ProsistExc_OBJ, "Database already exists");
		return;
	}
	if (is_unclonable(psrecp->root)){
		raise_exception(ist, ProsistExc_OBJ, "root object is not storable");
		return;
	}
	store_refs = new_list_obj(ist, 20); 
	list_append(ist, store_refs, psrecp->root);
	for (i=0; i < (int) list_len(ist, store_refs); i++) {
		obj_p item = list_item(ist, store_refs, i);
		append_refs(ist, psrecp, item, i, store_refs, FALSE, FALSE);
		IF_EXC_UNLKOBJS_RTRN();
	}
	llen = (int) list_len(ist, store_refs);
	for (i=0; i < llen; i++) {
		obj_p item = list_item(ist, store_refs, i);
		store_obj(ist, psrecp, item);
		IF_EXC_UNLKOBJS_RTRN();
	}
	unlock_objects(ist, psrecp);
	store_dbm_int(psrecp, "!nextid", psrecp->nextid);
#ifdef PROSIST_DEBUG
	printf ("\nEnding store_new_db\n\n");
#endif
}

//******************************** commit_db ************************************
void commit_db(isp ist, psrec_p psrecp) {
	obj_p store_refs;
	int i, llen;

#ifdef PROSIST_DEBUG
	printf ("\nStarting commit_db\n\n");
#endif	
	dict_clear(ist, psrecp->obj_to_proxy);

	store_refs = new_list_obj(ist, 20);
	list_append(ist, store_refs, psrecp->root);
	for (i=0; i < (int) list_len(ist, store_refs); i++) {
		obj_p item = list_item(ist, store_refs, i);
		append_refs(ist, psrecp, item, i, store_refs, TRUE, FALSE);
		IF_EXC_UNLKOBJS_RTRN();
	}
	llen = (int) list_len(ist, store_refs);
	for (i=0; i < llen; i++) {
		obj_p item = list_item(ist, store_refs, i);
		store_obj(ist, psrecp, item);
		IF_EXC_UNLKOBJS_RTRN();
	}
	unlock_objects(ist, psrecp);
	store_dbm_int(psrecp, "!nextid", psrecp->nextid);
#ifdef PROSIST_DEBUG
	printf ("\nEnding commit_db\n\n");
#endif
}

#define IF_EXC_UNLKLIST_RTRN				\
	if (ist->exception_obj) {				\
		unlock_list(ist, rollback_refs);	\
		return;								\
	}

//******************************** rollback_db ************************************
void rollback_db(isp ist, psrec_p psrecp) {
	obj_p rollback_refs;
	int i;
#ifdef PROSIST_DEBUG
	printf ("\nStarting rollback_db\n\n");
#endif
	rollback_refs = new_list_obj(ist, 20);
	list_append(ist, rollback_refs, psrecp->root);
	IF_EXC_UNLKLIST_RTRN;
	for (i=0; i < (int) list_len(ist, rollback_refs); i++) {
		obj_p item = list_item(ist, rollback_refs, i);
		append_refs(ist, psrecp, item, i, rollback_refs, TRUE, TRUE);
		IF_EXC_UNLKLIST_RTRN;
	}
	unlock_list(ist, rollback_refs);
#ifdef PROSIST_DEBUG
	printf ("\nEnding rollback_db\n\n");
#endif
}

//******************************** load_db ************************************
obj_p load_db(isp ist, psrec_p psrecp) {
	int llen, idx = 0;
	obj_p id_obj, idlist, root = NULL;
#ifdef PROSIST_DEBUG
	printf ("\nStarting load_db\n\n");
#endif
	psrecp->nextid = load_dbm_int(psrecp, "!nextid");
	idlist = new_list_obj(ist, 20);
	append_load_ids(ist, psrecp, idlist, 0); if_exc_return NULL;
	for (; idx < (int) list_len(ist, idlist); idx++) {
		psid_t id = (psid_t) (list_item(ist, idlist, idx)->data.i64);
		append_load_ids(ist, psrecp, idlist, id); if_exc_return NULL;
	}
	llen = (int) list_len(ist, idlist);
	for(idx=llen-1; idx >= 0; idx--) {
		id_obj = list_item(ist, idlist, idx);
		load_obj(ist, psrecp, id_obj); if_exc_return NULL;
		del_unlock(id_obj);
	}
	id_obj = NEW_INT(0);
	root = load_obj(ist, psrecp, id_obj);
	del_unlock(id_obj);
#ifdef PROSIST_DEBUG
	printf ("\nEnding load_db\n\n");
#endif
	return root;
}

//******************************** FUNCTION DEFS ******************************
MODULE_START(Prosist)
{
	PSNONE = NEW_OBJ(NULL);

	Prosist_OBJ = NEW_OBJ(NULL);
	MODULE_SET_DOC(Prosist, "persistant database module for Prothon objects");
	MODULE_ADD_TO_BASE(Prosist);

	ProsistExc_OBJ = NEW_OBJ(OBJ(EXCEPTION));
	MODULE_SET_DOC(ProsistExc, "Prosist module error");
	MODULE_ADD_TO_OBJ(ProsistExc, Prosist_OBJ, "ProsistException");

	MutWrap_OBJ = NEW_OBJ(NULL);
	MODULE_SET_DOC(MutWrap, "wrapper to make any object immutable and storable by object id");
	MODULE_ADD_TO_OBJ(MutWrap, Prosist_OBJ, "MutWrap");

	Prosist_OBJ->unclonable = TRUE;
	set_obj_rdacc(Prosist_OBJ, ACC_USER1);
	
	MODULE_CONSTANT_INT(Prosist, READONLY,   APR_DBM_READONLY ); // read-only access
	MODULE_CONSTANT_INT(Prosist, READWRITE,  APR_DBM_READWRITE); // read-write access
	MODULE_CONSTANT_INT(Prosist, RWCREATE,   APR_DBM_RWCREATE ); // r/w, create if needed
	MODULE_CONSTANT_INT(Prosist, RWTRUNCATE, APR_DBM_RWTRUNC  ); // r/w, truncate if already there
}

DEF( Prosist, init_, FPARM4( name,  NULL, 
                           root,  OBJ(NONE), 
                           mode,  Prosist_RWCREATE, 
                           uperm, NEW_INT(APR_UREAD|APR_UWRITE) ) ) {

	apr_status_t aprerr;
	apr_pool_t* cntxt;
	apr_dbm_p db;
	psrec_p psrecp;
	char *name;
    obj_p root_obj;
    int mode, uperm;
	
	BIN_EMPTY_CHK();
	STRING_PARAM(1, name);
	root_obj = parms[3];
	INT_32_PARAM(3, mode);
	INT_32_PARAM(4, uperm);

	if (root_obj == OBJ(NONE)) {
		if (mode == APR_DBM_RWTRUNC) {
			raise_exception(ist, ProsistExc_OBJ, "root == None incompatible with RWTRUNC mode");
			return NULL;
		}
	} else {
		if (mode != APR_DBM_RWCREATE && mode != APR_DBM_RWTRUNC) {
			raise_exception(ist, ProsistExc_OBJ, "root obj given but mode doesn't allow creation");
			return NULL;
		}
	}
	aprerr = apr_pool_create(&cntxt, get_pr_head_pool());
	IF_APR_ERR("out of memory opening Prosist database") return NULL;

	aprerr = apr_dbm_open_ex(&db, "SDBM", name, mode, uperm, cntxt);  
	IF_APR_ERR("opening Prosist database") return NULL;

	SET_TYPE_IF_EXC(Prosist_OBJ, self, DATA_TYPE_DATAPTR) return NULL;
	self->data.ptr		= psrecp = pr_malloc(sizeof(psrec_t));
	psrecp->db			= db;
	psrecp->pool		= cntxt;
	psrecp->id_to_idobj = NEW_DICT(4*INITIAL_OBJ_COUNT);
	psrecp->idobj_to_id = NEW_DICT(4*INITIAL_OBJ_COUNT);
	psrecp->id_to_obj   = NEW_DICT(4*INITIAL_OBJ_COUNT);
	psrecp->obj_to_id   = NEW_DICT(4*INITIAL_OBJ_COUNT);
	psrecp->obj_to_proxy = NEW_DICT(4*INITIAL_OBJ_COUNT);

	if (root_obj == OBJ(NONE)) {
		root_obj = psrecp->root = load_db(ist, psrecp);
	} else {
		psrecp->root   = root_obj;
		psrecp->nextid = 0;
		store_new_db(ist, psrecp);  if_exc_return NULL; 
	}
	read_unlock(ist, self);
	set_obj_rdacc(self, ACC_USER1);
	set_obj_doc(self, "Prosist DataBase");
	set_attr(ist, self, sym(ist, "filename"), parms[1]);
	set_attr(ist, self, sym(ist, "root"),     root_obj);
	read_lock(ist, self);
	return OBJ(NONE);
}

DEF(Prosist, rollBack, NULL) {
	psrec_p psrecp = self->data.ptr;
	BIN_CONTENT_CHK(Prosist);
	if (psrecp) 
		rollback_db(ist, psrecp);
	else {
		raise_exception(ist, ProsistExc_OBJ, "rollBack attempted on closed database");
		return NULL;
	}
	return self;
}

DEF(Prosist, commit, NULL) {
	psrec_p psrecp = self->data.ptr;
	BIN_CONTENT_CHK(Prosist);
	if (psrecp) 
		commit_db(ist, psrecp);
	else {
		raise_exception(ist, ProsistExc_OBJ, "commit attempted on closed database");
		return NULL;
	}
	return self;
}


DEF(Prosist, close, NULL) { 
	psrec_p psrecp = self->data.ptr;
	BIN_CONTENT_CHK(Prosist);
	if (psrecp) {
		apr_dbm_close(psrecp->db);
		apr_pool_destroy(psrecp->pool);
		self->data.ptr = NULL;
	}
	return self;
}

DEF(Prosist, closed_QUES, NULL) { 
	BIN_CONTENT_CHK(Prosist);
	if (self->data.ptr) return OBJ(PR_FALSE);
	else                return OBJ(PR_TRUE);
}

DEF(Prosist, objList_, FORM_RPARAM) {
	return parms[1];
}

DEF(MutWrap, hash_, NULL) {
	BIN_CONTENT_CHK(MutWrap);
	return new_hash_obj(ist, (i32_t)(intptr_t) self->data.ptr);
}

DEF(MutWrap, eq__QUES, FORM_RPARAM) {
	BIN_CONTENT_CHK(MutWrap);
	CHECK_TYPE_EXC(parms[1], MutWrap_OBJ, "immutable object wrapper");
	if (self->data.ptr == parms[1]->data.ptr) return OBJ(PR_TRUE);
	else									  return OBJ(PR_FALSE);
}

DEF(MutWrap, objList_, FORM_RPARAM) {
	return parms[1];
}

MAIN_MODULE_INIT(Prosist)
{
	MODULE_SUB_INIT(Prosist);
	MODULE_ADD_SYM(Prosist, init_);
	MODULE_ADD_SYM(Prosist, rollBack);
	MODULE_ADD_SYM(Prosist, commit);
	MODULE_ADD_SYM(Prosist, close);
	MODULE_ADD_SYM(Prosist, closed_QUES);
	MODULE_ADD_SYM(Prosist, objList_);

	MODULE_ADD_SYM(MutWrap, hash_);
	MODULE_ADD_SYM(MutWrap, eq__QUES);
	MODULE_ADD_SYM(MutWrap, objList_);

	check_exceptions(ist);
}
