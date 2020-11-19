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


// builtins-file.c

#include <prothon/prothon.h>
#include <stdio.h>
#include <string.h>
#include "object.h"
#include <prothon/prothon_dll.h>

#include <apr_file_io.h>
#include <apr_strings.h>

#define MIN_BUFSIZE  65536

typedef struct {
	apr_file_t	*stream;	/* APR file reference */
	int			 bufsize;	/* Size of our buffer */
	apr_pool_t	*pool;		/* Pool associated with this file */
	apr_int32_t	flags;		/* APR flags */
} file_t;

typedef file_t* file_p;

#define STREAM(obj)  (((file_p)((obj)->data.ptr))->stream)
#define BUFSIZE(obj) (((file_p)((obj)->data.ptr))->bufsize)
#define POOL(obj)    (((file_p)((obj)->data.ptr))->pool)
#define FLAGS(obj)   (((file_p)((obj)->data.ptr))->flags)
#define OPEN(obj)    (((obj) != File_OBJ) && STREAM(obj))

MODULE_DECLARE(Dir);
MODULE_DECLARE(File);
MODULE_DECLARE(TempDir);
MODULE_DECLARE(TempFile);
MODULE_DECLARE(TextFile);
MODULE_DECLARE(TextFileGen);
MODULE_DECLARE(TempTextFile);
MODULE_DECLARE(StdIn);
MODULE_DECLARE(StdOut);
MODULE_DECLARE(StdErr);

//*************************** set_file_obj_data *******************************
static void set_file_obj_data( isp ist, obj_p file_obj, obj_p filename, obj_p mode,
						  	   apr_file_t *fp, int bufsize, apr_int32_t flags) {
    apr_status_t aprerr;
	apr_pool_t *subpool = NULL;
	file_p filep;
	char* temp, *p2, *p1, *p0, *fullpath;
	obj_p new_obj;

	if (fp) 
		subpool = apr_file_pool_get(fp);
	else {
		aprerr = apr_pool_create(&subpool, get_pr_head_pool());
		IF_APR_ERR("out of memory in set_file_obj_data()") return;
	}
	temp = pr2c_strptr(ist, filename);
	if (strcmp(temp, "stdIn") && strcmp(temp, "stdOut") && strcmp(temp, "stdErr")) {
		aprerr = apr_filepath_merge( &fullpath, NULL, temp, 
									APR_FILEPATH_TRUENAME, subpool );
		IF_APR_ERR("error in filepath_merge") return;
	} else 
		fullpath = temp;
#ifdef WIN32
	{
		char *p;
		for (p=fullpath; *p; p++)
			*p = tolower(*p);
	}
#endif
	set_attr(ist, file_obj, sym(ist, "path"), NEW_STRING(fullpath));

	p0 = fullpath;
	p2 = p0 + strlen(p0); 
	if (*(p2-1) == '/') p2--;
	for (p1=p2-1; p1 >= p0 && *p1 != '/'; p1--);
	if (*p1 != '/') new_obj = NEW_STRING("");
	else            new_obj = NEW_STRINGN(p1+1, p2-p1-1);
	set_attr(ist, file_obj, sym(ist, "name"), new_obj);

	if (mode)
		set_attr(ist, file_obj, sym(ist, "mode"), mode);

	filep = pr_malloc(sizeof(file_t));

	if (bufsize < MIN_BUFSIZE)
		bufsize = MIN_BUFSIZE;

	filep->bufsize = bufsize;
	filep->stream  = fp;
	filep->pool = subpool;
	filep->flags = flags;
	file_obj->data.ptr  = filep;
	file_obj->rd_access = ACC_USER1;
	file_obj->wr_access = ACC_USER1;
}

//*************************** new_file_obj ************************************
static obj_p new_file_obj( isp ist,  obj_p filename, obj_p mode, apr_file_t *fp, 
						   int bufsize,  apr_int32_t flags ) {
	obj_p file_obj;
	file_obj = NEW_OBJ(File_OBJ);
	SET_TYPE_IF_EXC(File_OBJ, file_obj, DATA_TYPE_DATAPTR) return NULL;
	set_file_obj_data(ist, file_obj, filename, mode, fp, bufsize, flags);
	return file_obj;
}

//*************************** new_dir_obj *************************************
static obj_p new_dir_obj(isp ist, char* path) {
	obj_p dir_obj = NEW_OBJ(Dir_OBJ);
	char* temp = pr_malloc(strlen(path)+2);
	strcpy(temp, path);
	if (temp[strlen(temp)-1] != '/') strcat(temp, "/");
	set_file_obj_data(ist, dir_obj, NEW_STRING(temp), NULL, NULL, 0, APR_READ);
	SET_TYPE_IF_EXC(Dir_OBJ, dir_obj, DATA_TYPE_DATAPTR) return NULL;
	set_immutable(dir_obj);
	pr_free(temp);
	return dir_obj;
}

#define DLIST_COUNT			1
#define DLIST_SIZE			2
#define DLIST_FILENAMES		4
#define DLIST_COPY			8
#define DLIST_DELETE		16

obj_p dir_list( isp ist, obj_p self, int type, int dlist_action, 
			    obj_p pattern, obj_p dest_dir, int uperms ) {
	apr_finfo_t finfo;
	apr_dir_t *dir;
    apr_status_t aprerr; 
	const char *dirpath, *destpath = NULL;
	char *filepath, *filepath2;
	file_p filep;
	obj_p uperms_obj = NULL, res = OBJ(NONE);
	i64_t count = 0;
	filep = self->data.ptr;
	dirpath = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	if (dest_dir) {
		destpath = pr2c_strptr(ist, get_attr(ist, dest_dir, sym(ist, "path")));
		uperms_obj = NEW_INT(uperms);
	}
	if (dlist_action == DLIST_FILENAMES) res = new_list_obj(ist, 10);
	aprerr = apr_dir_open(&dir, dirpath, filep->pool);
	IF_APR_ERR("opening directory") return NULL;
	while (TRUE) {
		aprerr = apr_dir_read(&finfo, APR_FINFO_DIRENT, dir);
		if (aprerr == 720018) break;
		IF_APR_ERR("reading directory") return NULL;
		if (type == -1 || finfo.filetype == type) {
			if (finfo.filetype == APR_REG || (strcmp(finfo.name, ".") && strcmp(finfo.name, ".."))) {
				if (dlist_action & (DLIST_SIZE|DLIST_FILENAMES|DLIST_COPY|DLIST_DELETE)) {
					aprerr = apr_filepath_merge( &filepath, dirpath, finfo.name, 
												 APR_FILEPATH_TRUENAME, filep->pool );
					IF_APR_ERR("Unable to calculate filepath") return NULL;
				}
				switch (dlist_action) {
				case DLIST_COUNT: 
					count++;  
					break;
				case DLIST_SIZE:
					apr_stat(&finfo, filepath, APR_FINFO_SIZE, filep->pool);
					count += finfo.size;
					break;
				case DLIST_FILENAMES:
					list_append( ist, res, ( type == APR_REG ?
								 new_file_obj(ist, NEW_STRING(filepath), NULL, NULL, 0, 0) : 
								 new_dir_obj (ist, filepath) ) );
					break;
				case DLIST_COPY:
					aprerr = apr_filepath_merge( &filepath2, destpath, finfo.name, 
												APR_FILEPATH_TRUENAME, filep->pool );
					IF_APR_ERR("Unable to calculate filepath(2)") return NULL;
					if (finfo.filetype == APR_REG) {
						aprerr = apr_file_copy(filepath, filepath2, uperms, filep->pool );
						IF_APR_ERR("copying file") return NULL;
					} else {
						obj_p new_src_dir, new_dst_dir;
						new_src_dir = new_dir_obj(ist, filepath);  if_exc_return NULL;
						new_dst_dir = new_dir_obj(ist, filepath2); if_exc_return NULL;
						call_func1(ist, new_dst_dir, sym(ist, "make"), uperms_obj); if_exc_return NULL;
						call_func2(ist, new_src_dir, sym(ist, "copy"), new_dst_dir, uperms_obj); if_exc_return NULL;
						del_unlock(new_src_dir);
						del_unlock(new_dst_dir);
					}
                    break;
				case DLIST_DELETE:
					if (finfo.filetype == APR_REG) {
						aprerr = apr_file_remove(filepath, filep->pool );
						IF_APR_ERR("deleting file") return NULL;
					} else {
						obj_p new_dir = new_dir_obj(ist, filepath);   if_exc_return NULL;
						call_func0(ist, new_dir, sym(ist, "delete")); if_exc_return NULL;
						del_unlock(new_dir);
					}
					break;
				}
			}
		} else if (finfo.filetype != (type == APR_REG ? APR_DIR : APR_REG)) {
			raise_exception( ist, OBJ(IO_EXC), "invalid file type (%d)", finfo.filetype);
			return NULL;
		}
	}
	apr_dir_close(dir);
	if (dlist_action & (DLIST_COUNT|DLIST_SIZE)) res = NEW_INT(count);
	return res;
}

#define addch3(ch)  buf[buf_idx++].b0 = (u8_t)(ch)

//*************************** MODULE Dir **************************************

MODULE_START(Dir)
{
	Dir_OBJ = NEW_OBJ(NULL);
	set_obj_doc(Dir_OBJ, "Directory object prototype");

	MODULE_ADD_TO_BASE(Dir);
	Dir_OBJ->unclonable = TRUE;
}

DEF(Dir, init_, FPARM1(path, NEW_STRING("."))) {
	char *path, *temp;
	BIN_EMPTY_CHK();
	STRING_PARAM(1, path);
	read_unlock(ist, self);
	temp = pr_malloc(strlen(path)+2);
	strcpy(temp, path);
	if (temp[strlen(temp)-1] != '/') strcat(temp, "/");
	set_file_obj_data(ist, self, NEW_STRING(temp), NULL, NULL, 0, 0);
	SET_TYPE_IF_EXC(Dir_OBJ, self, DATA_TYPE_DATAPTR) {
		read_lock(ist, self);  return NULL;
	}
	read_lock(ist, self);
	set_immutable(self);
	pr_free(temp);
	return OBJ(NONE);
}


