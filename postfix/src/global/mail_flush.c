/*++
/* NAME
/*	mail_flush 3
/* SUMMARY
/*	mail flush service client interface
/* SYNOPSIS
/*	#include <mail_flush.h>
/*
/*	int	mail_flush_deferred()
/*
/*	int	mail_flush_site(site)
/*	const char *site;
/*
/*	int	mail_flush_append(site, queue_id)
/*	const char *site;
/*	const char *queue_id;
/* DESCRIPTION
/*	This module deals with delivery of delayed mail.
/*
/*	mail_flush_deferred() triggers delivery of all deferred
/*	or incoming mail.
/*
/*	The following services are available only for sites have a
/*	"fast flush" logfile. These files list all mail that is queued
/*	for a given site, and are created on demand when, for example,
/*	an eligible SMTP client issues the ETRN command.
/*
/*	mail_flush_site() uses the "fast flush" service to trigger
/*	delivery of messages queued for the specified site.
/*
/*	mail_flush_append() appends a record to the "fast flush"
/*	logfile of the specified site, with the queue ID of mail
/*	that should still be delivered.
/* DIAGNOSTICS
/*	The result codes and their meaning are (see mail_flush(5h)):
/* .IP MAIL_FLUSH_OK
/*	The request completed normally.
/* .IP MAIL_FLUSH_FAIL
/*	The request failed.
/* .IP MAIL_FLUSH_UNKNOWN
/*	The specified site has no "fast flush" logfile.
/* .IP MAIL_FLUSH_BAD
/*	The "fast flush" server rejected the request (invalid request
/*	parameter).
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

#include "sys_defs.h"
#include <unistd.h>
#include <stdarg.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>

/* Global library. */

#include <mail_proto.h>
#include <mail_flush.h>
#include <mail_params.h>

/* mail_flush_deferred - flush deferred queue */

int     mail_flush_deferred(void)
{
    static char qmgr_trigger[] = {
	QMGR_REQ_FLUSH_DEAD,		/* all hosts, all transports */
	QMGR_REQ_SCAN_ALL,		/* all time stamps */
	QMGR_REQ_SCAN_DEFERRED,		/* scan deferred queue */
	QMGR_REQ_SCAN_INCOMING,		/* scan incoming queue */
    };

    /*
     * Trigger the flush queue service.
     */
    return (mail_trigger(MAIL_CLASS_PUBLIC, MAIL_SERVICE_QUEUE,
			 qmgr_trigger, sizeof(qmgr_trigger)));
}

/* mail_flush_clnt - generic fast flush service client */

static int mail_flush_clnt(const char *format,...)
{
    VSTREAM *flush;
    int     status;
    va_list ap;

    /*
     * Connect to the fast flush service over local IPC.
     */
    if ((flush = mail_connect(MAIL_CLASS_PUBLIC, MAIL_SERVICE_FLUSH,
			      BLOCKING)) == 0)
	return (FLUSH_STAT_FAIL);

    /*
     * Do not get stuck forever.
     */
    vstream_control(flush,
		    VSTREAM_CTL_TIMEOUT, var_ipc_timeout,
		    VSTREAM_CTL_END);

    /*
     * Send a request with the site name, and receive the request completion
     * status.
     */
    va_start(ap, format);
    mail_vprint(flush, format, ap);
    va_end(ap);
    if (mail_scan(flush, "%d", &status) != 1)
	status = FLUSH_STAT_FAIL;

    /*
     * Clean up.
     */
    vstream_fclose(flush);

    return (status);
}

/* mail_flush_site - flush deferred mail for site */

int     mail_flush_site(const char *site)
{
    char   *myname = "mail_flush_site";
    int     status;

    if (msg_verbose)
	msg_info("%s: site %s", myname, site);
    status = mail_flush_clnt("%s %s", FLUSH_REQ_SEND, site);
    if (msg_verbose)
	msg_info("%s: site %s status %d", myname, site, status);

    return (status);
}

/* mail_flush_append - append record to fast flush log */

int     mail_flush_append(const char *site, const char *queue_id)
{
    char   *myname = "mail_flush_append";
    int     status;

    if (msg_verbose)
	msg_info("%s: site %s id %s", myname, site, queue_id);
    status = mail_flush_clnt("%s %s %s", FLUSH_REQ_ADD, site, queue_id);
    if (msg_verbose)
	msg_info("%s: site %s id %s status %d", myname, site, queue_id, status);

    return (status);
}
