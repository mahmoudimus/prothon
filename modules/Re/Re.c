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
 * Applications; All Rights reserved" are retained in Prothon alone or
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


// re.c - regex using POSIX regex()

#include <stdio.h>
#include <string.h>
#include <prothon/prothon_dll.h>
#include <boost/regex.h>

#define MAX_GROUPS   99
#define PROTO_FLAGS  REG_EXTENDED

#define numgrps(re_obj) (((regex_t*)((re_obj)->data.ptr))->re_nsub)

static obj_p reException_OBJ; 
static obj_p i_flg, s_flg;

MODULE_DECLARE(re);
MODULE_DECLARE(reCompiled);
MODULE_DECLARE(reMatch);

MODULE_START(re)
{
	re_OBJ = NEW_OBJ(NULL);
	MODULE_SET_DOC(re, "module for regular expressions");
	set_obj_id(re_OBJ, *, re);
	MODULE_ADD_TO_BASE(re);

	reException_OBJ = NEW_OBJ(OBJ(EXCEPTION));
	MODULE_SET_DOC(reException, "re module error");
	MODULE_ADD_TO_OBJ(reException, re_OBJ, "reException");

	i_flg = NEW_INT(REG_ICASE);
	s_flg = NEW_INT(REG_NEWLINE);   // reverse meaning

	set_attr(ist, re_OBJ, sym(ist, "I"),          i_flg);
	set_attr(ist, re_OBJ, sym(ist, "IGNORECASE"), i_flg);
	set_attr(ist, re_OBJ, sym(ist, "S"),          s_flg);
	set_attr(ist, re_OBJ, sym(ist, "DOTALL"),     s_flg);
}

DEF(re, compile, FPARM2( pattern, NULL, 
			             flags,   OBJ(ZERO_INT) ) ) {
	obj_p re_obj;
	regex_t* e = pr_malloc(sizeof(regex_t));
	int flags_in, flags=0, err;
	char* pattern;

	STRING_PARAM(1, pattern);
	INT_32_PARAM(2, flags_in);

	flags = flags_in & REG_ICASE;
	if (!(flags_in & REG_NEWLINE))   flags |= REG_NEWLINE;
	if ((err = regcomp(e, pattern, PROTO_FLAGS|flags))) {
		size_t buf_size;
		char msg[1024];
		if ((buf_size = regerror(err, NULL, msg, 1024)) > 1024) {
			char* msg2 = pr_malloc(buf_size+1);
			regerror(err, NULL, msg2, buf_size);
			raise_exception(ist, reException_OBJ, "%s", msg2);
			pr_free(msg2);
		} else 
			raise_exception(ist, reException_OBJ, "%s", msg);
		pr_free(e);
		return NULL;
	}
	if (e->re_nsub > MAX_GROUPS) {
		raise_exception(ist, reException_OBJ, "more than 99 groups in regular expression");
		return NULL;
	}
	re_obj = NEW_OBJ(reCompiled_OBJ);
	SET_TYPE_IF_EXC(reCompiled_OBJ, re_obj, DATA_TYPE_DATAPTR) return NULL;
	re_obj->data.ptr = e;
	set_attr(ist, re_obj, sym(ist, "pattern"), parms[1]);
	set_attr(ist, re_obj, sym(ist, "flags"),   parms[3]);
	return re_obj;
}


MODULE_START(reCompiled)
{
	reCompiled_OBJ = NEW_OBJ(NULL);
	MODULE_ADD_TO_OBJ(reCompiled, re_OBJ, "re");
	set_obj_id(reCompiled_OBJ, re, re);
	MODULE_SET_DOC(reCompiled, "prototype for compiled regular expressions");
}

