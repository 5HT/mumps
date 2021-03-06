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
#include "mdef.h"
#ifndef JNL_H_INCLUDED
#define JNL_H_INCLUDED

#ifndef JNLSP_H_INCLUDED
#include "jnlsp.h"
#endif

#define TID_STR_SIZE		8
#define JPV_LEN_NODE		16
#define JPV_LEN_USER		12
#define JPV_LEN_PRCNAM		16
#define JPV_LEN_TERMINAL	15

/* Whenever JNL_LABEL_TEXT changes, also change the following
 * 	1) Update JNL_VER_THIS
 * 	2) Add REPL_JNL_Vxx enum to repl_jnl_t typedef in repl_filter.h
 * 	3) Add one entry to jnl2filterfmt array in repl_filter.c
 * 	4) Add a row and column to repl_internal_filter array in repl_filter.c.
 * If the FILTER format is also changing, then do the following as well
 * 	5) Add REPL_FILTER_Vxx enum to repl_filter_t typedef in repl_filter.h
 * Also need to follow a set of directions (yet to be written 12/7/2000 -- nars) in case a new set of filters need to be written.
 */
#define JNL_LABEL_TEXT		"GDSJNL18" /* update JNL_VER_THIS and repl_internal_filter array if you update JNL_LABEL_TEXT */
#define JNL_VER_THIS		18
#define JNL_VER_EARLIEST_REPL	12 	   /* from GDSJNL12 (V4.3-000) */
#define	ALIGN_KEY		0xdeadbeef

#define JNL_ALLOC_DEF		100
#define JNL_ALLOC_MIN		10

/*	JNL_BUFFER_MIN	database block size / 512 + 1	*/
#define JNL_BUFFER_MAX		32768	/* # of 512-byte blocks = 16Mb journal buffer size */

/*	JNL_EXTEND_DEF	allocation size / 10
#define JNL_EXTEND_DEF_PERC	0.1
* 	Uncomment this section when code is ready to use extension = 10% of allocation
*/

#define JNL_EXTEND_MIN		0
#define JNL_EXTEND_DEF		100
#define JNL_EXTEND_MAX		65535
#define JNL_MIN_WRITE		32768
#define JNL_MAX_WRITE		65536
/* FE was changed to EB because, the bit pattern there seems to vary more than the one for "FE".
 * Also a research in ELWOOD journal file showed that "EB" was one of the few patterns that had the least occurrences */
#define JNL_REC_SUFFIX_CODE	0xEB
#define	JNL_WRT_START_MODULUS	512
#define JNL_WRT_START_MASK	~(JNL_WRT_START_MODULUS - 1)
#define	JNL_WRT_END_MODULUS	8
#define JNL_WRT_END_MASK	~(JNL_WRT_END_MODULUS - 1)
#define	JNL_MIN_ALIGNSIZE	(1 <<  5)	/*      32 disk blocks effectively  16K alignsize */
#define JNL_DEF_ALIGNSIZE	(1 <<  7)	/*     128 disk blocks effectively  64K alignsize */
#define	JNL_MAX_ALIGNSIZE	(1 << 22)	/* 4194304 disk blocks effectively   2G alignsize */
#define JNL_REC_START_BNDRY	8
#define MAX_LOGI_JNL_REC_SIZE	(MAX_DB_BLK_SIZE)			  /* maximum logical journal record size */
#define	MAX_JNL_REC_SIZE	(MAX_LOGI_JNL_REC_SIZE + DISK_BLOCK_SIZE) /* one more disk-block for PBLK record header/footer */

#define ALIGNMENT_SIZE		DISK_BLOCK_SIZE	/* For direct I/O, buffers need to be aligned on DISK_BLOCK_SIZE boundaries */

#define MIN_YIELD_LIMIT		0
#define MAX_YIELD_LIMIT		2048
#define DEFAULT_YIELD_LIMIT	8

/* Have a minimum jnl-file-auto-switch-limit of 128 align boundaries (currently each align boundary is 16K) */
#define	JNL_AUTOSWITCHLIMIT_MIN	(128 * JNL_MIN_ALIGNSIZE)
#define	JNL_AUTOSWITCHLIMIT_DEF	8388600	/* Instead of 8388607 it is adjusted for default allocation = extension = 100 */

/* options (sizeof(char)) to wcs_flu() (currently flush_hdr, write_epoch, sync_epoch) are bit-wise ored */
#define	WCSFLU_NONE		 0
#define	WCSFLU_FLUSH_HDR	 1
#define	WCSFLU_WRITE_EPOCH	 2
#define	WCSFLU_SYNC_EPOCH	 4
#define	WCSFLU_FSYNC_DB		 8	/* Currently used only in Unix wcs_flu() */
#define	WCSFLU_IN_COMMIT	16	/* Set if caller is t_end or tp_tend. See wcs_flu for explanation of when this is set */

/* EPOCHs are written unconditionally in Unix (assuming jnl is ON) while they are written only for BEFORE_IMAGE in VMS */
#define JNL_HAS_EPOCH(jnlfile)  UNIX_ONLY(TRUE) VMS_ONLY(jnlfile->before_images)

#ifdef DEBUG
#define	DEFAULT_EPOCH_INTERVAL_IN_SECONDS	30 /* exercise epoch-syncing code relatively more often in DBG */
#else
#define	DEFAULT_EPOCH_INTERVAL_IN_SECONDS	300
#endif

#define DEFAULT_EPOCH_INTERVAL	SECOND2EPOCH_SECOND(DEFAULT_EPOCH_INTERVAL_IN_SECONDS) /* ***MUST*** include math.h for VMS */

#define	MAX_EPOCH_INTERVAL	32767	/* in seconds. Amounts to nearly 10 hours. Don't want to keep db stale so long */

#define JNL_ENABLED(X)		((X)->jnl_state == jnl_open)		/* If TRUE, journal records are to be written */
#define JNL_ALLOWED(X)		((X)->jnl_state != jnl_notallowed)	/* If TRUE, journaling is allowed for the file */
#define REPL_ENABLED(X)		((X)->repl_state == repl_open)		/* If TRUE, replication records are to be written */
#define REPL_WAS_ENABLED(X)	((X)->repl_state == repl_was_open) /* If TRUE, replication is now closed, but was open earlier */
									/* In this state, replication records are not written */
#define REPL_ALLOWED(X)		((X)->repl_state != repl_closed)	/* If TRUE, replication records are/were written */

