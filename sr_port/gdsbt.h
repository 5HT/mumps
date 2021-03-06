/****************************************************************
 *								*
 *	Copyright 2001, 2008 Fidelity Information Services, Inc	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#ifndef GDSBT_H
#define GDSBT_H
/* this requires gdsroot.h */

#include <sys/types.h>
#ifdef MUTEX_MSEM_WAKE
#ifdef POSIX_MSEM
#  include <semaphore.h>
#else
#  include <sys/mman.h>
#endif
#endif

#ifdef VMS
#include <ssdef.h>	/* for SS$_WASSET */
#include "ast.h"	/* for ENABLE/DISABLE */
#endif

#include "gvstats_rec.h"

#define CR_NOTVALID (-1L)

#define WC_MAX_BUFFS 64*1024
#define WC_DEF_BUFFS 128
#define WC_MIN_BUFFS 64

#define MAX_LOCK_SPACE 65536 	/* need to change these whenever global directory defaults change */
#define MIN_LOCK_SPACE 10

#define MAX_REL_NAME	36
#define MAX_MCNAMELEN   256

#define GDS_LABEL_SZ 	12

#define MAX_DB_WTSTARTS		2			/* Max number of "flush-timer driven" simultaneous writers in wcs_wtstart */
#define MAX_WTSTART_PID_SLOTS	4 * MAX_DB_WTSTARTS	/* Max number of PIDs for wcs_wtstart to save */

#define BT_FACTOR(X) (X)
#define FLUSH_FACTOR(X) ((X)-(X)/16)
#define BT_QUEHEAD (-2)
#define BT_NOTVALID (-1)
#define BT_MAXRETRY 3
#define BT_SIZE(X) ((((sgmnt_data_ptr_t)X)->bt_buckets + 1 + ((sgmnt_data_ptr_t)X)->n_bts) * sizeof(bt_rec))
			/* parameter is *sgmnt_data*/
			/* note that the + 1 above is for the th_queue head which falls between the hash table and the */
			/* actual bts */
#define HEADER_UPDATE_COUNT 1024

typedef struct
{
	trans_num	curr_tn;
	trans_num	early_tn;
	trans_num	last_mm_sync;		/* Last tn where a full mm sync was done */
	trans_num	header_open_tn;		/* Tn to be compared against jnl tn on open */
	trans_num	mm_tn;			/* Used to see if CCP must update master map */
	uint4		lock_sequence;		/* Used to see if CCP must update lock section */
	uint4		ccp_jnl_filesize;	/* Passes size of journal file if extended */
	volatile uint4	total_blks;		/* Placed here so can be passed to other machines on cluster */
	volatile uint4	free_blocks;
} th_index;

typedef struct
{
	struct
	{
		int4 		fl;
		int4		bl;	/* self-relative queue entry */
	} blkque, tnque;		/* for block number hash, lru queue */
	trans_num	tn;		/* transaction # #*/
	trans_num	killtn;		/* last transaction when this block was updated as part of an M-kill */
	block_id	blk;		/* block #*/
	int4		cache_index;
	bool		flushing;	/* buffer is being flushed after a machine switch on a cluster */
	char		filler[3];
	int4		filler_int4;	/* maintain 8 byte alignment */
} bt_rec;				/* block table record */

/* This structure is used to access the transaction queue.  It points at all but the
   first two longwords of a bt_rec.  CAUTION:  there is no such thing as a queue of
   th_recs, they are always bt_recs, and the extra two longwords are always there */

typedef struct
{
	struct
	{
		int4 		fl;
		int4		bl;
	} tnque;
	trans_num	tn;
	trans_num	killtn;		/* last transaction when this block was updated as part of an M-kill */
	block_id	blk;
	int4		cache_index;
	bool		flushing;
	char		filler[3];
	int4		filler_int4;	/* maintain 8 byte alignment */
} th_rec;

/* This structure is used to maintain all cache records.  The BT queue contains
   a history of those blocks that have been updated.	*/


/*
 *	Definitions for GT.M Mutex Control
 *
 *	See MUTEX.MAR for VMS functional implementation
 */

typedef struct
{
	struct
	{
		int4	fl,
			bl;
	}		que;
	int4		pid;
	void		*super_crit;
#if defined(UNIX)
	/*
	 * If the following fields are to be made part of VMS too, change
	 * size of mutex_que_entry in mutex.mar (lines 118, 152).
	 * Make sure that the size of mutex_que_entry is a multiple of 8 bytes
	 * for quadword alignment requirements of remqhi and insqti.
	 */
	int4		mutex_wake_instance;
	int4		filler1; /* for dword alignment 		 */
#ifdef MUTEX_MSEM_WAKE
#ifdef POSIX_MSEM
	sem_t		mutex_wake_msem; /* Not two ints .. somewhat larger */
#else
	msemaphore	mutex_wake_msem; /* Two ints (incidentally two int4s) */
#endif

#endif
#endif
} mutex_que_entry;

