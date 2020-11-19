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


// clist.c

#include <string.h>
#include "clist.h"

//********************************* new_clist *********************************
// size 0 -> use default
clist_p new_clist(int initial_size){
	clist_p    list;
	void*		items;
	if(!initial_size) initial_size = DEFAULT_INITIAL_CLIST_SIZE;
	list  = pr_malloc(sizeof(clist_t));
	items = pr_malloc(initial_size*sizeof(clist_item_t));
	list->space = initial_size; 
	list->len   = 0;
	list->items = items;
	return list;
}

//********************************* ins_clist *********************************
clist_p ins_clist(clist_p list, void* item, int index){
	if (index < 0)         index = 0;
	if (index > list->len) index = list->len;
	if (list->len == list->space) {
		list->space *= CLIST_GROWTH_FACTOR;
		list->items = pr_realloc(list->items, list->space * sizeof(clist_item_t));
	}
	if (index < list->len)
		memmove( &list->items[index+1], &list->items[index], 
				 (list->len - index) * sizeof(clist_item_t) );
	list->items[index].ptr = item;
	list->len++;
	return list;
}

//********************************* ins_clist_num *********************************
clist_p ins_clist_num(clist_p list, int num, int index){
	if (index < 0)         index = 0;
	if (index > list->len) index = list->len;
	if (list->len == list->space) {
		list->space *= CLIST_GROWTH_FACTOR;
		list->items = pr_realloc(list->items, list->space * sizeof(clist_item_t));
	}
	if (index < list->len)
		memmove( &list->items[index+1], &list->items[index], 
				 (list->len - index) * sizeof(clist_item_t) );
	list->items[index].num = num;
	list->len++;
	return list;
}

//********************************* free_clist ********************************
void free_clist(clist_p list){
	if (list) {
		pr_free(list->items);
		pr_free(list);
	}
}

//********************************* free_all_in_clist *************************
void free_all_in_clist(clist_p list){
	int i;
	if (list) {
		for(i=0; i < list->len; i++)
			pr_free(list->items[i].ptr);
		pr_free(list->items);
		pr_free(list);
	}
}

//********************************* clist_read_unlock *************************
void clist_read_unlock(isp ist, clist_p list){
	int i;
	for(i=0; i < list->len; i++)
		if (list->items[i].ptr) {
			obj_p obj_debug = (obj_p)(list->items[i].ptr);
			read_unlock(ist, obj_debug);
		}
}

//********************************* clist_in **********************************
int clist_in(clist_p list, void* item) {
	int i;
	for(i=0; i < list->len; i++)
		if (list->items[i].ptr == item) return TRUE;
	return FALSE;
}

//********************************* clist_in_cnt ******************************
int clist_in_cnt(clist_p list, void* item){
	int i, cnt=0;
	for(i=0; i < list->len; i++)
		if (list->items[i].ptr == item) cnt++;
	return cnt;
}

//********************************* clist_pop_item ****************************
int clist_pop_item(clist_p list, void* item) {
	int i;
	for(i=list->len-1; i >= 0; i--)
		if (list->items[i].ptr == item) {
			if (i < list->len-1)
				memmove( list->items+i, list->items+i+1, 
					     (list->len-1 - i) * sizeof(clist_item_t) );
			list->len--;
			return FALSE;
		}
	return TRUE;
}

//********************************* clist_pop_to ******************************
void clist_pop_to(clist_p list, void* item) {
	int i;
	for(i=0; i < list->len; i++)
		if (list->items[i].ptr == item) {
			list->len = i;
			break;
		}
}

//********************************* clist_pop_dups ****************************
void* clist_pop_dups(clist_p list) {
	void* item = list->items[--(list->len)].ptr;
	int i;
	for (i=list->len-1; i >= 0; i--) 
		if (list->items[i].ptr == item) {
			if (i < list->len-1)
				memmove( &(list->items[i]), &(list->items[i+1]), 
						 (list->len-1-i) * sizeof(clist_item_t) );
			list->len--;
		}
	return item;
}
