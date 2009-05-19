/****************************************************************
 *								*
 *	Copyright 2001 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include <sys/types.h>

#define DIR_ROOT 1
#define CDB_STAGNATE 3
#define CDB_MAX_TRIES (CDB_STAGNATE + 2) /* used in defining arrays, usage requires it must be at least 1 more than CSB_STAGNATE*/
#define MAX_BT_DEPTH 7
#define GLO_NAME_MAXLEN 33	/* 1 for length, 4 for prefix, 15 for dvi, 1 for $, 12 for fid */
#define MAX_NUM_SUBSC_LEN 10	/* one for exponent, nine for the 18 significant digits */

typedef uint4	trans_num;
typedef int4    block_id;

/* Since it is possible that a block_id may live in shared memory, define a
   shared memory pointer type to it so the pointer will be 64 bits if necessary. */

#ifdef DB64
# ifdef __osf__
#  pragma pointer_size(save)
#  pragma pointer_size(long)
# else
#  error UNSUPPORTED PLATFORM
# endif
#endif

typedef block_id *block_id_ptr_t;

#ifdef DB64
# ifdef __osf__
#  pragma pointer_size(restore)
# endif
#endif

enum db_acc_method
{	dba_rms,
	dba_bg,
	dba_mm,
	dba_cm,
	dba_usr,
	n_dba
};

#define CREATE_IN_PROGRESS n_dba

typedef struct
{
	char dvi[16];
	unsigned short did[3];
	unsigned short fid[3];
} gds_file_id;

#if defined(VMS)
typedef struct           /* really just a place holder for gdsfhead.h union */
{
	unsigned int	inode;		/* ino_t really but VMS defines are not useful here */
	int		device;		/* dev_t really */
	unsigned int 	st_gen;
} unix_file_id;
#elif defined(UNIX)
typedef struct gd_id_struct  /* note this is not the same size on all platforms
			       but must be less than or equal to gds_file_id */
{	ino_t	inode;
	dev_t	device;
#if defined(__hpux) || defined(__linux__) || defined(_UWIN)
	unsigned int st_gen;
#elif defined(_AIX)
	ulong_t st_gen;
#else
	uint_t	st_gen;
#endif
} unix_file_id;
#else
# error Unsupported platform
#endif

typedef struct vms_lock_sb_struct
{
	short	cond;
	short	reserved;
	int4	lockid;
	int4	valblk[4];
}vms_lock_sb;

typedef struct
{
	uint4 low,high;
} date_time;
