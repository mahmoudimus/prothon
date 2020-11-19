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

// sqlite.c  Prothon wrapper module for sqlite DB
//
// Conforms to subset of Python DBAPI 2.0

#include <stdio.h>
#include <string.h>
#include <prothon/prothon_dll.h>
#include "sqlite.h"

#define INITIAL_ROW_LIST_SIZE 20

typedef struct sqlite* sqlite_p;

typedef struct {
	isp			ist;
	obj_p		conn_obj;
	obj_p		row_list;
	int			row_ptr;
} cursor_t;

typedef cursor_t* cursor_p;

MODULE_DECLARE(sqlite);
MODULE_DECLARE(Connection);
MODULE_DECLARE(Cursor);
MODULE_DECLARE(Error);

MODULE_START(sqlite)
{
	sqlite_OBJ = NEW_OBJ(NULL);
	MODULE_SET_DOC(sqlite, "sqlite database wrapper module");
	MODULE_ADD_TO_BASE(sqlite);

	Connection_OBJ = NEW_OBJ(NULL);
	MODULE_SET_DOC(Connection, "sqlite Connection prototype object");
	MODULE_ADD_TO_OBJ(Connection, sqlite_OBJ, "Connection");

	Cursor_OBJ = NEW_OBJ(NULL);
	MODULE_SET_DOC(Cursor, "sqlite Cursor prototype object");
	MODULE_ADD_TO_OBJ(Cursor, sqlite_OBJ, "Cursor");

	Error_OBJ = NEW_OBJ(OBJ(EXCEPTION));
	MODULE_SET_DOC(Error, "sqlite db error");
	MODULE_ADD_TO_OBJ(Error, sqlite_OBJ, "Error");

	Connection_OBJ->unclonable = TRUE;
	set_obj_rdacc(Connection_OBJ, ACC_USER1);
	Cursor_OBJ->unclonable = TRUE;
	set_obj_rdacc(Cursor_OBJ, ACC_USER1);
}

DEF(sqlite, connect, list2(ist, sym(ist, "database"), NULL)) {
	obj_p conn_obj;
	char *filename, *errmsg=NULL;
	CHECK_TYPE_EXC(parms[1], OBJ(STRING_PROTO), "string");
	filename = pr2c_strptr(ist, parms[1]);
	conn_obj = NEW_OBJ(Connection_OBJ);
	set_obj_doc(conn_obj, "sqlite connection");
	conn_obj->unclonable = TRUE;
	set_obj_rdacc(conn_obj, ACC_USER1);
	SET_TYPE_IF_EXC(Connection_OBJ, conn_obj, DATA_TYPE_DATAPTR) return NULL;
	conn_obj->data.ptr = sqlite_open(filename, 0, &errmsg);
	if (errmsg) {
		raise_exception(ist, Error_OBJ, errmsg);
		return NULL;
	}
	return conn_obj;
}

DEF(Connection, cursor, NULL) {
	BIN_CONTENT_CHK(Connection);
	if (self->data.ptr) {
		cursor_p cursorp;
		obj_p curs_obj = NEW_OBJ(Cursor_OBJ);
		set_obj_doc(curs_obj, "sqlite cursor");
		SET_TYPE_IF_EXC(Cursor_OBJ, curs_obj, DATA_TYPE_DATAPTR) return NULL;
		cursorp = curs_obj->data.ptr  = pr_malloc(sizeof(cursor_t));
		cursorp->ist	  = ist;
		cursorp->conn_obj = self;
		cursorp->row_list = NEW_LIST(INITIAL_ROW_LIST_SIZE);
		del_unlock(cursorp->row_list);
		return curs_obj;
	} else {
		raise_exception(ist, Error_OBJ, "connection closed");
		return NULL;
	}
}

DEF(Connection, rollBack, NULL) {
	BIN_CONTENT_CHK(Connection);
	if (self->data.ptr) {
		raise_exception(ist, Error_OBJ, "rollBack not implemented yet, use query");
		return NULL;
	} else {
		raise_exception(ist, Error_OBJ, "connection closed");
		return NULL;
	}
}

DEF(Connection, commit, NULL) {
	BIN_CONTENT_CHK(Connection);
	if (self->data.ptr) {
		raise_exception(ist, Error_OBJ, "commit not implemented yet, use query");
		return NULL;
	} else {
		raise_exception(ist, Error_OBJ, "connection closed");
		return NULL;
	}
}

DEF(Connection, close, NULL) {
	BIN_CONTENT_CHK(Connection);
	if (self->data.ptr) {
		sqlite_close((sqlite_p)(self->data.ptr));
		self->data.ptr = NULL;
	}
	return OBJ(NONE);
}

DEF(Connection, objList_, FORM_RPARAM) {
	return parms[1];
}