DEF(Dir, str_, NULL) {
	char buf[300], *path;
	BIN_STR_CHK(Dir);
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	apr_snprintf(buf, sizeof(buf), "<Dir:'%s'>", path);
	pr_free(path);
	return NEW_STRING(buf);
}

DEF(Dir, exists_QUES, NULL) {
	apr_finfo_t finfo;
	char* path;
	file_p filep;
	BIN_CONTENT_CHK(Dir);
	filep = self->data.ptr;
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	apr_stat(&finfo, path, APR_FINFO_MIN, filep->pool);
	if (finfo.filetype == APR_DIR)  return OBJ(PR_TRUE);
	else                            return OBJ(PR_FALSE);
}

DEF(Dir, make, FPARM1(uperms, NEW_INT(APR_UREAD|APR_UWRITE|APR_UEXECUTE))) {
    apr_status_t aprerr; 
	char* path;
	i32_t uperms;
	file_p filep;
	BIN_CONTENT_CHK(Dir);
	INT_32_PARAM(1, uperms);
	filep = self->data.ptr;
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	aprerr = apr_dir_make_recursive(path, uperms, filep->pool);
	IF_APR_ERR("Unable to make directory") return NULL;
	return self;
}

DEF(Dir, dir, NULL) {
    apr_status_t aprerr;
	const char *rootpath, *filepath;
	char *temp, *tempp, *respath;
	file_p filep;
	BIN_CONTENT_CHK(Dir);
	filep = self->data.ptr;
	filepath = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	tempp = temp = pr_malloc(strlen(filepath)+1);
	strcpy(temp, filepath);
	aprerr = apr_filepath_root(&rootpath, (const char **)&temp, APR_FILEPATH_TRUENAME, filep->pool);
	IF_APR_ERR("calculating root path") return NULL;
	if (!(*temp)) {
		raise_exception( ist, OBJ(IO_EXC), "directory is root and has no parent");
		return NULL;
	}
	pr_free(tempp);
	aprerr = apr_filepath_merge(&respath, filepath, "..", APR_FILEPATH_TRUENAME, filep->pool);
	IF_APR_ERR("Unable to calculate parent of directory") return NULL;
	return new_dir_obj(ist, respath);
}

DEF(Dir, isRoot_QUES, NULL) {
    apr_status_t aprerr; 
	const char *rootpath, *filepath;
	file_p filep;
	BIN_CONTENT_CHK(Dir);
	filep = self->data.ptr;
	filepath = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	aprerr = apr_filepath_root(&rootpath, &filepath, APR_FILEPATH_TRUENAME, filep->pool);
	IF_APR_ERR("calculating root path") return NULL;
	if (*filepath) return OBJ(PR_FALSE);
	else		   return OBJ(PR_TRUE);
}

DEF(Dir, files, NULL) {
	BIN_CONTENT_CHK(Dir);
	return dir_list(ist, self, APR_REG, DLIST_FILENAMES, NULL, NULL, 0);
}

DEF(Dir, numFiles, NULL) {
	BIN_CONTENT_CHK(Dir);
	return dir_list(ist, self, APR_REG, DLIST_COUNT, NULL, NULL, 0);
}

DEF(Dir, dirs, NULL) {
	BIN_CONTENT_CHK(Dir);
	return dir_list(ist, self, APR_DIR, DLIST_FILENAMES, NULL, NULL, 0);
}

DEF(Dir, numDirs, NULL) {
	BIN_CONTENT_CHK(Dir);
	return dir_list(ist, self, APR_DIR, DLIST_COUNT, NULL, NULL, 0);
}

DEF(Dir, len_, NULL) {
	BIN_CONTENT_CHK(Dir);
	return dir_list(ist, self, -1, DLIST_COUNT, NULL, NULL, 0);
}

DEF(Dir, size, NULL) {
	BIN_CONTENT_CHK(Dir);
	return dir_list(ist, self, APR_REG, DLIST_SIZE, NULL, NULL, 0);
}

DEF(Dir, rename, FPARM1(name, OBJ(NODEF))) {
	apr_status_t aprerr;
	char *name, *dirpath, *p;
	file_p filep;
	BIN_CONTENT_CHK(Dir);
	STRING_PARAM(1, name);
	filep = self->data.ptr;
	dirpath = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	for (p=name; *p; p++) 
		if (*p == '/' || *p == '\\') {
			raise_exception( ist, OBJ(VALUE_EXC), "slash chars not allowed in dir name: \"%s\"", name);
			return NULL;
		}
	aprerr = apr_file_rename(dirpath, name, filep->pool);
	IF_APR_ERR("renaming a directory") return NULL;
	return new_dir_obj(ist, name);
}

DEF(Dir, move, FPARM1(dir, OBJ(NODEF))) {
	apr_status_t aprerr;
	char *dirpath1, *dirpath2;
	file_p filep;
	BIN_CONTENT_CHK(Dir);
	CHECK_TYPE_EXC(parms[1], Dir_OBJ, Directory);
	filep = self->data.ptr;
	dirpath1 = pr2c_strptr(ist, get_attr(ist,     self, sym(ist, "path")));
	dirpath2 = pr2c_strptr(ist, get_attr(ist, parms[1], sym(ist, "path")));
	aprerr = apr_file_rename(dirpath1, dirpath2, filep->pool);
	IF_APR_ERR("moving a directory") return NULL;
	return parms[1];
}

DEF(Dir, copy, FPARM2(dir, OBJ(NODEF), uperms, NEW_INT(APR_UREAD|APR_UWRITE|APR_UEXECUTE))) {
	apr_status_t aprerr;
	i32_t uperms;
	char* path;
	file_p filep;
	BIN_CONTENT_CHK(Dir);
	CHECK_TYPE_EXC(parms[1], Dir_OBJ, Directory);
	INT_32_PARAM(2, uperms);
	filep = self->data.ptr;
	path = pr2c_strptr(ist, get_attr(ist, parms[1], sym(ist, "path")));
	aprerr = apr_dir_make_recursive(path, uperms, filep->pool);
	IF_APR_ERR("Unable to make directory") return NULL;
	dir_list(ist, self, -1, DLIST_COPY, NULL, parms[1], uperms);
	return parms[1];
}

DEF(Dir, delete, NULL) {
	apr_status_t aprerr;
	char* path;
	file_p filep;
	BIN_CONTENT_CHK(Dir);
	filep = self->data.ptr;
	dir_list(ist, self, -1, DLIST_DELETE, NULL, NULL, 0);
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	aprerr = apr_dir_remove(path, filep->pool );
	IF_APR_ERR("deleting directory") return NULL;
	return self;
}

DEF(Dir, eq__QUES, FORM_RPARAM) {
	obj_p self_path, other_path;
	BIN_CONTENT_CHK(Dir);
	CHECK_TYPE_EXC(parms[1], Dir_OBJ, Directory);
	self_path  = get_attr(ist,     self, sym(ist, "path"));
	other_path = get_attr(ist, parms[1], sym(ist, "path"));
	return call_func1(ist, self_path, SYM(EQ__QUES), other_path);
}

DEF(Dir, setUPerms, FPARM1(uperms, NEW_INT(APR_UREAD|APR_UWRITE|APR_UEXECUTE))) {
	apr_status_t aprerr;
	char* path;
	int uperms;
	BIN_CONTENT_CHK(Dir);
	INT_32_PARAM(1, uperms);
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	aprerr = apr_file_perms_set(path, uperms);
	IF_APR_ERR("setting unix permissions") return NULL;
	return self;
}

DEF(Dir, setAttrs, FPARM3(readOnly, OBJ(NONE), executable, OBJ(NONE), hidden, OBJ(NONE))) {
	apr_status_t aprerr;
	char* path;
	int attrs = 0, attrs_mask = 0;
	file_p filep;
	BIN_CONTENT_CHK(Dir);
	filep = self->data.ptr;
	if (parms[1] == OBJ(PR_TRUE)) { 
		attrs |= APR_FILE_ATTR_READONLY;   attrs_mask |= APR_FILE_ATTR_READONLY;   }
	if (parms[3] == OBJ(PR_TRUE)) { 
		attrs |= APR_FILE_ATTR_EXECUTABLE; attrs_mask |= APR_FILE_ATTR_EXECUTABLE; }
	if (parms[5] == OBJ(PR_TRUE)) {
		attrs |= APR_FILE_ATTR_HIDDEN;     attrs_mask |= APR_FILE_ATTR_HIDDEN;     }
	if (parms[1] == OBJ(PR_FALSE)) attrs_mask |= APR_FILE_ATTR_READONLY; 
	if (parms[3] == OBJ(PR_FALSE)) attrs_mask |= APR_FILE_ATTR_EXECUTABLE;
	if (parms[5] == OBJ(PR_FALSE)) attrs_mask |= APR_FILE_ATTR_HIDDEN;
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	aprerr = apr_file_attrs_set(path, attrs, attrs_mask, filep->pool );
	IF_APR_ERR("setting directory attributes") return NULL;
	return self;
}

DEF(Dir, setModTime, FPARM1(time, OBJ(NONE))) {
	apr_status_t aprerr;
	char* path;
	obj_p time_obj;
	file_p filep;
	BIN_CONTENT_CHK(Dir);
	filep = self->data.ptr;
	time_obj = parms[1];
	if (time_obj == OBJ(NONE)) time_obj = new_time_obj(ist, apr_time_now());
	CHECK_TYPE_EXC(time_obj, OBJ(DATETIME_PROTO), time);
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	path[strlen(path)-1] = 0;
	aprerr = apr_file_mtime_set(path, time_obj->data.i64, filep->pool);
	IF_APR_ERR("setting directory modification time") return NULL;
	return self;
}

