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

// DBM.c

#include <prothon/prothon_dll.h>

#include <apr_strings.h>
#include <apr_dbm.h>

typedef apr_dbm_t*   apr_dbm_p;

#define OPEN_CHK(msg)																	\
	if (!db) {																			\
		char buf[512];																	\
		apr_snprintf(buf, sizeof(buf), "attempted %s on closed DBM database", (msg));	\
		raise_exception(ist, DBM_Exc_OBJ, buf);											\
		return NULL;																	\
	}

#define IF_APR_DBM_ERR(db, msg1)											\
	if (aprerr != APR_SUCCESS){												\
		char *msg2, buf[256], errbuf[256];									\
		msg2 = apr_dbm_geterror((db), NULL,  errbuf,  sizeof(errbuf));		\
		apr_snprintf(buf, sizeof(buf), "dbm error in %s: %s", (msg1), msg2);\
		return NULL;														\
	}

MODULE_DECLARE(DBM);
MODULE_DECLARE(DB);
MODULE_DECLARE(DBM_Exc);

MODULE_CONSTANT_DECLARE(DBM, READONLY   ); 
MODULE_CONSTANT_DECLARE(DBM, READWRITE  ); 
MODULE_CONSTANT_DECLARE(DBM, RWCREATE   ); 
MODULE_CONSTANT_DECLARE(DBM, RWTRUNCATE ); 

MODULE_START(DBM)
{
	DBM_OBJ = NEW_OBJ(NULL);
	set_obj_doc(DBM_OBJ, "DBM module");
	set_obj_id(DBM_OBJ, *, DBM);
	MODULE_ADD_TO_BASE(DBM);
	DBM_OBJ->unclonable = TRUE;

	DB_OBJ = NEW_OBJ(NULL);
	set_obj_doc(DB_OBJ, "DBM DB object prototype");
	set_obj_id(DB_OBJ, DBM, DB);
	MODULE_ADD_TO_OBJ(DB, DBM_OBJ, "DB");
	DBM_OBJ->unclonable = TRUE;

	DBM_Exc_OBJ = NEW_OBJ(NULL);
	set_obj_doc(DBM_Exc_OBJ, "DBM DB error");
	set_obj_id(DBM_Exc_OBJ, DBM, DBMError);
	MODULE_ADD_TO_OBJ(DBM_Exc, DBM_OBJ, "DBMError");
	DBM_OBJ->unclonable = TRUE;

	MODULE_CONSTANT_INT(DBM, READONLY,   APR_DBM_READONLY  ); // read-only access
	MODULE_CONSTANT_INT(DBM, READWRITE,  APR_DBM_READWRITE ); // read-write access
	MODULE_CONSTANT_INT(DBM, RWCREATE,   APR_DBM_RWCREATE  ); // r/w, create if needed
	MODULE_CONSTANT_INT(DBM, RWTRUNCATE, APR_DBM_RWTRUNC   ); // r/w, truncate if already there
}

DEF( DB, getNames, FPARM2( name,  NULL, 
	                       type,  NEW_STRING("SDBM") ) ) {
	apr_status_t aprerr;
	apr_pool_t* pool;
	obj_p res;
	char *name, *type;
	char *used1, *used2;

	BIN_CONTENT_CHK(DB);

	STRING_PARAM(1, name);
	STRING_PARAM(2, type);

	aprerr = apr_pool_create(&pool, get_pr_head_pool());
	IF_APR_ERR("out of memory in getNames()") return NULL;

	aprerr = apr_dbm_get_usednames_ex(pool, type, name, (const char **)&used1,
					  (const char **)&used2);
	IF_APR_ERR("getNames() in dbm") return NULL;

	res = NEW_LIST(2);
	list_append(ist, res, NEW_STRING(used1));
	// apr_free(used1); XXX how do I free these?
	if (used2) {
		list_append(ist, res, NEW_STRING(used2));
		// apr_free(used2); XXX how do I free these?
	} else
		list_append(ist, res, OBJ(NONE));
	set_immutable(res);
	switch_proto(ist, res, OBJ(TUPLE_PROTO));
	return res;
}

DEF( DB, init_, FPARM4( name,  NULL, 
	                       mode,  DBM_RWCREATE, 
	                       type,  NEW_STRING("SDBM"), 
						   uperm, NEW_INT(APR_UREAD|APR_UWRITE) ) ) {
	apr_status_t aprerr;
	apr_dbm_p db;
	apr_pool_t* cntxt;
	char *name, *type;
	int mode, uperm;

	BIN_EMPTY_CHK();
	STRING_PARAM(1, name);
	INT_32_PARAM(2, mode);
	STRING_PARAM(3, type);
	INT_32_PARAM(4, uperm);

	aprerr = apr_pool_create(&cntxt, get_pr_head_pool());
	IF_APR_ERR("out of memory opening dbm database") return NULL;

	aprerr = apr_dbm_open_ex(&db, type, name, mode, uperm, cntxt);  
	IF_APR_ERR("opening dbm database") return NULL;

	read_unlock(ist, self);
	set_attr(ist, self, sym(ist, "name"),  parms[1]);
	set_attr(ist, self, sym(ist, "mode"),  parms[3]);
	set_attr(ist, self, sym(ist, "type"),  parms[5]);
	set_attr(ist, self, sym(ist, "uperm"), parms[7]);
	read_lock(ist, self);

	SET_TYPE_IF_EXC(DB_OBJ, self, DATA_TYPE_DATAPTR) return NULL;
	self->data.ptr  = db;

	return OBJ(NONE);
}

