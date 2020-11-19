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


// prList.h

#define DEFAULT_INITIAL_LIST_SIZE	8
#define LIST_GROWTH_FACTOR			2

#define list_sizeof(n)	((data_size_t)(sizeof(list_hdr_t)+((n)*sizeof(obj_p))))

typedef struct {
	data_size_t	size;
	size_t		len;
} list_hdr_t;

typedef struct {
	list_hdr_t  hdr;
	obj_p		item[];
} list_t;

typedef list_t* list_p;

#define lstpsetsize(list_ptr,n) ((((list_p)(list_ptr))->hdr.size)=list_sizeof(n))
#define lstpsize(list_ptr)		(((((list_p)(list_ptr))->hdr.size)-sizeof(list_hdr_t))/sizeof(obj_p))
#define lstplen(list_ptr)		(((list_p)(list_ptr))->hdr.len)
#define lstpitem(list_ptr,i)	(((list_p)(list_ptr))->item[i])

#define listsetsize(list_obj,n) lstpsetsize((list_obj)->data.ptr, n)
#define listsize(list_obj)		lstpsize((list_obj)->data.ptr)
#define listlen(list_obj)		lstplen((list_obj)->data.ptr)
#define listitem(list_obj,i)	lstpitem((list_obj)->data.ptr,i)