DEF(Dir, modTime, NULL) {
	apr_finfo_t info;
	char* path;
	file_p filep;
	BIN_CONTENT_CHK(Dir);
	filep = self->data.ptr;
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	apr_stat(&info, path, APR_FINFO_MTIME, filep->pool);
	if ((info.valid & APR_FINFO_MTIME) == 0) {
		raise_exception(ist, OBJ(IO_EXC), "unable to get directory modification time");
		return NULL;
	}
	return new_time_obj(ist, info.mtime);
}


DEF(Dir, objList_, FORM_RPARAM) {
	return parms[1];
}

//*************************** MODULE TempDir **************************************
#define TEMP_DIR_PREFIX		"prothon-temp-dir-"
#define	TEMP_DIR_PREFIX_LEN	17
#define RAND_STR_LEN		16
#define TEMP_DIR_NAME_LEN	(TEMP_DIR_PREFIX_LEN+RAND_STR_LEN)

void create_prtempdir(isp ist, obj_p obj, char* temp_path) {
	apr_status_t aprerr;
	char *full_temp_path, buf[TEMP_DIR_NAME_LEN+1];
	apr_pool_t *subpool;
	int i;
	aprerr = apr_pool_create(&subpool, get_pr_head_pool());
	IF_APR_ERR("getting memory for TempFile") return;
	strcpy(buf, TEMP_DIR_PREFIX);
	buf[TEMP_DIR_NAME_LEN] = 0;
	while (TRUE) {
		aprerr = apr_generate_random_bytes(buf+TEMP_DIR_PREFIX_LEN, RAND_STR_LEN);
		IF_APR_ERR("generating random bytes") return;
		for (i=TEMP_DIR_PREFIX_LEN; i < TEMP_DIR_NAME_LEN; i++) 
			buf[i] = 'a' + (buf[i] & 0x0f);
		aprerr = apr_filepath_merge( &full_temp_path, temp_path, buf, 
									 APR_FILEPATH_TRUENAME, subpool );
		IF_APR_ERR("error in filepath_merge") return;
		aprerr = apr_dir_make_recursive(full_temp_path, 0x600, subpool);
		if (aprerr == APR_SUCCESS) break;
	}
	set_file_obj_data(ist, obj, NEW_STRING(full_temp_path), NULL, NULL, 0, 0);
	SET_TYPE_IF_EXC(Dir_OBJ, obj, DATA_TYPE_DATAPTR) return;
}

void check_TempDir_proto(isp ist) {
	apr_status_t aprerr;
	apr_pool_t *subpool;
	char* temp_dir;
	int i;
	if (TempDir_OBJ->data.ptr) return;
	aprerr = apr_pool_create(&subpool, get_pr_head_pool());
	IF_APR_ERR("getting memory for TempFile") return;
	aprerr = apr_temp_dir_get((const char **)&temp_dir, subpool);
	IF_APR_ERR("getting temp dir") return;
	su(i);
	create_prtempdir(ist, TempDir_OBJ, temp_dir);
	un_su(i);
}

MODULE_START(TempDir) {
	TempDir_OBJ = NEW_OBJ(Dir_OBJ);
	set_obj_doc(TempDir_OBJ, "Temporary Directory object prototype");

	MODULE_ADD_TO_BASE(TempDir);
	TempDir_OBJ->unclonable = TRUE;
}

DEF(TempDir, init_, NULL) {
	char *pr_temp_dir;
	BIN_EMPTY_CHK();
	read_unlock(ist, TempDir_OBJ);
	check_TempDir_proto(ist);
	read_lock(ist, TempDir_OBJ);
	pr_temp_dir = pr2c_strptr(ist, get_attr(ist, TempDir_OBJ, sym(ist, "path")));
	read_unlock(ist, self);
	create_prtempdir(ist, self, pr_temp_dir);
	read_lock(ist, self);
	return OBJ(NONE);
}

DEF(TempDir, str_, NULL) {
	char buf[300], *path;
	BIN_STR_CHK(TempDir);
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	apr_snprintf(buf, sizeof(buf), "<TempDir:'%s'>", path);
	pr_free(path);
	return NEW_STRING(buf);
}

DEF(TempDir, final_, NULL) {
	if (self->data.ptr)
		call_func0(ist, self, sym(ist, "delete"));
	return OBJ(NONE);
}


//*************************** MODULE File *************************************

MODULE_START(File)
{
	File_OBJ = NEW_OBJ(NULL);
	set_obj_doc(File_OBJ, "File object prototype");

	MODULE_ADD_TO_BASE(File);
	File_OBJ->unclonable = TRUE;
}

DEF(File, init_, FPARM3( path,    OBJ(NODEF), 
	                     mode,    NEW_STRING(""),
                         bufsize, NEW_INT(-1)   ) ) {
	apr_file_t *f;
	apr_status_t aprerr;
	apr_pool_t *subpool;
	apr_int32_t flags = 0;
	char err_buf[1024];
	char *path, *mode;
	int bufsize;
	
	BIN_EMPTY_CHK();
	STRING_PARAM(1, path);
	STRING_PARAM(2, mode);
	INT_32_PARAM(3, bufsize);

	if (strlen(mode) == 0) {
		read_unlock(ist, self);
		SET_TYPE_IF_EXC(File_OBJ, self, DATA_TYPE_DATAPTR) return NULL;
		set_file_obj_data(ist, self, parms[1], NULL, NULL, 0, 0);
		read_lock(ist, self);
		self->unclonable = TRUE;
		return OBJ(NONE);
	}

	if ((mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a') ||
	    (mode[1] != '+' && mode[1] != '\0')) {
		raise_exception(ist, OBJ(TYPE_EXC), "File mode `%s' is not valid", mode);
		return NULL;
	}

	if (mode[0] == 'r') {
		flags = APR_READ | APR_BINARY;
		if (mode[1] == '+')
			flags |= APR_WRITE;
	} else if (mode[0] == 'w') {
		flags = APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY;
		if (mode[1] == '+')
			flags |= APR_READ;
	} else if (mode[0] == 'a') {
		flags = APR_WRITE | APR_CREATE | APR_APPEND | APR_BINARY;
		if (mode[1] == '+')
			flags |= APR_READ;
	}

	aprerr = apr_pool_create(&subpool, get_pr_head_pool());
	if (aprerr != APR_SUCCESS) {
		raise_exception(ist, OBJ(OUTOFMEMORY_EXC), "Out of memory opening files");
		return NULL;
	}

	aprerr = apr_file_open(&f, path, flags, APR_OS_DEFAULT, subpool);
	if (aprerr != APR_SUCCESS) {
		raise_exception(ist, OBJ(IO_EXC), "Unable to open path %s: %s",
				path, apr_strerror(aprerr, err_buf, sizeof(err_buf)));
		apr_pool_destroy(subpool);
		return NULL;
	}
	read_unlock(ist, self);
	SET_TYPE_IF_EXC(File_OBJ, self, DATA_TYPE_DATAPTR) return NULL;
	set_file_obj_data(ist, self, parms[1], parms[3], f, bufsize, flags);
	read_lock(ist, self);
	self->unclonable = TRUE;
	return OBJ(NONE);
}

DEF(File, str_, NULL) {
	char buf[300], *path;
	BIN_STR_CHK(File);
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	apr_snprintf(buf, sizeof(buf), "<File:'%s'>", path);
	pr_free(path);
	return NEW_STRING(buf);
}

DEF(File, exists_QUES, NULL) {
	apr_finfo_t finfo;
	char *path;
	file_p filep;
	BIN_CONTENT_CHK(File);
	filep = self->data.ptr;
	if (OPEN(self)) return OBJ(PR_TRUE);
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	apr_stat(&finfo, path, APR_FINFO_MIN, filep->pool);
	if (finfo.filetype == APR_REG)  return OBJ(PR_TRUE);
	else                            return OBJ(PR_FALSE);
	return OBJ(NONE);
}

DEF(File, rename, FPARM1(name, OBJ(NODEF))) {
	apr_status_t aprerr;
	char *name, *path, *p;
	file_p filep;
	BIN_CONTENT_CHK(File);
	STRING_PARAM(1, name);
	if (OPEN(self)) {
		raise_exception( ist, OBJ(IO_EXC), "attempt to rename an open file");
		return NULL;
	}
	for (p=name; *p; p++) 
		if (*p == '/' || *p == '\\') {
			raise_exception( ist, OBJ(VALUE_EXC), "slash chars not allowed in file name: \"%s\"", name);
			return NULL;
		}
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	filep = self->data.ptr;
	aprerr = apr_file_rename(path, name, filep->pool);
	IF_APR_ERR("renaming a file") return NULL;
	return new_file_obj(ist, NEW_STRING(name), NULL, NULL, 0, 0);
}

DEF(File, move, FPARM1(dir, OBJ(NODEF))) {
	apr_status_t aprerr;
	obj_p res;
	char *srcfilepath, *filename, *dstpath, *dstfilepath;
	file_p filep;
	BIN_CONTENT_CHK(File);
	CHECK_TYPE_EXC(parms[1], Dir_OBJ, Directory);
	if (OPEN(self)) {
		raise_exception( ist, OBJ(IO_EXC), "attempt to move an open file");
		return NULL;
	}
	filep = self->data.ptr;
	srcfilepath = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	filename    = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "name")));
	dstpath     = pr2c_strptr(ist, get_attr(ist, parms[1], sym(ist, "path")));
	dstfilepath = pr_malloc(strlen(dstpath) + strlen(filename) + 1);
	strcpy(dstfilepath, dstpath);  strcat(dstfilepath, filename);
	aprerr = apr_file_rename(srcfilepath, dstfilepath, filep->pool);
	IF_APR_ERR("moving a file") return NULL;
	res = new_file_obj(ist, NEW_STRING(dstfilepath), NULL, NULL, 0, 0);
	pr_free(dstfilepath);
	return res;
}

