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

// Prothon.h    -i -dpbc ..\pr\test\test.pr
 
#ifndef PROTHON_H
#define PROTHON_H

//**************************** DEBUG DEFINITIONS ******************************
 
#define CMDLINE_FLAGS
#define TRACE_PARSER
#define DUMP_MODULE_CODE
#define TRACE_INTERPRETER
#define DUMP_OBJECTS_AT_END
#define DEBUG_THREADS

//#define DEBUG_MEM_MGR
//#define PROSIST_DEBUG


//*****************************************************************************

#include <apr.h>
#include <apr_pools.h>
#include <apr_thread_proc.h>
#include <apr_portable.h>
#include <apr_strings.h>

#ifdef __cplusplus
extern "C" {
#endif
 
#define PROTHON_VERSION_NUMBER	"0.1"
#define PROTHON_VERSION_BUILD   "$Rev: 710$"  
#define PROTHON_VERSION_DATE	__DATE__

#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	1
#endif

#ifdef WIN32
#define PR_DECLARE_EXPORT		__declspec(dllexport)
#else
#define PR_DECLARE_EXPORT
#endif

//************************* SOME APPLICATION TUNING ***************************
#define AS_STR_MAX_DEPTH	16
#define MAX_UNICODE_VALUE	0x10ffff

//************************* BASIC TYPE DEFINITIONS ****************************

typedef signed char			i8_t;
typedef apr_int16_t			i16_t;
typedef apr_int32_t			i32_t;
typedef apr_int64_t			i64_t;
typedef apr_byte_t			u8_t;
typedef apr_uint16_t		u16_t;
typedef apr_uint32_t		u32_t;
typedef apr_uint64_t		u64_t;

#ifdef WIN32
// temporary kludge until Visual Studio problem resolved
#define pr_assert(expr)									\
do {char buf[80];										\
	if (!(expr)) {										\
		apr_snprintf(buf, sizeof(buf),					\
			"Assertion failed at file %s, line %d\n",	\
								__FILE__, __LINE__);	\
		printf(buf);									\
		pr_exit(1);										\
	}													\
} while (0)

#define MAX_INT_VAL		0x7fffffffffffffff
#else
#define pr_assert(expr)	assert(expr)
#define MAX_INT_VAL		0x7fffffffffffffffLL
#endif

#undef min
# define min(a,b) (((a)<(b)) ? (a) : (b))
#undef max
# define max(a,b) (((a)<(b)) ? (b) : (a))
#undef exch
#define exch(a,b) (tmp=a, a=b, b=tmp)
#ifdef WIN32
#undef abs
#define abs(x) ((x) < 0 ? -(x) : (x))
#else
#include <stdlib.h>
#endif

// neg key value means entry is deleted
#define ENTRY_DELETED	-1

typedef i32_t			attr_key_t;
typedef struct obj_s*	obj_p;

typedef enum {
	OBJ_STATE_INVALID = -1,
	OBJ_STATE_NEW_OR_MARKED,
	OBJ_STATE_UNMARKED,
	OBJ_STATE_DELETED
} obj_state_t;

typedef struct {
	int			size;
	int			len;
} attr_proto_hdr_t;

typedef struct {
	int			owns_binary;
	obj_p		proto;
} proto_entry_t;

typedef struct {
	attr_key_t	key;
	obj_p		value;
} attr_entry_t;

typedef union {
	attr_proto_hdr_t	hdr;
	proto_entry_t		proto;
	attr_entry_t 		attr;
} attr_t;

typedef attr_t* attr_p;

typedef union {
	attr_p	attrs;
	obj_p	proto;
} attr_proto_t;

typedef enum {
	DATA_TYPE_NONE = 0,
	DATA_TYPE_IMMDATA,
	DATA_TYPE_DATAPTR,
	DATA_TYPE_EXTPTR
} data_type_t;

typedef u32_t data_size_t;

#define IMM_DATA_LEN 8
#define DATA_SIZE(obj)	(*((data_size_t*)((obj)->data.ptr)))

typedef struct {
	u8_t	b0;
	u8_t	b1;
	u8_t	b2;
} ch3_t;

typedef ch3_t* ch3_p;

typedef union {
	attr_key_t	key;
	double		f64;
	i64_t		i64;
	i32_t		i32[IMM_DATA_LEN/4];
	ch3_t		str[IMM_DATA_LEN/3];
	u8_t		bytes[IMM_DATA_LEN];
	void*		ptr;
} obj_data_t;

typedef u8_t access_t;

#define ACC_GUEST	0
#define ACC_USER1	1
#define ACC_USER2	2
#define ACC_SYSTEM	3

typedef struct obj_s {
	obj_state_t		state			:3; // u8_t :2 when not debugging
	u8_t			scanned			:1;	
	data_type_t		data_type		:3; // u8_t :2 when not debugging
	u8_t			imm_data_len	:4;
	access_t		rd_access		:2;	
	access_t		wr_access		:2;	
	u8_t			archived		:1;	
	u8_t			rd_notify		:1;	
	u8_t			wr_notify		:1;	
	u8_t			del_notify		:1;	
	u8_t 			has_attrs		:1;
	u8_t			unclonable		:1;
	u8_t			immutable		:1;
	u8_t			del_locked		:1;
	u8_t			rdlock_cnt		:7;
	u8_t			wrlock_req_cnt	:7;
	i32_t			wrlock;
	attr_proto_t	attr_proto;
	obj_data_t		data;
	obj_p			next_obj;
} obj_t;

typedef struct {
	data_size_t	size;
	u8_t		byt[];
} byt_t;

typedef byt_t* byt_p;

typedef struct {
	data_size_t	size;
	ch3_t		str[];
} str_t;

typedef str_t* str_p;

typedef struct frame_t* frame_p;
typedef struct interp_state_s* isp;

typedef struct user_thread_s {
	obj_p func;
	obj_p params;
} user_thread_t;

typedef user_thread_t* user_thread_p;

typedef struct {
	data_size_t			size;
	apr_os_thread_t		apr_os_thread;
	apr_thread_t*		apr_thread;
	isp					ist;
	int					main_id;
	user_thread_t		user_thread_data;
	int					running;
	frame_p				frame;
	int					save_frame;
} pr_thread_t;

typedef pr_thread_t* pr_thread_p;

typedef u16_t digit;

typedef struct {
	data_size_t	size;
	i32_t		dsize;
	digit		digit[];
} long_t;

typedef long_t* long_p;

//************************* MALLOC FUNCTIONS **********************************
// These behave identical to the standard OS functions, but on segments smaller
// than 64 bytes are much faster than most OS implementations.  See prmalloc.c.
// These also implement proper locking.
void *pr_malloc(size_t nbytes);
void *pr_realloc(void *ptr, size_t nbytes);
void  pr_free(void *ptr);

//************************* WELL-KNOWN-OBJECTS ********************************
// these are created at object system initialization
typedef enum {
// "unique" objects
	PR_FALSE=0,			// 0 False
	PR_TRUE,			// 1 True
	OBJECT,				// 2 Object
	NONE,				// 3 None

	NOKEY,				// 4
	PARAM_STAR,			// 5
	PARAM_STAR_STAR,	// 6
	NODEF,				// 7

// prototype objects used as protos for basic "types"
	INT_PROTO,			// 8 Int
	FLOAT_PROTO,		// 9 Float
	IMAG_PROTO,			// 10 Imaginary
	SYMBOL_PROTO,		// 11 Symbol
	DATETIME_PROTO,		// 12 Time
	SEQ_PROTO,			// 13 Sequence
	BYTES_PROTO,		// 14 Bytes
	STRING_PROTO,		// 15 String
	TUPLE_PROTO,		// 16 Tuple
	LIST_PROTO,			// 17 List
	DICT_PROTO,			// 18 Dict
	FUNC_PROTO,			// 19 Func
	METHOD_PROTO,		// 20 Method
	HASH_PROTO,			// 21 Hash
	SLICE_PROTO,		// 22 Slice
	LOCAL_PROTO,		// 23
	OUTER_PROTO,		// 24
	SUPER_PROTO,		// 25
	GEN_PROTO,			// 26
	ITER_PROTO,			// 
	THREAD_PROTO,		//   Thread
	MUTEX_PROTO,		//   Mutex
	PARMPTR_PROTO,		//
	PROP_PROTO,			//
  
// Exceptions
	EXCEPTION,			//    Exception
	INTERNAL_EXC,		//    InternalError
	PARSEERROR_EXC,		//   
	INTERPRETER_EXC,	//   
	ASSERTION_EXC,		//    AssertionError
	NAME_EXC,			//    NameError
	INDEX_EXC,			//   IndexError
	TYPE_EXC,			//   TypeError
	VALUE_EXC,			//   ValueError
	MUTABLE_EXC,		//	 Mutable Error
	DIVIDEZERO_EXC,		//  
	OVERFLOW_EXC,		//   OverflowError
	OUTOFMEMORY_EXC,	//  
	IO_EXC,		//  
	FUNCNOTFOUND_EXC,	//  
	FILENOTFOUND_EXC,	//  
	STOP_ITERATION_EXC,	//  
	LOCK_EXC,			//  
	PERMISSION_EXC,		//  
	COMMAND_EXC,		//  
 
// constants
	ZERO_INT,			//  

// Containers
	ROOT_GLOBALS,		//   Root_Globals
	SYMBOLS,			//   Symbols_dict
	MODULES,			//   Modules
	SYS,				//	 sys module

	OBJ_ENUM_COUNT
} obj_enum_t;

extern obj_p OBJ[OBJ_ENUM_COUNT];

// Use OBJ(OBJNAME) to access well-known objects
// example:  OBJ(OBJECT) or OBJ(PR_TRUE)

//************************* WELL-KNOWN-SYMBOLS ********************************
// actual symbols are lower-case versions of these constants
typedef enum {
	STRING_,
	NOT__QUES,		
	POS_,		
	NEG_,		
	INV_,		

	IN__QUES,		
	NOTIN__QUES,	
	IS__QUES,			
	ISNOT__QUES,		

	LT__QUES,		// LT__QUES thru EQ__QUES	
	LE__QUES,		// must stay in this sequence	
	GT__QUES,		// see binary() in parser_routines.c
	GE__QUES,		
	NE__QUES,		
	EQ__QUES,		

	CMP_,
	OR_,		
	XOR_,	
	AND_,	
	RSHIFT_,	
	LSHIFT_,	
	ADD_,	
	SUB_,	
	MUL_,	
	DIV_,	
	FLOORDIV_,
	MOD_,		
	POW_,		

	IOR_,		
	IXOR_,	
	IAND_,	
	IRSHIFT_,	
	ILSHIFT_,	
	IADD_,	
	ISUB_,	
	IMUL_,	
	IDIV_,	
	IFLOORDIV_,
	IMOD_,		
	IPOW_,		

	RIN__QUES,			
	RNOTIN__QUES,		
	RLT__QUES,			
	RLE__QUES,			
	RGT__QUES,			
	RGE__QUES,		
	RRSHIFT_,	
	RLSHIFT_,	
	RSUB_,	
	RDIV_,
	RFLOORDIV_,
	RMOD_,		
	RPOW_,		
	RCMP_,

	INIT_,
	BOOL__QUES,	
	GETCMP_,
	CMPVALOBJ_,
	ITER_,
	ID_,
	DOC_,
	HASH_,
	FPARAMS_,
	GLOBALS_,
	SYN_FUNC_,
	SYN_LOCALS_,
	RPARAM,
	PARAM1,
	PARAM2,
	ARGS,
	KWARGS,
	COPY_,
	COVERS_,
	COERCE_,
	STR_,
	NEXT,
	GETITEM_,
	SETITEM_,
	DELITEM_,
	OBJLIST_,
	FINAL_,
	LEN_,
	MAX,
	MIN,
	ORD_,
	CHR_,
	COPY,
	TOSTORPROXY_,
	FROMSTORPROXY_,
	CALL_,
	BINDOBJ_,
	PREVSCOPE_,
	ACCLEVEL_,
	CONT_,
	NAME_,
	ATTRS_,
	PROTOS_,
	COMMAND_,
	NOTIFYCALLBACK_,
	GET_,
	SET_,
	DEL_, 

	SYM_ENUM_COUNT
} sym_enum_t;

typedef struct {
	char*	str;
	obj_p	id;
} sym_id_entry_t;

sym_id_entry_t sym_id_table[SYM_ENUM_COUNT];

// You may get any symbol-obj from a string using sym(ist, "xxx").
// Use SYM(XXX_) instead of sym(ist, "xxx_") when using a well-known
// symbol for speed, although using sym(ist, "xxx_") with a well-known would
// get you the exact same object.  Use symch(symbol_obj) for the reverse operation.


//*****************************************************************************
//***************** GENERAL OBJECT ACCESS FUNCTIONS ***************************

//************************* INTERPRETER STATE *********************************
// The function calling from Prothon to C and from C to Prothon in some of the
// sections below require that a pointer to an interpreter state structure called
// "ist" be passed back and forth.  This is important as Prothon supports
// multiple interpreter threads and even multiple interpreters.
// When a Prothon function calls C code, that C code must keep ist intact and
// pass that ist value on in any call to a Prothon function.  The C code is only
// allowed to call one Prothon function at a time with that value in the same thread.
// ist also provides the exception mechanism (see EXCEPTION section below). 
// Access determines the access priveleges the thread has in accessing objects.
// Each object has a threshold access for reading and for writing.  See the definition
// of struct obj_s.
// "module" is the main module of the code for this interpreter.
// NOTE: The pointer type "isp" and the variable name "ist" were chosen to be short 
// because they are used in almost every function call in Prothon C code.  You must 
// use these names as given because many macros expect these names.  Don't worry, 
// you will have them memorized in no time.
typedef struct interp_state_s {
	obj_p		module;
	access_t	access;
	void*		lock_stack;
	frame_p		frame;
	int			in_console;
	int			notify_flags;
	void*		as_str_stack;
	obj_p		exception_obj;
} interp_state_t;
 
// NEW_OBJECT: Create a new empty object with only proto pointer
// Creates a new empty object with a single proto obj.  If proto is
// NULL then the proto defaults to OBJ(OBJECT).  The new object
// has the del_locked bit set so that the memory manager (garbage
// collector) will not delete the object.  It is the programmer's
// responsibility to clear this bit with a del_unlock(obj) call
// after the object has been stored in the root_global hierarchy, 
// or if you are finished with the object and want it deleted.  If you
// do not clear this bit the object will exist forever and cause
// a memory leak.  See DELETE LOCKING section below.
obj_p new_object(isp ist, obj_p proto);
#define NEW_OBJ(proto) new_object(ist, proto)

// COPY_OBJECT_DATA: Copy object data from one object to another
// Destination object (copy) must already exist.
// Clobbers binary data in dest object (copy)
void copy_object_data(isp ist, obj_p copy, obj_p obj);

// SET_TYPE_IF_EXC: Set data type and owner, throw exception if already set
#define SET_TYPE_IF_EXC(owner, obj, type)				\
	if ((obj)->data_type != DATA_TYPE_NONE)				\
		raise_exception(ist, OBJ(INTERNAL_EXC),			\
          "object binary data overwrite attempt");		\
	else { (obj)->data_type = (type);					\
		    obj_set_data_owner(ist, obj, owner);		\
	} if (ist->exception_obj)							\
			(obj)->data_type = DATA_TYPE_NONE;			\
	if (ist->exception_obj) 
  
// PROTO_OWNS_BINARY: Prototype owns binary data in obj
// for example, if proto is Int, then binary data is in Int format
int proto_owns_binary(isp ist, obj_p proto, obj_p obj);
 
// OBJ_SET_DATA_OWNER: Set the owner of the binary data in obj to owner prototype
void obj_set_data_owner(isp ist, obj_p obj, obj_p owner);

// GET_ATTR: Find attr in object that matches a key and return the attr value.
// Searches object for attr with key that matches the given key symbol object.
// If no attr is found with a matching key, then NULL is returned.
obj_p get_attr(isp ist, obj_p object, obj_p key);

// SET_ATTR: Replace matching key attr with new value, creating new attr if needed.
// Much like get-attr, except that when an old attr is found the value param replaces 
// the old attr value.  If no attr is found a new one is created.
void set_attr(isp ist, obj_p object, obj_p key, obj_p value);

// GET_PROTO_ATTR: Same as get_attr except that it will also try proto objects.
// This tries the given object and then if no attr matches it will try all
// proto objects (aka parents, aka ancestors) up to and including OBJ(OBJECT).  
// It trys them in a specific order such that the "diamond problem" from multiple 
// inheritance is solved.  When it returns an attr you need to check *proto_pp
// to see which object the attr actually came from.  If proto_pp is NULL then this 
// info is not returned.
// When super_obj is set, the search starts after the object super_obj in the 
// prototype chain.
// The ist param may return an exception object if proto pointers are missing 
// or have a circular reference.  See section "INTERPRETER STATE".
obj_p get_proto_attr(isp ist, obj_p object, obj_p key, obj_p *proto_pp, obj_p super_obj);

// DEL_ATTR: Find attr in object that matches a key and delete it.
// Much like get-attr, except that when an attr is found it is deleted.
// If no attr is found to delete it returns FALSE, else TRUE.
int del_attr(isp ist, obj_p obj, obj_p key);

// GET_SCOPE_ATTR: Same as get_attr except that it will also try local scope chain.
// same as get_attr but will look in all surrounding syntactical scopes as well
// if scope_pp not null, then will return scope obj that attr was found in
// if super_obj specified, then only objects after super_obj after searched
obj_p get_scope_attr(isp ist, obj_p scope, obj_p key, obj_p *scope_pp, obj_p super_obj);

// ATTR_xxx: functions to scan attribute table
// key is the unique integer assigned to each symbol by the symbol table
// you can use key_to_sym(key) to find the symbol object for an attribute
// and symch(ist, symbol) to get the C string version of the symbol object
attr_key_t attr_first_key(isp ist, obj_p obj);
attr_key_t attr_next_key(isp ist, obj_p obj, attr_key_t key);
obj_p attr_value_by_key(isp ist, obj_p obj, attr_key_t key);
obj_p key_to_sym(isp ist, attr_key_t key);

// ADD_PROTO: Append an object to the list of proto objects.
// This is the immediate protos for an object, not the reachable.
// Duplicates will be ignored.  TRUE will be returned if the proto
// was appended, FALSE if not.
int add_proto(isp ist, obj_p obj, obj_p proto);

// DEL_PROTO: Delete an object from the list of proto objects.
// This is from the immediate proto list for an object, not the reachable.
// You must pass the exact object to be deleted.
// The last proto may not be deleted.  TRUE will be returned if the proto
// was found and deleted, FALSE if not.
int del_proto(isp ist, obj_p obj, obj_p proto);

// PROTO_LEN: Get number of protos in an object
int proto_len(isp ist, obj_p obj);

// PROTO_ITEM: Retrieve Nth proto in object's proto list
// This is from the immediate proto list for an object, not the reachable.
// You must pass the index from 0 to the proto_len(ist, obj)-1.
obj_p proto_item(isp ist, obj_p obj, int i);

// PROTO_LIST: Returns a list object containing reachable proto objects.
// The proto list is the list of all objects that can be reached by walking all 
// proto pointers until you reach the root object. Proto objects are sorted into the 
// special order that solves the "diamond problem" found in multiple inheritance 
// situations.  The passed object is always the first entry returned and OBJ(OBJECT) 
// is always the last entry returned in the list.
// The ist param may return an exception object if proto pointers are missing 
// or have a circular reference.  See section "INTERPRETER STATE" below.
obj_p proto_list(isp ist, obj_p object);

// HAS_PROTO:  Returns TRUE if second object is in reachable proto list of first object
// The ist param may return an exception object if proto pointers are missing 
// or have a circular reference.  See section "INTERPRETER STATE" below.
int has_proto(isp ist, obj_p obj, obj_p obj_proto);

// CHECK_TYPE_EXC: Convenience macro for type validation of objects
// (usually parms[1])
#define CHECK_TYPE_EXC(__obj, __proto_obj, name)			\
do {														\
	if (!(__obj) || !has_proto(ist, __obj, __proto_obj)) {	\
		raise_exception(ist, OBJ(TYPE_EXC),					\
				"object is not of type " #name);			\
		return NULL;										\
	}														\
} while(0)

// IF_APR_ERR: Convenience macro for apr call error checking
#define IF_APR_ERR(msg)													\
	if (aprerr != APR_SUCCESS) {										\
		char buf[80];  apr_strerror(aprerr, buf, sizeof(buf));			\
		raise_exception(ist, OBJ(INTERNAL_EXC), "%s: %s", (msg), buf);	\
	}																	\
	if (aprerr != APR_SUCCESS)

// SWITCH_PROTO_TO: Replace all old protos with a new single proto object
void switch_proto(isp ist, obj_p obj, obj_p new_proto);

//***************** SPECIFIC OBJECT TYPE FUNCTIONS **************************

// SYM: Create (or access an existing) symbol object from a C string.
// Symbols are not garbage collected as they are automatically
// added to the OBJ(SYMBOLS) dictionary with a int object (key code)
// as the key and the symbol as the value.  No two symbols have the
// same int key code value. 
// A Prothon variable such as "x" in x=1 is a symbol.
obj_p sym(isp ist, char* symbol_string);

// SYMCH: Find the C string for a symbol object.
// This is the inverse function of sym().
// returns null if the id is invalid.
char* symch(isp ist, obj_p sym_obj);

// NEW_INT_OBJ: Create a new integer object from a 64-bit C int.
obj_p new_int_obj(isp ist, i64_t num);
#define NEW_INT(num) new_int_obj(ist, num)

// NEW_FLOAT_OBJ: Create a new float object from a 64-bit C double.
obj_p new_float_obj(isp ist, double num);
#define NEW_FLOAT(num) new_float_obj(ist, num)

// NEW_BYTES_OBJ: Create a new bytes object from a data buffer.
obj_p new_bytes_obj(isp ist, const u8_t* buffer, size_t len);
#define NEW_BYTES(buf, len) new_bytes_obj(ist, buf, len)

// NEW_STRING_OBJ: Create a new string object from a C string.
obj_p new_string_obj(isp ist, char* string);
#define NEW_STRING(string)  new_string_obj(ist, string)

// NEW_STRING_OBJ_N: Create a new string object from a C string of N chars long.
// Same as new_string_obj but string length is set by n, not null char at end.
obj_p new_string_n_obj(isp ist, char* string, size_t n);
#define NEW_STRINGN(string, n)  new_string_n_obj(ist, string, n)

// NEW_STRING_CH3_OBJ: Create a new string object from a 24-bit char string of N chars long.
obj_p new_string_ch3_obj(isp ist, ch3_p ch3p, size_t n);
#define NEW_STRING_CH3(ch3p, n)  new_string_ch3_obj(ist, ch3p, n)

// SET_STRING_DATA: Change an existing object's data to string data
// clobbers any esisting data pointer
void c2pr_strcpy(isp ist, obj_p obj, char* p, size_t len);

// PR_STRPTR: retreive string object as a C string
char* pr2c_strptr(isp ist, obj_p str_obj);

// PR_STRLEN: macro to retreive string length of a string object (type data_size_t)
#define pr_strlen(str_obj)																		    \
	( ((str_obj)->data_type == DATA_TYPE_IMMDATA) ? (((data_size_t)((str_obj)->imm_data_len))/3):	\
	 (((str_obj)->data_type == DATA_TYPE_DATAPTR) ? ((DATA_SIZE(str_obj)-sizeof(str_t))/3) : 0 ) )

// PR_CH3PTR: macro to retreive ptr to chars for a string object (type ch3_p)
#define pr_ch3ptr(str_obj)																		\
	( ((str_obj)->data_type == DATA_TYPE_IMMDATA) ? (str_obj->data.str)					:		\
	 (((str_obj)->data_type == DATA_TYPE_DATAPTR) ? (((str_p)(str_obj->data.ptr))->str) : NULL ) )

// NEW_TIME_OBJ: Create a new time object with a given initial time
obj_p new_time_obj(isp ist, i64_t usecs);
#define NEW_TIME(usecs) new_time_obj(ist, usecs)

// NEW_LIST_OBJ: Create a new list object with a given initial size
obj_p new_list_obj(isp ist, size_t initial_size);
#define NEW_LIST(initial_size) new_list_obj(ist, initial_size)

// INIT_LIST_DATA: Change an existing object's data to a list object's data
// clobbers any existing data pointer
void init_list_data(isp ist, obj_p obj, int initial_size);

// NEW_TUPLE_OBJ: Create a new immutable tuple object
// A tuple is identical to a list except that at the Prothon level it is
// immutable.  At the C level this means that you should not modify it
// once you have given it away to other code.  It is implemented the same
// way as a list so all the list functions will work on a tuple.
// This creation function actually calls NEW_LIST() and changes the
// proto to TUPLE_PROTO. After calling this you should set all the data 
// items with list_append() before releasing the object to other code.
obj_p new_tuple_obj(isp ist, int fixed_size);
#define NEW_TUPLE(fixed_size)  new_tuple_obj(ist, fixed_size)

// NEW_DICT_OBJ: Create a new dictionary object
// A dictionary stores objects much like the attributes are stored except that
// any immutable object can be the key, not just a symbol.  Lookups are more
// expensive since hashes are used and an eq__QUES call must be made during the
// lookup unlike attributes that just use the key identity for comparisom.
// Make sure the initial size is at least twice as large as the intended number
// of objects you will be adding initially.  Expanding a dictionary is an expensive
// operation.
obj_p new_dict_obj(isp ist, int initial_size);
#define NEW_DICT(initial_size)  new_dict_obj(ist, initial_size);

// DICT_CLEAR: Remove all contents of the dictionary
void dict_clear(isp ist, obj_p dict_obj);

// CLONE_LIST_OBJ: Create a new list object that's a shallow copy of another
obj_p copy_list_obj(isp ist, obj_p list_obj);

// LIST_APPEND: Append object item to existing list object
obj_p list_append(isp ist, obj_p list, obj_p item);

// LIST_LEN: Get length of list
size_t list_len(isp ist, obj_p list);

// LIST_CLEAR: Set length of list to zero
void list_clear(isp ist, obj_p list);

// LIST_ITEM: Access object i from list
obj_p list_item(isp ist, obj_p list, int i);

// LIST_ITEM_SET: Set object i in list
void list_item_set(isp ist, obj_p list, int i, obj_p new_item);

// TUPLEn: Convenience tuple create functions
// tuple1, tuple2 are convenience functions that allow you to create
// new tuple objects that contain 1 or 2 item objects.
obj_p tuple1(isp ist, obj_p p1);
obj_p tuple2(isp ist, obj_p p1, obj_p p2);

// LISTn: Convenience list create functions
// list1, list2, ... are convenience functions that allow you to create
// new list objects that contain 1 or more item objects.
obj_p list1(isp ist, obj_p p1);
obj_p list2(isp ist, obj_p p1, obj_p p2);
obj_p list3(isp ist, obj_p p1, obj_p p2, obj_p p3);
obj_p list4(isp ist, obj_p p1, obj_p p2, obj_p p3, obj_p p4);
obj_p list6(isp ist, obj_p item1, obj_p item2, obj_p item3, obj_p item4, 
			         obj_p item5, obj_p item6 );
obj_p list8(isp ist, obj_p item1, obj_p item2, obj_p item3, obj_p item4, 
			         obj_p item5, obj_p item6, obj_p item7, obj_p item8 );
obj_p list10(isp ist,obj_p item1, obj_p item2, obj_p item3, obj_p item4, 
			         obj_p item5, obj_p item6, obj_p item7, obj_p item8,
					 obj_p item9, obj_p item10 );
obj_p list16(isp ist, obj_p  item1, obj_p  item2, obj_p  item3, obj_p  item4, 
			          obj_p  item5, obj_p  item6, obj_p  item7, obj_p  item8,
			          obj_p  item9, obj_p item10, obj_p item11, obj_p item12,
			          obj_p item13, obj_p item14, obj_p item15, obj_p item16);
// DICT_ADD: Add a key/value pair to the dictionary.  Replace old one if key exists.
// Key equality is determined by the eq__QUES function on the key objects.  The eq__QUES
// function is called on the existing key in the dict with the passed key as the parameter.
// If the keys match, then the value in the dictionary is replaced and the existing key
// is left in place.  If there is no match the ky and value are added.
// The function returns TRUE if there was no match and a new entry was added and it returns
// false if a value was replaced for an existing key.
int dict_add(isp ist, obj_p dict_obj, obj_p key_in, obj_p value_in);

// DICT_LEN: Return the number of key/value pairs in the dictionary.
size_t dict_len(isp ist, obj_p dict);

// DICT_ITEM:  Look up a value in the dictionary for the given key.
// Find a key using the same eq__QUES match criteria given for 
// dict_add and return the corresponding value object.  If no key 
// matches then return null.
obj_p dict_item(isp ist, obj_p dict, obj_p key);

// DICT_VALUE: Identical to dict_item, provided for consistency in naming.
#define dict_value(ist, dict, key)  dict_item(ist, dict, key)

// DICT_KEYS: Return a list object containing all keys in the dictionary.
// The set of all keys in the dictionary will be returned in a list object.
// The order will be random, but if the list is unmodified (such as the case
// when read-locked) then the order will stay the same.  The order will match
// the order of values given by dict_values() if they are both called while the
// dict is read-locked.
#define dict_keys(ist, dict_obj)   dict_keys_values(ist, dict_obj, 1)

// DICY_KEYS_VALUES: Implmentation for getting a list of keys, values or
// key/values in a list from a dict. Possible args for key_flg:
//
// 0 = List of values
// 1 = List of keys
// 2 = List of keys and values (odd list items are keys, even are values)
obj_p dict_keys_values(isp ist, obj_p dict_obj, int key_flg);

// DICT_VALUES: Return a list object containing all values in the dictionary.
// The set of all values in the dictionary will be returned in a list object.
// The order will be random, but if the list is unmodified (such as the case
// when read-locked) then the order will stay the same.  The order will match
// the order of keys given by dict_keys() if they are both called while the
// dict is read-locked.
#define dict_values(ist, dict_obj) dict_keys_values(ist, dict_obj, 0)

// HASH_MASK: Maximum value for any hash function to return.
// Minimum value is 1.
#define HASH_MASK 0x7fffffff

// SET_OBJ_ID: Mark an object as a identity object with a name
// This is used by Prosist, the persistence engine and other documentation.
// Any object that occupies a constant attribute slot at all times
// should be marked with this attribute so that it can be found.
// Being constant means it has a unique identity across runs of the 
// interpreter and different interpreters should be marked as such.
// Not two objects in the same namespace should have this same name.
// The module name and object name are convenience for finding the obj.
// The following should be true at all times: obj = module.name
// module == * -> obj = Object.name
// module and container equal to * means Object.name.
// no quotes are needed, ex: set_obj_id(Match_OBJ, Re_OBJ, Match);
// All Prototypes should be marked with this.
#define set_obj_id(obj, module, name)				 \
	set_attr(ist, (obj), SYM(ID_),					 \
				  tuple2(ist, NEW_STRING((#module)), \
	                          NEW_STRING((#name)) ) )

// SET_OBJ_DOC: Add document string (doc_) to object
// This can be used on any object.  It is recommended that it be used
// liberally to make browsing objects easier and to make dumps readable.
void set_obj_doc_f(isp ist, obj_p obj, char* str);
#define set_obj_doc(obj, str) set_obj_doc_f(ist, (obj), (str))

// NEW_THREAD_OBJ: Create a new thread object and start the thread
// thread_func is the C function to execute.  thread_data points to 
// arugments for the thread_func function.
// Note: thread_func must call register_thread() (see below) immediately after
// starting.
// You may provide a blank thread_obj as an input parameter if you wish.
// You must provide a thread obj or an acc level.
obj_p new_thread_obj( isp ist, apr_thread_start_t thread_func, 
					  void *thread_data, obj_p thread_obj, int acc_level );

// REGISTER_THREAD: Register a newly started thread
// This tells Prothon that a thread has actually started, releasees the
// thread_registry_lock so that other threads can start, and returns the
// thread object for the thread that has just started running.
// The parameter "this_thread" that is needed for this call is passed to 
// the thread_func function (see APR docs).
// Example thread_func: void *main_thread(apr_thread_t *handle, void *argv);
// The handle parameter is used for this_thread:  register_thread(handle);
obj_p register_thread(apr_thread_t *this_thread);

//****************** HIGHER LEVEL OBJECT ACCESS ROUTINES ************************

// xxx_ITEM: These are higher-level access routines for attributes and data
// If you pass a symbol for the key value then you can get, set, or delete
// object attributes.  If you pass a slice object, then you can operate on any 
// mutable sequence object such as a list or dictionary.  If you pass a super 
// object as a key then you can access attributes of proto objects.  These are the
// routines called by the Prothon interpreter for general access.
obj_p get_item(isp ist, obj_p obj, obj_p key, int prop_flg);
void  set_item(isp ist, obj_p obj, obj_p key, obj_p value, int prop_flg);
void  del_item(isp ist, obj_p obj, obj_p key, int prop_flg);

// NEW_SLICE_OBJ: This is the object that represents a slice of a sequence.
// A slice is one, two, or three objects such as the two ints in x[5:9].  A slice 
// is implemented as a list of objects with a SLICE_PROTO proto.  When an
// indexing operation such as this is done, a slice object is created and 
// passed to the sequence object.  Each of the three index parameters in this func
// may have the value SLICEPARAM_EMPTY to indicate the default value in the
// slice such as this case: x[:15].  The second and third parameters may have
// the value SLICEPARAM_MISSING to indicate the number of params to include.
// example: x[1:] -> new_slice_obj(ist, int1, SLICEPARAM_EMPTY, SLICEPARAM_MISSING)
// where int1 = NEW_INT(1);
// You can also use OBJ(NONE) instead of SLICEPARAM_EMPTY.
obj_p new_slice_obj(isp ist, obj_p ind1, obj_p ind2, obj_p ind3);

#define SLICEPARAM_EMPTY	((obj_p)3)
#define SLICEPARAM_MISSING	((obj_p)4)

// SLICE1: Convenience macro for creating slice object with one param
// Note that slices for dictionaries should always have only one param.
#define slice1(ind) new_slice_obj(ist, ind, SLICEPARAM_MISSING, SLICEPARAM_MISSING)

// SLICE_ALL: Convenience macro for creating slice object representing entire sequence
#define SLICE_ALL new_slice_obj(ist, SLICEPARAM_EMPTY, SLICEPARAM_EMPTY, SLICEPARAM_MISSING)

// NEW_LOCAL_OBJ: This represents the reference of a local variable
// This is much like a slice in that it indicates how data is to be referenced
// in an object.  In this case the data is an local variable with no prefix.
obj_p new_local_obj(isp ist, obj_p symbol);

// NEW_SUPER_OBJ: This represents the reference of an super variable like "super.var"
// This is much like a slice in that it indicates how data is to be referenced
// in an object.  In this case the data is an super prefixed variable like "super.var".
// An Super reference is the same as "self" with lookup of the var happening on the
// prototype of "self" except that all matches are ignored until the prototype
// "container" is passed by on the prototype chain of "self".
obj_p new_super_obj(isp ist, obj_p container, obj_p symbol) ;

//************************* Prothon C FUNCTION ********************************
// This is the type of C function that the dll author must provide to implement
// a Prothon function in C.  A pointer to this type is passed to new_C_func_obj.
// FUNCTION CALLING:
// Interp is a pointer to the interpreter state (see above).
// Self is the object that the function was called on.  lbl_parm_arr is an array
// of paired objects containing the resolved labelled parameters from the passed 
// actual parameters.  The first object (and every even numbered object) is a label 
// (symbol object).  The second object (and every odd numbered object) is the passed 
// value object that corresponds to that label.  Note that a param value in the
// param array might actually be a list or dictionary of many parameters if the 
// *list or **dictionary type formal parameters were used in new_C_func_obj.
// In this case the symbol object ptr for the label will be OBJ(PARAM_STAR) or OBJ(PARAM_STAR_STAR).
// Dyn-locals (dynamic locals) are the locals of the calling function.   This allows
// you to access vars in the same manner that the @ operator does in Prothon.
// The function must always return an object-id (OBJ(NONE) if nothing else).
typedef obj_p (pr_func)( isp ist, obj_p self, 
						 int parm_cnt, obj_p* lbl_parm_arr, obj_p dyn_locals ); 
   
// NEW_FUNC_OBJ: Create a new function object from a C function and formal parameters
// Func_ptr should point to a function of the type defined above.
// The formal parameters must be defined by the label_def_list which holds the parameter
// labels and default values (in even and odd positions) and optionally the *list and 
// **dictionary args which are supported by putting OBJ(PARAM_STAR) or OBJ(PARAM_STAR_STAR) in 
// the label (even) position and the list or dict symbol in the default (odd) position.
// example:  in "def func(x, y="3", *z)", the label_def_list would be:
//    list6(ist,NULL, sym(ist, "x"), NULL, sym(ist, "y"), NEW_STRING("3"), OBJ(PARAM_STAR), sym(ist, "z"));
// If the formal param list is empty, you can put NULL instead of the list.
obj_p new_C_func_obj(isp ist, pr_func* func_ptr, obj_p label_def_list);

// FPARMn: Convenience macros for formal params in function definitions
#define FPARM1(lab1, val1)										\
	list2(ist, sym(ist, #lab1), val1)
#define FPARM2(lab1, val1, lab2, val2)							\
	list4(ist, sym(ist, #lab1), val1, sym(ist, #lab2), val2)	
#define FPARM3(lab1, val1, lab2, val2, lab3, val3)				\
	list6(ist, sym(ist, #lab1), val1, sym(ist, #lab2), val2,    \
               sym(ist, #lab3), val3 )
#define FPARM4(lab1, val1, lab2, val2, lab3, val3, lab4, val4)	\
	list8(ist, sym(ist, #lab1), val1, sym(ist, #lab2), val2,    \
               sym(ist, #lab3), val3, sym(ist, #lab4), val4 )
#define FPARM5(lab1, val1, lab2, val2, lab3, val3, lab4, val4,  \
               lab5, val5)                                      \
	list10(ist, sym(ist, #lab1), val1, sym(ist, #lab2), val2,   \
                sym(ist, #lab3), val3, sym(ist, #lab4), val4,   \
			    sym(ist, #lab5), val5 )
#define FPARM8(lab1, val1, lab2, val2, lab3, val3, lab4, val4,  \
               lab5, val5, lab6, val6, lab7, val7, lab8, val8)  \
	list16(ist, sym(ist, #lab1), val1, sym(ist, #lab2), val2,   \
                sym(ist, #lab3), val3, sym(ist, #lab4), val4,   \
                sym(ist, #lab5), val5, sym(ist, #lab6), val6,   \
                sym(ist, #lab7), val7, sym(ist, #lab8), val8 )

// xxx_PARAM: Convenience macro for validation and assignment of params
// index is 1..n where 1 is first parameter and n is last
// when index is 0 .. -n, parms[1] is a list and -index is list index
#define ITEM (index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index))

#define INT____PARAM(index, var)	/* obj_t var; */						\
{	if (!has_proto(ist, (index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index)), OBJ(INT_PROTO))) {						\
		raise_exception(ist, OBJ(TYPE_EXC),									\
				"expected a integer in parameter " #index);					\
		return (index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index));														\
	}																		\
	(var) = (index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index)); }

#define INT_32_PARAM(index, var)	/* i32_t var; */						\
do {	if (!has_proto(ist, (index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index)), OBJ(INT_PROTO))) {						\
		raise_exception(ist, OBJ(TYPE_EXC),									\
				"expected a integer in parameter " #index);					\
		return (index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index));														\
	}																		\
	(var) = int2i32t(ist, index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index)); \
} while (FALSE)
i32_t int2i32t(isp ist, obj_p self);

#define INT_64_PARAM(index, var)	/* i64_t var; */						\
do { obj_p item = index > 0 ? parms[((index)-1)*2+1] :                      \
                              list_item(ist, parms[1], -index);             \
     if (!has_proto(ist, item, OBJ(INT_PROTO))) {                           \
		 raise_exception(ist, OBJ(TYPE_EXC),							    \
				             "expected a integer in parameter " #index);    \
		 return NULL; }  (var) = int2i64t(ist, item);                       \
} while (FALSE)

i64_t int2i64t(isp ist, obj_p self);

#define BYTES__PARAM(index, var)	/* obj_p var; */						\
{	obj_p __pobj = ( index > 0 ? parms[((index)-1)*2+1] :					\
                                 list_item(ist, parms[1], -index));			\
	if (!has_proto(ist, __pobj, OBJ(BYTES_PROTO))) {						\
		raise_exception( ist, OBJ(TYPE_EXC),								\
				         "expected bytes in parameter " #index );			\
		return NULL;														\
	}   var = __pobj; }

#define STRING_PARAM(index, var)	/* char* var; */						\
{	if (!has_proto(ist, (index > 0 ? parms[((index)-1)*2+1] :               \
                   list_item(ist, parms[1], -index)), OBJ(STRING_PROTO))) {	\
		raise_exception(ist, OBJ(TYPE_EXC),									\
				"expected a string in parameter " #index);					\
		return NULL;														\
	} (var) = pr2c_strptr(ist, (index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index))); }

#define FUNC___PARAM(index, var)	/* obj_p var; */						\
{	if (!has_proto(ist, (index > 0 ? parms[((index)-1)*2+1] :			    \
								list_item(ist, parms[1], -index)) ,			\
							OBJ(FUNC_PROTO))) {								\
		raise_exception(ist, OBJ(TYPE_EXC),									\
				"expected a function obj in parameter " #index);			\
		return NULL;														\
	}																		\
	(var) = (index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index));	}

#define FLOAT__PARAM(index, var)	/* double var; */						\
{	if (!has_proto(ist, (index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index)) , OBJ(FLOAT_PROTO))) {					\
		raise_exception(ist, OBJ(TYPE_EXC),									\
				"expected a float in parameter " #index);					\
		return NULL;														\
	}																		\
	(var) = (index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index))->data.f64;	}

#define STRBIN_PARAM(index, var)	/* apr_datum_t var; */					\
{	if (!has_proto(ist, (index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index)), OBJ(STRING_PROTO))) {	\
		raise_exception(ist, OBJ(TYPE_EXC),									\
				"expected a string in parameter " #index);					\
		return NULL;														\
	}																		\
	(var).dptr  = pr2c_strptr(ist, (index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index)));	\
	(var).dsize = pr_strlen((index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index))); }

#define SEQ_NS_PARAM(index, var)	/* obj_p var; */						\
{	if ( has_proto(ist, (index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index)), OBJ(STRING_PROTO)) ||					\
		!has_proto(ist, (index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index)), OBJ(SEQ_PROTO))) {						\
		raise_exception(ist, OBJ(TYPE_EXC),									\
				"expected a Tuple or List in parameter " #index);			\
		return NULL;														\
	}																		\
	(var) = (index > 0 ? parms[((index)-1)*2+1] : list_item(ist, parms[1], -index)); }


#define SEQ_OR_PARAM(index, var)	/* obj_p var; */						\
	if ( has_proto(ist, ITEM, OBJ(STRING_PROTO)) ||					        \
		!has_proto(ist, ITEM, OBJ(SEQ_PROTO))) {						    \
		list = NEW_LIST(1);													\
		list_append(ist, list, ITEM);										\
		(var) = list;														\
	} else																	\
		(var) = ITEM

// FORM_RPARAM: convenience macro for single formal right parameter
// Defines a formal label/default-value list that matches  func(rparam).
// This is the call made in a binary operation such a a+b.  "a" is self and 
// "b" is the rparam.  The "rparam" label doesn't really matter since the 
// call is made positionally.
// see Int.c for example of usage
#define FORM_RPARAM     (list2(ist, SYM(RPARAM), OBJ(NODEF)))

// FORM_PARAM2: same for 2 right params
// example: cmp(a,b)
#define FORM_PARAM2    (list4(ist, SYM(PARAM1), OBJ(NODEF), SYM(PARAM2), OBJ(NODEF)))

// FORM_STARn_PARAM: convenience macros for variable number of formal right parameters
// equivalent to func(*args), func(**kwargs), and func(*args, **kwargs)
#define FORM_STAR_PARAM  (list2(ist, SYM(ARGS),   OBJ(PARAM_STAR)      ))
#define FORM_STAR2_PARAM (list2(ist, SYM(KWARGS), OBJ(PARAM_STAR_STAR) ))
#define FORM_STAR3_PARAM (list4(ist, SYM(ARGS),   OBJ(PARAM_STAR),       \
                                     SYM(KWARGS), OBJ(PARAM_STAR_STAR) ))

// CALL_FUNC: Call a function, either Prothon or C based
// This C function provides a way to call any Prothon or C function from C.
// A attr named func_sym is searched for in self or any proto of self and the function object
// found in that attr is called with the actual parameters in the lbl_val_arr array.
// The lbl_val_arr array holds the actual parameter labels and values (in even and 
// odd positions) and optionally the *list and **dictionary args which are supported by 
// putting OBJ(PARAM_STAR) or OBJ(PARAM_STAR_STAR) in the label (even) position and the list or 
// dictionary object in the value (odd) position.  parm_cnt is the length of the array,
// (2 times the number of actual parameters).
// When super_obj is set, the search for the function starts after the object super_obj
// in the prototype chain.
// An object if always returned, at least OBJ(NONE) if nothing else.  This object will 
// usually have the del_locked bit set so call del_unlock(obj) after you are done with
// the object (see OBJECT LOCKING section below).
// To call a function without looking it up or having a self object (an unbound function 
// call), just pass the function object in the func_sym parameter and leave self NULL.
// NULL may be passed for lbl_val_arr and/or dyn_locals if those features aren't needed.
// Example: "sum=x+y"  =>  sum = call_func(ist, x, sym(ist, "add_"), 2, y_arr, NULL);
//         where y_arr =>  obj_p y_arr[2] = {NULL, y};
obj_p call_func( isp ist, obj_p cont_self, obj_p func_sym, obj_p self,
				 int parm_cnt, obj_p* lbl_val_arr, obj_p super_obj );


//********************** CONVENIENT FUNCTION CALLS ****************************

// CALL_FUNCn: Macros to easily call functions with zero or more params
obj_p call_func1_f(isp ist, obj_p self, obj_p sym, obj_p parm);
obj_p call_func2_f(isp ist, obj_p self, obj_p sym, obj_p parm1, obj_p parm2);
#define call_func0(ist, self, sym)					call_func((ist), (self), (sym), NULL, 0, NULL, NULL)
#define call_func1(ist, self, sym, parm)			call_func1_f((ist), (self), (sym), (parm))
#define call_func2(ist, self, sym, parm1, parm2)	call_func2_f((ist), (self), (sym), (parm1), (parm2))

// AS_STR: Call str_ function on any object
// Get C string representation of any object by calling str_ function on it.
#define as_str(ist, obj) pr2c_strptr(ist, call_func0((ist), (obj), SYM(STR_)))

// COVERS: Macro to determine if a type can be coerced to another type
// Returns true if data type of o1 can represent all data that type of o2 can
// represent.  For example, float type "covers" int type.
// Covering a value may mean that some precision will be lost.
// This test determines when coercion is allowed.  It is up to the prototype
// objects to decide which objects they cover by offering the covers_ function.
#define covers(o1, o2) (call_func1((ist), (o1), SYM(COVERS_), (o2)) == OBJ(PR_TRUE))


//********************************* EXCEPTIONS ******************************************
// Exceptions are raised by calling raise_exception(ist, proto, doc). Proto is a proto
// exception proto object (or NULL to default to OBJ(EXCEPTION)).  Doc is an
// optional string to document the exception cause (stored as doc_).  The exception is
// created and stored in the ist structure only.  It is the programmers job to then 
// return from the function cleanly.  You may call raise_exception more than once, but all calls 
// other than the last will have no effect.
// After calling a function (Prothon or C) with call_func, you can check for an exception
// by calling the except(ist, proto, proto_list) function which matches the Prothon try/accept 
// statement logic. This function will check for a returned exception object against an 
// exception proto proto and/or a list of exception proto proto objects.  The seperate proto
// and proto list parameters are just a convenience so that if you only have one proto you don't
// have to create a list. If an exception occurs and it has one of the protos as a
// proto then the function will return the exception object.  If you leave proto and proto_list 
// both NULL, then it will catch all exception objects.  When this function catches the 
// exception and returns it, the exception object is no longer raised unless you call
// raise_exception again with that object.

void raise_exception(isp ist, obj_p proto_obj, const char *format, ...)
	                       __attribute__((format(printf,3,4)));

obj_p catch_exception(isp ist, obj_p proto, obj_p proto_list);

// EXCEPT: Convenience macro to catch all exceptions and return if exception caught
// example usage:   call_func(...); if_exc_return NULL;
#define if_exc_return	if(ist && ist->exception_obj) return

// CHECK_EXCEPTIONS: print exception stack to stdout and remove exception
obj_p check_exceptions(isp ist);

//***************** LOW-LEVEL OBJECT ACCESS FUNCTIONS **************************

// OBJ_MALLOC_RL, OBJ_REALLOC, & OBJ_FREE:  Memory allocation for objects.
// Each object has one memory space attached for general usage by C code.
// This storage holds the actual data such as integer values, lists, strings, etc.
// When the size is 8 bytes or less, the data is stored in the actual object
// structure.  Larger data up to 63 bytes in stored in a fast allocation area.
// Memory segments larger than 63 bytes use the OS heap. obj_malloc automatically
// chooses which one to use and obj_realloc shuffles between these.  Obj_free should 
// never be needed since the memory manager calls obj_free when it removes an obj.
// WARNING: obj_malloc does not internally lock the object it is modifying.  This is 
// because it is usually used on a new object.  If you use it on a public object, be
// sure to write_lock the object before calling obj_malloc.
void* obj_malloc (isp ist, obj_p proto, obj_p obj, size_t size);
void* obj_realloc(isp ist, obj_p obj, size_t size);
void  obj_free   (isp ist, obj_p obj);

// OBJ_DATAPTR: returns the C ptr to the data area for an object.
#define obj_dataptr(obj)														\
	(                                  !obj ?   NULL            :				\
	( (obj)->data_type == DATA_TYPE_IMMDATA ? &((obj)->data)    :				\
	( (obj)->data_type == DATA_TYPE_DATAPTR ?   (obj)->data.ptr :	NULL ) ) )

// OBJ_DATALEN: returns the data length for an object.
#define obj_datalen(obj)														    \
	(                                  !obj ? 0                     :				\
	( (obj)->data_type == DATA_TYPE_IMMDATA ? (obj)->imm_data_len   :				\
	( (obj)->data_type == DATA_TYPE_DATAPTR ? DATA_SIZE(obj)        :	NULL ) ) )


//************************** OBJECT LOCKING ***********************************
// Prothon allows multiple threads to run at once and even multiple interpreters.
// Data is shared at the object level.  There is only one set of objects that
// all threads share.  Each object has it's own read locks and write lock.
// An object can be read-locked by many threads at once.  When an object has any
// read locks, it cannot be modified at all, it can only be read.  An object can
// be write-locked by only one thread and when it is write-locked there can be no
// read locks.  So when a write_lock(ist, obj) call is made, all read_lock(ist, obj) calls
// are suspended and the write_lock(ist, obj) call is suspended until all the threads
// (that have a read-lock) call read_unlock(ist, obj) so that there are no read-locks.
// Then the write_lock(ist, obj) call succeeds and the thread now has a write-lock
// on the object.  Once it has the write-lock it is free to modify the object all
// it wants since no one can read the object while it is write-locked.  Once the
// writing is finished then write_unlock(ist, obj) is called and the suspended 
// read_lock(ist, obj) calls can proceed and read the newly modified object.
// SUMMARY:  Surround reading of objects with read_lock/read_unlock calls and 
// surround writing of objects with write_lock/write_unlock calls.  A newly created
// object does not have to be protected since no one else has access to it.
// WARNING: Do not try to write-lock an object that you have already read-locked in 
// your own thread as it will lock-up waiting forever for you to read-unlock it.
// NOTE: DLLS have all parameters read-locked automatically before the routines
// are called, so you do not have to read-lock and read-unlock them (although it
// does no harm to add extra matching calls).  Also, the write_lock and write_unlock 
// routines in DLLs handle disabling the read-locks before calling write_locks.  
// See the routine read_unlock_write_lock in object.c.
// Note: If object has immutable bit set, then read_lock call is effectively a no-op
// and write-lock call will cause an exception.
// Note: If read or write lock returns TRUE, then that means there was an error
// and the lock failed because either the caller did not have the permission rights
// for the operation or a write attempt was made on an immutable object or a write
// attempt was made on an object already read locked by the caller.
// When an error is returned, the ist also contains an exception.
int  read_lock    (isp ist, obj_p obj);
void read_unlock  (isp ist, obj_p obj);	
int  write_lock   (isp ist, obj_p obj);		
void write_unlock (isp ist, obj_p obj);	

// XXX_LOCK_YYY:  These macros read or write lock and check for errors
#define if_rdlock(obj)	 if (read_lock(ist, obj)) 
#define rdlock_brk(obj)	 if (read_lock(ist, obj)) break
#define rdlock_rtrn(obj) if (read_lock(ist, obj)) return
#define if_wrlock(obj)	 if (write_lock(ist, obj)) 
#define wrlock_brk(obj)	 if (write_lock(ist, obj)) break
#define wrlock_rtrn(obj) if (write_lock(ist, obj)) return
#define def_write_lock(obj)  read_unlock(ist, obj);      \
	if_wrlock(obj) { read_lock(ist, obj); return NULL; }
#define def_write_unlock(obj)  write_unlock(ist, obj); read_lock(ist, obj)

//************************** IMMUTABLE BIT ***********************************
// Each object has a bit called immutable.  This bit is effectively a write-protect
// bit for the object.  When set the object cannot be modified.  Any attempt
// to write_lock the object with immutable set will cause an exception.  This bit
// also speeds up processing since the read_lock becomes a no_op when this bit
// is set.  This bit is set for many common data types such as Int, String, Etc.
void set_immutable(obj_p obj);		
void clr_immutable(obj_p obj);
int   is_immutable(obj_p obj);

//************************** UNCLONABLE BIT ***********************************
// Each object has a bit called unclonable.  This bit tells the interpreter
// that the object represents a real-world object such as a file, or it
// represents a time-dependent object such as a mutex lock.  When this bit
// is set, then the copy() function in Object knows that this object should
// never be copied.  Systems that store objects such as the Prosist persistance
// storage database also use this bit as an indicator that this object should
// not be stored.  Some objects such as prototypes also set this bit if they
// don't want more than one copy of themself around (this is similar to a
// singleton in a class-based system).
void set_unclonable(obj_p obj);		
void clr_unclonable(obj_p obj);
int   is_unclonable(obj_p obj);

//************************** ARCHIVED BIT ***********************************
// Each object has a bit called archived.  This bit tells whether an object has
// been write-locked (modified) since the last time it was stored via whatever
// perstistance storage system is controlling this bit, such as prosist.  The
// interpreter sets this bit to false when the object is created and when the
// object is write locked and never sets it to true.  The persistence application
// is responsbile for maintaining this bit otherwise.
void set_archived(obj_p obj);		
void clr_archived(obj_p obj);
int   is_archived(obj_p obj);

//************************** DELETE LOCKING ***********************************
// The garbage collection process in the memory manager will delete any object
// that isn't recursively contained in the root globals container.  This means
// that an object freshly created may accidently be deleted immediately before
// it could be stored in a container.  To solve this dilemma, there is a lock bit
// on every object that when set, prevents the memory manager from deleting the
// object.  This bit is set automatically every time an object is created.  When 
// you create an object, you should place it in some container in the root globals
// hierarchy and then call del_unlock(obj) to clear this bit.  You can also set
// the bit manually if you need to remove an object from the hierarchy temporarily.
// WARNING: Leaving this bit set will create a memory leak just like not calling
// free() from a malloc() call in C.
void del_lock  (obj_p obj);		
void del_unlock(obj_p obj);
int is_del_locked(obj_p obj);

//************************** ACCESS PERMISSION ********************************
// Every thread as an access level defined at any one time.  The access levels are
// ACC_GUEST, ACC_USER1, ACC_USER2, & ACC_SYSTEM.  System can read or write any
// object and guest can do very little.  User2 is the normal programming level 
// and main module threads start out at this level.
// Each object has an access level set for read access and write access.  The
// thread must match the read level to read_lock the object or a read access
// exception will be thrown and the the thread level must likewise match the 
// write level to write_lock the object or a write access exception will occur.
// The C code can change any levels at any time, but the programmer should
// follow the access logic rules to keep the security tight.

// SU, UN_SU: get super_user access for thread,  restore normal access
// A temp int variable must be available to save the current level.
// Use this to temporarily access things like Object attributes.
// DO NOT FORGET AND LEAVE ACCESS OPEN.
#define su(save_int)    ((save_int) = ist->access, ist->access = ACC_SYSTEM)
#define un_su(save_int) (ist->access = (save_int))

// XET_OBJ_XXACC: get/set object access
// Get or set read or write access level of an object
// Use constants ACC_GUEST, ACC_USER1, ACC_USER2, & ACC_SYSTEM for access param.
void set_obj_rdacc(obj_p obj, int access);
void set_obj_wracc(obj_p obj, int access);
int get_obj_rdacc(obj_p obj);
int get_obj_wracc(obj_p obj);

// DUMP: dump object store details to debug text file
// Object tree starting at specified object is dumped to the debug text file.
void dump(isp ist, char* dumpfilename, obj_p obj);

#ifdef __cplusplus
}
#endif

//************************** PLATFORM SPECIFIC CONFIGURATIONS *****************
/* PR_ARITHMETIC_RIGHT_SHIFT
 * (from python)
 * C doesn't define whether a right-shift of a signed integer sign-extends
 * or zero-fills.  Here a macro to force sign extension:
 * PR_ARITHMETIC_RIGHT_SHIFT(TYPE, I, J)
 *    Return I >> J, forcing sign extension.
 * Requirements:
 *    I is of basic signed type TYPE (char, short, int, long, or long long).
 *    TYPE is one of char, short, int, long, or long long, although long long
 *    must not be used except on platforms that support it.
 *    J is an integer >= 0 and strictly less than the number of bits in TYPE
 *    (because C doesn't define what happens for J outside that range either).
 * Caution:
 *    I may be evaluated more than once.
 */
// define this only for the Macintosh
#ifdef SIGNED_RIGHT_SHIFT_ZERO_FILLS
#define PR_ARITHMETIC_RIGHT_SHIFT(TYPE, I, J) \
	((I) < 0 ? ~((~(unsigned TYPE)(I)) >> (J)) : (I) >> (J))
#else
#define PR_ARITHMETIC_RIGHT_SHIFT(TYPE, I, J) ((I) >> (J))
#endif


/* HUGE_VAL is supposed to expand to a positive double infinity.  Python
 * uses Py_HUGE_VAL instead because some platforms are broken in this
 * respect.  We used to embed code in pyport.h to try to worm around that,
 * but different platforms are broken in conflicting ways.  If you're on
 * a platform where HUGE_VAL is defined incorrectly, fiddle your Python
 * config to #define Py_HUGE_VAL to something that works on your platform.
 */
//#ifndef HUGE_VAL
//#define HUGE_VAL  ????  
//#endif

/* Py_OVERFLOWED(X)
 * Return 1 iff a libm function overflowed.  Set errno to 0 before calling
 * a libm function, and invoke this macro after, passing the function
 * result.
 * Caution:
 *    This isn't reliable.  C99 no longer requires libm to set errno under
 *	  any exceptional condition, but does require +- HUGE_VAL return
 *	  values on overflow.  A 754 box *probably* maps HUGE_VAL to a
 *	  double infinity, and we're cool if that's so, unless the input
 *	  was an infinity and an infinity is the expected result.  A C89
 *	  system sets errno to ERANGE, so we check for that too.  We're
 *	  out of luck if a C99 754 box doesn't map HUGE_VAL to +Inf, or
 *	  if the returned result is a NaN, or if a C89 box returns HUGE_VAL
 *	  in non-overflow cases.
 *    X is evaluated more than once.
 * Some platforms have better way to spell this, so expect some #ifdef'ery.
 *
 * OpenBSD uses 'isinf()' because a compiler bug on that platform causes
 * the longer macro version to be mis-compiled. This isn't optimal, and 
 * should be removed once a newer compiler is available on that platform.
 * The system that had the failure was running OpenBSD 3.2 on Intel, with
 * gcc 2.95.3.
 *
 * According to Tim's checkin, the FreeBSD systems use isinf() to work 
 * around a FPE bug on that platform.
 */

//  We need to define HUGE_VAL and I don't know what value to use  -- mch
#if defined(__FreeBSD__) || defined(__OpenBSD__)
#define Py_OVERFLOWED(X) isinf(X)
#else
#define Py_OVERFLOWED(X) ((X) != 0.0 && (errno == ERANGE))
#endif
/* #define Py_OVERFLOWED(X) ((X) != 0.0 && (errno == ERANGE ||    \
					 (X) == Py_HUGE_VAL || \
					 (X) == -Py_HUGE_VAL)) */

#endif // PROTHON_H
