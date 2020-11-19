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


// prothon_dll.h

#ifndef PROTHON_DLL_H
#define PROTHON_DLL_H

#include <prothon/prothon.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	void		(*raise_exception)(isp ist, obj_p proto_obj, const char *format, ...)
		                            __attribute__((format(printf,3,4)));
	apr_pool_t* (*get_pr_head_pool)(void);
	isp			(*new_ist)(access_t access);
	obj_p		(*get_attr)(isp ist, obj_p obj, obj_p key);
	void		(*set_attr)(isp ist, obj_p object, obj_p key, obj_p value);
	int			(*del_attr)(isp ist, obj_p obj, obj_p key);
	int			(*add_proto)(isp ist, obj_p obj, obj_p proto);
	int			(*del_proto)(isp ist, obj_p obj, obj_p proto);
	int			(*has_proto)(isp ist, obj_p obj, obj_p obj_proto);
	void		(*switch_proto)(isp ist, obj_p obj, obj_p new_proto);
	obj_p		(*list_append)(isp ist, obj_p list_obj, obj_p item);
	obj_p		(*list_item)(isp ist, obj_p list, int i);
	void		(*list_item_set)(isp ist, obj_p list_obj, int i, obj_p new_item);
	size_t		(*list_len)(isp ist, obj_p list);
	void		(*list_clear)(isp ist, obj_p list_obj);
	obj_p		(*tuple1)(isp ist, obj_p p1);
	obj_p		(*tuple2)(isp ist, obj_p p1, obj_p p2);
	obj_p		(*list1)(isp ist, obj_p p1);
	obj_p		(*list2)(isp ist, obj_p p1, obj_p p2);
	obj_p		(*list3)(isp ist, obj_p p1, obj_p p2, obj_p p3);
	obj_p		(*list4)(isp ist, obj_p p1, obj_p p2, obj_p p3, obj_p p4);
	obj_p		(*list6)(isp ist, obj_p p1, obj_p p2, obj_p p3, obj_p p4, 
		                          obj_p p5, obj_p p6);
	obj_p		(*list8)(isp ist, obj_p p1, obj_p p2, obj_p p3, obj_p p4, 
		                          obj_p p5, obj_p p6, obj_p p7, obj_p p8);
	obj_p		(*list10)(isp ist, obj_p p1, obj_p p2, obj_p p3, obj_p p4, 
		                           obj_p p5, obj_p p6, obj_p p7, obj_p p8,
								   obj_p p9, obj_p p10);
	obj_p		(*list16)(isp ist, obj_p  item1, obj_p  item2, obj_p  item3, obj_p  item4, 
			          obj_p  item5, obj_p  item6, obj_p  item7, obj_p  item8,
			          obj_p  item9, obj_p item10, obj_p item11, obj_p item12,
			          obj_p item13, obj_p item14, obj_p item15, obj_p item16);
	obj_p		(*new_hash_obj)(isp ist, i32_t num);
	obj_p		(*new_int_obj)(isp ist, i64_t num);
	obj_p		(*new_float_obj)(isp ist, double num);
	obj_p		(*new_string_obj)(isp ist, char* string);
	obj_p		(*new_string_n_obj)(isp ist, char* string, size_t n);
	void		(*c2pr_strcpy)(isp ist, obj_p obj, char* p, size_t len);
	obj_p		(*new_tuple_obj)(isp ist, int fixed_size);
	obj_p		(*new_list_obj)(isp ist, size_t initial_size);
	void		(*init_list_data)(isp ist, obj_p obj, int initial_size);
	obj_p		(*copy_list_obj)(isp ist, obj_p list_obj);
	obj_p		(*new_C_func_obj)(isp ist, pr_func* func_ptr, obj_p form_lbl_val_list);
	obj_p		(*call_func)( isp ist, obj_p cont_self, obj_p func_sym, obj_p self,
		                      int parm_cnt, obj_p* lbl_val_arr, obj_p dyn_locals );
	obj_p		(*call_func1_f)(isp ist, obj_p self, obj_p sym, obj_p parm);
	obj_p		(*call_func2_f)(isp ist, obj_p self, obj_p sym, obj_p parm1, obj_p parm2);
	obj_p   	(*sym)(isp ist, char* symbol);
	obj_p		(*new_object)(isp ist, obj_p proto);
	void		(*copy_object_data)(isp ist, obj_p copy, obj_p obj);
	int			(*proto_owns_binary)(isp ist, obj_p proto, obj_p obj);
	void		(*obj_set_data_owner)(isp ist, obj_p obj, obj_p owner);
	void		(*set_obj_doc_f)(isp ist, obj_p obj, char* str);
	obj_p	    (*dict_keys_values)(isp ist, obj_p dict_obj, int key_flg);
	obj_p		(*check_exceptions)(isp ist);
	attr_key_t  (*attr_first_key)(isp ist, obj_p obj);
	attr_key_t  (*attr_next_key)(isp ist, obj_p obj, attr_key_t key);
	obj_p		(*attr_value_by_key)(isp ist, obj_p obj, attr_key_t key);
	int			(*dict_add)(isp ist, obj_p dict_obj, obj_p key_in, obj_p value_in);
	obj_p		(*dict_item)(isp ist, obj_p dict_obj, obj_p key_in);
	int			(*is_unclonable)(obj_p obj);
	char*		(*symch)(isp ist, obj_p sym_obj);
	obj_p		(*key_to_sym)(isp ist, attr_key_t key);
	obj_p		(*new_dict_obj)(isp ist, int initial_size);
	void		(*dict_clear)(isp ist, obj_p dict_obj);
	int			(*proto_len)(isp ist, obj_p obj);
	obj_p		(*proto_item)(isp ist, obj_p obj, int i);
	void		(*dump)(isp ist, char* dumpfilename, obj_p obj);
	i32_t		(*int2i32t)(isp ist, obj_p self);
	i64_t		(*int2i64t)(isp ist, obj_p self);
	char*		(*pr2c_strptr)(isp ist, obj_p str_obj);

	void*		(*pr_malloc)(size_t nbytes);
	void*		(*pr_realloc)(void *p, size_t nbytes);
	void		(*pr_free)(void *p);

	void*		(*obj_malloc) (isp ist, obj_p proto, obj_p obj, size_t size);
	void*		(*obj_realloc)(isp ist, obj_p obj, size_t size);
	void		(*obj_free)   (isp ist, obj_p obj);

	int		       (*read_lock)(isp ist, obj_p obj);	
	void		 (*read_unlock)(isp ist, obj_p obj);	
	int		      (*write_lock)(isp ist, obj_p obj);		
	void		(*write_unlock)(isp ist, obj_p obj);

	void		    (*del_lock)(obj_p obj);		
	void		  (*del_unlock)(obj_p obj);	
	void	   (*set_immutable)(obj_p obj);		
	void	   (*clr_immutable)(obj_p obj);
	void		(*set_archived)(obj_p obj);		
	void		(*clr_archived)(obj_p obj);
	int          (*is_archived)(obj_p obj);

	void		(*set_obj_rdacc)(obj_p obj, int access);
	void		(*set_obj_wracc)(obj_p obj, int access);
	int			(*get_obj_rdacc)(obj_p obj);

	int			(*get_obj_wracc)(obj_p obj);
				
	obj_p* 				OBJ;
	sym_id_entry_t* 	sym_id_table;

} pr_services_t;