typedef struct
{
	struct
	{
		int4	fl,
			bl;
	}		que;
	global_latch_t	latch;
} mutex_que_head;

typedef struct
{
#if defined(UNIX)
	global_latch_t	semaphore;
	CACHELINE_PAD(sizeof(global_latch_t), 1)
	latch_t		crashcnt;
	int4		filler1;	/* for alignment */
	global_latch_t	crashcnt_latch;
	CACHELINE_PAD(sizeof(latch_t) + sizeof(latch_t) + sizeof(global_latch_t), 2)
	latch_t		queslots;
	int4		filler2;	/* for alignment */
	CACHELINE_PAD(sizeof(latch_t) + sizeof(latch_t), 3)
	mutex_que_head	prochead;
	CACHELINE_PAD(sizeof(mutex_que_head), 4)
	mutex_que_head	freehead;
#elif defined(VMS)
	short		semaphore;
	unsigned short	wrtpnd;
	short		crashcnt,
			queslots;
#ifdef __alpha
/* use constant instead of defining CACHELINE_SIZE since we do not want to affect other structures
      the 64 must match padding in mutex.mar and mutex_stoprel.mar */
        char            filler1[64 - sizeof(short)*4];
#endif
        mutex_que_entry prochead;
#ifdef __alpha
        char            filler2[64 - sizeof(mutex_que_entry)];
#endif
        mutex_que_entry freehead;
#else
#error UNSUPPORTED PLATFORM
#endif
} mutex_struct;

typedef struct { /* keep this structure and member offsets defined in sr_avms/mutex.mar in sync */
	int4	mutex_hard_spin_count;
	int4	mutex_sleep_spin_count;
	int4	mutex_spin_sleep_mask;
	int4	filler1;
} mutex_spin_parms_struct;

enum crit_ops
{	crit_ops_gw = 1,	/* grab [write] crit */
	crit_ops_rw,		/* rel [write] crit */
	crit_ops_nocrit		/* did a rel_crit when now_crit flag was off */
};

typedef struct
{
	caddr_t		call_from;
	enum crit_ops	crit_act;
	int4		epid;
} crit_trace;

#define OP_LOCK_SIZE	4

/* Structure to hold lock history */
typedef struct
{
	sm_int_ptr_t	lock_addr;		/* Address of actual lock */
	caddr_t		lock_callr;		/* Address of (un)locker */
	int4		lock_pid;		/* Process id of (un)locker */
	int4		loop_cnt;		/* iteration count of lock retry loop */
	char		lock_op[OP_LOCK_SIZE];	/* Operation performed (either OBTN or RLSE) */
} lockhist;

#define	CRIT_OPS_ARRAY_SIZE	512
#define	LOCKHIST_ARRAY_SIZE	512
#define	SECSHR_OPS_ARRAY_SIZE	1023		/* 1 less than 1K to accommodate the variable secshr_ops_index */

/* SECSHR_ACCOUNTING macro assumes csa->nl is dereferencible and does accounting if variable "do_accounting" is set to TRUE */
#define		SECSHR_ACCOUNTING(value)								\
{													\
	if (do_accounting)										\
	{												\
		if (csa->nl->secshr_ops_index < SECSHR_OPS_ARRAY_SIZE)					\
			csa->nl->secshr_ops_array[csa->nl->secshr_ops_index] = (INTPTR_T)(value);	\
		csa->nl->secshr_ops_index++;								\
	}												\
}