/* Logical records should be written if journaling is enabled in the region OR if replication state is WAS_ON (repl_was_open).
 * In the former case, the journal records will be written to the journal pool, journal buffer and journal file.
 * In the latter case, the journal records will be written to the journal pool but not to the journal buffer and journal file.
 * All code that generates logical journal records should use the below macro instead of JNL_ENABLED macro.
 * Note that replication does not care about non-logical records (PBLK/AIMG/INCTN etc.) and hence code that generates them does
 * not need to (and should not) use this macro.
 */
#define	JNL_WRITE_LOGICAL_RECS(X)	(JNL_ENABLED(X) || REPL_WAS_ENABLED(X))

/* The following macro should be used to invoke the function "jnl_write" for any logical record. This macro
 * checks if journaling is enabled and if so invokes "jnl_write" else it invokes "jnl_write_poolonly" which
 * writes only to the journal pool.
 */
#define	JNL_WRITE_APPROPRIATE(CSA, JPC, RECTYPE, JREC, BLKPTR, JFB)					\
{													\
	assert(JNL_ENABLED(CSA) || REPL_WAS_ENABLED(CSA));						\
	if (JNL_ENABLED(CSA))										\
		jnl_write(JPC, RECTYPE, JREC, BLKPTR, JFB); /* write to jnlbuffer, jnlfile, jnlpool */	\
	else												\
		jnl_write_poolonly(JPC, RECTYPE, JREC, JFB);	/* write to jnlpool only */		\
}

#define MUEXTRACT_TYPE(A) 	(((A)[0]-'0')*10 + ((A)[1]-'0')) /* A is a character pointer */

#define PADDED			PADDING

#ifdef BIGENDIAN
#define THREE_LOW_BYTES(x)	((uchar_ptr_t)((uchar_ptr_t)&x + 1))
#else
#define THREE_LOW_BYTES(x)	((uchar_ptr_t)(&x))
#endif
#define EXTTIME(S)					extract_len = exttime(S, murgbl.extr_buff, extract_len)

/* This macro should be used to initialize jgbl.gbl_jrec_time to the system time. The reason is that it does additional checks. */
#define	SET_GBL_JREC_TIME				\
{							\
	assert(!jgbl.dont_reset_gbl_jrec_time);		\
	JNL_SHORT_TIME(jgbl.gbl_jrec_time);		\
}

/* This macro ensures that journal records are written in non-decreasing time order in each journal file.
 * It is passed the time field to adjust and a pointer to the journal buffer of the region.
 * The journal buffer holds the timestamp of the most recently written journal record.
 */
#define	ADJUST_GBL_JREC_TIME(jgbl, jbp)				\
{								\
	if (jgbl.gbl_jrec_time < jbp->prev_jrec_time)		\
	{							\
		assert(!jgbl.dont_reset_gbl_jrec_time);		\
		jgbl.gbl_jrec_time = jbp->prev_jrec_time;	\
	}							\
}

/* Token generation used in non-replicated journaled environment. Note the assumption here
   that sizeof(token_split_t) == sizeof(token_build) which will be asserted in gvcst_init().
   The TOKEN_SET macro below depends on this assumption.
*/
typedef struct token_split_t_struct
{
	uint4	regnum;
	uint4	tn;
} token_split_t;

typedef union
{
	token_split_t	t_piece;
	token_num	token;
} token_build;

/* To assist in setting token value, the following macro is supplied to handle the two token parts */
#define TOKEN_SET(BASE, TN, REGNUM) (((token_build_ptr_t)(BASE))->t_piece.tn = (uint4)(TN), \
				     ((token_build_ptr_t)(BASE))->t_piece.regnum = (REGNUM))

enum jpv_types
{
        CURR_JPV = 0,
        ORIG_JPV,
        JPV_COUNT
};
/* Note we have two process verctors now for a pini record */
typedef struct jnl_process_vector_struct	/* name needed since this is used in cmmdef.h for "pvec" member */
{
	uint4		jpv_pid;			/* Process id */
	int4		jpv_image_count;		/* Image activations [VMS only] */
	jnl_proc_time	jpv_time;			/* Timestamp of the process genarating this.
								(This could be different than the journal record timestamp) */
	jnl_proc_time	jpv_login_time;			/* Used for process initialization time */
	char		jpv_node[JPV_LEN_NODE],		/* Node name */
			jpv_user[JPV_LEN_USER],		/* User name */
			jpv_prcnam[JPV_LEN_PRCNAM],	/* Process name [VMS only] */
			jpv_terminal[JPV_LEN_TERMINAL];	/* Login terminal */
	unsigned char	jpv_mode;			/* a la JPI$_MODE [VMS only] */
	int4		filler;
	/* sizeof(jnl_process_vector) must be a multiple of sizeof(int4) */
} jnl_process_vector;

enum pini_rec_stat
{
	IGNORE_PROC = 0,
	ACTIVE_PROC = 1,
	FINISHED_PROC = 2,
	BROKEN_PROC = 4
};

typedef struct pini_list
{
	uint4			pini_addr;
	uint4			new_pini_addr;	/* used in forward phase of recovery */
	jnl_process_vector	jpv;		/* CURR_JPV. Current process's JPV. For GTCM server we also use this. */
	jnl_process_vector	origjpv;	/* ORIG_JPV. Used for GTCM client only */
	enum pini_rec_stat	state;		/* used for show qualifier */
} pini_list_struct;


enum jnl_record_type
{
#define JNL_TABLE_ENTRY(rectype, extract_rtn, label, update, fixed_size, is_replicated)	rectype,
#include "jnl_rec_table.h"
#undef JNL_TABLE_ENTRY

	JRT_RECTYPES		/* Total number of journal record types */
};

enum jnl_state_codes
{
	jnl_notallowed,
	jnl_closed,
	jnl_open
};

enum repl_state_codes
{
	repl_closed,	/* region not replicated, no records are written */
	repl_open,	/* region is replicated, and records are written */
	repl_was_open	/* region is currently not replicated, but it was earlier; jnl_file_lost() changes open to was_open */
};

