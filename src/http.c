/* Avalanche
 * (c) 2023 Chris Young
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

#include "http.h"
#include "locale.h"
#include "misc.h"
#include "req.h"

#include "Avalanche_rev.h"

enum {
	AHTTP_ERR_NONE = 0,
	AHTTP_ERR_NOTCPIP,
	AHTTP_ERR_AMISSL,
	AHTTP_ERR_UNKNOWN
};

char *err_txt = NULL;

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

	err_txt = strdup(str);

	return 1;
}

static BOOL http_get_url(char *url, SSL_CTX *sslctx, char *buffer, ULONG bufsize)
{
	STACK_OF(CONF_VALUE) *headers = NULL;
	BIO *bio, *bio_err;
	ULONG length;
	char app[50];

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
		while ((length = BIO_read(bio, buffer, bufsize)) > 0)
		{
			/* do nothing */
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


long http_get_version(char *buffer, ULONG bufsize)
{
	SSL_CTX *SSL_ctx = NULL;
	int res = 0;
	BOOL result=FALSE;

	SocketBase = OpenLibrary("bsdsocket.library",4);
	if (!SocketBase)
	{
		return(AHTTP_ERR_NOTCPIP);
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
				if((SSL_ctx = SSL_CTX_new(TLS_client_method()))!=NULL) result = TRUE;
			}
#ifdef __amigaos4__
		}
#endif
	}

	if(result == FALSE) {
		return AHTTP_ERR_AMISSL;
	}

	res = http_get_url("https://www.unsatisfactorysoftware.co.uk/ver.php?f=avalanche", SSL_ctx, buffer, bufsize);

	SSL_CTX_free(SSL_ctx);
	http_closesocketlibs();

	if(res) {
		return AHTTP_ERR_NONE;
	} else {
		return AHTTP_ERR_UNKNOWN;
	}
}

BOOL http_check_version(void *awin)
{
	ULONG bufsize = 10;
	char *buffer = AllocVec(bufsize + 1, MEMF_CLEAR);
	BOOL update_available = FALSE;

	if(buffer) {
		long res = http_get_version(buffer, bufsize);

		if(res == AHTTP_ERR_NONE) {
			char *dot = NULL;

			long val = strtol(buffer, &dot, 10);

			if(val > VERSION) {
				update_available = TRUE;
			}

			if(val == VERSION) {
				long rev = strtol(dot + 1, NULL, 10);

				if(rev > REVISION) {
					update_available = TRUE;
				}
			}

			char message[101];

			if(update_available) {
				snprintf(message, 100, locale_get_string(MSG_NEWVERSION), buffer);
			} else {
				snprintf(message, 100, locale_get_string(MSG_NONEWVERSION));
			}

			open_info_req(message, locale_get_string(MSG_OK), awin);

		} else if(res == AHTTP_ERR_NOTCPIP) {
			open_error_req(locale_get_string(MSG_ERR_NOTCPIP), locale_get_string(MSG_OK), awin);
		} else if(res == AHTTP_ERR_AMISSL) {
			open_error_req(locale_get_string(MSG_ERR_AMISSL), locale_get_string(MSG_OK), awin);
		} else {
			if(err_txt) {
				open_error_req(err_txt, locale_get_string(MSG_OK), awin);
				free(err_txt);
			} else {
				open_error_req(locale_get_string(MSG_ERR_UNKNOWN), locale_get_string(MSG_OK), awin);
			}
		}

		FreeVec(buffer);

		return TRUE;
	} 

	return FALSE;
}