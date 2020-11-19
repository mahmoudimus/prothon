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


// clist.h

#ifndef CLIST_H
#define CLIST_H

#include "object.h"

#define OBJ_LIST_MIN_SIZE			10
#define OBJ_LIST_GROWTH_FACTOR      2

#define DEFAULT_INITIAL_CLIST_SIZE	16
#define CLIST_GROWTH_FACTOR			2
#define CLIST_APPEND				0x7FFFFFFF

typedef union {
	int   num;
	void* ptr;
} clist_item_t;

typedef struct {
	int				space;
	int				len;
	clist_item_t*	items;
} clist_t;

typedef clist_t* clist_p;

clist_p new_clist(int initial_size);
clist_p ins_clist(clist_p list, void* item, int index);
clist_p ins_clist_num(clist_p list, int num, int index);
void free_clist(clist_p list);
void free_all_in_clist(clist_p list);
void clist_read_unlock(isp ist, clist_p list);
int clist_in(clist_p list, void* item);
int clist_in_cnt(clist_p list, void* item);
int clist_pop_item(clist_p list, void* item);
void clist_pop_to(clist_p list, void* item);
void* clist_pop_dups(clist_p list);

#define new_clist_1(item)				ins_clist(new_clist(2), (item), 0)
#define new_clist_2(item1, item2)		ins_clist(new_clist_1(item1), (item2), CLIST_APPEND)

#define clist_len(list)					(((clist_p)(list))->len)
#define clist_item(list, index)			(((clist_p)(list))->items[index].ptr)
#define clist_num(list, index)			(((clist_p)(list))->items[index].num)
#define clist_tos(list)					((list)->items[(list)->len-1].ptr)
#define clist_tos_num(list)				((list)->items[(list)->len-1].num)
#define clist_append(list, item)		ins_clist(list, item, CLIST_APPEND)
#define clist_append_num(list, num)		ins_clist_num(list, num, CLIST_APPEND)
#define clist_push(list, item)			ins_clist(list, item, CLIST_APPEND)
#define clist_push_num(list, num)		ins_clist_num(list, num, CLIST_APPEND)
#define clist_pop(list)					((list)->items[--(list)->len].ptr)				
#define clist_pop_num(list)				((list)->items[--(list)->len].num)				

#endif // #define CLIST_H