typedef struct
{
 	trans_num		eov_tn;		/* curr_tn is saved as eov_tn by jnl_write_epoch. Used by recover/rollback */
	volatile trans_num	epoch_tn;	/* Transaction number for current epoch */
	seq_num			end_seqno;		/* reg_seqno saved by jnl_write_epoch. Used by recover/rollback */
	int4			min_write_size,	/* if unwritten data gets to this size, write it */
				max_write_size, /* maximum size of any single write */
				size;		/* buffer size */
	int4			epoch_interval;	/* Time between successive epochs in epoch-seconds */
	boolean_t		before_images;	/* If TRUE, before-image processing is enabled */
						/* end not volatile QUAD */
	uintszofptr_t		buff_off;	/* relative offset to DISK_BLOCK_SIZE aligned buffer start */
	volatile int4		free;		/* relative index of first byte to write in buffer */
	volatile uint4		freeaddr,	/* virtual on-disk address which will correspond to free, when it is written */
				end_of_data,	/* Synched offset updated by jnl_write_epoch. Used by recover/rollback */
				filesize;	/* highest virtual address available in the file (units in disk-blocks) */
						/* end mainline QUAD */
	volatile int4		blocked;
	volatile uint4	 	fsync_dskaddr;  /* dskaddr upto which fsync is done */
	volatile int4		dsk;		/* relative index of 1st byte to write to disk;
						 * if free == dsk, buffer is empty */
	volatile int4		wrtsize;	/* size of write in progress */
        volatile uint4		dskaddr,	/* virtual on-disk address corresponding to dsk */
				now_writer,	/* current owner of io_in_prog (VMS-only) */
				image_count;	/* for VMS is_proc_alive */
	volatile struct				/* must be at least word aligned for memory coherency */
	{
		short		cond;
		unsigned short	length;
		int4		dev_specific;
	}			iosb;
	/* alignsize is removed and log2_of_alignsize introduced */
	uint4         		log2_of_alignsize;      /* Ceiling of log2(alignsize) */
	jnl_tm_t		eov_timestamp;		/* jgbl.gbl_jrec_time saved by jnl_write_epoch. Used by recover/rollback */
	uint4			cycle;			/* shared copy of the number of the current journal file generation */
	volatile int4		qiocnt,			/* Number of qio's issued */
				bytcnt,			/* Number of bytes written */
				errcnt,			/* Number of errors during writing */
				reccnt[JRT_RECTYPES];	/* Number of records written per opcode */
	int			filler_align[29 - JRT_RECTYPES];	/* So buff below starts on even (QW) keel */
	/* Note the above filler will fail if JRT_RECTYPES grows beyond 29 elements and give compiler warning in VMS
	 * if JRT_RECTYPES equals 29. In that case, change the start num to the next odd number above JRT_RECTYPES.
	 */
	volatile jnl_tm_t	prev_jrec_time;		/* to ensure that time never decreases across successive jnl records */
	volatile int4		free_update_pid;	/* pid that is updating jb->free and jb->freeaddr */
	volatile uint4		next_epoch_time;	/* Time when next epoch is to be written (in epoch-seconds) */
	volatile boolean_t	need_db_fsync;          /* need an fsync of the db file */
	volatile int4		io_in_prog;		/* VMS only: write in progress indicator (NOTE: must manipulate
										only with interlocked instructions */
	uint4			enospc_errcnt;		/* number of times jb->errcnt was last incremented due to ENOSPC error
							 * when writing to this journal file */
	/* CACHELINE_PAD macros provide spacing between the following latches so that they do
	   not interfere with each other which can happen if they fall in the same data cacheline
	   of a processor.
	*/
	CACHELINE_PAD(sizeof(global_latch_t), 0)	/* start next latch at a different cacheline than previous fields */
	global_latch_t		io_in_prog_latch;	/* UNIX only: write in progress indicator */
	CACHELINE_PAD(sizeof(global_latch_t), 1)	/* pad enough space so next latch falls in different cacheline */
	global_latch_t		fsync_in_prog_latch;	/* fsync in progress indicator */
        CACHELINE_PAD(sizeof(global_latch_t), 2)	/* pad enough space so next non-filler byte falls in different cacheline */
/******************************************************************************************************/
	/* Important: must keep header structure quadword (8 byte)  aligned for buffers used in QIO's */
	/**********************************************************************************************/
	unsigned char		buff[1];		/* Actually buff[size] */
} jnl_buffer;

#define	FIX_NONZERO_FREE_UPDATE_PID(csa, jbp)										\
{															\
	assert(csa->now_crit);	/* hold crit before manipulating freeaddr/free */					\
	assert(jbp->free_update_pid);											\
	UNIX_ONLY(assert(!is_proc_alive(jbp->free_update_pid, 0));)							\
	VMS_ONLY(assert(FALSE);) /* secshr_db_clnup should have cleaned up this field even in case of STOP/ID */	\
	if ((jbp->freeaddr % jbp->size) != jbp->free)									\
	{	/* Previous process in jnl_write got killed after incrementing freeaddr but before incrementing		\
		 * free. Recalculate jbp->free based on current value of jbp->freeaddr. */				\
		jbp->free = jbp->freeaddr % jbp->size;									\
		jbp->free_update_pid = 0;										\
	}														\
	DBG_CHECK_JNL_BUFF_FREEADDR(jbp);										\
}

#define	DBG_CHECK_JNL_BUFF_FREEADDR(jbp)							\
{												\
	assert((jbp->freeaddr % jbp->size) == jbp->free);					\
	assert((jbp->freeaddr >= jbp->dskaddr)							\
		|| (gtm_white_box_test_case_enabled						\
			&& (WBTEST_JNL_FILE_LOST_DSKADDR == gtm_white_box_test_case_number)));	\
}

#ifdef DB64
# ifdef __osf__
#  pragma pointer_size(save)
#  pragma pointer_size(long)
# else
#  error UNSUPPORTED PLATFORM
# endif
#endif

typedef jnl_buffer	*jnl_buffer_ptr_t;
typedef token_build	*token_build_ptr_t;

#ifdef DB64
# ifdef __osf__
#  pragma pointer_size(restore)
# endif
#endif

typedef struct jnl_private_control_struct
{
	jnl_buffer_ptr_t	jnl_buff;		/* pointer to shared memory */
	gd_region		*region;		/* backpointer to region head */
	fd_type			channel,		/* output channel, aka fd in UNIX */
				old_channel;		/* VMS only - for dealing with deferred deassign */
	gd_id			fileid;			/* currently initialized and used only by source-server */
	vms_lock_sb		*jnllsb;		/* VMS only */
	uint4			pini_addr,		/* virtual on-disk address for JRT_PINI record, if journaling */
				new_freeaddr;
	int4			temp_free;		/* M Temp copy of free relative index until full write done */
	double			filler_q0;		/* reset QUAD end mainline */
	int4			new_dsk;		/* A VMS only */
	uint4			new_dskaddr;		/* A VMS only */
	int4			status;			/* A for error reporting */
	volatile boolean_t	dsk_update_inprog;	/* A VMS only */
	volatile boolean_t	qio_active;		/* jnl buffer write in progress in THIS process (recursion indicator) */
	boolean_t		fd_mismatch;		/* TRUE when jpc->channel does not point to the active journal */
	volatile boolean_t	sync_io;		/* TRUE if the process is using O_SYNC/O_DSYNC for this jnl (UNIX) */
							/* TRUE if writers open NOCACHING to bypass XFC cache (VMS) */
	uint4			status2;		/* for secondary error status, currently used only in VMS */
	uint4			cycle;			/* private copy of the number of this journal file generation */
} jnl_private_control;