DEF(reCompiled, findAll, FORM_RPARAM) {
	char* s;
	regmatch_t* m;
	obj_p res, tuple_obj;
	regoff_t sp, ep;
	int i, err, ngrps = numgrps(self);
	regex_t* e = self->data.ptr;

	BIN_CONTENT_CHK(reCompiled);
	STRING_PARAM(1, s);

	res = NEW_LIST(10);
	m = pr_malloc((ngrps+1)*sizeof(regmatch_t));
	m[0].rm_so = -1; m[0].rm_eo = 0;
	while(TRUE) {
		if (m[0].rm_so == m[0].rm_eo) m[0].rm_eo++;
		m[0].rm_so = m[0].rm_eo;  m[0].rm_eo = strlen(s); 
		if ((err = regexec(e, s, ngrps+1, m, REG_STARTEND|(m[0].rm_so ? REG_NOTBOL : 0)))) {
			size_t buf_size;
			char msg[1024];
			if (err == REG_NOMATCH) 
				break;
			if ((buf_size = regerror(err, NULL, msg, 1024)) > 1024) {
				char* msg2 = pr_malloc(buf_size+1);
				regerror(err, NULL, msg2, buf_size);
				raise_exception(ist, reException_OBJ, "%s", msg2);
				pr_free(msg2);
			} else 
				raise_exception(ist, reException_OBJ, "%s", msg);
			pr_free(m);
			return NULL;
		}
		if (ngrps == 0) {
			sp = m[0].rm_so;
			list_append(ist, res, NEW_STRINGN(s+sp, m[0].rm_eo-sp));
		} else if (ngrps == 1) {
			if ((sp = m[1].rm_so) == -1) 
				list_append(ist, res, NEW_STRING(""));
			else 
				list_append(ist, res, NEW_STRINGN(s+sp, m[1].rm_eo-sp));
		} else {
			tuple_obj = NEW_TUPLE(ngrps);
			for (i=1; i <= ngrps; i++) {
				sp = m[i].rm_so;  ep = m[i].rm_eo;
				if (sp == -1) list_append(ist, tuple_obj, NEW_STRING(""));
				else          list_append(ist, tuple_obj, NEW_STRINGN(s+sp,ep-sp));
			}
			list_append(ist, res, tuple_obj);
			tuple_obj->immutable = TRUE;
		}
	}
	pr_free(m);
	return res;
}

DEF( reCompiled, search, FPARM3( s,     NULL, 
							     start, OBJ(ZERO_INT),
							     end,   NEW_INT(MAX_INT_VAL) ) ) {
	obj_p match_obj, lastindex_obj;
	int err;
	unsigned int i, last_index=0, ngrps = numgrps(self);
	char* s;
	regmatch_t* m;
	regex_t* e = self->data.ptr;
	i64_t start, end;

	BIN_CONTENT_CHK(reCompiled);
	STRING_PARAM(1, s);
	INT_64_PARAM(2, start);
	INT_64_PARAM(3, end);

	m = pr_malloc((ngrps+1) * sizeof(regmatch_t));
	m[0].rm_so = (start > 0         ? (regoff_t)(start) : 0);
	m[0].rm_eo = (end   < strlen(s) ? (regoff_t)(end)   : (regoff_t)strlen(s));
	if ((err = regexec(e, s, ngrps+1, m, 0))) {
		size_t buf_size;
		char msg[1024];
		if (err == REG_NOMATCH) return OBJ(NONE);
		if ((buf_size = regerror(err, NULL, msg, 1024)) > 1024) {
			char* msg2 = pr_malloc(buf_size+1);
			regerror(err, NULL, msg2, buf_size);
			raise_exception(ist, reException_OBJ, "%s", msg2);
			pr_free(msg2);
		} else 
			raise_exception(ist, reException_OBJ, "%s", msg);
		pr_free(m);
		return NULL;
	}
	match_obj = NEW_LIST((ngrps+1)*2);
	switch_proto(ist, match_obj, reMatch_OBJ);
	for(i=0; i <= numgrps(self); i++) {
		if (m[i].rm_so > -1) last_index = i;
		list_append(ist, match_obj, NEW_INT(m[i].rm_so));
		list_append(ist, match_obj, NEW_INT(m[i].rm_eo));
	}
	pr_free(m);
	if (last_index) lastindex_obj = NEW_INT(last_index);
	else            lastindex_obj = OBJ(NONE);
	set_attr(ist, match_obj, sym(ist, "re"),			self);
	set_attr(ist, match_obj, sym(ist, "string"),		parms[1]);
	set_attr(ist, match_obj, sym(ist, "pos"),			parms[3]);
	set_attr(ist, match_obj, sym(ist, "endpos"),		parms[5]);
	set_attr(ist, match_obj, sym(ist, "lastindex"),	lastindex_obj);
	return match_obj;
}