/* Mapped space local to each node on the cluster */
typedef struct node_local_struct
{
	unsigned char   label[GDS_LABEL_SZ];			/* 12	signature for GDS shared memory */
	unsigned char	fname[MAX_FN_LEN + 1];			/* 256	filename of corresponding database */
	char		now_running[MAX_REL_NAME];		/* 36	current active GT.M version stamp */
	char		machine_name[MAX_MCNAMELEN];		/* 256	machine name for clustering */
	sm_off_t	bt_header_off;				/* (QW alignment) offset to hash table */
	sm_off_t	bt_base_off;				/* bt first entry */
	sm_off_t	th_base_off;
	sm_off_t	cache_off;
	sm_off_t	cur_lru_cache_rec_off;			/* current LRU cache_rec pointer offset */
	sm_off_t	critical;
	sm_off_t	jnl_buff;
	sm_off_t	shmpool_buffer;				/* Shared memory buffer pool area */
	sm_off_t	lock_addrs;
	sm_off_t	hdr;					/* Offset to file-header (BG mode ONLY!) */
	volatile int4	in_crit;
	int4		in_reinit;
	unsigned short	ccp_cycle;
	unsigned short	filler;					/* Align for ccp_cycle. Not changing to int
								   as that would perturb to many things at this point */
	boolean_t	ccp_crit_blocked;
	int4		ccp_state;
	boolean_t	ccp_jnl_closed;
	boolean_t	glob_sec_init;
	uint4		wtstart_pid[MAX_WTSTART_PID_SLOTS];	/* Maintain pids of wcs_wtstart processes */
	int4		filler8_int;				/* 8-byte alignment filler */
	global_latch_t	wc_var_lock;                            /* latch used for access to various wc_* ref counters */
	CACHELINE_PAD(sizeof(global_latch_t), 1)		/* Keep these two latches in separate cache lines */
	global_latch_t	db_latch;                               /* latch for interlocking on hppa and tandem */
	CACHELINE_PAD(sizeof(global_latch_t), 2)
	int4		cache_hits;
	int4		wc_in_free;                             /* number of write cache records in free queue */
	/* All the counters below (declared using CNTR4DCL are 4-byte counters. We would like to keep them in separate
	 * cachelines on load-lock/store-conditional platforms particularly and other platforms too just to be safe.
	 */
	volatile CNTR4DCL(wcs_timers, 1);			/* number of write cache timers in use - 1 */
	CACHELINE_PAD(4, 3)
	volatile CNTR4DCL(wcs_active_lvl, 2);			/* number of entries in active queue */
	CACHELINE_PAD(4, 4)
	volatile CNTR4DCL(wcs_staleness, 3);
	CACHELINE_PAD(4, 5)
	volatile CNTR4DCL(ref_cnt, 4);				/* reference count. How many people are using the database */
	CACHELINE_PAD(4, 6)
	volatile CNTR4DCL(intent_wtstart, 5);			/* Count of processes that INTEND to enter wcs_wtstart code */
	CACHELINE_PAD(4, 7)
	volatile CNTR4DCL(in_wtstart, 6);			/* Count of processes that are INSIDE wcs_wtstart code */
	CACHELINE_PAD(4, 8)
	volatile CNTR4DCL(wcs_phase2_commit_pidcnt, 7);		/* number of processes actively finishing phase2 commit */
	CACHELINE_PAD(4, 9)
	int4            mm_extender_pid;			/* pid of the process executing gdsfilext in MM mode */
	int4            highest_lbm_blk_changed;                /* Records highest local bit map block that
									changed so we know how much of master bit
									map to write out. Modified only under crit */
	int4		nbb;                                    /* Next backup block -- for online backup */
	int4		lockhist_idx;				/* (DW alignment) "circular" index into lockhists array */
	int4		crit_ops_index;				/* "circular" index into crit_ops_array */
	lockhist	lockhists[LOCKHIST_ARRAY_SIZE];		/* Keep lock histories here */
	crit_trace	crit_ops_array[CRIT_OPS_ARRAY_SIZE];	/* space for CRIT_TRACE macro to record info */
	unique_file_id	unique_id;
	uint4		owner_node;
	volatile int4   wcsflu_pid;				/* pid of the process executing wcs_flu in BG mode */
	int4		creation_date_time4;			/* Lower order 4-bytes of database's creation time to be
								 * compared at sm attach time */
	int4		inhibit_kills;				/* inhibit new KILLs while MUPIP BACKUP, INTEG or FREEZE are
					 			 * waiting for kill-in-progress to become zero
					 			 */
	boolean_t	remove_shm;				/* can this shm be removed by the last process to rundown */
	union
	{
		gds_file_id	jnl_file_id;  	/* needed on UNIX to hold space */
		unix_file_id	u;		/* from gdsroot.h even for VMS */
	} jnl_file;	/* Note that in versions before V4.3-001B, "jnl_file" used to be a member of sgmnt_data.
			 * Now it is a filler there and rightly used here since it is non-zero only when shared memory is active.
			 */
	boolean_t	donotflush_dbjnl; /* whether database and journal can be flushed to disk or not (TRUE for mupip recover) */
	int4		n_pre_read;
	char		replinstfilename[MAX_FN_LEN + 1];/* 256 : Name of the replication instance file corresponding to this db.
							  *       In VMS, this is the name of the corresponding global directory.
							  */
   	int4		secshr_ops_index;
   	INTPTR_T	secshr_ops_array[SECSHR_OPS_ARRAY_SIZE]; /* taking up 4K(on 32-bit platform) and 8K(on 64-bit platforms) */
	gvstats_rec_t	gvstats_rec;
} node_local;