typedef enum
{
	JNL_KILL,
	JNL_SET,
	JNL_ZKILL,
	JNL_INCTN
} jnl_action_code;

typedef enum
{
 /* 00 */ MUEXT_NULL,
 /* 01 */ MUEXT_PINI,
 /* 02 */ MUEXT_PFIN,
 /* 03 */ MUEXT_EOF,
 /* 04 */ MUEXT_KILL,
 /* 05 */ MUEXT_SET,
 /* 06 */ MUEXT_ZTSTART,
 /* 07 */ MUEXT_ZTCOMMIT,
 /* 08 */ MUEXT_TSTART,
 /* 09 */ MUEXT_TCOMMIT,
 /* 10 */ MUEXT_ZKILL,

	MUEXT_MAX_TYPES
} muextract_type;

typedef struct
{
	mval			*val;
	jnl_action_code		operation;
	gv_key			*key;
} jnl_action;

#define	JNL_FENCE_LIST_END	((sgmnt_addrs *)-1L)

typedef struct
{
	sgmnt_addrs		*fence_list;
	int			level;
	token_num		token;
} jnl_fence_control;

typedef struct
{
	uint4			jrec_type : 8;		/* Offset:0 :: Actually, enum jnl_record_type */
	uint4			forwptr : 24;		/* Offset:3 :: Offset to beginning of next record */
	off_jnl_t		pini_addr;		/* Offset:4 :: Offset in the journal file which contains pini record */
	jnl_tm_t		time;			/* Offset:8 :: 4-byte time stamp both for UNIX and VMS */
	uint4			checksum;		/* Offset:12 :: Generated from journal record */
	trans_num		tn;			/* Offset:16 */
} jrec_prefix;	/* 24-byte */

typedef struct
{
	uint4 			backptr : 24;		/* Offset to beginning of current record */
	uint4 			suffix_code : 8;	/* JNL_REC_SUFFIX_CODE */
} jrec_suffix;	/* 4-byte */

typedef union
{
	seq_num			jnl_seqno;
	token_num		token;
} token_seq_t;

typedef struct
{
	char			label[sizeof(JNL_LABEL_TEXT) - 1];
	jnl_process_vector	who_created,	/* Process who created */
				who_opened;	/* Process who last opened */
	jnl_proc_time		bov_timestamp,	/* 8-byte time when journal was created */
				eov_timestamp;	/* 8-byte time when journal was last updated
							Updated by cre_jnl_file/jnl_file_extend/jnl_file_close */
	trans_num		bov_tn, 	/* Beginning journal record's transaction number */
				eov_tn;		/* End transaction number.
							Updated by cre_jnl_file/jnl_file_extend/jnl_file_close */
	seq_num			start_seqno;	/* reg_seqno when this journal file was created */
	seq_num			end_seqno;	/* reg_seqno when this journal file was closed or last extended.
							Updated by cre_jnl_file/jnl_file_extend/jnl_file_close */
	off_jnl_t		end_of_data;	/* Offset of beginning of last record.
							Updated by cre_jnl_file/jnl_file_extend/jnl_file_close */
	off_jnl_t		prev_recov_end_of_data;	/* Recovered/Rolled back journal's turn around point's offset.
						This offset was supposed to have EOF_RECORD before recover switched journal.
						A non-zero value means this journal was recovered and had the turn around point. */
	off_jnl_t		virtual_size;	/* Allocation + n * Extension (in blocks). jnl_file_extend updates it */
	boolean_t		crash;		/* crashed before jnl_file_close() completed */
	boolean_t		recover_interrupted;	/* true when recover creates the journal file; false after success. */
	off_jnl_t		turn_around_offset;	/* At turn around point journal record's (EPOCH) offset */
	jnl_tm_t		turn_around_time;	/* At turn around point journal record's timestamp */
	boolean_t		before_images;	/* before image enabled in this journal */
	uint4			alignsize;	/* align size of journal (where a valid record start) */
	int4			epoch_interval;	/* Time between successive epochs in epoch-seconds */
	int4			repl_state;	/* To state whether replication is turned on for this journal file */
 	uint4			autoswitchlimit;/* Limit in disk blocks (max 4GBytes) when jnl should be auto switched */
	uint4			jnl_alq;	/* initial allocation (in blocks) */
	uint4			jnl_deq;	/* extension (in blocks) */
#ifdef VMS
	boolean_t		update_disabled;/* If the secondary side has database update disabled. For rollback. */
#else
	boolean_t		filler_update_disabled;	/* obsoleted as part of multi-site replication changes */
#endif
	int4			max_phys_reclen;/* Maximum journal record size in binary form (on disk). We need this in case
						   database is not available */
	int4			max_logi_reclen;/* Maximum record size of a logical record (on disk). We need this in case
						   database is not available */
	unsigned char		data_file_name[JNL_NAME_SIZE];		/* Database file name */
	unsigned char		prev_jnl_file_name[JNL_NAME_SIZE];	/* Previous generation journal file name */
	unsigned char		next_jnl_file_name[JNL_NAME_SIZE];	/* Next generation journal file name */
	uint4 			data_file_name_length;			/* Length of data_file_name */
	uint4 			prev_jnl_file_name_length;		/* Length of prev_jnl_file_name */
	uint4 			next_jnl_file_name_length;		/* Length of next_jnl_file_name */
	uint4 			checksum;	/* Calculate from journal file id */
	uint4			prev_recov_blks_to_upgrd_adjust;	/* amount to adjust filehdr "blks_to_upgrd" if ever
									 * backward recovery goes back past this journal file */
	char			filler[960];
} jnl_file_header;

typedef struct
{
	int4			status,
				alloc,
				extend,
				buffer;
	trans_num		tn;
	seq_num			reg_seqno;
	unsigned char		jnl[JNL_NAME_SIZE],
		                *fn;
	uint4			max_phys_reclen;
	uint4			max_logi_reclen;
	short			fn_len,
				jnl_len,
				jnl_def_len;
	bool			before_images;
	bool			filler_bool[1];
	uint4			alignsize;
 	int4			autoswitchlimit;	/* limit in disk blocks (8388607 blocks)
							 * when jnl should be auto switched */
	int4			epoch_interval;		/* Time between successive epochs in epoch-seconds */
	char			*prev_jnl;
	int4			prev_jnl_len;
	int4                    jnl_state;              /* current csd->jnl_state */
	int4			repl_state;
	uint4			status2;		/* for secondary error status information in VMS */
	boolean_t		no_rename;
	boolean_t		no_prev_link;
	int4			blks_to_upgrd;		/* Blocks not at current block version level */
	uint4 			checksum;
	uint4			free_blocks;		/* free  blocks counter at time of epoch */
	uint4			total_blks;		/* total blocks counter at time of epoch */
} jnl_create_info;