DEF( reCompiled, split, FPARM2( s,        NULL, 
							    maxsplit, OBJ(ZERO_INT) ) ) {
	regmatch_t* m;
	obj_p res;
	regoff_t sp, ep, last_split;
	regex_t* e = self->data.ptr;
	int i, split_count=0, maxsplit, err, ngrps = numgrps(self);
	char* s;

	BIN_CONTENT_CHK(reCompiled);
	STRING_PARAM(1, s);
	INT_32_PARAM(2, maxsplit);

	res = NEW_LIST(10);
	m = pr_malloc((ngrps+1)*sizeof(regmatch_t));
	m[0].rm_so = -1; m[0].rm_eo = 0;
	last_split = 0;
	while(TRUE) {
		if (m[0].rm_so == m[0].rm_eo) m[0].rm_eo++;
		m[0].rm_so = m[0].rm_eo;  m[0].rm_eo = strlen(s); 
		if ((err = regexec(e, s, ngrps+1, m, REG_STARTEND|(m[0].rm_so ? REG_NOTBOL : 0)))) {
			size_t buf_size;
			char msg[1024];
			if (err == REG_NOMATCH)
				break;
			if ((buf_size = regerror(err, NULL, msg, 1024)) > 1024) {
				char* msg2 = pr_malloc(buf_size+1);
				regerror(err, NULL, msg2, buf_size);
				raise_exception(ist, reException_OBJ, "%s", msg2);
				pr_free(msg2);
			} else 
				raise_exception(ist, reException_OBJ, "%s", msg);
			pr_free(m);
			return NULL;
		}
		if (m[0].rm_so == m[0].rm_eo) continue;
		list_append(ist, res, NEW_STRINGN(s+last_split, m[0].rm_so-last_split));
		for(i=1; i <= ngrps; i++) {
			if ((sp = m[i].rm_so) == -1) 
				list_append(ist, res, OBJ(NONE));
			else 
				list_append(ist, res, NEW_STRINGN(s+sp, m[i].rm_eo-sp));
		}
		last_split = m[0].rm_eo;
		if (maxsplit && ++split_count == maxsplit) break;
	}
	if ((sp=last_split) != (ep=strlen(s)))
		list_append(ist, res, NEW_STRINGN(s+sp, ep-sp));
	pr_free(m);
	return res;
}

void inc_p2(char* *p2, char* *s, char* *s_lim) {
	if (++(*p2) == *s_lim) {
		size_t size = *(s_lim)-(*s);
		size_t old_size = size;
		size *= 2;
		*s = pr_realloc(*s, size);
		*s_lim = (*s) + size;
		*p2 = (*s) + old_size;
	}
}
int unescape( isp ist, obj_p mlist, char* orig_s, 
			  char* *p1, char* *p2, char* *s, char* *s_lim ) {
	int c1, c2, c3;
	while(*(*p1)) {
		if ((c1=*((*p1)++)) != '\\') {
			*(*p2) = c1;
			inc_p2(p2, s, s_lim);
		} else {
			switch (c2 = *((*p1)++)){
			case '\\': c3 = '\\'; break;
			case '\'': c3 = '\''; break;
			case '\"': c3 = '\"'; break;
			case 'a':  c3 = '\a'; break;
			case 'b':  c3 = '\b'; break;
			case 'f':  c3 = '\f'; break;
			case 'n':  c3 = '\n'; break;
			case 'r':  c3 = '\r'; break;
			case 't':  c3 = '\t'; break;
			case 'v':  c3 = '\v'; break;
			case '0':
				if ((c2=*((*p1)++)) < '0' || c2 > '3' ) goto err;
				c3 = c2-'0';
				if ((c2=*((*p1)++)) < '0' || c2 > '7' ) goto err;
				c3 = c3*8+(c2-'0');
				if ((c2=*((*p1)++)) < '0' || c2 > '7' ) goto err;
				c3 = c3*8+(c2-'0');
				break;
			case 'x':
			case 'X':  
				if ((c2=*((*p1)++)) >= '0' && c2 <= '9' )
					c3 = (c2-'0');
				else if (c2 >= 'a' && c2 <= 'f' )
					c3 = (c2-'a'+10);
				else if (c2 >= 'A' && c2 <= 'F' )
					c3 = (c2-'A'+10);
				else goto err;
				if ((c2=*((*p1)++)) >= '0' && c2 <= '9' )
					c3 = c3*16+(c2-'0');
				else if (c2 >= 'a' && c2 <= 'f' )
					c3 = c3*16+(c2-'a'+10);
				else if (c2 >= 'A' && c2 <= 'F' )
					c3 = c3*16+(c2-'A'+10);
				else goto err;
			default:
				if (c2 > '0' && c2 <= '9') {
					char* t;
					u32_t n;
					regoff_t sp, ep;
					if ((c3=*((*p1)++)) >= '0' && c3 <= '9') 
						n = (c2-'0')*10+(c3-'0');
					else {(*p1)--; n = c2-'0';}
					if (n >= list_len(ist, mlist)/2) goto err;
					sp = (int)list_item(ist, mlist, n*2  )->data.i64;
					ep = (int)list_item(ist, mlist, n*2+1)->data.i64;
					if (sp == -1) continue;
					for(t=orig_s+sp; t < orig_s+ep; t++) {
						*(*p2) = *t;
						inc_p2(p2, s, s_lim);
					}
					continue;
				}
				goto err;
			}
			*(*p2) = c3;
			inc_p2(p2, s, s_lim);
		}
	}
	*(*p2)=0;
	return FALSE;
err:
	raise_exception(ist, reException_OBJ, "invalid escape sequence");
	return TRUE;
}