DEF(File, copy, FPARM2(file, OBJ(NODEF), uperms, NEW_INT(APR_UREAD|APR_UWRITE|APR_UEXECUTE))) {
	apr_status_t aprerr;
	i32_t uperms;
	char *path1, *path2;
	file_p filep;
	BIN_CONTENT_CHK(File);
	CHECK_TYPE_EXC(parms[1], File_OBJ, File);
	INT_32_PARAM(2, uperms);
	filep = self->data.ptr;
	path1 = pr2c_strptr(ist, get_attr(ist,     self, sym(ist, "path")));
	path2 = pr2c_strptr(ist, get_attr(ist, parms[1], sym(ist, "path")));
	aprerr = apr_file_copy(path1, path2, uperms, filep->pool);
	IF_APR_ERR("copying file") return NULL;
	return parms[1];
}

DEF(File, delete, NULL) {
	apr_status_t aprerr;
	char* path;
	file_p filep;
	BIN_CONTENT_CHK(File);
	if (OPEN(self)) {
		raise_exception( ist, OBJ(IO_EXC), "attempt to delete an open file");
		return NULL;
	}
	filep = self->data.ptr;
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	aprerr = apr_file_remove(path, filep->pool );
	IF_APR_ERR("deleting file") return NULL;
	return self;
}

DEF(File, eq__QUES, FORM_RPARAM) {
	obj_p self_path, other_path;
	BIN_CONTENT_CHK(File);
	CHECK_TYPE_EXC(parms[1], File_OBJ, Directory);
	self_path  = get_attr(ist,     self, sym(ist, "path"));
	other_path = get_attr(ist, parms[1], sym(ist, "path"));
	return call_func1(ist, self_path, SYM(EQ__QUES), other_path);
}

DEF(File, setUPerms, FPARM1(uperms, NEW_INT(APR_UREAD|APR_UWRITE|APR_UEXECUTE))) {
	apr_status_t aprerr;
	char* path;
	int uperms;
	BIN_CONTENT_CHK(File);
	INT_32_PARAM(1, uperms);
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	aprerr = apr_file_perms_set(path, uperms);
	IF_APR_ERR("setting unix permissions") return NULL;
	return self;
}

DEF(File, setAttrs, FPARM3(readOnly, OBJ(NONE), executable, OBJ(NONE), hidden, OBJ(NONE))) {
	apr_status_t aprerr;
	char* path;
	int attrs = 0, attrs_mask = 0;
	file_p filep;
	BIN_CONTENT_CHK(File);
	filep = self->data.ptr;
	if (parms[1] == OBJ(PR_TRUE)) { 
		attrs |= APR_FILE_ATTR_READONLY;   attrs_mask |= APR_FILE_ATTR_READONLY;   }
	if (parms[3] == OBJ(PR_TRUE)) { 
		attrs |= APR_FILE_ATTR_EXECUTABLE; attrs_mask |= APR_FILE_ATTR_EXECUTABLE; }
	if (parms[5] == OBJ(PR_TRUE)) {
		attrs |= APR_FILE_ATTR_HIDDEN;     attrs_mask |= APR_FILE_ATTR_HIDDEN;     }
	if (parms[1] == OBJ(PR_FALSE)) attrs_mask |= APR_FILE_ATTR_READONLY; 
	if (parms[3] == OBJ(PR_FALSE)) attrs_mask |= APR_FILE_ATTR_EXECUTABLE;
	if (parms[5] == OBJ(PR_FALSE)) attrs_mask |= APR_FILE_ATTR_HIDDEN;
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	aprerr = apr_file_attrs_set(path, attrs, attrs_mask, filep->pool );
	IF_APR_ERR("setting file attributes") return NULL;
	return self;
}

DEF(File, setModTime, FPARM1(time, OBJ(NONE))) {
	apr_status_t aprerr;
	char* path;
	obj_p time_obj;
	file_p filep;
	BIN_CONTENT_CHK(File);
	filep = self->data.ptr;
	time_obj = parms[1];
	if (time_obj == OBJ(NONE)) time_obj = new_time_obj(ist, apr_time_now());
	CHECK_TYPE_EXC(time_obj, OBJ(DATETIME_PROTO), time);
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	aprerr = apr_file_mtime_set(path, time_obj->data.i64, filep->pool);
	IF_APR_ERR("setting file modification time") return NULL;
	return self;
}

DEF(File, modTime, NULL) {
	apr_finfo_t info;
	char* path;
	file_p filep;
	BIN_CONTENT_CHK(File);
	filep = self->data.ptr;
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	apr_stat(&info, path, APR_FINFO_MTIME, filep->pool);
	if ((info.valid & APR_FINFO_MTIME) == 0) {
		raise_exception(ist, OBJ(IO_EXC), "unable to get file modification time");
		return NULL;
	}
	return NEW_TIME(info.mtime);
}

DEF(File, size, NULL) {
	apr_finfo_t info;
	char* path;
	file_p filep;
	BIN_CONTENT_CHK(File);
	filep = self->data.ptr;
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	apr_stat(&info, path, APR_FINFO_SIZE, filep->pool);
	if ((info.valid & APR_FINFO_SIZE) == 0) {
		raise_exception(ist, OBJ(IO_EXC), "unable to get file size");
		return NULL;
	}
	return NEW_INT(info.size);
}

DEF(File, open, FPARM2( mode,    NEW_STRING(""),
                        bufsize, NEW_INT(-1)   ) ) {
	apr_file_t *f;
	apr_status_t aprerr;
	apr_int32_t flags = 0;
	char err_buf[1024];
	char *path, *mode;
	int bufsize;	
	file_p filep;

	BIN_CONTENT_CHK(File);
	STRING_PARAM(1, mode);
	INT_32_PARAM(2, bufsize);
	if (OPEN(self)) {
		raise_exception(ist, OBJ(TYPE_EXC), "file is already open");
		return NULL;
	}
	filep = self->data.ptr;
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));

	if (strlen(mode) == 0) {
		read_unlock(ist, self);
		SET_TYPE_IF_EXC(File_OBJ, self, DATA_TYPE_DATAPTR) return NULL;
		set_file_obj_data(ist, self, NEW_STRING(path), NULL, NULL, 0, 0);
		read_lock(ist, self);
		self->unclonable = TRUE;
		return OBJ(NONE);
	}

	if ((mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a') ||
	    (mode[1] != '+' && mode[1] != '\0')) {
		raise_exception(ist, OBJ(TYPE_EXC), "File mode `%s' is not valid", mode);
		return NULL;
	}

	if (mode[0] == 'r') {
		flags = APR_READ | APR_BINARY;
		if (mode[1] == '+')
			flags |= APR_WRITE;
	} else if (mode[0] == 'w') {
		flags = APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY;
		if (mode[1] == '+')
			flags |= APR_READ;
	} else if (mode[0] == 'a') {
		flags = APR_WRITE | APR_CREATE | APR_APPEND | APR_BINARY;
		if (mode[1] == '+')
			flags |= APR_READ;
	}

	aprerr = apr_file_open(&f, path, flags, APR_OS_DEFAULT, filep->pool);
	if (aprerr != APR_SUCCESS) {
		raise_exception(ist, OBJ(IO_EXC), "Unable to open path %s: %s",
				path, apr_strerror(aprerr, err_buf, sizeof(err_buf)));
		return NULL;
	}

	if (bufsize < MIN_BUFSIZE)
		bufsize = MIN_BUFSIZE;

	filep->bufsize = bufsize;
	filep->stream  = f;
	filep->flags = flags;
	return OBJ(NONE);
}

DEF(File, close, NULL) {
	BIN_CONTENT_CHK(File);
	if (OPEN(self)) {
		apr_file_close(STREAM(self));
		STREAM(self) = NULL;
		apr_pool_destroy(POOL(self));
	}
	return OBJ(NONE);
}

DEF(File, open_QUES, NULL) { 
	BIN_CONTENT_CHK(File);
	if (!has_proto(ist, self, File_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "closed can only be called on a File object");
		return NULL;
	}
	if (OPEN(self)) return OBJ(PR_TRUE);
	else			return OBJ(PR_FALSE);
}

DEF(File, closed_QUES, NULL) { 
	BIN_CONTENT_CHK(File);
	if (!has_proto(ist, self, File_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "closed can only be called on a File object");
		return NULL;
	}
	if (OPEN(self)) return OBJ(PR_FALSE);
	else			return OBJ(PR_TRUE);
}