/* Journal record definitions */
/* change  GET_JNL_STR_LEN and PUT_JNL_STR_LEN, if jnl_str_len_t is changed from uint4 */
#define jnl_str_len_t		uint4
typedef struct
{
	jnl_str_len_t		length;
	char			text[1];		/* Actually text[length] */
} jnl_string;
#define GET_JNL_STR_LEN(X, Y)		\
{					\
	uint4 		temp_uint4;	\
	GET_ULONG(temp_uint4, Y);	\
	X = temp_uint4;			\
}
#define PUT_JNL_STR_LEN(X, Y)		\
{					\
	uint4		temp_uint4;	\
	temp_uint4 = Y;			\
	PUT_ULONG(X, temp_uint4);	\
}
typedef struct jnl_format_buff_struct
{
	que_ent				free_que;
	struct  jnl_format_buff_struct	*next;
	enum jnl_record_type		rectype;
	int4				record_size;
	char 				*buff;
	uint4				checksum;
	jnl_action			ja;
} jnl_format_buffer;

/* All fixed size recrods are 8-byte-multiple size.
 * All variable size records are made 8-byte multiple size by run-time process */

/* struct_jrec_upd for non-TP or TP but not ZTP
 * Note that if AREG has before image journaling and DEFAULT has replication,
 * AREG will keep token_num but DEFAULT will keep jnl_seqno.
 * Later recover/rollback can fail.
 * But currently we do not support this kind of mixing : Layek 1/3/2003
 */
typedef struct	/* variable length */
{
	jrec_prefix		prefix;
	token_seq_t		token_seq;	/* must start at 8-byte boundary */
	jnl_string		mumps_node;	/* For set/kill/zkill 	: {jnl_str_len_t key_len, char key[key_len]} */
	 					/* For set additionally : {mstr_len_t data_len, char data[data_len]} */
} struct_jrec_upd;

/* struct_jrec_ztp_upd for ZTP only */
typedef struct	/* variable length */
{
	jrec_prefix		prefix;
	seq_num			jnl_seqno;	/* must start at 8-byte boundary */
	token_num		token;		/* must start at 8-byte boundary */
	jnl_string		mumps_node;	/* For set/kill/zkill 	: {jnl_str_len_t key_len, char key[key_len]} */
	 					/* For set additionally : {mstr_len_t data_len, char data[data_len]} */
} struct_jrec_ztp_upd;

typedef struct	/* variable length */
{
	jrec_prefix		prefix;
	block_id		blknum;
	uint4			bsiz;
	enum db_ver		ondsk_blkver;		/* Previous version of block from cache_rec */
	int4			filler;
	char			blk_contents[1];	/* Actually blk_contents[bsiz] */
} struct_jrec_blk;

typedef struct	/* variable length */
{
	jrec_prefix		prefix;
	jnl_string		align_str;
	/* Note: Actual string follows the align_string and then jrec_suffix */
} struct_jrec_align;

/* Please change the "GBLDEF struct_jrec_tcom" initialization, if below is changed */
typedef struct	/* fixed length */
{
	jrec_prefix		prefix;
	token_seq_t		token_seq;	/* must start at 8-byte boundary */
	char			jnl_tid[TID_STR_SIZE];
	uint4			participants;
	jrec_suffix		suffix;
} struct_jrec_tcom;

/* Please change the "static struct_jrec_ztcom" initialization in op_ztcommmit.c, if below is changed */
typedef struct	/* fixed length */
{
	jrec_prefix		prefix;
	seq_num			jnl_seqno;	/* must start at 8-byte boundary */
	token_num		token;		/* must start at 8-byte boundary */
	uint4			participants;
	jrec_suffix		suffix;
} struct_jrec_ztcom;

typedef union
{
	int4		blks_to_upgrd_delta; /* Delta to adjust csd->blks_to_upgrade (opcode = inctn_gdsfilext_*) */
	block_id	blknum;		     /* block that got upgraded or downgraded (opcode = inctn_blk*grd) */
} inctn_detail_t;

typedef struct	/* fixed length */
{
	jrec_prefix		prefix;
	uint4			opcode;
	inctn_detail_t		detail;
	uint4			filler;
	jrec_suffix		suffix;
} struct_jrec_inctn;

typedef struct	/* fixed length */
{
	jrec_prefix		prefix;
	jnl_process_vector	process_vector[JPV_COUNT];
	int4			filler;
	jrec_suffix		suffix;
} struct_jrec_pini;

typedef struct	/* fixed length */
{
	jrec_prefix		prefix;
	uint4			filler;
	jrec_suffix		suffix;
} struct_jrec_pfin;

/* Following 3 are same structures. In case we change it in future, let's define them separately */
typedef struct	/* fixed length */
{
	jrec_prefix		prefix;
	seq_num			jnl_seqno;		/* must start at 8-byte boundary */
	uint4			filler;
	jrec_suffix		suffix;
} struct_jrec_null;

typedef struct	/* fixed length */
{
	jrec_prefix		prefix;
	seq_num			jnl_seqno;		/* must start at 8-byte boundary */
	uint4			blks_to_upgrd;		/* blocks-to-upgrade counter at time of epoch */
	uint4			free_blocks;		/* free  blocks counter at time of epoch */
	uint4			total_blks;		/* total blocks counter at time of epoch */
	jrec_suffix		suffix;
} struct_jrec_epoch;

typedef struct	/* fixed length */
{
	jrec_prefix		prefix;
	seq_num			jnl_seqno;		/* must start at 8-byte boundary */
	uint4			filler;
	jrec_suffix		suffix;
} struct_jrec_eof;