DEF( reCompiled, sub, FPARM3( repl,  NULL, 
						      s,	 NULL, 
						      count, OBJ(ZERO_INT) ) ) {

	size_t size;
	regmatch_t* m;
	regoff_t last_split;
	obj_p lastindex_obj, repl_obj, res_obj, match_obj, arr[2];
	regex_t* e = self->data.ptr;
	char *repl = "", *orig_s, *p1, *p2, *s, *s_lim, *res;
	int i, last_index=0, repl_is_func=FALSE, count, err, ngrps = numgrps(self);

	BIN_CONTENT_CHK(reCompiled);
	if ( !has_proto(ist, parms[1], OBJ(STRING_PROTO)) &&
		 !(repl_is_func=has_proto(ist, parms[1], OBJ(FUNC_PROTO))) ) {
		raise_exception(ist, OBJ(TYPE_EXC), "sub repl parameter must be a string or function");
		return NULL;
	}

	STRING_PARAM(2, orig_s);
	INT_32_PARAM(3, count);

	match_obj = NEW_LIST((ngrps+1)*2);
	switch_proto(ist, match_obj, reMatch_OBJ);
	set_attr(ist, match_obj, sym(ist, "re"),			self);
	set_attr(ist, match_obj, sym(ist, "string"),		parms[3]);
	lastindex_obj = NEW_INT(last_index);
	set_attr(ist, match_obj, sym(ist, "lastindex"),	lastindex_obj);
	arr[0] = OBJ(NOKEY); arr[1] = match_obj;
	if (!repl_is_func) 
		repl  = pr2c_strptr(ist, parms[1]);
	m = pr_malloc((ngrps+1)*sizeof(regmatch_t));
	m[0].rm_so = -1; m[0].rm_eo = 0;
	last_split = 0;
	res = pr_malloc(1); res[0]=0;
	while(TRUE) {
		if (m[0].rm_so == m[0].rm_eo) m[0].rm_eo++;
		m[0].rm_so = m[0].rm_eo;  m[0].rm_eo = strlen(orig_s); 
		if ((err = regexec(e, orig_s, ngrps+1, m, REG_STARTEND|(m[0].rm_so ? REG_NOTBOL : 0)))) {
			size_t buf_size;
			char msg[1024];
			if (err == REG_NOMATCH)
				break;
			if ((buf_size = regerror(err, NULL, msg, 1024)) > 1024) {
				char* msg2 = pr_malloc(buf_size+1);
				regerror(err, NULL, msg2, buf_size);
				raise_exception(ist, reException_OBJ, "%s", msg2);
				pr_free(msg2);
			} else 
				raise_exception(ist, reException_OBJ, "%s", msg);
			pr_free(m);
			return NULL;
		}
		if (m[0].rm_so != last_split) {
			size = strlen(res)+(m[0].rm_so-last_split);
			res = pr_realloc(res, size+1);
			strncat(res, orig_s+last_split, m[0].rm_so-last_split);
			res[size] = 0;
		}
		last_split = m[0].rm_eo;
		last_index = 0;
		list_clear(ist, match_obj);
		for(i=0; i <= ngrps; i++) {
			if (m[i].rm_so > -1) last_index = i;
			list_append(ist, match_obj, NEW_INT(m[i].rm_so));
			list_append(ist, match_obj, NEW_INT(m[i].rm_eo));
		}
		lastindex_obj->data.i64 = last_index;
		if (repl_is_func) {
			read_unlock(ist, parms[1]);
			repl_obj = call_func(ist, NULL, parms[1], NULL, 2, arr, NULL); 
			read_lock(ist, parms[1]);
			if_exc_return NULL;
			if (repl_obj == OBJ(NONE)) repl = "";
			else {
				if (!has_proto(ist, repl_obj, OBJ(STRING_PROTO))) {
					raise_exception(ist, OBJ(TYPE_EXC), "repl callback function must return a string");
					return NULL;
				}
				repl = pr2c_strptr(ist, repl_obj);
			}
		}
		p1 = repl;
		size = (strlen(p1) * 2 > 256 ? strlen(p1) * 2 : 256);
		s = pr_malloc(size); 
		s_lim = s + size;
		p2 = s;
		if (unescape(ist, match_obj, orig_s, &p1, &p2, &s, &s_lim)) return NULL;
		size = strlen(res)+strlen(s);
		res = pr_realloc(res, size+1);
		strcat(res, s);
		pr_free(s);
	}
	if (strlen(orig_s) != last_split) {
		size = strlen(res)+(strlen(orig_s)-last_split);
		res = pr_realloc(res, size+1);
		strncat(res, orig_s+last_split, strlen(orig_s)-last_split);
		res[size] = 0;
	}
	res_obj = NEW_STRING(res);
	del_unlock(match_obj);
	pr_free(res);
	pr_free(m);
	return res_obj;
}