int xCallback(void *pArg, int argc, char **argv, char **columnNames) {
	int i;
	cursor_p cursorp  = (cursor_p) pArg;
	isp      ist      = cursorp->ist;
	obj_p    row      = NEW_LIST(argc);
	for(i=0; i < argc; i++)
		list_append(ist, row, NEW_STRING(argv[i]));
	switch_proto(ist, row, OBJ(TUPLE_PROTO));
	set_immutable(row);
	list_append(ist, cursorp->row_list, row);
  return 0;
}

DEF(Cursor, execute_BANG, list4(ist, sym(ist,  "statement"), NULL,
	                            sym(ist, "parameters"), OBJ(NONE) ) ) {
	cursor_p cursorp;
	sqlite_p db;
    char *sql, *errmsg=NULL;

	BIN_CONTENT_CHK(Cursor);
	CHECK_TYPE_EXC(parms[1], OBJ(STRING_PROTO), "string");
	sql = pr2c_strptr(ist, parms[1]);
	if (parms[3] != OBJ(NONE)) {
		raise_exception(ist, Error_OBJ, "parameters not implemented yet");
		return NULL;
	}
	cursorp  = (cursor_p)(self->data.ptr);
	clr_immutable(cursorp->row_list);
	list_clear(ist, cursorp->row_list);
	db = (sqlite_p)(cursorp->conn_obj->data.ptr);
	sqlite_exec(db, sql, xCallback, cursorp, &errmsg);
	if (errmsg) {
		raise_exception(ist, Error_OBJ, errmsg);
		return NULL;
	}
	cursorp->row_ptr = 0;
	switch_proto(ist, cursorp->row_list, OBJ(TUPLE_PROTO));
	set_immutable(cursorp->row_list);
	return self;
}

DEF(Cursor, fetchOne, NULL) {
	int llen, p;
	obj_p res;
	cursor_p cursorp  = (cursor_p)(self->data.ptr);

	BIN_CONTENT_CHK(Cursor);
	p = cursorp->row_ptr;
	llen = (int) list_len(ist, cursorp->row_list);
	if (p < llen) {
		res = list_item(ist, cursorp->row_list, p);
		cursorp->row_ptr = p+1;
	} else
		res = OBJ(NONE);
	return res;
}

DEF(Cursor, fetchMany, list2(ist, sym(ist, "n"), NULL)) {
	int i, n, llen, p, e;
	obj_p res;
	cursor_p cursorp  = (cursor_p)(self->data.ptr);

	BIN_CONTENT_CHK(Cursor);
	CHECK_TYPE_EXC(parms[1], OBJ(INT_PROTO), "integer");
	n = (int) parms[1]->data.i64;
	p = cursorp->row_ptr;
	llen = (int) list_len(ist, cursorp->row_list);
	e = p+n;
	e = min(e, llen);
	res = NEW_LIST(e-p);
	for (i=p; i < e; i++) 
		list_append(ist, res, list_item(ist, cursorp->row_list, i));
	switch_proto(ist, res, OBJ(TUPLE_PROTO));
	set_immutable(res);
	cursorp->row_ptr = e;
	return res;
}

DEF(Cursor, fetchAll, NULL) {
	int i, llen, p;
	obj_p res;
	cursor_p cursorp  = (cursor_p)(self->data.ptr);

	BIN_CONTENT_CHK(Cursor);
	p = cursorp->row_ptr;
	llen = (int) list_len(ist, cursorp->row_list);
	res = NEW_LIST(llen-p);
	for (i=p; i < llen; i++) 
		list_append(ist, res, list_item(ist, cursorp->row_list, i));
	switch_proto(ist, res, OBJ(TUPLE_PROTO));
	set_immutable(res);
	cursorp->row_ptr = llen;
	return res;
}

DEF(Cursor, rowCount, NULL) {
	cursor_p cursorp  = (cursor_p)(self->data.ptr);
	BIN_CONTENT_CHK(Cursor);
	if (cursorp->row_list) 
		return NEW_INT(list_len(ist, cursorp->row_list));
	return NEW_INT(-1);
}

DEF(Cursor, objList_, FORM_RPARAM) {
	cursor_p cursorp  = (cursor_p)(self->data.ptr);
	if (!self->data.ptr) return parms[1];
	if (cursorp->row_list)
		return cursorp->row_list;
	return parms[1];
}

MAIN_MODULE_INIT(sqlite)
{
	MODULE_SUB_INIT(sqlite);
	MODULE_ADD_SYM(sqlite, connect);

	MODULE_ADD_SYM(Connection, cursor);
	MODULE_ADD_SYM(Connection, rollBack);
	MODULE_ADD_SYM(Connection, commit);
	MODULE_ADD_SYM(Connection, close);
	MODULE_ADD_SYM(Connection, objList_);

	MODULE_ADD_SYM(Cursor, execute_BANG);
	MODULE_ADD_SYM(Cursor, fetchOne);
	MODULE_ADD_SYM(Cursor, fetchMany);
	MODULE_ADD_SYM(Cursor, fetchAll);
	MODULE_ADD_SYM(Cursor, rowCount);
	MODULE_ADD_SYM(Cursor, objList_);

	check_exceptions(ist);
}