typedef union
{
	jrec_prefix			prefix;
	struct_jrec_upd			jrec_kill,
					jrec_set,
					jrec_zkill,
					jrec_tkill,
					jrec_tset,
					jrec_tzkill,
					jrec_ukill,
					jrec_uset,
					jrec_uzkill;
	struct_jrec_ztp_upd		jrec_fkill,
					jrec_fset,
					jrec_fzkill,
					jrec_gkill,
					jrec_gset,
					jrec_gzkill;
	struct_jrec_blk			jrec_pblk,
					jrec_aimg;
	struct_jrec_align		jrec_align;
	/** All below are fixed size and above are variable size records */
	struct_jrec_tcom		jrec_tcom;
	struct_jrec_ztcom		jrec_ztcom;
	struct_jrec_inctn               jrec_inctn;
	struct_jrec_pini		jrec_pini;
	struct_jrec_pfin		jrec_pfin;
	struct_jrec_null		jrec_null;
	struct_jrec_epoch		jrec_epoch;
	struct_jrec_eof			jrec_eof;
} jnl_record;


/* Macro to access fixed size record's size */
#define	TCOM_RECLEN		SIZEOF(struct_jrec_tcom)
#define	ZTCOM_RECLEN		SIZEOF(struct_jrec_ztcom)
#define	INCTN_RECLEN		SIZEOF(struct_jrec_inctn)
#define	PINI_RECLEN		SIZEOF(struct_jrec_pini)
#define	PFIN_RECLEN		SIZEOF(struct_jrec_pfin)
#define	NULL_RECLEN		SIZEOF(struct_jrec_null)
#define	EPOCH_RECLEN		SIZEOF(struct_jrec_epoch)
#define	EOF_RECLEN 		SIZEOF(struct_jrec_eof)
/* Macro to access variable size record's fixed part's size */
#define FIXED_UPD_RECLEN	OFFSETOF(struct_jrec_upd, mumps_node)
#define FIXED_ZTP_UPD_RECLEN	OFFSETOF(struct_jrec_ztp_upd, mumps_node)
#define MIN_ALIGN_RECLEN	(OFFSETOF(struct_jrec_align, align_str.text[0]) + JREC_SUFFIX_SIZE)
#define FIXED_ALIGN_RECLEN	OFFSETOF(struct_jrec_align, align_str.text[0])
#define FIXED_BLK_RECLEN 	OFFSETOF(struct_jrec_blk, blk_contents[0])
#define FIXED_PBLK_RECLEN 	OFFSETOF(struct_jrec_blk, blk_contents[0])
#define FIXED_AIMG_RECLEN 	OFFSETOF(struct_jrec_blk, blk_contents[0])
#define MIN_PBLK_RECLEN		(OFFSETOF(struct_jrec_blk, blk_contents[0]) + JREC_SUFFIX_SIZE)
#define MIN_AIMG_RECLEN		(OFFSETOF(struct_jrec_blk, blk_contents[0]) + JREC_SUFFIX_SIZE)

#define JREC_PREFIX_SIZE	SIZEOF(jrec_prefix)
#define JREC_SUFFIX_SIZE	SIZEOF(jrec_suffix)
#define MIN_JNLREC_SIZE		(JREC_PREFIX_SIZE + JREC_SUFFIX_SIZE)
#define JREC_PREFIX_UPTO_LEN_SIZE	(offsetof(jrec_prefix, pini_addr))

typedef struct set_jnl_options_struct
{
	int			cli_journal, cli_enable, cli_on, cli_replic_on;
	boolean_t		alignsize_specified,
				allocation_specified,
				autoswitchlimit_specified,
				image_type_specified,	/* beofre/nobefore option specified */
				buffer_size_specified,
				epoch_interval_specified,
				extension_specified,
				filename_specified,
				sync_io_specified,
				yield_limit_specified;
	/* since jnl_create_info does not have following fields, we need them here */
	boolean_t		sync_io;
	int4			yield_limit;
} set_jnl_options;

/* rlist_state needed to be moved here to use with mu_set_reglist */
enum rlist_state {
	NONALLOCATED,
	ALLOCATED,
	DEALLOCATED
};

/* mu_set_reglist needed to be moved here for the journal specific fields */
/* ATTN: the first four items in this structure need to be identical to those
 *	 in structure tp_region in tp.h.
 */
typedef struct mu_set_reglist
{
	struct mu_set_reglist	*fPtr;		/* all fields after this are used for mupip_set_journal.c */
	gd_region		*reg;
	char			unique_id[UNIQUE_ID_SIZE];
	enum rlist_state	state;
	sgmnt_data_ptr_t 	sd;
	bool			exclusive;	/* standalone access is required for this region */
	int			fd;
	enum jnl_state_codes	jnl_new_state;
	enum repl_state_codes	repl_new_state;
	boolean_t		before_images;
} mu_set_rlist;

/* The enum codes below correspond to code-paths that can call set_jnl_file_close() in VMS */
typedef enum
{
        SET_JNL_FILE_CLOSE_BACKUP = 1,	/* just for safety a non-zero value to start with */
	SET_JNL_FILE_CLOSE_SETJNL,
	SET_JNL_FILE_CLOSE_EXTEND,
	SET_JNL_FILE_CLOSE_RUNDOWN,
        SET_JNL_FILE_CLOSE_INVALID_OP
} set_jnl_file_close_opcode_t;

typedef	void	(*pini_addr_reset_fnptr)(void);

typedef struct
{
	token_seq_t           	mur_jrec_token_seq;	/* always set for fenced transaction or replication */
	token_num         	mur_jrec_seqno;		/* This is jnl_seqno. For ZTP mur_jrec_token_seq will have token */
	VMS_ONLY(seq_num	max_resync_seqno;)	/* for update process and rollback fetchresync */
	UNIX_ONLY(seq_num	max_dualsite_resync_seqno;)	/* for update process and rollback fetchresync */
	int         		mur_jrec_participants;
	jnl_tm_t           	gbl_jrec_time;
	boolean_t       	forw_phase_recovery;
	struct pini_list	*mur_plst;		/* pini_addr hash-table entry of currently simulating GT.M process */
	boolean_t		mur_rollback;		/* a copy of mur_options.rollback to be accessible to runtime code */
	boolean_t		mupip_journal;		/* the current command is a MUPIP JOURNAL command */
	boolean_t		dont_reset_gbl_jrec_time;	/* Do not reset gbl_jrec_time */
	pini_addr_reset_fnptr	mur_pini_addr_reset_fnptr;	/* function pointer to invoke mur_pini_addr_reset() */
	uint4			cumul_jnl_rec_len;	/* cumulative length of the replicated journal records
								for the current TP or non-TP transaction */
	boolean_t		wait_for_jnl_hard;
DEBUG_ONLY(
	uint4			cumul_index;
	uint4			cu_jnl_index;
)
} jnl_gbls_t;


#define JNL_SHARE_SIZE(X)	(JNL_ALLOWED(X) ? 							\
				(ROUND_UP(JNL_NAME_EXP_SIZE + sizeof(jnl_buffer), OS_PAGE_SIZE)		\
                                + ROUND_UP(((sgmnt_data_ptr_t)X)->jnl_buffer_size * DISK_BLOCK_SIZE, 	\
					OS_PAGE_SIZE) + OS_PAGE_SIZE) : 0)