/* XXX: Need to document all this stuff */
#define MODULE_DECLARE(name)    \
static isp ist;                 \
static pr_services_t *services; \
static obj_p name##_OBJ;		\
static obj_p _ncfo_;

/* Should be used to initialize the name##_OBJ */
#define MODULE_START(name) \
static void name##_initialize_module(void)

#define DEF(slf, func, fparam_list)								\
static obj_p slf##func(isp ist, obj_p self, int pcnt,			\
		       obj_p* parms, obj_p dlocals);					\
static void slf##func##init(void) {								\
        set_attr(ist, slf##_OBJ, sym(ist, #func),				\
		 _ncfo_ = new_C_func_obj(ist, slf##func, fparam_list));	\
}																\
static obj_p slf##func(isp ist, obj_p self, int pcnt,			\
		       obj_p* parms, obj_p dlocals)

#define BIN_EMPTY_CHK()									\
	if (self->data_type != DATA_TYPE_NONE) {			\
		raise_exception(ist, OBJ(INTERNAL_EXC),			\
			"object has data, expected none");	\
		return NULL; }									

	//if (self == slf##_OBJ)										
	//	return call_func(ist, self, SYM(STR_), 0, NULL, slf##_OBJ);	
#define BIN_STR_CHK(slf)											\
	if (self->data_type == DATA_TYPE_NONE) {						\
		char msg[80];												\
		apr_snprintf(msg, sizeof(msg), "<%s:Uninitialized>", #slf);	\
		return NEW_STRING(msg); }										

#define BIN_EMPTY_OR_NOT_CHK(slf)								\
	if (self->data_type != DATA_TYPE_NONE) {					\
		if (!proto_owns_binary(ist, slf##_OBJ, self) ) {		\
		raise_exception(ist, OBJ(INTERNAL_EXC),					\
			"object has wrong binary data, expected %s", #slf);	\
		return NULL; }											\
		if (self->data_type == DATA_TYPE_DATAPTR)				\
			pr_free(self->data.ptr);							\
		self->data_type = DATA_TYPE_NONE; }

#define BIN_CONTENT_CHK(slf)									\
	if (self->data_type == DATA_TYPE_NONE) {					\
		raise_exception(ist, OBJ(INTERNAL_EXC),					\
			"object has no binary data, expected %s", #slf);	\
		return NULL; }											\
	if (!proto_owns_binary(ist, slf##_OBJ, self)) {				\
		raise_exception(ist, OBJ(INTERNAL_EXC),					\
			"object has wrong binary data, expected %s", #slf);	\
		return NULL; }											

#define MAIN_MODULE_INIT(name)									\
static void name##_module_install(void);						\
PR_DECLARE_EXPORT void name##_dll_entry(pr_services_t* srvcs)	\
{																\
	services = srvcs;											\
	ist = new_ist(ACC_SYSTEM);									\
	name##_module_install();									\
}																\
static void name##_module_install(void)

#define BUILTIN_LOAD(name)					\
do {										\
	extern void name##_dll_entry(void);		\
	name##_dll_entry();						\
} while(0)

#define MODULE_ADD_TO_BASE(name)					\
do {												\
	set_attr(ist, OBJ(MODULES), sym(ist, #name),	\
		 name##_OBJ);								\
	set_attr(ist, OBJ(OBJECT), sym(ist, #name),		\
		 name##_OBJ);								\
	check_exceptions(ist);							\
} while(0)

#define MODULE_ADD_TO_OBJ(name, obj, __sym)				\
do {													\
	set_attr(ist, obj, sym(ist, __sym), name##_OBJ);	\
	check_exceptions(ist);								\
} while(0)

#define MODULE_CONSTANT_DECLARE(module, name)  obj_p module##_##name

#define MODULE_CONSTANT_INT(module, name, val)						\
	module##_##name = NEW_INT(val);									\
	clr_immutable(module##_##name);									\
	set_obj_id(module##_##name, module, name);						\
	set_immutable(module##_##name);									\
	set_attr(ist, module##_OBJ, sym(ist, #name), module##_##name)

#define MODULE_CONSTANT_OBJ(module, name, obj)						\
	module##_##name = obj;											\
	clr_immutable(module##_##name);									\
	set_obj_id(module##_##name, module, name);						\
	set_immutable(module##_##name);									\
	set_attr(ist, module##_OBJ, sym(ist, #name), module##_##name)

#define MODULE_SUB_INIT(name)	\
	name##_initialize_module();	\
	check_exceptions(ist);

#define MODULE_ADD_SYM(name, __sym) \
	name##__sym##init();			\
	check_exceptions(ist);

#define MARK_AS_COMMAND set_attr(ist, _ncfo_, SYM(COMMAND_), OBJ(PR_TRUE));

#define MODULE_SET_DOC(name, docstr)					\
	set_obj_doc(name##_OBJ, #name ": " docstr);	\
	check_exceptions(ist);

#ifndef OBJECT_H

#define new_ist				(*services->new_ist)
#define get_attr			(*services->get_attr)
#define set_attr			(*services->set_attr)
#define del_attr			(*services->del_attr)
#define add_proto			(*services->add_proto)
#define del_proto			(*services->del_proto)
#define has_proto		    (*services->has_proto)
#define switch_proto		(*services->switch_proto)
#define list_append			(*services->list_append)
#define list_item			(*services->list_item)
#define list_item_set		(*services->list_item_set)
#define list_len			(*services->list_len)
#define list_clear			(*services->list_clear)
#define new_hash_obj		(*services->new_hash_obj)
#define new_C_func_obj		(*services->new_C_func_obj)
#define call_func			(*services->call_func)
#define call_func1_f		(*services->call_func1_f)
#define call_func2_f		(*services->call_func2_f)
#define new_int_obj			(*services->new_int_obj)
#define new_float_obj		(*services->new_float_obj)
#define new_string_obj		(*services->new_string_obj)
#define new_string_n_obj	(*services->new_string_n_obj)
#define c2pr_strcpy		(*services->c2pr_strcpy)
#define new_tuple_obj		(*services->new_tuple_obj)
#define new_list_obj		(*services->new_list_obj)
#define init_list_data		(*services->init_list_data)
#define copy_list_obj		(*services->copy_list_obj)
#define sym					(*services->sym)
#define tuple1				(*services->tuple1)
#define tuple2				(*services->tuple2)
#define list1				(*services->list1)
#define list2				(*services->list2)
#define list3				(*services->list3)
#define list4				(*services->list4)
#define list6				(*services->list6)
#define list8				(*services->list8)
#define list10				(*services->list10)
#define list16				(*services->list16)
#define raise_exception		(*services->raise_exception)
#define new_object			(*services->new_object)
#define copy_object_data	(*services->copy_object_data)
#define proto_owns_binary	(*services->proto_owns_binary)
#define obj_set_data_owner	(*services->obj_set_data_owner)
#define set_obj_doc_f		(*services->set_obj_doc_f)
#define pr_malloc			(*services->pr_malloc)
#define pr_realloc			(*services->pr_realloc)
#define pr_free				(*services->pr_free)
#define obj_malloc			(*services->obj_malloc)
#define obj_realloc			(*services->obj_realloc)
#define obj_free			(*services->obj_free)
#define OBJ(x) 				(services->OBJ[x])
#define SYM(symenum)  		(services->sym_id_table[symenum].id)
#define read_lock			(*services->read_lock)
#define read_unlock			(*services->read_unlock)	
#define write_lock			(*services->write_lock)	
#define write_unlock		(*services->write_unlock)	
#define del_lock			(*services->del_lock)	
#define del_unlock			(*services->del_unlock)	
#define set_immutable		(*services->set_immutable)	
#define clr_immutable		(*services->clr_immutable)	
#define get_pr_head_pool	(*services->get_pr_head_pool)
#define dict_keys_values	(*services->dict_keys_values)
#define check_exceptions	(*services->check_exceptions)
#define set_obj_rdacc		(*services->set_obj_rdacc)
#define set_obj_wracc		(*services->set_obj_wracc)
#define get_obj_rdacc		(*services->get_obj_rdacc)
#define get_obj_wracc		(*services->get_obj_wracc)
#define attr_first_key   	(*services->attr_first_key)   
#define attr_next_key	 	(*services->attr_next_key)	 
#define attr_value_by_key	(*services->attr_value_by_key)
#define dict_add		 	(*services->dict_add)
#define dict_item		 	(*services->dict_item)		 
#define is_unclonable	 	(*services->is_unclonable)
#define symch	 	        (*services->symch)	 
#define key_to_sym	 	    (*services->key_to_sym)	 
#define new_dict_obj	 	(*services->new_dict_obj)	 
#define dict_clear		 	(*services->dict_clear)	 
#define proto_len		 	(*services->proto_len)		 
#define proto_item		 	(*services->proto_item)		 
#define set_archived	 	(*services->set_archived)
#define clr_archived	 	(*services->clr_archived)
#define is_archived	 		(*services->is_archived)
#define dump		 		(*services->dump)
#define int2i64t		 	(*services->int2i64t)
#define int2i32t	 		(*services->int2i32t)
#define pr2c_strptr	 		(*services->pr2c_strptr)

#endif // OBJECT_H

#ifdef __cplusplus
}
#endif

#endif // PROTHON_DLL_H
