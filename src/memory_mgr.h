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


// memory_mgr.h

#ifndef MEMORY_MGR_H
#define MEMORY_MGR_H

#include "object.h"
#include "clist.h"
#include <time.h>

#include <apr_thread_proc.h>

#define MEMMGR_TIMEOUT	(200/PAUSE_MS)  /* 200 ms */

#define	FATAL_TIMEOUT_SECS	5
#define TIMEOUTS_PER_SEC	10
#define	FATAL_TRIES			FATAL_TIMEOUT_SECS*TIMEOUTS_PER_SEC
#define TIMEOUT				(clock()+CLOCKS_PER_SEC/TIMEOUTS_PER_SEC)

#define MMERR_NORMAL_TERMINATION	((obj_p) 1)
#define MMERR_TIME_OUT				((obj_p) 2)
#define mmerr(obj)					(!obj && ((int)(i64_t)obj) < 3)

#define MM_PHASE_CLEARING_MARKS	0
#define MM_PHASE_MARKING		1
#define MM_PHASE_DELETING		2

extern int mem_mgr_phase;
extern int mem_mgr_req_termination;
extern int mem_mgr_terminated;

void *mem_mgr_thread(apr_thread_t *handle, void *dummy);

extern clist_p  to_be_scanned_list;
extern pr_lock_t to_be_scanned_list_lock;

extern i64_t hi_mem_boundary;
extern i64_t lo_mem_boundary;

void stop_mem_mgr();

#endif // #define MEMORY_MGR_H