/* pass address of jnl_buffer to get address of expanded jnl file name */
#define JNL_GDID_PVT(CSA)        ((CSA)->jnl->fileid)

#ifdef UNIX
#define JNL_GDID_PTR(CSA)	((gd_id_ptr_t)(&((CSA)->nl->jnl_file.u)))
#else
#define JNL_GDID_PTR(CSA)	((gd_id_ptr_t)(&((CSA)->nl->jnl_file.jnl_file_id)))
#endif

/* Note that since "cycle" (in jpc and jb below) can rollover the 4G limit back to 0, it should
 * only be used to do "!=" checks and never to do ordered checks like "<", ">", "<=" or ">=".
 */
#define JNL_FILE_SWITCHED(JPC) 	((JPC)->cycle != (JPC)->jnl_buff->cycle)

#define REG_STR		"region"
#define FILE_STR	"database file"

/* Given a journal record, get_jnl_seqno returns the jnl_seqno field
 * Now all replication type records, EOF and EPOCH have the jnl_seqno at the same offset.
 * Modify the macro GET_JNL_SEQNO if offset of jnl_seqno is changed for any journal records
 */
#define GET_JNL_SEQNO(j)	(((jnl_record *)(j))->jrec_null.jnl_seqno)
#define GET_REPL_JNL_SEQNO(j)	(IS_REPLICATED(((jrec_prefix *)j)->jrec_type) ? GET_JNL_SEQNO(j) : 0)

/* Given a journal record, GET_TN returns the tn field
 */
#define GET_TN(j)		(((*jrec_prefix)(j))->prefix.tn)

/* In t_end(), we need to write the after-image if DSE or mupip recover/rollback is playing it.
 * But to write it out, we should have it already built before bg_update().
 * Hence, we pre-build the block here itself before invoking t_end().
 */
#define	BUILD_AIMG_IF_JNL_ENABLED(csa, csd, jfb, cse)					\
{											\
	if (JNL_ENABLED(csd))								\
	{										\
		cse = (cw_set_element *)(&cw_set[0]);					\
		cse->new_buff = jfb;							\
		gvcst_blk_build(cse, (uchar_ptr_t)cse->new_buff, csa->ti->curr_tn);	\
		cse->done = TRUE;							\
	}										\
}

#define	JNL_HDR_LEN		SIZEOF(jnl_file_header)
#define	JNL_FILE_FIRST_RECORD	JNL_HDR_LEN

/* Minimum possible journal file size */
#define MIN_JNL_FILE_SIZE	(JNL_HDR_LEN + PINI_RECLEN + EPOCH_RECLEN + PFIN_RECLEN + EOF_RECLEN)

/* maximum required journal file size (in 512-byte blocks), if the current transaction was the only one in a fresh journal file */
#define	MAX_REQD_JNL_FILE_SIZE(tot_jrec_size) DIVIDE_ROUND_UP((tot_jrec_size + MIN_JNL_FILE_SIZE), DISK_BLOCK_SIZE)

/* this macro aligns the input size to account that journal file sizes can increase only in multiples of the extension size */
#define	ALIGNED_ROUND_UP(tmp_tot_jrec_size, jnl_alq, jnl_deq)					\
	(((tmp_tot_jrec_size) <= (jnl_alq) || !(jnl_deq))					\
	 	? (jnl_alq) 									\
		: ((jnl_alq) + ROUND_UP((tmp_tot_jrec_size) - (jnl_alq), (jnl_deq))))

/* this macro aligns the input size to account that journal file sizes can increase only in multiples of the extension size */
#define	ALIGNED_ROUND_DOWN(tmp_tot_jrec_size, jnl_alq, jnl_deq)					\
	(((tmp_tot_jrec_size) <= (jnl_alq) || !(jnl_deq))					\
	 	? (jnl_alq) 									\
		: ((jnl_alq) + ROUND_DOWN((tmp_tot_jrec_size) - (jnl_alq), (jnl_deq))))

/* the following macro uses 8-byte quantities (gtm_uint64_t) to perform additions that might cause a 4G overflow */
#define	DISK_BLOCKS_SUM(freeaddr, jrec_size)	DIVIDE_ROUND_UP((((gtm_uint64_t)(freeaddr)) + (jrec_size)), DISK_BLOCK_SIZE)

#if defined(UNIX)
/* For future portability JNLBUFF_ALLOC is defined in jnl.h instead of jnlsp.h */
#define JPC_ALLOC(csa)								\
{										\
	csa->jnl = (jnl_private_control *)malloc(sizeof(*csa->jnl));		\
	memset(csa->jnl, 0, sizeof(*csa->jnl));					\
}
#define	ASSERT_JNLFILEID_NOT_NULL(csa)						\
{										\
	assert(0 != csa->nl->jnl_file.u.inode);					\
	assert(0 != csa->nl->jnl_file.u.device);				\
}
#define NULLIFY_JNL_FILE_ID(csa) 			\
{							\
		csa->nl->jnl_file.u.inode = 0;		\
		csa->nl->jnl_file.u.device = 0;		\
}
#elif defined(VMS)
#define JPC_ALLOC(csa)								\
{										\
	vms_lock_sb	*tmp_jnllsb;						\
	if (NULL == csa->jnl)							\
	{									\
		csa->jnl = (jnl_private_control *)malloc(sizeof(*csa->jnl));	\
		memset(csa->jnl, 0, sizeof(*csa->jnl));				\
		csa->jnl->jnllsb = malloc(sizeof(vms_lock_sb));			\
	} else									\
	{									\
		tmp_jnllsb = csa->jnl->jnllsb;					\
		memset(csa->jnl, 0, sizeof(*csa->jnl));				\
		csa->jnl->jnllsb = tmp_jnllsb;					\
	}									\
	memset(csa->jnl->jnllsb, 0, sizeof(vms_lock_sb));			\
}
#define	ASSERT_JNLFILEID_NOT_NULL(csa) assert(0 != memcmp(csa->nl->jnl_file.jnl_file_id.fid, zero_fid, sizeof(zero_fid)));
#define NULLIFY_JNL_FILE_ID(csa) memset(&csa->nl->jnl_file.jnl_file_id, 0, sizeof(gds_file_id))
#endif
#define JNL_INIT(csa, reg, csd)												\
{															\
	csa->jnl_state = csd->jnl_state;										\
	csa->jnl_before_image = csd->jnl_before_image;									\
	csa->repl_state = csd->repl_state;										\
	if JNL_ALLOWED(csa)												\
	{														\
		JPC_ALLOC(csa);												\
		csa->jnl->region = reg;											\
		csa->jnl->jnl_buff = (jnl_buffer_ptr_t)((sm_uc_ptr_t)(csa->nl) + NODE_LOCAL_SPACE + JNL_NAME_EXP_SIZE);	\
		csa->jnl->channel = NOJNL;										\
	} else														\
		csa->jnl = NULL;											\
}
#define MAX_EPOCH_DELAY		30
#define EXT_NEW 		"_new"
#define PREFIX_ROLLED_BAK	"rolled_bak_"
#define REC_TOKEN(jnlrec)	((struct_jrec_upd *)jnlrec)->token_seq.token
#define REC_JNL_SEQNO(jnlrec)	((struct_jrec_upd *)jnlrec)->token_seq.jnl_seqno
#define REC_LEN_FROM_SUFFIX(ptr, reclen)	((jrec_suffix *)((unsigned char *)ptr + reclen - JREC_SUFFIX_SIZE))->backptr