#ifdef DEBUG
/* The following macro does not use a separate semaphore to protect its maintenance of the shared memory
 * value crit_ops_index (which would complicate precisely the situation it was created to examine) therefore,
 * in order to to maximize the chances of gathering meaningful data, it seems better placed after grab_crit
 * and before rel_crit. Also we will increment the index first and cache it so we can shorten our exposure window.
 */
#define CRIT_TRACE(X)								\
{										\
	int4			coidx;						\
	node_local_ptr_t	cnl;						\
	boolean_t		in_ast;						\
	unsigned int		ast_status;					\
										\
	if (csa && csa->nl)							\
	{									\
		cnl = csa->nl;							\
		assert(NULL != (node_local_ptr_t)cnl);				\
		coidx = ++cnl->crit_ops_index;					\
		if (CRIT_OPS_ARRAY_SIZE <= coidx)				\
			coidx = cnl->crit_ops_index = 0;			\
		VMS_ONLY(							\
			in_ast = lib$ast_in_prog();				\
			if (!in_ast)						\
				ast_status = sys$setast(DISABLE);		\
		)								\
		cnl->crit_ops_array[coidx].call_from = (caddr_t)caller_id();	\
		VMS_ONLY(							\
			if ((!in_ast) && (SS$_WASSET == ast_status))		\
				sys$setast(ENABLE);				\
		)								\
		cnl->crit_ops_array[coidx].epid = process_id;			\
		cnl->crit_ops_array[coidx].crit_act = (X);			\
	}									\
}

/* The following macro checks that curr_tn and early_tn are equal right before beginning a transaction commit.
 * The only exception we know of is if a process in the midst of commit had been killed (kill -9 or STOP/ID)
 * after having incremented early_tn but before it finished the commit (and therefore incremented curr_tn).
 * In that case another process that did a rundown (and executed secshr_db_clnup) at around the same time
 * could have cleaned up the CRIT lock (sensing that the crit holder pid is no longer alive) making the crit
 * lock available for other processes. To check if that is the case, we need to go back the crit_ops_array and check that
 *	a) the most recent crit operation was a grab crit done by the current pid (crit_act == crit_ops_gw) AND
 *	b) the immediately previous crit operation should NOT be a release crit crit_ops_rw but instead should be a crit_ops_gw
 *	c) there are two exceptions to this and they are
 *		(i) that there could be one or more crit_ops_nocrit actions from processes that tried releasing crit
 *			even though they dont own it (cases we know of are in gds_rundown and in t_end/tp_tend if
 *			t_commit_cleanup completes the transaction after a mid-commit error).
 *		(ii) there could be one or more crit_ops_gw/crit_ops_rw pair of operations by a pid in between.
 */
#define	ASSERT_CURR_TN_EQUALS_EARLY_TN(csa, currtn)						\
{												\
	GBLREF	uint4 			process_id;						\
												\
	assert((currtn) == csa->ti->curr_tn);							\
	if (csa->ti->early_tn != (currtn))							\
	{											\
		int4			coidx, lcnt;						\
		node_local_ptr_t	cnl;							\
		uint4			expect_gw_pid = 0;					\
												\
		cnl = csa->nl;									\
		assert(NULL != (node_local_ptr_t)cnl);						\
		coidx = cnl->crit_ops_index;							\
		assert(CRIT_OPS_ARRAY_SIZE > coidx);						\
		assert(crit_ops_gw == cnl->crit_ops_array[coidx].crit_act);			\
		assert(process_id == cnl->crit_ops_array[coidx].epid);				\
		for (lcnt = 0; CRIT_OPS_ARRAY_SIZE > lcnt; lcnt++)				\
		{										\
			if (coidx)								\
				coidx--;							\
			else									\
				coidx = CRIT_OPS_ARRAY_SIZE - 1;				\
			if (crit_ops_nocrit == cnl->crit_ops_array[coidx].crit_act)		\
				continue;							\
			if (crit_ops_rw == cnl->crit_ops_array[coidx].crit_act)			\
			{									\
				assert(0 == expect_gw_pid);					\
				expect_gw_pid = cnl->crit_ops_array[coidx].epid;		\
			} else if (crit_ops_gw == cnl->crit_ops_array[coidx].crit_act)		\
			{									\
				if (!expect_gw_pid)						\
					break;	/* found lone grab-crit */			\
				assert(expect_gw_pid == cnl->crit_ops_array[coidx].epid);	\
				expect_gw_pid = 0;/* found paired grab-crit. continue search */	\
			}									\
		}										\
		assert(CRIT_OPS_ARRAY_SIZE > lcnt); /* assert if did not find lone grab-crit */	\
	}											\
}