DEF(File, flush, NULL) { 
	BIN_CONTENT_CHK(File);
	if (!has_proto(ist, self, File_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "flush can only be called on a File object");
		return NULL;
	}
	if (!OPEN(self)) {
		raise_exception(ist, OBJ(IO_EXC), "flush called on closed file");
		return NULL;
	}
	if (apr_file_flush(STREAM(self)) == APR_SUCCESS) {
		raise_exception(ist, OBJ(IO_EXC), "flushing file");
		return NULL;
	}
	return OBJ(NONE);
}
DEF(File, read, FPARM1(size, NEW_INT(-1))) {
	int readall = FALSE;
	size_t size, total_read=0, num_read;
	obj_p byt_obj;
	byt_p obj_str;
	u8_t* buf;
	apr_status_t aprerr;
	char err_buf[1024];
	i64_t size_in;

	BIN_CONTENT_CHK(File);
	INT_64_PARAM(1, size_in);

	if (!has_proto(ist, self, File_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "read can only be called on a File object");
		return NULL;
	}
	if (!OPEN(self)) {
		raise_exception(ist, OBJ(IO_EXC), "read called on closed file");
		return NULL;
	}
	if (!(FLAGS(self) & APR_READ)) {
		raise_exception(ist, OBJ(IO_EXC), "read called on file opened without read perms");
		return NULL;
	}
	if (!size_in) return NEW_BYTES(NULL, 0);
	if (size_in < 0) {
		readall = TRUE;
		size = BUFSIZE(self);
	} else
		size = (size_t) size_in;
	byt_obj = NEW_OBJ(OBJ(BYTES_PROTO));
	if (size < IMM_DATA_LEN) {
		SET_TYPE_IF_EXC(OBJ(BYTES_PROTO), byt_obj, DATA_TYPE_IMMDATA) return NULL;
		buf = byt_obj->data.bytes;
	} else {
		obj_str = obj_malloc(ist, OBJ(BYTES_PROTO), byt_obj, sizeof(str_t)+size+1);
		buf = obj_str->byt;
	}
	num_read = size;
	aprerr = apr_file_read(STREAM(self), buf, &num_read);
	if (aprerr != APR_SUCCESS && aprerr != APR_EOF) {
		raise_exception(ist, OBJ(IO_EXC), "%s",
				apr_strerror(aprerr, err_buf, sizeof(err_buf)));
		return NULL;
	}
	total_read += num_read;
	while(aprerr != APR_EOF && (readall || total_read < size)) {
		size_t next_read = (readall ? size : size - total_read);
		size_t new_len   =  total_read + next_read;
		if (byt_obj->data_type == DATA_TYPE_IMMDATA && new_len >= IMM_DATA_LEN) {
			obj_str = obj_malloc(ist, OBJ(BYTES_PROTO), byt_obj, sizeof(str_t)+new_len+1);
			buf = obj_str->byt;
		} else {
			obj_str = obj_realloc(ist, byt_obj, sizeof(str_t)+new_len+1);
			buf = obj_str->byt;
		}
		num_read = next_read;
		aprerr = apr_file_read(STREAM(self), buf + total_read, &num_read);
		if (aprerr != APR_SUCCESS && aprerr != APR_EOF) {
			raise_exception(ist, OBJ(IO_EXC), "%s",
					apr_strerror(aprerr, err_buf, sizeof(err_buf)));
			return NULL;
		}
		total_read += num_read;
	}
	if (byt_obj->data_type == DATA_TYPE_IMMDATA)
		byt_obj->imm_data_len = (int) total_read;
	else {
		obj_str = obj_realloc(ist, byt_obj, sizeof(str_t)+total_read+1);
		obj_str->size = (data_size_t) (sizeof(str_t)+total_read);
		buf = obj_str->byt;
	}
	buf[total_read] = 0;
	byt_obj->immutable = TRUE;
	return byt_obj;
}

DEF(File, seek, FPARM2( pos, OBJ(NODEF),
                        how, NEW_INT(0) ) ) {
	apr_off_t pos;
	apr_seek_where_t origin;
	apr_status_t aprerr;
	char err_buf[1024];
	i64_t pos_in, how;

	BIN_CONTENT_CHK(File);
	INT_64_PARAM(1, pos_in);
	INT_64_PARAM(2, how);

	if (!has_proto(ist, self, File_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "seek can only be called on a File object");
		return NULL;
	}
	if (!OPEN(self)) {
		raise_exception(ist, OBJ(IO_EXC), "seek called on closed file");
		return NULL;
	}
	pos = (apr_off_t)pos_in;
	switch(how) {
		case 0: origin = APR_SET; break;
		case 1: origin = APR_CUR; break;
		case 2: origin = APR_END; break;
		default :
			raise_exception(ist, OBJ(TYPE_EXC), "file seek how parameter must be 0, 1, or 2");
			return NULL;
	}

	aprerr = apr_file_seek(STREAM(self), origin, &pos);
	if (aprerr != APR_SUCCESS) {
		raise_exception(ist, OBJ(IO_EXC), "%s",
				apr_strerror(aprerr, err_buf, sizeof(err_buf)));
		return NULL;
	}
	return OBJ(NONE);
}

DEF(File, truncate, FPARM1(size, OBJ(NODEF))) {
	apr_status_t aprerr;
	char err_buf[1024];
	apr_off_t size;
	i64_t size_in;

	BIN_CONTENT_CHK(File);
	INT_64_PARAM(1, size_in);

	if (!has_proto(ist, self, File_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "truncate can only be called on a File object");
		return NULL;
	}
	if (!OPEN(self)) {
		raise_exception(ist, OBJ(IO_EXC), "truncate called on closed file");
		return NULL;
	}

	/* XXX Need a way to check for upper bounds portably. This can be
	 * different on 64-bit systems, or systems which support LFS */
	if (size_in < 0) {
		raise_exception(ist, OBJ(TYPE_EXC), "file truncate size value out of range");
		return NULL;
	}
	size = (apr_off_t)(size_in);

	aprerr = apr_file_trunc(STREAM(self), size);
	if(aprerr != APR_SUCCESS) {
		raise_exception(ist, OBJ(IO_EXC), "%s",
				apr_strerror(aprerr, err_buf, sizeof(err_buf)));
		return NULL;
	}
	return OBJ(NONE);
}

DEF(File, write, FPARM1(bytes, OBJ(NODEF))) {
	apr_size_t size;
	apr_status_t aprerr;
	char err_buf[1024];
	obj_p bytes;
	u8_t* byts;

	BIN_CONTENT_CHK(File);
	BYTES__PARAM(1, bytes);
	byts = bytes_ptr(bytes);
	size = bytes_len(bytes);

	if (!has_proto(ist, self, File_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "write can only be called from a File object");
		return NULL;
	}
	if (!OPEN(self)) {
		raise_exception(ist, OBJ(IO_EXC), "write called on closed file");
		return NULL;
	}
	if (!(FLAGS(self) & APR_WRITE)) {
		raise_exception(ist, OBJ(IO_EXC), "write called on file opened without write perms");
		return NULL;
	}
	aprerr = apr_file_write(STREAM(self), byts, &size);
	if (aprerr != APR_SUCCESS) {
		raise_exception(ist, OBJ(IO_EXC), "%s",
				apr_strerror(aprerr, err_buf, sizeof(err_buf)));
		return NULL;
	}
	return parms[1];
}

#define __STDIN		0
#define __STDOUT	1
#define __STDERR	2


DEF(File, objList_, FORM_RPARAM) {
	return parms[1];
}


//*************************** MODULE TempFile *************************************

MODULE_START(TempFile)
{
	TempFile_OBJ = NEW_OBJ(File_OBJ);
	set_obj_doc(TempFile_OBJ, "TempFile object prototype");

	MODULE_ADD_TO_BASE(TempFile);
	TempFile_OBJ->unclonable = TRUE;
}

DEF(TempFile, init_, FPARM2( bufsize, NEW_INT(-1), delOnClose, OBJ(PR_FALSE))) {
	apr_status_t aprerr;
	apr_file_t *fp;
	apr_pool_t *subpool;
	char *temp_dir, *fullpath;
	int bufsize, delOnClose = FALSE, flags = 0;
	
	BIN_EMPTY_CHK();
	INT_32_PARAM(1, bufsize);
	if (parms[3] == OBJ(PR_TRUE)) delOnClose = TRUE;

	aprerr = apr_pool_create(&subpool, get_pr_head_pool());
	IF_APR_ERR("getting memory for TempFile") return NULL;

	check_TempDir_proto(ist);
	temp_dir = pr2c_strptr(ist, get_attr(ist, TempDir_OBJ, sym(ist, "path")));
	aprerr = apr_filepath_merge( &fullpath, temp_dir, "prothon-temp-file-XXXXXX", 
		                         APR_FILEPATH_TRUENAME, subpool );
	IF_APR_ERR("merging filepath") return NULL;

	if (delOnClose) flags = (APR_CREATE | APR_READ | APR_WRITE | APR_EXCL | APR_BINARY | APR_DELONCLOSE);
	else            flags = (APR_CREATE | APR_READ | APR_WRITE | APR_EXCL | APR_BINARY);
	aprerr = apr_file_mktemp(&fp, fullpath, flags, subpool);
	IF_APR_ERR("opening temporary file") return NULL;

	read_unlock(ist, self);
	SET_TYPE_IF_EXC(TempFile_OBJ, self, DATA_TYPE_DATAPTR) return NULL;
	set_file_obj_data(ist, self, NEW_STRING(fullpath), NULL, fp, bufsize, flags);
	read_lock(ist, self);
	self->unclonable = TRUE;
	return OBJ(NONE);
}

DEF(TempFile, str_, NULL) {
	char buf[300], *path;
	BIN_STR_CHK(TempFile);
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	apr_snprintf(buf, sizeof(buf), "<TempFile:'%s'>", path);
	pr_free(path);
	return NEW_STRING(buf);
}

//*************************** MODULE TextFile *************************************

MODULE_START(TextFile)
{
	TextFile_OBJ = NEW_OBJ(File_OBJ);
	set_obj_doc(TextFile_OBJ, "TextFile object prototype");

	MODULE_ADD_TO_BASE(TextFile);
	TextFile_OBJ->unclonable = TRUE;
}