DEF(reCompiled, objList_, FORM_RPARAM) {
	BIN_CONTENT_CHK(reCompiled);
	return parms[1];
}

DEF(reCompiled, final_, NULL) {
	if (self->data.ptr)
		regfree((regex_t*)(self->data.ptr));
	return OBJ(NONE);
}


MODULE_START(reMatch)
{
	reMatch_OBJ = NEW_OBJ(OBJ(LIST_PROTO));
	MODULE_SET_DOC(reMatch, "prototype for regular expression match objects");
	set_obj_id(reMatch_OBJ, re, Match);
	MODULE_ADD_TO_OBJ(reMatch, re_OBJ, "Match");
}

DEF(reMatch, objList_, FORM_RPARAM) {
	BIN_CONTENT_CHK(reMatch);
	return parms[1];
}

DEF(reMatch, start, FPARM1(groupid, OBJ(ZERO_INT))) {
	u32_t grp;
	i64_t groupid;

	BIN_CONTENT_CHK(reMatch);
	INT_64_PARAM(1, groupid);
	grp = (u32_t) groupid;
	if (grp < 0 || grp >= list_len(ist, self)/2) {
		raise_exception(ist, reException_OBJ, "start function groupid parameter value invalid");
		return NULL;
	}
	return list_item(ist, self, 2*grp);
}

DEF(reMatch, end, FPARM1(groupid, OBJ(ZERO_INT))) {
	u32_t grp;
	i64_t groupid;

	BIN_CONTENT_CHK(reMatch);
	INT_64_PARAM(1, groupid);
	grp = (u32_t) groupid;
	if (grp < 0 || grp >= list_len(ist, self)/2) {
		raise_exception(ist, reException_OBJ, "end function groupid parameter value invalid");
		return NULL;
	}
	return list_item(ist, self, 2*grp+1);
}

DEF(reMatch, span, FPARM1(groupid, OBJ(ZERO_INT))) {
	u32_t grp;
	obj_p res;
	i64_t groupid;

	BIN_CONTENT_CHK(reMatch);
	INT_64_PARAM(1, groupid);
	grp = (u32_t) groupid;
	if (grp < 0 || grp >= list_len(ist, self)/2) {
		raise_exception(ist, reException_OBJ, "span function groupid parameter value invalid");
		return NULL;
	}
	res = NEW_TUPLE(2);
	list_append(ist, res, list_item(ist, self, 2*grp  ));
	list_append(ist, res, list_item(ist, self, 2*grp+1));
	res->immutable = TRUE;
	return res;
}

DEF(reMatch, expand, FORM_RPARAM) {
	size_t size;
	char *p1, *p2, *s, *s_lim, *orig_s;
	obj_p orig_s_obj, res;

	BIN_CONTENT_CHK(reMatch);
	STRING_PARAM(1, p1);

	size = (strlen(p1) * 2 > 256 ? strlen(p1) * 2 : 256);
	s = pr_malloc(size); 
	s_lim = s + size;
	p2 = s;
	orig_s_obj = get_attr(ist, self, sym(ist, "string")); if_exc_return NULL;
	orig_s = pr2c_strptr(ist, orig_s_obj);
	if (unescape(ist, self, orig_s, &p1, &p2, &s, &s_lim)) return NULL;
	res = NEW_STRING(s);
	pr_free(s);
	return res;
}

