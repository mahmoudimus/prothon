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


// symbol.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "object.h"
#include "clist.h"

// this must match exact order in enum sym_enum_t in bytecodes.h
sym_id_entry_t sym_id_table[SYM_ENUM_COUNT] = {
	{ "string_",		0 }, // string_ must be first (==0)
	{ "not__QUES",		0 },
	{ "pos_",			0 },
	{ "neg_",			0 },
	{ "invert_",		0 },
	{ "in__QUES",		0 },
	{ "notIn__QUES",	0 },
	{ "is__QUES",		0 },
	{ "isNot__QUES",	0 },
	{ "lt__QUES",		0 },
	{ "le__QUES",		0 },
	{ "gt__QUES",		0 },
	{ "ge__QUES",		0 },
	{ "ne__QUES",		0 },
	{ "eq__QUES",		0 },
	{ "cmp_",			0 },
	{ "or_",			0 },
	{ "xor_",			0 },
	{ "and_",			0 },
	{ "rShift_",		0 },
	{ "lShift_",		0 },
	{ "add_",			0 },
	{ "sub_",			0 },
	{ "mul_",			0 },
	{ "div_",			0 },
	{ "floorDiv_",		0 },
	{ "mod_",			0 },
	{ "pow_",			0 },
	{ "ior_",			0 },
	{ "ixor_",			0 },
	{ "iand_",			0 },
	{ "irshift_",		0 },
	{ "ilshift_",		0 },
	{ "iadd_",			0 },
	{ "isub_",			0 },
	{ "imul_",			0 },
	{ "idiv_",			0 },
	{ "ifloordiv_",		0 },
	{ "imod_",			0 },
	{ "ipow_",			0 },
	{ "rIn__QUES",		0 },		
	{ "rNotIn__QUES",	0 },	
	{ "rlt_",			0 },			
	{ "rle_",			0 },
	{ "rgt_",			0 },
	{ "rge_",			0 },
	{ "rrshift_",		0 },
	{ "rlshift_",		0 },
	{ "rSub_",			0 },
	{ "rDiv_",			0 },
	{ "rfloordiv_",		0 },
	{ "rmod_",			0 },
	{ "rPow_",			0 },
	{ "rCmp_",			0 },
	{ "init_",			0 }, 
	{ "bool__QUES",		0 }, 
	{ "getCmp_",		0 }, 
	{ "cmpvalobj_",		0 }, 
	{ "iter_",			0 }, 
	{ "id_",			0 },
	{ "doc_",			0 }, 
	{ "hash_",			0 },
	{ "fparams_",		0 },
	{ "globals_",		0 },
	{ "syn_func_",		0 },
	{ "syn_locals_",	0 },
	{ "rparam",			0 },
	{ "param1",			0 },
	{ "param2",			0 },
	{ "args",	        0 },
	{ "kwargs",	        0 },
	{ "copy_",			0 },
	{ "covers__QUES",	0 },
	{ "coerce_",		0 },
	{ "str_",			0 },
	{ "next",			0 },
	{ "getItem_",		0 },
	{ "setItem_",		0 },
	{ "delItem_",		0 },
	{ "objList_",		0 }, // f3
	{ "final_",			0 },
	{ "len_",			0 },
	{ "max",			0 },
	{ "min",			0 },
	{ "ord_",			0 },
	{ "chr_",			0 },
	{ "copy",			0 },
	{ "toStorProxy_",	0 },
	{ "fromStorProxy_", 0},
	{ "call_",          0},
	{ "bindObj_",		0},
	{ "prevScope_",     0},
	{ "accLevel_",      0},
	{ "cont_",          0},
	{ "name_",          0},
	{ "attrs_",         0},
	{ "protos_",        0},
	{ "command_",       0},
	{ "notifyCallback_",0},
	{ "get_",           0},
	{ "set_",           0},
	{ "del_",           0}
};

typedef struct trie_node_s* trie_p;

typedef union {
	trie_p	ptr;
	obj_p	key;
} pp_t;

typedef struct trie_node_s {
    char	splitchar;
    trie_p	lokid;
    pp_t	eqkid;
    trie_p	hikid;
} trie_node_t;

obj_p	  STRING_SYM = NULL;

attr_key_t     next_key    = 0;
trie_p    symbol_tree = NULL;

static DECLARE_PR_LOCK(symbol_tree_lock);