DEF(TextFile, init_, FPARM5( path,		OBJ(NODEF), 
	                         mode,		NEW_STRING(""),
							 encoding,	NEW_STRING("ASCII"),
							 magic,		OBJ(PR_FALSE),
                             bufsize,	NEW_INT(-1)   ) ) {
	apr_file_t *f;
	apr_status_t aprerr;
	apr_pool_t *subpool;
	apr_int32_t flags = 0;
	char err_buf[1024];
	char *path, *mode, *encoding;
	int magic, bufsize;

	BIN_EMPTY_CHK();
	STRING_PARAM(1, path);
	STRING_PARAM(2, mode);
	STRING_PARAM(3, encoding);
	magic = (parms[7] == OBJ(PR_TRUE));
	INT_32_PARAM(5, bufsize);

	read_unlock(ist, self);
	set_attr(ist, self, sym(ist, "encoding"), NEW_STRING(encoding));
	if (magic) set_attr(ist, self, sym(ist, "magic"), OBJ(PR_TRUE));
	if (strlen(mode) == 0) {
		SET_TYPE_IF_EXC(TextFile_OBJ, self, DATA_TYPE_DATAPTR) return NULL;
		set_file_obj_data(ist, self, parms[1], NULL, NULL, 0, 0);
		read_lock(ist, self);
		self->unclonable = TRUE;
		return OBJ(NONE);
	}
	read_lock(ist, self);

	if ((mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a') ||
	    (mode[1] != '+' && mode[1] != '\0')) {
		raise_exception(ist, OBJ(TYPE_EXC), "TextFile mode `%s' is not valid", mode);
		return NULL;
	}

	if (mode[0] == 'r') {
		flags = APR_READ | APR_BINARY;
		if (mode[1] == '+')
			flags |= APR_WRITE;
	} else if (mode[0] == 'w') {
		flags = APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY;
		if (mode[1] == '+')
			flags |= APR_READ;
	} else if (mode[0] == 'a') {
		flags = APR_WRITE | APR_CREATE | APR_APPEND | APR_BINARY;
		if (mode[1] == '+')
			flags |= APR_READ;
	}

	aprerr = apr_pool_create(&subpool, get_pr_head_pool());
	if (aprerr != APR_SUCCESS) {
		raise_exception(ist, OBJ(OUTOFMEMORY_EXC), "Out of memory opening files");
		return NULL;
	}

	aprerr = apr_file_open(&f, path, flags, APR_OS_DEFAULT, subpool);
	if (aprerr != APR_SUCCESS) {
		raise_exception(ist, OBJ(IO_EXC), "Unable to open path %s: %s",
				path, apr_strerror(aprerr, err_buf, sizeof(err_buf)));
		apr_pool_destroy(subpool);
		return NULL;
	}
	read_unlock(ist, self);
	SET_TYPE_IF_EXC(TextFile_OBJ, self, DATA_TYPE_DATAPTR) return NULL;
	set_file_obj_data(ist, self, parms[1], parms[3], f, bufsize, flags);
	read_lock(ist, self);
	self->unclonable = TRUE;
	return OBJ(NONE);
}
DEF(TextFile, open, FPARM4( mode,		NEW_STRING("r"),
							encoding,	NEW_STRING("ASCII"),
							magic,		OBJ(PR_FALSE),
                            bufsize,	NEW_INT(-1)   ) ) {
	apr_file_t *f;
	apr_status_t aprerr;
	apr_int32_t flags = 0;
	char err_buf[1024];
	char *path, *mode, *encoding;
	int magic, bufsize;
	file_p filep;

	BIN_CONTENT_CHK(TextFile);
	STRING_PARAM(1, mode);
	STRING_PARAM(2, encoding);
	magic = (parms[5] == OBJ(PR_TRUE));
	INT_32_PARAM(4, bufsize);
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));

	filep = self->data.ptr;
	read_unlock(ist, self);
	set_attr(ist, self, sym(ist, "encoding"), NEW_STRING(encoding));
	if (magic) set_attr(ist, self, sym(ist, "magic"), OBJ(PR_TRUE));
	else       del_attr(ist, self, sym(ist, "magic"));

	if (strlen(mode) == 0) {
		set_file_obj_data(ist, self, NEW_STRING(path), NULL, NULL, 0, 0);
		read_lock(ist, self);
		self->unclonable = TRUE;
		return OBJ(NONE);
	}
	read_lock(ist, self);

	if ((mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a') ||
	    (mode[1] != '+' && mode[1] != '\0')) {
		raise_exception(ist, OBJ(TYPE_EXC), "TextFile mode `%s' is not valid", mode);
		return NULL;
	}

	if (mode[0] == 'r') {
		flags = APR_READ | APR_BINARY;
		if (mode[1] == '+')
			flags |= APR_WRITE;
	} else if (mode[0] == 'w') {
		flags = APR_WRITE | APR_CREATE | APR_TRUNCATE | APR_BINARY;
		if (mode[1] == '+')
			flags |= APR_READ;
	} else if (mode[0] == 'a') {
		flags = APR_WRITE | APR_CREATE | APR_APPEND | APR_BINARY;
		if (mode[1] == '+')
			flags |= APR_READ;
	}

	aprerr = apr_file_open(&f, path, flags, APR_OS_DEFAULT, filep->pool);
	if (aprerr != APR_SUCCESS) {
		raise_exception(ist, OBJ(IO_EXC), "Unable to open path %s: %s",
				path, apr_strerror(aprerr, err_buf, sizeof(err_buf)));
		return NULL;
	}
	read_unlock(ist, self);
	set_file_obj_data(ist, self, NEW_STRING(path), NEW_STRING(mode), f, bufsize, flags);
	read_lock(ist, self);
	self->unclonable = TRUE;
	return OBJ(NONE);
}

DEF(TextFile, str_, NULL) {
	char buf[300], *path;
	BIN_STR_CHK(TextFile);
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	apr_snprintf(buf, sizeof(buf), "<TextFile:'%s'>", path);
	pr_free(path);
	return NEW_STRING(buf);
}
DEF(TextFile, read, FPARM1(size, NEW_INT(-1))) {
	int readall = FALSE, latin = FALSE;
	data_size_t size, total_read=0, num_read;
	obj_p str_obj;
	u8_t* buf;
	apr_status_t aprerr;
	char *encoding, err_buf[1024];
	i64_t size_in;

	BIN_CONTENT_CHK(File);
	INT_64_PARAM(1, size_in);
	encoding = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "encoding")));
	if (!strcmp(encoding, "latin-1")) latin = TRUE;
	else if (strcmp(encoding, "ASCII")) {
		raise_exception(ist, OBJ(VALUE_EXC), "encoding not supported: \"%s\"", encoding);
		return NULL;
	}
	if (!has_proto(ist, self, File_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "read can only be called on a File object");
		return NULL;
	}
	if (!OPEN(self)) {
		raise_exception(ist, OBJ(IO_EXC), "read called on closed file");
		return NULL;
	}
	if (!(FLAGS(self) & APR_READ)) {
		raise_exception(ist, OBJ(IO_EXC), "read called on file opened without read perms");
		return NULL;
	}
	if (!size_in) return NEW_STRING("");
	if (size_in < 0) {
		readall = TRUE;
		size = BUFSIZE(self);
	} else
		size = (data_size_t) size_in;
	buf = pr_malloc(size);
	num_read = size;
	aprerr = apr_file_read(STREAM(self), buf, &num_read);
	if (aprerr != APR_SUCCESS && aprerr != APR_EOF) {
		raise_exception(ist, OBJ(IO_EXC), "%s",
				apr_strerror(aprerr, err_buf, sizeof(err_buf)));
		return NULL;
	}
	total_read += num_read;
	while(aprerr != APR_EOF && (readall || total_read < size)) {
		size_t next_read = (readall ? size : size - total_read);
		size_t new_len   =  total_read + next_read;
		buf = pr_realloc(buf, new_len);
		num_read = (data_size_t) next_read;
		aprerr = apr_file_read(STREAM(self), buf + total_read, &num_read);
		if (aprerr != APR_SUCCESS && aprerr != APR_EOF) {
			raise_exception(ist, OBJ(IO_EXC), "%s",
					apr_strerror(aprerr, err_buf, sizeof(err_buf)));
			return NULL;
		}
		total_read += num_read;
	}
	str_obj = NEW_OBJ(OBJ(STRING_PROTO));
	c2pr_bytcpy(ist, str_obj, buf, total_read, latin);
	pr_free(buf);
	str_obj->immutable = TRUE;
	return str_obj;
}
DEF(TextFile, readLine, FPARM1(size, NEW_INT(-1))) {
	int readall = FALSE, latin = FALSE;
	data_size_t size, total_read;
	obj_p str_obj;
	u8_t* buf;
	apr_status_t aprerr;
	char *encoding, err_buf[1024];
	i64_t  size_in;

	BIN_CONTENT_CHK(File);
	INT_64_PARAM(1, size_in);

	encoding = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "encoding")));
	if (!strcmp(encoding, "latin-1")) latin = TRUE;
	else if (strcmp(encoding, "ASCII")) {
		raise_exception(ist, OBJ(VALUE_EXC), "encoding not supported: \"%s\"", encoding);
		return NULL;
	}
	if (!has_proto(ist, self, File_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "readLine can only be called on a File object");
		return NULL;
	}
	if (!OPEN(self)) {
		raise_exception(ist, OBJ(IO_EXC), "readLine called on closed file");
		return NULL;
	}
	if (!(FLAGS(self) & APR_READ)) {
		raise_exception(ist, OBJ(IO_EXC), "readLine called on file opened without read perms");
		return NULL;
	}
	if (!size_in) return NEW_STRING("");
	if (size_in < 0) {
		readall = TRUE;
		size = BUFSIZE(self);
	} else
		size = (data_size_t) size_in;
	buf = pr_malloc(size+1);
	aprerr = apr_file_gets(buf, (int)(size + 1), STREAM(self));
	if (aprerr != APR_SUCCESS && aprerr != APR_EOF) {
		raise_exception(ist, OBJ(IO_EXC), "%s",
				apr_strerror(aprerr, err_buf, sizeof(err_buf)));
		return NULL;
	}
	total_read = (data_size_t) strlen(buf);
	while (aprerr != APR_EOF && buf[total_read-1] != '\n' && (readall || total_read < size)) {
		size_t next_read = (readall ? size : size - total_read);
		size_t new_len = total_read + next_read;
		buf = pr_realloc(buf, new_len+1);
		aprerr = apr_file_gets(buf + total_read, (int)(next_read + 1), STREAM(self));
		if (aprerr != APR_SUCCESS && aprerr != APR_EOF) {
			raise_exception(ist, OBJ(IO_EXC), "%s",
					apr_strerror(aprerr, err_buf, sizeof(err_buf)));
			return NULL;
		}
		total_read = (data_size_t) strlen(buf);
	}
	str_obj = NEW_OBJ(OBJ(STRING_PROTO));
	c2pr_bytcpy(ist, str_obj, buf, total_read, latin);
	pr_free(buf);
	str_obj->immutable = TRUE;
	return str_obj;
}

