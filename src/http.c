/* Avalanche
 * (c) 2023-5 Chris Young
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <proto/dos.h>

#include <proto/exec.h>
#include <proto/utility.h>

#ifdef __amigaos4__
#include <proto/bsdsocket.h>
#endif

#include <proto/socket.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef __amigaos4__
#define AMISSL_INLINE_H
#define _OPENSSL_init_ssl(opts, settings) OPENSSL_init_ssl(opts, settings)
#endif

#include <proto/amisslmaster.h>
#include <proto/amissl.h>
#include <libraries/amisslmaster.h>
#include <libraries/amissl.h>
#include <amissl/amissl.h>
#include <dos/dostags.h>

struct Library *SocketBase;
struct Library *AmiSSLMasterBase;

#ifdef __amigaos4__
struct SocketIFace *ISocket;
struct AmiSSLMasterIFace *IAmiSSLMaster;
struct AmiSSLIFace *IAmiSSL;
#else
struct Library *AmiSSLBase = NULL;
struct Library *AmiSSLExtBase = NULL;
#endif

#include "avalanche.h"
#include "arexx.h"
#include "config.h"
#include "http.h"
#include "locale.h"
#include "misc.h"
#include "req.h"
#include "update.h"
#include "win.h"

#include "deark.h"
#include "module.h"
#include "xad.h"
#include "xfd.h"
#include "xvs.h"

#include "Avalanche_rev.h"

enum {
	AHTTP_ERR_NONE = 0,
	AHTTP_ERR_NOTCPIP,
	AHTTP_ERR_AMISSL,
	AHTTP_ERR_UNKNOWN
};

static char *err_txt = NULL;
static struct Task* avalanche_process = NULL;
static struct Process *check_ver_process = NULL;

void http_cleanup(void)
{
	if(SocketBase) CloseLibrary(SocketBase);
#ifdef __amigaos4__
	if (ISocket) DropInterface((struct Interface *)ISocket);
#endif
}

void http_closesocketlibs()
{
	if(AmiSSLMasterBase)
	{
		CloseAmiSSL();
#ifdef __amigaos4__
		if(IAmiSSLMaster) DropInterface((struct Interface *)IAmiSSLMaster);
		IAmiSSLMaster = NULL;
#endif
		CloseLibrary((struct Library *)AmiSSLMasterBase);
		AmiSSLMasterBase = NULL;
	}
	
#ifdef __amigaos4__
		DropInterface((struct Interface *)ISocket);
#endif
		CloseLibrary(SocketBase);
		SocketBase=NULL;
}

/* Required callback to enable HTTPS connection, when necessary
 */
SAVEDS STDARGS BIO *HTTP_TLS_cb(BIO *bio, void *arg, int connect, int detail)
{
	if (connect && detail)
	{
	 	/* Connect with TLS */
		BIO *sbio = BIO_new_ssl((SSL_CTX *)arg, 1);
		bio = (sbio != NULL) ? BIO_push(sbio, bio) : NULL;
	}

	return bio;
}

/* Stub function used when freeing our stack of headers
 */
SAVEDS STDARGS void stub_X509V3_conf_free(CONF_VALUE *val)
{
	X509V3_conf_free(val);
}

static int err_cb(const char *str, size_t len, void *u)
{
#ifdef __amigaos4__
	DebugPrintF("[Avalanche] %s\n", str);
#endif

	err_txt = strdup_vec(str);

	return 1;
}

