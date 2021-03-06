/****************************************************************
 *								*
 *	Copyright 2006 Fidelity Information Services, Inc.*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "gtm_time.h"
#include "gtm_fcntl.h"
#include "gtm_unistd.h"
#include "gtm_inet.h"
#include "gtm_string.h"

#include <errno.h>
#ifdef VMS
#include <descrip.h> /* Required for gtmsource.h */
#endif

#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "repl_msg.h"
#include "gtmsource.h"
#include "repl_dbg.h"
#include "gtm_stdio.h"
#include "repl_shutdcode.h"
#include "repl_sem.h"
#include "repl_log.h"

GBLREF	jnlpool_addrs		jnlpool;
GBLREF	gtmsource_options_t	gtmsource_options;
GBLREF	boolean_t		holds_sem[NUM_SEM_SETS][NUM_SRC_SEMS];

int gtmsource_mode_change(int to_mode)
{
	uint4		savepid;
	int		exit_status;
	int		status, detach_status, remove_status;

	assert(holds_sem[SOURCE][JNL_POOL_ACCESS_SEM]);
	repl_log(stdout, TRUE, TRUE, "Initiating %s operation on source server pid [%d] for secondary instance [%s]\n",
		(GTMSOURCE_MODE_ACTIVE == to_mode) ? "ACTIVATE" : "DEACTIVATE",
		jnlpool.gtmsource_local->gtmsource_pid, jnlpool.gtmsource_local->secondary_instname);
	if (jnlpool.gtmsource_local->mode == to_mode)
	{
		repl_log(stderr, FALSE, TRUE, "Source Server already %s, not changing mode\n",
				(to_mode == GTMSOURCE_MODE_ACTIVE) ? "ACTIVE" : "PASSIVE");
		return (ABNORMAL_SHUTDOWN);
	}
	assert(ROOTPRIMARY_UNSPECIFIED != gtmsource_options.rootprimary);
	if ((GTMSOURCE_MODE_ACTIVE == to_mode)
			&& (ROOTPRIMARY_SPECIFIED == gtmsource_options.rootprimary) && jnlpool.jnlpool_ctl->upd_disabled)
	{	/* ACTIVATE is specified with ROOTPRIMARY on a journal pool that was created with PROPAGATEPRIMARY.
		 * This is a case of transition from propagating primary to root primary. Enable updates in this journal pool
		 * and append a triple to the replication instance file. The function "gtmsource_rootprimary_init" does just that.
		 */
		gtmsource_rootprimary_init(jnlpool.jnlpool_ctl->jnl_seqno);
	}
	grab_lock(jnlpool.jnlpool_dummy_reg);
	/* Any ACTIVATE/DEACTIVATE versus ROOTPRIMARY/PROPAGATE incompatibilities have already been checked in the
	 * function "jnlpool_init" so go ahead and document the impending activation/deactivation and return.
	 * This flag will be eventually detected by the concurrently running source server which will then change mode.
	 */
	if (GTMSOURCE_MODE_ACTIVE == to_mode)
	{
		jnlpool.gtmsource_local->secondary_port = gtmsource_options.secondary_port;
		jnlpool.gtmsource_local->secondary_inet_addr = gtmsource_options.sec_inet_addr;
		STRCPY(jnlpool.gtmsource_local->secondary_host, gtmsource_options.secondary_host);
		memcpy(&jnlpool.gtmsource_local->connect_parms[0], &gtmsource_options.connect_parms[0],
				sizeof(gtmsource_options.connect_parms));
	}
	if ('\0' != gtmsource_options.log_file[0] && 0 != STRCMP(jnlpool.gtmsource_local->log_file, gtmsource_options.log_file))
	{
		repl_log(stdout, FALSE, TRUE, "Signaling change in log file from %s to %s\n",
				jnlpool.gtmsource_local->log_file, gtmsource_options.log_file);
		STRCPY(jnlpool.gtmsource_local->log_file, gtmsource_options.log_file);
		jnlpool.gtmsource_local->changelog |= REPLIC_CHANGE_LOGFILE;
	}
	if (0 != gtmsource_options.src_log_interval && jnlpool.gtmsource_local->log_interval != gtmsource_options.src_log_interval)
	{
		repl_log(stdout, FALSE, TRUE, "Signaling change in log interval from %u to %u\n",
				jnlpool.gtmsource_local->log_interval, gtmsource_options.src_log_interval);
		jnlpool.gtmsource_local->log_interval = gtmsource_options.src_log_interval;
		jnlpool.gtmsource_local->changelog |= REPLIC_CHANGE_LOGINTERVAL;
	}
	jnlpool.gtmsource_local->mode = to_mode;
	rel_lock(jnlpool.jnlpool_dummy_reg);
	REPL_DPRINT1("Change mode signalled\n");
	return (NORMAL_SHUTDOWN);
}