DEF(DB, fetch, FPARM1(key, NULL)) {
	apr_status_t aprerr;
	apr_dbm_p db = self->data.ptr;
	apr_datum_t key, value;

	BIN_CONTENT_CHK(DB);
	OPEN_CHK("fetch(key)");
	STRBIN_PARAM(1, key);

	aprerr = apr_dbm_fetch (db, key, &value);
	IF_APR_DBM_ERR(db, "fetch(key)");

	if (value.dptr)
		return NEW_STRINGN(value.dptr, value.dsize);
	else
		return OBJ(NONE);
}

DEF(DB, store, FPARM2(key, NULL, value, NULL)) {
	apr_status_t aprerr;
	apr_dbm_p db = self->data.ptr;
	apr_datum_t key, value;

	BIN_CONTENT_CHK(DB);
	OPEN_CHK("store(key, value)");
	STRBIN_PARAM(1, key);
	STRBIN_PARAM(2, value);

	aprerr = apr_dbm_store (db, key, value);
	IF_APR_DBM_ERR(db, "store(key,value)");

	return parms[3];
}

DEF(DB, delete, FPARM1(key, NULL)) {
	apr_status_t aprerr;
	apr_dbm_p db = self->data.ptr;
	apr_datum_t key;

	BIN_CONTENT_CHK(DB);
	OPEN_CHK("delete(key)");
	STRBIN_PARAM(1, key);

	aprerr = apr_dbm_delete(db, key);
	IF_APR_DBM_ERR(db, "delete(key)");

	return parms[1];
}

DEF(DB, exists_QUES, FPARM1(key, NULL)) {
	apr_dbm_p db = self->data.ptr;
	apr_datum_t key;

	BIN_CONTENT_CHK(DB);
	OPEN_CHK("exists?(key)");
	STRBIN_PARAM(1, key);

	if (apr_dbm_exists(db, key)) return OBJ(PR_TRUE);
	else                         return OBJ(PR_FALSE);
}

DEF(DB, firstKey, NULL) {
	apr_status_t aprerr;
	apr_dbm_p db = self->data.ptr;
	apr_datum_t key;

	BIN_CONTENT_CHK(DB);
	OPEN_CHK("firstKey()");

	aprerr = apr_dbm_firstkey(db, &key);
	IF_APR_DBM_ERR(db, "firstKey()");

	if (key.dptr)
		return NEW_STRINGN(key.dptr, key.dsize);
	else
		return OBJ(NONE);
}

DEF(DB, nextKey, FPARM1(key, NULL)) {
	apr_status_t aprerr;
	apr_dbm_p db = self->data.ptr;
	apr_datum_t key;

	BIN_CONTENT_CHK(DB);
	OPEN_CHK("nextKey(key)");
	STRBIN_PARAM(1, key);

	aprerr = apr_dbm_nextkey(db, &key);
	IF_APR_DBM_ERR(db, "nextKey(key)");

	if (key.dptr)
		return NEW_STRINGN(key.dptr, key.dsize);
	else
		return OBJ(NONE);
}

DEF(DB, freeDatum, FPARM1(data, NULL)) {
	apr_dbm_p db = self->data.ptr;
	apr_datum_t data;

	BIN_CONTENT_CHK(DB);
	OPEN_CHK("freeDatum(data)");
	STRBIN_PARAM(1, data);

	apr_dbm_freedatum(db, data);
	return parms[1];
}

DEF(DB, close, NULL) {
	apr_dbm_p db = self->data.ptr;
	BIN_CONTENT_CHK(DB);
	if (db) apr_dbm_close(db);
	self->data.ptr = NULL;
	return self;
}

DEF(DB, closed_QUES, NULL) {
	apr_dbm_p db = self->data.ptr;
	BIN_CONTENT_CHK(DB);
	if (db) return OBJ(PR_FALSE);
	else    return OBJ(PR_TRUE);
}

DEF(DB, objList_, FORM_RPARAM) {
	return parms[1];
}

MAIN_MODULE_INIT(DBM) {
	MODULE_SUB_INIT(DBM);
	MODULE_ADD_SYM(DB, getNames);
	MODULE_ADD_SYM(DB, fetch);
	MODULE_ADD_SYM(DB, init_);
	MODULE_ADD_SYM(DB, store);
	MODULE_ADD_SYM(DB, delete);
	MODULE_ADD_SYM(DB, exists_QUES);
	MODULE_ADD_SYM(DB, firstKey);
	MODULE_ADD_SYM(DB, nextKey);
	MODULE_ADD_SYM(DB, freeDatum);
	MODULE_ADD_SYM(DB, close);
	MODULE_ADD_SYM(DB, closed_QUES);
	MODULE_ADD_SYM(DB, objList_);

	check_exceptions(ist);
}