BOOL http_get_url(char *url, void *ssl_ctx, char *buffer, ULONG bufsize, BPTR fh)
{
	STACK_OF(CONF_VALUE) *headers = NULL;
	BIO *bio, *bio_err;
	ULONG length;
	char app[50];
	SSL_CTX *sslctx = (SSL_CTX *)ssl_ctx;

	/* Allow unsafe legacy renegotiation */
	//SSL_CTX_set_options(sslctx, SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION);

	/* Add our own HTTP headers */
	snprintf(app, 49, "Avalanche/%d.%d", VERSION, REVISION);
	X509V3_add_value("User-Agent", app, &headers);

	/* Initiate the request (supports redirection) */
	if ((bio = OSSL_HTTP_get(url, NULL /* proxy */, NULL /* no_proxy */,
				 NULL /* bio */, NULL /* rbio */,
				 (BIO *(*)(BIO *, void *, int, int))HTTP_TLS_cb, sslctx,
				 0 /* buf_size */, headers,
				 NULL /* expected_content_type */, 0 /* expect_asn1 */,
				 0 /* max_resp_len */, 60 /* timeout */)) != NULL)
	{
		/* HTTP request succeeded */
		while(1) {
			while((length = BIO_read(bio, buffer, bufsize)) > 0) {
				/* Only checking version so not interested beyond the first 1K */
				if(fh == 0) break;

				/* Write data to file */
				if(Write(fh, buffer, length) != length) {
					break;
				}
			}
			if(BIO_should_read(bio) == 0)
				break;
		}

		BIO_free(bio);
	}
	else if((bio_err = BIO_new(BIO_s_file())) != NULL)
	{
		/* HTTP request failed */
/*
		BIO_set_fp_amiga(bio_err, Output(), BIO_NOCLOSE | BIO_FP_TEXT);
		ERR_print_errors(bio_err);
*/

		ERR_print_errors_cb(err_cb, 0);
		BIO_free(bio_err);
	}

	/* Free our custom headers */
	sk_CONF_VALUE_pop_free(headers,(void (*)(CONF_VALUE *))stub_X509V3_conf_free);

	return (bio != NULL);
}