DEF(TextFile, readLines, FPARM1(size, NEW_INT(-1))) {
	int readall = FALSE, latin = FALSE;
	data_size_t size, total_read=0, num_read;
	obj_p str_obj, list_obj;
	u8_t* buf;
	apr_status_t aprerr;
	char *encoding, err_buf[1024];
	i64_t  size_in;

	BIN_CONTENT_CHK(File);
	INT_64_PARAM(1, size_in);

	encoding = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "encoding")));
	if (!strcmp(encoding, "latin-1")) latin = TRUE;
	else if (strcmp(encoding, "ASCII")) {
		raise_exception(ist, OBJ(VALUE_EXC), "encoding not supported: \"%s\"", encoding);
		return NULL;
	}
	if (!has_proto(ist, self, File_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "readLines can only be called on a File object");
		return NULL;
	}
	if (!OPEN(self)) {
		raise_exception(ist, OBJ(IO_EXC), "readLines called on closed file");
		return NULL;
	}
        if (!(FLAGS(self) & APR_READ)) {
                raise_exception(ist, OBJ(IO_EXC), "readLines called on file opened without read perms");
                return NULL;
        }
	if (!size_in) return NEW_STRING("");
	if (size_in < 0) {
		readall = TRUE;
		size = BUFSIZE(self);
	} else
		size = (data_size_t) size_in;
	list_obj = NEW_LIST(10);
	aprerr = 0;
	while (aprerr != APR_EOF && (readall || total_read < size)) {
		buf = pr_malloc(size+1);
		aprerr = apr_file_gets(buf, (int)(size - total_read + 1), STREAM(self));
		if (aprerr != APR_SUCCESS && aprerr != APR_EOF) {
			raise_exception(ist, OBJ(IO_EXC), "%s",
					apr_strerror(aprerr,
					err_buf, sizeof(err_buf)));
			return NULL;
		}
		total_read += num_read = (data_size_t) strlen(buf);
		while ( aprerr != APR_EOF && buf[num_read-1] != '\n' &&
				                   (readall || total_read < size) ) {
			size_t next_read = (readall ? size : size - total_read);
			size_t new_len = num_read + next_read;
			buf = pr_realloc(buf, new_len+1);
			aprerr = apr_file_gets(buf + num_read, (int)(next_read + 1), STREAM(self));
			if (aprerr != APR_SUCCESS && aprerr != APR_EOF) {
				raise_exception(ist, OBJ(IO_EXC), "%s",
						apr_strerror(aprerr,
						err_buf, sizeof(err_buf)));
				return NULL;
			}
			total_read += num_read = (data_size_t) strlen(buf);
		}
		str_obj = NEW_OBJ(OBJ(STRING_PROTO));
		c2pr_bytcpy(ist, str_obj, buf, num_read, latin);
		pr_free(buf);
		str_obj->immutable = TRUE;
		list_append(ist, list_obj, str_obj);
	}
	return list_obj;
}

DEF(TextFile, write, FPARM1(str, OBJ(NODEF))) {
	apr_size_t size;
	apr_status_t aprerr;
	char *encoding, err_buf[1024];
	u8_t* buf;
	int latin = FALSE;

	BIN_CONTENT_CHK(File);
	CHECK_TYPE_EXC(parms[1], OBJ(STRING_PROTO), String);

	encoding = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "encoding")));
	if (!strcmp(encoding, "latin-1")) latin = TRUE;
	else if (strcmp(encoding, "ASCII")) {
		raise_exception(ist, OBJ(VALUE_EXC), "encoding not supported: \"%s\"", encoding);
		return NULL;
	}
	if (!has_proto(ist, self, File_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "write can only be called from a File object");
		return NULL;
	}
	if (!OPEN(self)) {
		raise_exception(ist, OBJ(IO_EXC), "write called on closed file");
		return NULL;
	}
	if (!(FLAGS(self) & APR_WRITE)) {
		raise_exception(ist, OBJ(IO_EXC), "write called on file opened without write perms");
		return NULL;
	}
	buf = pr_malloc(size = pr_strlen(parms[1]));
	pr_str2bytes(ist, buf, parms[1], latin);

	aprerr = apr_file_write_full(STREAM(self), buf, size, NULL);
	if (aprerr != APR_SUCCESS) {
		raise_exception(ist, OBJ(IO_EXC), "%s",
				apr_strerror(aprerr, err_buf, sizeof(err_buf)));
		return NULL;
	}
	pr_free(buf);
	return parms[1];
}

DEF(TextFile, writeLines, FPARM1(lst, OBJ(NODEF))) {
	apr_size_t size;
	apr_status_t aprerr;
	char *encoding, err_buf[1024];
	u8_t* buf;
	int latin = FALSE;

	if (!has_proto(ist, parms[1], OBJ(SEQ_PROTO)) && !has_proto(ist, parms[1], OBJ(STRING_PROTO))) {
		raise_exception(ist, OBJ(TYPE_EXC), "file writeLines parameter must be a sequence or a string");
		return NULL;
	}
	if (!has_proto(ist, self, File_OBJ)) {
		raise_exception(ist, OBJ(TYPE_EXC), "writeLines can only be called on a File object");
		return NULL;
	}
	if (!OPEN(self)) {
		raise_exception(ist, OBJ(IO_EXC), "write called on closed file");
		return NULL;
	}
	if (!(FLAGS(self) & APR_WRITE)) {
		raise_exception(ist, OBJ(IO_EXC), "writeLines called on file opened without write perms");
		return NULL;
	}
	encoding = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "encoding")));
	if (!strcmp(encoding, "latin-1")) latin = TRUE;
	else if (strcmp(encoding, "ASCII")) {
		raise_exception(ist, OBJ(VALUE_EXC), "encoding not supported: \"%s\"", encoding);
		return NULL;
	}
	if (has_proto(ist, parms[1], OBJ(STRING_PROTO))) {
		buf = pr_malloc(size = pr_strlen(parms[1]));
		pr_str2bytes(ist, buf, parms[1], latin);
		aprerr = apr_file_write_full(STREAM(self), buf, size, NULL);
		if (aprerr != APR_SUCCESS) {
			raise_exception(ist, OBJ(IO_EXC), "%s",
					apr_strerror(aprerr, err_buf, sizeof(err_buf)));
			return NULL;
		}
		pr_free(buf);
	} else {
		int i, llen = (int) list_len(ist, parms[1]);
		for(i = 0; i < llen; i++) {
			obj_p item = list_item(ist, parms[1], i);
			if (!has_proto(ist, item, OBJ(STRING_PROTO))) {
				raise_exception(ist, OBJ(TYPE_EXC), "writeLines list items must be strings");
				return NULL;
			}
			buf = pr_malloc(size = pr_strlen(item));
			pr_str2bytes(ist, buf, item, latin);
			aprerr = apr_file_write_full(STREAM(self), buf, size, NULL);
			if (aprerr != APR_SUCCESS) {
				raise_exception(ist, OBJ(IO_EXC), "%s",
						apr_strerror(aprerr, err_buf, sizeof(err_buf)));
				return NULL;
			}
			pr_free(buf);
		}
	}
	return parms[1];
}

//*************************** MODULE TEMPTEXTFILE *************************************

MODULE_START(TempTextFile)
{
	TempTextFile_OBJ = NEW_OBJ(TextFile_OBJ);
	set_obj_doc(TempTextFile_OBJ, "TempTextFile object prototype");

	MODULE_ADD_TO_BASE(TempTextFile);
	TempTextFile_OBJ->unclonable = TRUE;
}

DEF( TempTextFile, init_, FPARM4( encoding,		NEW_STRING("ASCII"),
							      magic,		OBJ(PR_FALSE),
								  bufsize,		NEW_INT(-1), 
								  delOnClose,	OBJ(PR_FALSE) ) ) {
	apr_status_t aprerr;
	apr_file_t *fp;
	apr_pool_t *subpool;
	char *temp_dir, *fullpath, *encoding;
	int bufsize, delOnClose = FALSE, flags = 0, magic;
	
	BIN_EMPTY_CHK();
	STRING_PARAM(1, encoding);
	magic = (parms[3] == OBJ(PR_TRUE));
	INT_32_PARAM(3, bufsize);
	if (parms[7] == OBJ(PR_TRUE)) delOnClose = TRUE;

	read_unlock(ist, self);
	set_attr(ist, self, sym(ist, "encoding"), NEW_STRING(encoding));
	if (magic) set_attr(ist, self, sym(ist, "magic"), OBJ(PR_TRUE));
	read_lock(ist, self);

	aprerr = apr_pool_create(&subpool, get_pr_head_pool());
	IF_APR_ERR("getting memory for TempTextFile") return NULL;

	check_TempDir_proto(ist);
	temp_dir = pr2c_strptr(ist, get_attr(ist, TempDir_OBJ, sym(ist, "path")));
	aprerr = apr_filepath_merge( &fullpath, temp_dir, "prothon-temp-file-XXXXXX", 
		                         APR_FILEPATH_TRUENAME, subpool );
	IF_APR_ERR("merging filepath") return NULL;

	if (delOnClose) flags = (APR_CREATE | APR_READ | APR_WRITE | APR_EXCL | APR_BINARY | APR_DELONCLOSE);
	else            flags = (APR_CREATE | APR_READ | APR_WRITE | APR_EXCL | APR_BINARY);
	aprerr = apr_file_mktemp(&fp, fullpath, flags, subpool);
	IF_APR_ERR("opening temporary file") return NULL;

	read_unlock(ist, self);
	SET_TYPE_IF_EXC(TempTextFile_OBJ, self, DATA_TYPE_DATAPTR) return NULL;
	set_file_obj_data(ist, self, NEW_STRING(fullpath), NULL, fp, bufsize, flags);
	read_lock(ist, self);
	self->unclonable = TRUE;
	return OBJ(NONE);
}