DEF( reMatch, group, list4( ist, sym(ist, "groupid"),  OBJ(ZERO_INT),
							     sym(ist, "groupids"), OBJ(PARAM_STAR) ) ) {
	int i, j, llen, ngrps = ((int)list_len(ist, self))/2, sp, ep;
	char* orig_s;
	obj_p orig_s_obj, res;

	BIN_CONTENT_CHK(reMatch);
	INT_32_PARAM(1, j);

	if (j < 0 || j > ngrps) {
		raise_exception(ist, OBJ(TYPE_EXC), "groupid value out of range");
		return NULL;
	}
	if (parms[3]) {
		int llen = (int)list_len(ist, parms[3]);
		for (i=0; i < llen; i++) {
			obj_p item = list_item(ist, parms[3], i);
			if (!has_proto(ist, item, OBJ(INT_PROTO))) {
				raise_exception(ist, OBJ(TYPE_EXC), "groupid parameters must be integers");
				return NULL;
			}
			j = (int)(item->data.i64);
			if (j < 0 || j > ngrps) {
				raise_exception(ist, OBJ(TYPE_EXC), "groupid value out of range");
				return NULL;
			}
		}
	}
	orig_s_obj = get_attr(ist, self, sym(ist, "string")); if_exc_return NULL;
	orig_s = pr2c_strptr(ist, orig_s_obj);
	if (!parms[3] || !(llen = (int)list_len(ist, parms[3]))) {
		j = (int)(parms[1]->data.i64);
		sp = (int)(list_item(ist, self, j*2  )->data.i64);
		ep = (int)(list_item(ist, self, j*2+1)->data.i64);
		if (sp == -1) return OBJ(NONE);
		else		  return NEW_STRINGN(orig_s+sp, ep-sp);
	} else { 
		res = NEW_TUPLE(llen);
		for (i=0; i < llen; i++) {
			obj_p item = list_item(ist, parms[3], i);
			j = (int)(item->data.i64);
			sp = (int)(list_item(ist, self, j*2  )->data.i64);
			ep = (int)(list_item(ist, self, j*2+1)->data.i64);
			if (sp == -1) list_append(ist, res, OBJ(NONE));
			else		  list_append(ist, res, NEW_STRINGN(orig_s+sp, ep-sp));
		}
		res->immutable = TRUE;
		return res;
	}
}

DEF( reMatch, groups, FPARM1(default, OBJ(NONE))) {
	int i, ngrps = (int)list_len(ist, self)/2, sp, ep;
	char* orig_s;
	obj_p orig_s_obj, res = NEW_TUPLE(ngrps);
	orig_s_obj = get_attr(ist, self, sym(ist, "string")); if_exc_return NULL;
	orig_s = pr2c_strptr(ist, orig_s_obj);
	BIN_CONTENT_CHK(reMatch);
	for (i=0; i < ngrps; i++) {
		sp = (int)(list_item(ist, self, i*2  )->data.i64);
		ep = (int)(list_item(ist, self, i*2+1)->data.i64);
		if (sp == -1) list_append(ist, res, parms[1]);
		else		  list_append(ist, res, NEW_STRINGN(orig_s+sp, ep-sp));
	}
	res->immutable = TRUE;
	return res;
}


MAIN_MODULE_INIT(re)
{
	MODULE_SUB_INIT(re);
	MODULE_ADD_SYM(re, compile);

	MODULE_SUB_INIT(reCompiled);
	MODULE_ADD_SYM(reCompiled, findAll);
	MODULE_ADD_SYM(reCompiled, search);
	MODULE_ADD_SYM(reCompiled, split);
	MODULE_ADD_SYM(reCompiled, sub);
	MODULE_ADD_SYM(reCompiled, final_);
	MODULE_ADD_SYM(reCompiled, objList_);

	MODULE_SUB_INIT(reMatch);
	MODULE_ADD_SYM(reMatch, start);
	MODULE_ADD_SYM(reMatch, end);
	MODULE_ADD_SYM(reMatch, span);
	MODULE_ADD_SYM(reMatch, expand);
	MODULE_ADD_SYM(reMatch, group);
	MODULE_ADD_SYM(reMatch, groups);
	MODULE_ADD_SYM(reMatch, objList_);

	check_exceptions(ist);
}