/*
 * The following macro places lock history entries in an array for debugging.
 * NOTE: Users of this macro, set either of the following prior to using this macro.
 * 	 (i) gv_cur_region to the region whose history we are storing.
 * 	(ii) global variable "locknl" to correspond to the node-local of the region whose history we are storing.
 * If "locknl" is non-NULL, it is used to store the lock history. If not only then is gv_cur_region used.
 */
#define LOCK_HIST(OP, LOC, ID, CNT)					\
{									\
	GBLREF	node_local_ptr_t	locknl;				\
									\
	int			lockidx;				\
	node_local_ptr_t	lcknl;					\
									\
	if (NULL == locknl)						\
	{								\
		assert(NULL != gv_cur_region);				\
		lcknl = FILE_INFO(gv_cur_region)->s_addrs.nl;		\
		assert(NULL != lcknl);					\
	} else								\
		lcknl = locknl;						\
	lockidx = ++lcknl->lockhist_idx;				\
	if (LOCKHIST_ARRAY_SIZE <= lockidx)				\
		lockidx = lcknl->lockhist_idx = 0;			\
	GET_LONGP(&lcknl->lockhists[lockidx].lock_op[0], (OP));		\
	lcknl->lockhists[lockidx].lock_addr = (sm_int_ptr_t)(LOC);	\
	lcknl->lockhists[lockidx].lock_callr = (caddr_t)caller_id();	\
	lcknl->lockhists[lockidx].lock_pid = (int4)(ID);		\
	lcknl->lockhists[lockidx].loop_cnt = (int4)(CNT);		\
}

#define DUMP_LOCKHIST() dump_lockhist()
#else
#define CRIT_TRACE(X)
#define	ASSERT_CURR_TN_EQUALS_EARLY_TN(csa, currtn)
#define LOCK_HIST(OP, LOC, ID, CNT)
#define DUMP_LOCKHIST()
#endif

#define BT_NOT_ALIGNED(bt, bt_base)		(!IS_PTR_ALIGNED((bt), (bt_base), sizeof(bt_rec)))
#define BT_NOT_IN_RANGE(bt, bt_lo, bt_hi)	(!IS_PTR_IN_RANGE((bt), (bt_lo), (bt_hi)))

#define NUM_CRIT_ENTRY		1024
#define CRIT_SPACE		(NUM_CRIT_ENTRY * sizeof(mutex_que_entry) + sizeof(mutex_struct))
#define NODE_LOCAL_SIZE		(ROUND_UP(sizeof(node_local), OS_PAGE_SIZE))
#define NODE_LOCAL_SPACE	(ROUND_UP(CRIT_SPACE + NODE_LOCAL_SIZE, OS_PAGE_SIZE))
/* In order for gtmsecshr not to pull in OTS library, NODE_LOCAL_SIZE_DBS is used in secshr_db_clnup instead of NODE_LOCAL_SIZE */
#define NODE_LOCAL_SIZE_DBS	(ROUND_UP(sizeof(node_local), DISK_BLOCK_SIZE))

/* Define pointer types for above structures that may be in shared memory and need 64
   bit pointers. */
#ifdef DB64
# ifdef __osf__
#  pragma pointer_size(save)
#  pragma pointer_size(long)
# else
#  error UNSUPPORTED PLATFORM
# endif
#endif

typedef bt_rec	*bt_rec_ptr_t;
typedef th_rec	*th_rec_ptr_t;
typedef th_index *th_index_ptr_t;
typedef mutex_struct *mutex_struct_ptr_t;
typedef mutex_spin_parms_struct *mutex_spin_parms_ptr_t;
typedef mutex_que_entry	*mutex_que_entry_ptr_t;
typedef node_local *node_local_ptr_t;

#ifdef DB64
# ifdef __osf__
#  pragma pointer_size(restore)
# endif
#endif

#include "cdb_sc.h"

bt_rec_ptr_t bt_get(int4 block);
void dump_lockhist(void);
void wait_for_block_flush(bt_rec *bt, block_id block);

#endif