DEF(TempTextFile, str_, NULL) {
	char buf[300], *path;
	BIN_STR_CHK(TempTextFile);
	path = pr2c_strptr(ist, get_attr(ist, self, sym(ist, "path")));
	apr_snprintf(buf, sizeof(buf), "<TempTextFile:'%s'>", path);
	pr_free(path);
	return NEW_STRING(buf);
}
//*************************** MODULE TEXTFILEGEN *************************************

DEF(TextFile, iter_, NULL) {
	obj_p new_obj = NEW_OBJ(TextFileGen_OBJ);
	BIN_CONTENT_CHK(File);
	if (!OPEN(self)) {
		raise_exception(ist, OBJ(IO_EXC), "iter_ called on closed file");
		return NULL;
	}
	SET_TYPE_IF_EXC(TextFileGen_OBJ, new_obj, DATA_TYPE_EXTPTR) return NULL;
	new_obj->data.ptr = self;
	return new_obj;
}

MODULE_START(TextFileGen) {
	TextFileGen_OBJ = NEW_OBJ(OBJ(ITER_PROTO));
	set_obj_doc(TextFileGen_OBJ, "TextFileGen object prototype");

	MODULE_ADD_TO_BASE(TextFileGen);
	TextFileGen_OBJ->unclonable = TRUE;
}

DEF(TextFileGen, next, NULL) {
	obj_p res, file_obj = self->data.ptr;
	if (!OPEN(file_obj)) {
		raise_exception(ist, OBJ(STOP_ITERATION_EXC), NULL);
		return NULL;
	}
	res = call_func0(ist, file_obj, sym(ist, "readLine"));
	if_exc_return NULL;
	if (pr_strlen(res) == 0) {
		del_unlock(res);
		call_func0(ist, file_obj, sym(ist, "close"));
		raise_exception(ist, OBJ(STOP_ITERATION_EXC), NULL);
		return NULL;
	}
	return res;
}

//*************************** StdIn StdOut StdErr *****************************

MODULE_START(StdIn)
{
	apr_status_t aprerr;
	apr_pool_t *subpool;
	apr_file_t *fp;

	/* Create base file objects for std{in,out,err} */
	aprerr = apr_pool_create(&subpool, get_pr_head_pool());
	if (aprerr != APR_SUCCESS) {
		raise_exception(ist, OBJ(IO_EXC), "out of memory");
		return;
	}
	StdIn_OBJ = NEW_OBJ(TextFile_OBJ);
	set_obj_doc(StdIn_OBJ, "stdIn unique object");
	aprerr = apr_file_open_stdin(&fp, subpool);
	IF_APR_ERR("opening stdin") return;
	SET_TYPE_IF_EXC(TextFile_OBJ, StdIn_OBJ, DATA_TYPE_DATAPTR) return;
	set_file_obj_data(ist, StdIn_OBJ, NEW_STRING("stdIn"), NEW_STRING("r"), 
						   fp, 0, APR_READ);
	set_attr(ist, StdIn_OBJ, sym(ist, "encoding"), NEW_STRING("ASCII"));
	StdIn_OBJ->unclonable = TRUE;

	StdOut_OBJ = NEW_OBJ(TextFile_OBJ);
	set_obj_doc(StdOut_OBJ, "stdOut unique object");
	aprerr = apr_file_open_stdout(&fp, subpool);
	IF_APR_ERR("opening stdout") return;
	SET_TYPE_IF_EXC(TextFile_OBJ, StdOut_OBJ, DATA_TYPE_DATAPTR) return;
	set_file_obj_data(ist, StdOut_OBJ, NEW_STRING("stdOut"), NEW_STRING("w"), 
						   fp, 0, APR_WRITE);
	set_attr(ist, StdOut_OBJ, sym(ist, "encoding"), NEW_STRING("ASCII"));
	StdOut_OBJ->unclonable = TRUE;

	StdErr_OBJ = NEW_OBJ(TextFile_OBJ);
	set_obj_doc(StdErr_OBJ, "stdErr unique object");
	aprerr = apr_file_open_stderr(&fp, subpool);
	IF_APR_ERR("opening stderr") return;
	SET_TYPE_IF_EXC(TextFile_OBJ, StdErr_OBJ, DATA_TYPE_DATAPTR) return;
	set_file_obj_data(ist, StdErr_OBJ, NEW_STRING("stdErr"), NEW_STRING("w"), 
						   fp, 0, APR_WRITE);
	set_attr(ist, StdErr_OBJ, sym(ist, "encoding"), NEW_STRING("ASCII"));
	StdErr_OBJ->unclonable = TRUE;

	set_attr(ist, OBJ(SYS), sym(ist, "stdIn"),  StdIn_OBJ);
	set_attr(ist, OBJ(SYS), sym(ist, "stdOut"), StdOut_OBJ);
	set_attr(ist, OBJ(SYS), sym(ist, "stdErr"), StdErr_OBJ);
}

DEF(StdIn,  call_, FORM_STAR3_PARAM) { return StdIn_OBJ;  }
DEF(StdOut, call_, FORM_STAR3_PARAM) { return StdOut_OBJ; }
DEF(StdErr, call_, FORM_STAR3_PARAM) { return StdErr_OBJ; }

//*************************** MAIN_MODULE_INIT ********************************

MAIN_MODULE_INIT(File) {

	MODULE_SUB_INIT(Dir);
	MODULE_ADD_SYM(Dir, init_);
	MODULE_ADD_SYM(Dir, str_);
	MODULE_ADD_SYM(Dir, exists_QUES);
	MODULE_ADD_SYM(Dir, make);
	MODULE_ADD_SYM(Dir, dir);
	MODULE_ADD_SYM(Dir, isRoot_QUES);
	MODULE_ADD_SYM(Dir, files);
	MODULE_ADD_SYM(Dir, dirs);
	MODULE_ADD_SYM(Dir, numFiles);
	MODULE_ADD_SYM(Dir, numDirs);
	MODULE_ADD_SYM(Dir, len_);
	MODULE_ADD_SYM(Dir, size);
	MODULE_ADD_SYM(Dir, rename);
	MODULE_ADD_SYM(Dir, move);
	MODULE_ADD_SYM(Dir, copy);
	MODULE_ADD_SYM(Dir, delete);
	MODULE_ADD_SYM(Dir, eq__QUES);
	MODULE_ADD_SYM(Dir, setUPerms);
	MODULE_ADD_SYM(Dir, setAttrs);
	MODULE_ADD_SYM(Dir, setModTime);
	MODULE_ADD_SYM(Dir, modTime);
	MODULE_ADD_SYM(Dir, objList_);

	MODULE_SUB_INIT(TempDir);
	MODULE_ADD_SYM(TempDir, init_);
	MODULE_ADD_SYM(TempDir, str_);
	MODULE_ADD_SYM(TempDir, final_);

	MODULE_SUB_INIT(File);
	MODULE_ADD_SYM(File, init_);
	MODULE_ADD_SYM(File, str_);
	MODULE_ADD_SYM(File, exists_QUES);
	MODULE_ADD_SYM(File, rename);
	MODULE_ADD_SYM(File, move);
	MODULE_ADD_SYM(File, copy);
	MODULE_ADD_SYM(File, delete);
	MODULE_ADD_SYM(File, eq__QUES);
	MODULE_ADD_SYM(File, setUPerms);
	MODULE_ADD_SYM(File, setAttrs);
	MODULE_ADD_SYM(File, setModTime);
	MODULE_ADD_SYM(File, modTime);
	MODULE_ADD_SYM(File, size);
	MODULE_ADD_SYM(File, read);
	MODULE_ADD_SYM(File, write);
	MODULE_ADD_SYM(File, flush);
	MODULE_ADD_SYM(File, seek);
	MODULE_ADD_SYM(File, truncate);
	MODULE_ADD_SYM(File, open);
	MODULE_ADD_SYM(File, close);
	MODULE_ADD_SYM(File, open_QUES);
	MODULE_ADD_SYM(File, closed_QUES);
	MODULE_ADD_SYM(File, objList_);

	MODULE_SUB_INIT(TempFile);
	MODULE_ADD_SYM(TempFile, init_);
	MODULE_ADD_SYM(TempFile, str_);

	MODULE_SUB_INIT(TextFile);
	MODULE_ADD_SYM(TextFile, init_);
	MODULE_ADD_SYM(TextFile, open);
	MODULE_ADD_SYM(TextFile, str_);
	MODULE_ADD_SYM(TextFile, read);
	MODULE_ADD_SYM(TextFile, readLine);
	MODULE_ADD_SYM(TextFile, readLines);
	MODULE_ADD_SYM(TextFile, write);
	MODULE_ADD_SYM(TextFile, writeLines);
	MODULE_ADD_SYM(TextFile, iter_);

	MODULE_SUB_INIT(TempTextFile);
	MODULE_ADD_SYM(TempTextFile, init_);
	MODULE_ADD_SYM(TempTextFile, str_);

	MODULE_SUB_INIT(TextFileGen);
	MODULE_ADD_SYM(TextFileGen, next);

	MODULE_SUB_INIT(StdIn);
	MODULE_ADD_SYM(StdIn,  call_);
	MODULE_ADD_SYM(StdOut, call_);
	MODULE_ADD_SYM(StdErr, call_);

	check_exceptions(ist);
}