void *http_open_socket_libs(void)
{
	SSL_CTX *SSL_ctx = NULL;
	BOOL result = FALSE;

	SocketBase = OpenLibrary("bsdsocket.library", 4);
	if (!SocketBase)
	{
		return NULL;
	}
#ifdef __amigaos4__
	else
	{
		ISocket = (struct SocketIFace *)GetInterface(SocketBase,"main",1,NULL);
	}
#endif

	if((AmiSSLMasterBase = OpenLibrary("amisslmaster.library", AMISSLMASTER_MIN_VERSION))!=NULL) {
#ifdef __amigaos4__
		if((IAmiSSLMaster = (struct AmiSSLMasterIFace *)GetInterface((struct Library *)AmiSSLMasterBase, "main", 1, NULL))!=0) {
			if(OpenAmiSSLTags(AMISSL_CURRENT_VERSION,
	                  AmiSSL_UsesOpenSSLStructs, TRUE,
	                  AmiSSL_GetIAmiSSL, &IAmiSSL,
	                  AmiSSL_ISocket, ISocket,
//	                  AmiSSL_ErrNoPtr, &errno,
	                  TAG_DONE) == 0) {
#else
			if(OpenAmiSSLTags(AMISSL_CURRENT_VERSION,
                  AmiSSL_UsesOpenSSLStructs, TRUE,
                  AmiSSL_GetAmiSSLBase, &AmiSSLBase,
                  AmiSSL_GetAmiSSLExtBase, &AmiSSLExtBase,
                  AmiSSL_SocketBase, SocketBase,
                  AmiSSL_ErrNoPtr, &errno,
                  TAG_DONE) == 0) {
#endif
				if((SSL_ctx = SSL_CTX_new(TLS_client_method()))!=NULL) {
					SSL_CTX_set_mode(SSL_ctx, SSL_MODE_AUTO_RETRY);
					result = TRUE;
				}
			}
#ifdef __amigaos4__
		}
#endif
	}

	if(result == FALSE) {
		return NULL;
	}

	return (void *)SSL_ctx;

}

void http_ssl_free_ctx(void *SSL_ctx)
{
	SSL_CTX_free((SSL_CTX *)SSL_ctx);
	http_closesocketlibs();
}

static BOOL http_get_version(char *buffer, ULONG bufsize, SSL_CTX *SSL_ctx, const char *url, ULONG ver, ULONG rev, ULONG *upd_ver, ULONG *upd_rev)
{
	int res = 0;
	long val = 0;
	long urev = 0;
	BOOL update_available = FALSE;

	res = http_get_url(url, SSL_ctx, buffer, bufsize, 0);

	if(res) {
		char *dot = NULL;
		char *p = buffer;
		char *lf = NULL;

		while(lf = strchr(p, '\n')) {
			if((strncmp(p, "Version: ", 9) == 0) || (strncmp(p, "version: ", 9) == 0)) {
				p += 9;
				break;
			}
			p = lf + 1;
		}

		val = strtol(p, &dot, 10);

		if(val > ver) {
			update_available = TRUE;
		}
		
		urev = strtol(dot + 1, NULL, 10);

		if(val == ver) {
			if(urev > rev) {
				update_available = TRUE;
			}
		}
	} else {
		if(err_txt) {
			open_error_req(err_txt, locale_get_string(MSG_OK), NULL);
			free(err_txt);
		} else {
			open_error_req(locale_get_string(MSG_ERR_UNKNOWN), locale_get_string(MSG_OK), NULL);
		}
	}
	
	*upd_ver = val;
	*upd_rev = urev;
	
	return update_available;
}

static BOOL http_check_version_internal(void *awin, struct MsgPort *winport, struct MsgPort *appport, struct MsgPort *appwin_mp)
{
	ULONG bufsize = 1024;
	char *buffer = AllocVec(bufsize + 1, MEMF_CLEAR);
	BOOL update_available = FALSE;
	ULONG ver, rev;
	ULONG upd_ver, upd_rev;
	static struct avalanche_version_numbers avn[] = {
		{	.name = "Avalanche\0",
			.check_url = "https://aminet.net/util/arc/avalanche.readme\0",
			.download_url = "https://aminet.net/util/arc/avalanche.lha\0"
		},
		{	.name = "xadmaster.library\0",
#ifndef __amigaos4__ /* Included with OS4, so not upgradeable here */
			.check_url = "https://aminet.net/util/arc/xadmaster000.readme\0",
			.download_url = "https://aminet.net/util/arc/xadmaster000.lha\0",
#else
			.check_url = NULL,
			.download_url = NULL,
#endif
		},
		{	.name = "xfdmaster.library\0",
#ifndef __amigaos4__
			.check_url = "https://aminet.net/util/pack/xfdmaster.readme\0", /* Version number is in the wrong format on this */
			.download_url = "https://aminet.net/util/pack/xfdmaster.lha\0",
#else
			.check_url = "https://aminet.net/util/libs/xfdmaster_os4.readme\0",
			.download_url = "https://aminet.net/util/libs/xfdmaster_os4.lha\0",
#endif
		},
		{	.name = "xvs.library\0",
			.check_url = "https://aminet.net/util/virus/xvslibrary.readme\0",
			.download_url = "https://aminet.net/util/virus/xvslibrary.lha\0",
		},
		{	.name = "Deark\0",
#ifdef __amigaos4__
			.check_url = "https://os4depot.net/share/utility/archive/deark_lha.readme\0",
			.download_url = "https://os4depot.net/share/utility/archive/deark.lha\0",
#else
			.check_url = "https://aminet.net/util/arc/deark.readme\0",
			.download_url = "https://aminet.net/util/arc/deark.lha\0",
#endif
		},
		{	.name = "LhA\0",
#ifdef __amigaos4__
			.check_url = "https://aminet.net/util/arc/lha_os4.readme\0",
			.download_url = "https://aminet.net/util/arc/lha_os4.lha\0",
#else
			.check_url = "https://aminet.net/util/arc/lha_68k.readme\0",
			.download_url = "https://aminet.net/util/arc/lha_68k.lha\0",
#endif
		},
#ifdef __amigaos4__
		{	.name = "zip.library\0",
			.check_url = "https://os4depot.net/share/library/misc/zip_lib_lha.readme\0",
			.download_url = "https://os4depot.net/share/library/misc/zip_lib.lha\0",
		}
#endif
	};
	
	if(buffer) {
		SSL_CTX *SSL_ctx = http_open_socket_libs();

		if(SSL_ctx == NULL) {
			FreeVec(buffer);
			return FALSE;
		}

		for(int i = 0; i < ACHECKVER_MAX; i++) {
			switch(i) {
				case ACHECKVER_XAD:
					xad_get_ver(&avn[ACHECKVER_XAD].current_version, &avn[ACHECKVER_XAD].current_revision);
				break;
				
				case ACHECKVER_XFD:
					avn[ACHECKVER_XFD].current_version = 0;
					avn[ACHECKVER_XFD].current_revision = 0;
					xfd_get_ver(&avn[ACHECKVER_XFD].current_version, &avn[ACHECKVER_XFD].current_revision);
				break;
				
				case ACHECKVER_XVS:
					avn[ACHECKVER_XVS].current_version = 0;
					avn[ACHECKVER_XVS].current_revision = 0;
					xvs_get_ver(&avn[ACHECKVER_XVS].current_version, &avn[ACHECKVER_XVS].current_revision);
				break;
					
				case ACHECKVER_AVALANCHE:
					avn[ACHECKVER_AVALANCHE].current_version = VERSION;
					avn[ACHECKVER_AVALANCHE].current_revision = REVISION;
				break;
				
				case ACHECKVER_DEARK:
					/* TODO: Fix version having x.y.z format */
					avn[ACHECKVER_DEARK].current_version = 0;
					avn[ACHECKVER_DEARK].current_revision = 0;
					deark_get_ver(&avn[ACHECKVER_DEARK].current_version, &avn[ACHECKVER_DEARK].current_revision);
				break;
				
				case ACHECKVER_LHA:
					avn[ACHECKVER_LHA].current_version = 0;
					avn[ACHECKVER_LHA].current_revision = 0;
					mod_lha_get_ver(&avn[ACHECKVER_LHA].current_version, &avn[ACHECKVER_LHA].current_revision);
				break;
				
#ifdef __amigaos4__
				case ACHECKVER_ZIP:
					avn[ACHECKVER_ZIP].current_version = 0;
					avn[ACHECKVER_ZIP].current_revision = 0;
					mod_zip_get_ver(&avn[ACHECKVER_ZIP].current_version, &avn[ACHECKVER_ZIP].current_revision);
				break;
#endif
			}

			if(avn[i].check_url) {
				avn[i].update_available = http_get_version(buffer, bufsize, SSL_ctx, avn[i].check_url, avn[i].current_version, avn[i].current_revision, &avn[i].latest_version, &avn[i].latest_revision);
			} else {
				avn[i].latest_version = 0;
				avn[i].latest_revision = 0;
			}
		}

		FreeVec(buffer);
		http_ssl_free_ctx(SSL_ctx);

		update_gui(avn);


		return TRUE;
	}

	return FALSE;
}

#ifdef __amigaos4__
static void http_check_version_p(void)
#else
static void __saveds http_check_version_p(void)
#endif
{
	http_check_version_internal(NULL, NULL, NULL, NULL);

	check_ver_process = NULL;

	/* Tell the main process we are exiting */
	Signal(avalanche_process, SIGBREAKF_CTRL_E);
}

BOOL http_check_version(void *awin, struct MsgPort *winport, struct MsgPort *appport, struct MsgPort *appwin_mp, BOOL np)
{
	if(np) {
		if(check_ver_process == NULL) {
			avalanche_process = FindTask(NULL);
			check_ver_process = CreateNewProcTags(NP_Entry, http_check_version_p, NP_Name, "Avalanche version check process", TAG_DONE);
		} else {
			/* Already running - signal running process with Ctrl-F (bring to front) */
			Signal(check_ver_process, SIGBREAKF_CTRL_F);
		}
	} else {
		return http_check_version_internal(awin, winport, appport, appwin_mp);
	}
}

struct Process *http_get_process_check_version(void)
{
	return check_ver_process;
}
