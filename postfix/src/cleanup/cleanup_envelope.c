/*++
/* NAME
/*	cleanup_envelope 3
/* SUMMARY
/*	process envelope segment
/* SYNOPSIS
/*	#include <cleanup.h>
/*
/*	void	cleanup_envelope(state, type, buf, len)
/*	CLEANUP_STATE *state;
/*	int	type;
/*	const char *buf;
/*	int	len;
/* DESCRIPTION
/*	This module processes envelope records and writes the result
/*	to the queue file.  It validates the message structure, rewrites
/*	sender/recipient addresses to canonical form, and expands recipients
/*	according to entries in the virtual table. This routine absorbs but
/*	does not emit the envelope to content boundary record.
/*
/*	Arguments:
/* .IP state
/*	Queue file and message processing state. This state is updated
/*	as records are processed and as errors happen.
/* .IP type
/*	Record type.
/* .IP buf
/*	Record content.
/* .IP len
/*	Record content length.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>
#include <stdlib.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <mymalloc.h>
#include <stringops.h>
#include <nvtable.h>

/* Global library. */

#include <record.h>
#include <rec_type.h>
#include <cleanup_user.h>
#include <tok822.h>
#include <mail_params.h>
#include <ext_prop.h>
#include <mail_addr.h>
#include <canon_addr.h>
#include <verp_sender.h>

/* Application-specific. */

#include "cleanup.h"

#define STR	vstring_str

static void cleanup_envelope_process(CLEANUP_STATE *, int, const char *, int);

/* cleanup_envelope - initialize message envelope */

void    cleanup_envelope(CLEANUP_STATE *state, int type,
			         const char *str, int len)
{

    /*
     * The message size and count record goes first, so it can easily be
     * updated in place. This information takes precedence over any size
     * estimate provided by the client. It's all in one record for forward
     * compatibility so we can switch back to an older Postfix version.
     */
    cleanup_out_format(state, REC_TYPE_SIZE, REC_TYPE_SIZE_FORMAT,
		       (REC_TYPE_SIZE_CAST1) 0,
		       (REC_TYPE_SIZE_CAST2) 0,
		       (REC_TYPE_SIZE_CAST3) 0);

    /*
     * Pass control to the actual envelope processing routine.
     */
    state->action = cleanup_envelope_process;
    cleanup_envelope_process(state, type, str, len);
}

/* cleanup_envelope_process - process one envelope record */

static void cleanup_envelope_process(CLEANUP_STATE *state, int type,
				             const char *buf, int len)
{
    char   *attr_name;
    char   *attr_value;
    const char *error_text;
    int     extra_flags;

    /*
     * On the transition from envelope segment to content segment, do some
     * sanity checks and add some records.
     */
    if (type == REC_TYPE_MESG) {
	if (state->sender == 0 || state->time == 0) {
	    msg_warn("%s: missing sender or time envelope record",
		     state->queue_id);
	    state->errs |= CLEANUP_STAT_BAD;
	} else {
	    if (state->warn_time == 0 && var_delay_warn_time > 0)
		state->warn_time = state->time + var_delay_warn_time;
	    if (state->warn_time)
		cleanup_out_format(state, REC_TYPE_WARN, REC_TYPE_WARN_FORMAT,
				   state->warn_time);
	    state->action = cleanup_message;
	}
	return;
    }
    if (type == REC_TYPE_FLGS) {
	if (msg_verbose)
	    msg_info("envelope %c %.*s", type, len, buf);
	extra_flags = atol(buf);
	if (extra_flags & ~CLEANUP_FLAG_MASK_EXTRA)
	    msg_warn("%s: bad extra flags: 0x%x", state->queue_id, extra_flags);
	else
	    state->flags |= extra_flags;
	return;
    }
    if (strchr(REC_TYPE_ENVELOPE, type) == 0) {
	msg_warn("%s: unexpected record type %d in envelope",
		 state->queue_id, type);
	state->errs |= CLEANUP_STAT_BAD;
	return;
    }
    if (msg_verbose)
	msg_info("envelope %c %.*s", type, len, buf);

    if (type != REC_TYPE_RCPT) {
	if (state->orig_rcpt != 0) {
	    if (type != REC_TYPE_DONE)
		msg_warn("%s: out-of-order original recipient record <%.200s>",
			 state->queue_id, state->orig_rcpt);
	    myfree(state->orig_rcpt);
	    state->orig_rcpt = 0;
	}
    }
    if (type == REC_TYPE_TIME) {
	state->time = atol(buf);
	cleanup_out(state, type, buf, len);
    } else if (type == REC_TYPE_FULL) {
	state->fullname = mystrdup(buf);
    } else if (type == REC_TYPE_FROM) {
	if (state->sender != 0) {
	    msg_warn("%s: too many envelope sender records", state->queue_id);
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
	cleanup_addr_sender(state, buf);
    } else if (type == REC_TYPE_RCPT) {
	if (state->sender == 0) {		/* protect showq */
	    msg_warn("%s: envelope recipient precedes sender",
		     state->queue_id);
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
	if (state->orig_rcpt == 0)
	    state->orig_rcpt = mystrdup(buf);
	cleanup_addr_recipient(state, buf);
	myfree(state->orig_rcpt);
	state->orig_rcpt = 0;
    } else if (type == REC_TYPE_DONE) {
	 /* void */ ;
    } else if (type == REC_TYPE_WARN) {
	if ((state->warn_time = atol(buf)) < 0) {
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
    } else if (type == REC_TYPE_VERP) {
	if (state->sender == 0 || *state->sender == 0) {
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
	if (verp_delims_verify(buf) == 0) {
	    cleanup_out(state, type, buf, len);
	} else {
	    msg_warn("%s: bad VERP delimiters: \"%s\"", state->queue_id, buf);
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
    } else if (type == REC_TYPE_ATTR) {
	char   *sbuf;

	if (state->attr->used >= var_qattr_count_limit) {
	    msg_warn("%s: queue file attribute count exceeds safety limit: %d",
		     state->queue_id, var_qattr_count_limit);
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
	cleanup_out(state, type, buf, len);
	sbuf = mystrdup(buf);
	if ((error_text = split_nameval(sbuf, &attr_name, &attr_value)) != 0) {
	    msg_warn("%s: malformed attribute: %s: %.100s",
		     state->queue_id, error_text, buf);
	    state->errs |= CLEANUP_STAT_BAD;
	    myfree(sbuf);
	    return;
	}
	nvtable_update(state->attr, attr_name, attr_value);
	myfree(sbuf);
    } else if (type == REC_TYPE_ORCP) {
	state->orig_rcpt = mystrdup(buf);
    } else {
	cleanup_out(state, type, buf, len);
    }
}