#define JNL_MAX_PHYS_LOGI_RECLEN(JINFO, CSD)											\
{																\
	/* Add MIN_ALIGN_RECLEN since the condition to write an align record is "(this rec size + MIN_ALIGN_RECLEN) crosses	\
	 * align boundary". The largest record will be of type JRT_ALIGN that is at most "the largest possible non ALIGN	\
	 * type record + MIN_ALIGN_RECLEN" */											\
	(JINFO)->max_phys_reclen = ROUND_UP2(MIN_PBLK_RECLEN + (CSD)->blk_size, JNL_REC_START_BNDRY) + MIN_ALIGN_RECLEN;	\
	/* max_logi_reclen must ideally be based on (CSD)->max_rec_size, but since max_rec_size can be changed independent of	\
	 * journal file creation, we consider the max possible logical record size. */						\
	assert(FIXED_UPD_RECLEN <= FIXED_ZTP_UPD_RECLEN);									\
	/* fixed size part of ztp update record + MAX possible (key + data) len + keylen + datalen */				\
	(JINFO)->max_logi_reclen = ROUND_UP2(FIXED_ZTP_UPD_RECLEN + ((CSD)->blk_size - USIZEOF(blk_hdr) - USIZEOF(rec_hdr)) +	\
			                     USIZEOF(jnl_str_len_t) + USIZEOF(mstr_len_t) + JREC_SUFFIX_SIZE, JNL_REC_START_BNDRY);\
	assert((JINFO)->max_phys_reclen >= (JINFO)->max_logi_reclen);								\
}

/* Macro that checks that the region seqno in the filehdr is never more than the seqno in the journal pool */
#define	ASSERT_JNL_SEQNO_FILEHDR_JNLPOOL(csd, jnlpool_ctl)						\
{	/* The seqno in the file header should be at most 1 greater than that in the journal pool.	\
	 * See step (5) of of commit logic flow in secshr_db_clnup.c for why. Assert that.		\
	 */												\
	assert((NULL == jnlpool_ctl) || (csd->reg_seqno <= (jnlpool_ctl->jnl_seqno + 1)));		\
}

/* jnl_ prototypes */
int	jnl_file_extend(jnl_private_control *jpc, uint4 total_jnl_rec_size); /***type int added***/
void	jnl_file_lost(jnl_private_control *jpc, uint4 jnl_stat);
uint4	jnl_qio_start(jnl_private_control *jpc);
uint4	jnl_write_attempt(jnl_private_control *jpc, uint4 threshold);
void	jnl_format(jnl_format_buffer *jfb);
void	jnl_prc_vector(jnl_process_vector *pv);
void	jnl_send_oper(jnl_private_control *jpc, uint4 status);
uint4	cre_jnl_file(jnl_create_info *info);
uint4 	cre_jnl_file_common(jnl_create_info *info, char *rename_fn, int rename_fn_len);
void	jfh_from_jnl_info (jnl_create_info *info, jnl_file_header *header);
uint4	jnl_ensure_open(void);
void	set_jnl_info(gd_region *reg, jnl_create_info *set_jnl_info);
void	jnl_write_epoch_rec(sgmnt_addrs *csa);
void	jnl_write_inctn_rec(sgmnt_addrs	*csa);
void	jnl_write_logical(sgmnt_addrs *csa, jnl_format_buffer *jfb);
void	jnl_write_ztp_logical(sgmnt_addrs *csa, jnl_format_buffer *jfb);
void	jnl_write_eof_rec(sgmnt_addrs *csa, struct_jrec_eof *eof_record);
void	jnl_write_poolonly(jnl_private_control *jpc, enum jnl_record_type rectype, jnl_record *jnl_rec, jnl_format_buffer *jfb);

#ifdef VMS
void	finish_active_jnl_qio(void);
void	jnl_start_ast(jnl_private_control *jpc);
uint4	jnl_permit_ast(jnl_private_control *jpc);
void	jnl_qio_end(jnl_private_control *jpc);
#endif

void	wcs_defer_wipchk_ast(jnl_private_control *jpc);
uint4	set_jnl_file_close(set_jnl_file_close_opcode_t set_jnl_file_close_opcode);
uint4 	jnl_file_open_common(gd_region *reg, off_jnl_t os_file_size);
uint4	jnl_file_open_switch(gd_region *reg, uint4 sts);
void	jnl_file_close(gd_region *reg, bool clean, bool dummy);
int	jnl_file_extend(jnl_private_control *jpc, uint4 total_jnl_rec_size);

/* Consider putting followings in a mupip only header file  : Layek 2/18/2003 */
boolean_t  mupip_set_journal_parse(set_jnl_options *jnl_options, jnl_create_info *jnl_info);
uint4	mupip_set_journal_newstate(set_jnl_options *jnl_options, jnl_create_info *jnl_info, mu_set_rlist *rptr);
void	mupip_set_journal_fname(jnl_create_info *jnl_info);
uint4	mupip_set_jnlfile_aux(jnl_file_header *header, char *jnl_fname);
void	jnl_setver(void);
void	jnl_extr_init(void);
int 	exttime(uint4 time, char *buffer, int extract_len);
char	*ext2jnlcvt(char *ext_buff, int4 ext_len, jnl_record *rec);
char	*ext2jnl(char *ptr, jnl_record *rec);
char	*jnl2extcvt(jnl_record *rec, int4 jnl_len, char *ext_buff);
char	*jnl2ext(char *jnl_buff, char *ext_buff);

#endif /* JNL_H_INCLUDED */
