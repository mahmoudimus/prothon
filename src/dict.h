
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

// dict.h

#define DEFAULT_INITIAL_DICT_SIZE  8
#define DICT_MAX_PERCENT_FULL  		66
#define DICT_PAD_PERCENT_FACTOR		300

typedef struct {
	data_size_t		size_bytes;
	size_t			size;
	size_t			len;
} dict_hdr_t;

typedef struct {
	i32_t	hash;
	obj_p	key;
	obj_p	value;
} dict_entry_t;

typedef union {
	dict_hdr_t		hdr;
	dict_entry_t 	entry;
} dict_t;

typedef dict_t*  dict_p;

#define DICT_HDR_INDEX			0
#define DICT_OVERHEAD			1

#define dict_sizeof(n)			(((n)+DICT_OVERHEAD) * sizeof(dict_t))

#define dict_d(dict_obj)		((dict_p)((dict_obj)->data.ptr))
#define dictsize_bytes(dict)	(((dict_p)(dict))[DICT_HDR_INDEX].hdr.size_bytes)
#define dictsize(dict)			(((dict_p)(dict))[DICT_HDR_INDEX].hdr.size)
#define dictlen(dict)			(((dict_p)(dict))[DICT_HDR_INDEX].hdr.len)
#define dictptr(dict, i)		( ((dict_p)(dict)) + DICT_OVERHEAD + ((i)%dictsize(dict)) )
#define dicthash(dict,i)		(dictptr(((dict_p)(dict)),i)->entry.hash)
#define dictkey(dict, i)		(dictptr(((dict_p)(dict)),i)->entry.key)
#define dictitem(dict,i)		(dictptr(((dict_p)(dict)),i)->entry.value)