trie_p ins_trie(trie_p p, char *s, obj_p key) {
	if (p == 0) {
        p = (trie_p) pr_malloc(sizeof(trie_node_t));
        p->splitchar = *s;
        p->lokid = p->eqkid.ptr = p->hikid = 0;
    }
    if (*s < p->splitchar)
        p->lokid = ins_trie(p->lokid, s, key);
    else if (*s == p->splitchar) {
		if (*s == 0)
            p->eqkid.key = key;
		else
            p->eqkid.ptr = ins_trie(p->eqkid.ptr, ++s, key);
    } else
        p->hikid = ins_trie(p->hikid, s, key);
    return p;
}
obj_p search_trie(trie_p root, char *s) {
	trie_p p;
    p = root;
    while (p) {
        if (*s < p->splitchar)
            p = p->lokid;
        else if (*s == p->splitchar)  {
            if (*s++ == 0)
                return p->eqkid.key;
            p = p->eqkid.ptr;
        } else
            p = p->hikid;
    }
    return NULL;
}
/*
void free_trie(trie_p p) {
	if (p) {
		free_trie(p->lokid);
		if (p->splitchar) free_trie(p->eqkid.ptr);
		free_trie(p->hikid);
		pr_free(p);
    }
}
*/

//********************************* sym ***************************************
obj_p sym(isp ist, char* s_in) {
	obj_p sym;
	int i;
	size_t len = strlen(s_in);
	char *p, *s = pr_malloc(len+1);
	strcpy(s, s_in);
	if (!strcmp(s+len-5, "_BANG")) strcpy(s+len-5, "!");
	if (!strcmp(s+len-5, "_QUES")) strcpy(s+len-5, "?");
	if (!*s) goto bad_sym;
	if (! ( (*s >= 'a' && *s <= 'z') || 
		    (*s >= 'A' && *s <= 'Z') || *s == '_' ) ) goto bad_sym;
	for (p=s+1; *p; p++) {
		if (! ( (*p >= 'a' && *p <= 'z') || 
				(*p >= 'A' && *p <= 'Z') || 
				(*p >= '0' && *p <= '9') || 
				(*p == '_' || *p == '!') || *p == '?' ) ) goto bad_sym;
		if ((*p == '!' || *p == '?') && *(p+1)) goto bad_sym;
	}
	goto good_sym;
bad_sym:
	raise_exception(ist, OBJ(INTERPRETER_EXC), "invalid symbol: '%s'", s);
	pr_free(s);
	return NULL;
good_sym:
	pr_lock(&symbol_tree_lock);
	sym = search_trie(symbol_tree, s);
	if (sym) {
		pr_unlock(&symbol_tree_lock);
		pr_free(s);
		return sym;
	}
	sym = NEW_OBJ(OBJ(SYMBOL_PROTO));
	SET_TYPE_IF_EXC(OBJ(SYMBOL_PROTO), sym, DATA_TYPE_IMMDATA) return NULL;
	sym->imm_data_len = 4;
	if (!(next_key = (next_key+3) & 0x7fffffff)) {
		printf("Out of unique symbol keys.  Did we really use two billion?\n");
		pr_exit(1);
	}
	sym->data.key = next_key;
	symbol_tree = ins_trie(symbol_tree, s, sym);
	if (!STRING_SYM) STRING_SYM = sym;
	pr_unlock(&symbol_tree_lock);
	set_attr(ist, sym, STRING_SYM, NEW_STRING(s));
	sym->wr_access = ACC_SYSTEM;
	su(i);
	dict_add(ist, OBJ(SYMBOLS), NEW_INT(sym->data.key), sym);
	un_su(i);
	pr_free(s);
	set_immutable(sym);
	return sym;
}

//********************************* symch *************************************
char* symch(isp ist, obj_p sym_obj){
	obj_p s_obj;
	if (sym_obj) {
		s_obj = get_attr(ist, sym_obj, SYM(STRING_));
		if (s_obj) {
			char* tmp = pr2c_strptr(ist, s_obj);
			return tmp;
		}
	}
	return NULL;
}

//********************************* init_symbol *******************************
void init_symbol(isp ist){
	int i;
	for(i = 0; i < SYM_ENUM_COUNT; i++)
		sym_id_table[i].id = sym(ist, sym_id_table[i].str);
}
