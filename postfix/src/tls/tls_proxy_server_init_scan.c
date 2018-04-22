/*++
/* NAME
/*	tls_proxy_server_init_scan
/* SUMMARY
/*	read TLS_SERVER_INIT_PROPS from stream
/* SYNOPSIS
/*	#include <tls_proxy.h>
/*
/*	int	tls_proxy_server_init_scan(scan_fn, stream, flags, ptr)
/*	ATTR_SCAN_MASTER_FN scan_fn;
/*	VSTREAM *stream;
/*	int     flags;
/*	void    *ptr;
/*
/*	tls_proxy_server_init_free(props)
/*	TLS_SERVER_INIT_PROPS *props;
/* DESCRIPTION
/*	tls_proxy_server_init_scan() reads a TLS_SERVER_INIT_PROPS
/*	structure from the named stream using the specified attribute
/*	scan routine. tls_proxy_server_init_scan() is meant to be passed
/*	as a call-back function to attr_scan(), as shown below.
/*
/*	tls_proxy_server_init_free() destroys a TLS_SERVER_INIT_PROPS
/*	structure that was created by tls_proxy_server_init_scan().
/*	This must be called even if the tls_proxy_server_init_scan()
/*	call returned an error.
/*
/*	TLS_SERVER_INIT_PROPS *props = 0;
/*	...
/*	... RECV_ATTR_FUNC(tls_proxy_server_init_scan, (void *) &props), ...
/*	...
/*	if (props)
/*	    tls_proxy_client_init_free(props);
/* DIAGNOSTICS
/*	Fatal: out of memory.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#ifdef USE_TLS

/* System library. */

#include <sys_defs.h>

/* Utility library */

#include <attr.h>

/* TLS library. */

#include <tls.h>
#include <tls_proxy.h>

/* tls_proxy_server_init_scan - receive TLS_SERVER_INIT_PROPS from stream */

int     tls_proxy_server_init_scan(ATTR_SCAN_MASTER_FN scan_fn, VSTREAM *fp,
				           int flags, void *ptr)
{
    TLS_SERVER_INIT_PROPS *props
    = (TLS_SERVER_INIT_PROPS *) mymalloc(sizeof(*props));
    int     ret;
    VSTRING *log_param = vstring_alloc(25);
    VSTRING *log_level = vstring_alloc(25);
    VSTRING *cache_type = vstring_alloc(25);
    VSTRING *cert_file = vstring_alloc(25);
    VSTRING *key_file = vstring_alloc(25);
    VSTRING *dcert_file = vstring_alloc(25);
    VSTRING *dkey_file = vstring_alloc(25);
    VSTRING *eccert_file = vstring_alloc(25);
    VSTRING *eckey_file = vstring_alloc(25);
    VSTRING *CAfile = vstring_alloc(25);
    VSTRING *CApath = vstring_alloc(25);
    VSTRING *protocols = vstring_alloc(25);
    VSTRING *eecdh_grade = vstring_alloc(25);
    VSTRING *dh1024_param_file = vstring_alloc(25);
    VSTRING *dh512_param_file = vstring_alloc(25);
    VSTRING *mdalg = vstring_alloc(25);

    /*
     * Note: memset() is not a portable way to initialize non-integer types.
     */
    memset(props, 0, sizeof(*props));
    ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		  RECV_ATTR_STR(TLS_ATTR_LOG_PARAM, log_param),
		  RECV_ATTR_STR(TLS_ATTR_LOG_LEVEL, log_level),
		  RECV_ATTR_INT(TLS_ATTR_VERIFYDEPTH, &props->verifydepth),
		  RECV_ATTR_STR(TLS_ATTR_CACHE_TYPE, cache_type),
		  RECV_ATTR_INT(TLS_ATTR_SET_SESSID, &props->set_sessid),
		  RECV_ATTR_STR(TLS_ATTR_CERT_FILE, cert_file),
		  RECV_ATTR_STR(TLS_ATTR_KEY_FILE, key_file),
		  RECV_ATTR_STR(TLS_ATTR_DCERT_FILE, dcert_file),
		  RECV_ATTR_STR(TLS_ATTR_DKEY_FILE, dkey_file),
		  RECV_ATTR_STR(TLS_ATTR_ECCERT_FILE, eccert_file),
		  RECV_ATTR_STR(TLS_ATTR_ECKEY_FILE, eckey_file),
		  RECV_ATTR_STR(TLS_ATTR_CAFILE, CAfile),
		  RECV_ATTR_STR(TLS_ATTR_CAPATH, CApath),
		  RECV_ATTR_STR(TLS_ATTR_PROTOCOLS, protocols),
		  RECV_ATTR_STR(TLS_ATTR_EECDH_GRADE, eecdh_grade),
		  RECV_ATTR_STR(TLS_ATTR_DH1K_PARAM_FILE, dh1024_param_file),
		  RECV_ATTR_STR(TLS_ATTR_DH512_PARAM_FILE, dh512_param_file),
		  RECV_ATTR_INT(TLS_ATTR_ASK_CCERT, &props->ask_ccert),
		  RECV_ATTR_STR(TLS_ATTR_MDALG, mdalg),
		  ATTR_TYPE_END);
    props->log_param = vstring_export(log_param);
    props->log_level = vstring_export(log_level);
    props->cache_type = vstring_export(cache_type);
    props->cert_file = vstring_export(cert_file);
    props->key_file = vstring_export(key_file);
    props->dcert_file = vstring_export(dcert_file);
    props->dkey_file = vstring_export(dkey_file);
    props->eccert_file = vstring_export(eccert_file);
    props->eckey_file = vstring_export(eckey_file);
    props->CAfile = vstring_export(CAfile);
    props->CApath = vstring_export(CApath);
    props->protocols = vstring_export(protocols);
    props->eecdh_grade = vstring_export(eecdh_grade);
    props->dh1024_param_file = vstring_export(dh1024_param_file);
    props->dh512_param_file = vstring_export(dh512_param_file);
    props->mdalg = vstring_export(mdalg);
    *(TLS_SERVER_INIT_PROPS **) ptr = props;
    return (ret == 19 ? 1 : -1);
}

/* tls_proxy_server_init_free - destroy TLS_SERVER_INIT_PROPS structure */

void    tls_proxy_server_init_free(TLS_SERVER_INIT_PROPS *props)
{
    myfree((void *) props->log_param);
    myfree((void *) props->log_level);
    myfree((void *) props->cache_type);
    myfree((void *) props->cert_file);
    myfree((void *) props->key_file);
    myfree((void *) props->dcert_file);
    myfree((void *) props->dkey_file);
    myfree((void *) props->eccert_file);
    myfree((void *) props->eckey_file);
    myfree((void *) props->CAfile);
    myfree((void *) props->CApath);
    myfree((void *) props->protocols);
    myfree((void *) props->eecdh_grade);
    myfree((void *) props->dh1024_param_file);
    myfree((void *) props->dh512_param_file);
    myfree((void *) props->mdalg);
    myfree((void *) props);
}

#endif
