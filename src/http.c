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
#include "http.h"
#include "locale.h"
#include "misc.h"
#include "req.h"
#include "win.h"

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

static BOOL http_get_url(char *url, SSL_CTX *sslctx, char *buffer, ULONG bufsize, BPTR fh)
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
			/* Only checking version so not interested beyond to first 1K */
			if(fh == 0) break;

			/* Write data to file */
			if(Write(fh, buffer, length) != length) {
				break;
			}
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


SSL_CTX *http_open_socket_libs(void)
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
				if((SSL_ctx = SSL_CTX_new(TLS_client_method()))!=NULL) result = TRUE;
			}
#ifdef __amigaos4__
		}
#endif
	}

	if(result == FALSE) {
		return NULL;
	}

	return SSL_ctx;

}

void http_ssl_free_ctx(SSL_CTX *SSL_ctx)
{
	SSL_CTX_free(SSL_ctx);
	http_closesocketlibs();
}

long http_get_version(char *buffer, ULONG bufsize, SSL_CTX *SSL_ctx)
{
	int res = 0;

	/* TODO: Also check https://aminet.net/util/virus/xvslibrary.readme against current ver */
	res = http_get_url("https://aminet.net/util/arc/avalanche.readme", SSL_ctx, buffer, bufsize, 0);

	if(res) {
		return AHTTP_ERR_NONE;
	} else {
		return AHTTP_ERR_UNKNOWN;
	}
}

BOOL http_check_version(void *awin, struct MsgPort *winport, struct MsgPort *appport, struct MsgPort *appwin_mp)
{
	ULONG bufsize = 1024;
	char *buffer = AllocVec(bufsize + 1, MEMF_CLEAR);
	BOOL update_available = FALSE;

	if(buffer) {
		SSL_CTX *SSL_ctx = http_open_socket_libs();

		if(SSL_ctx == NULL) {
			FreeVec(buffer);
			return AHTTP_ERR_AMISSL;
		}

		long res = http_get_version(buffer, bufsize, SSL_ctx);

		if(res == AHTTP_ERR_NONE) {
			char *dot = NULL;
			char *p = buffer;
			char *lf = NULL;

			while(lf = strchr(p, '\n')) {
				if(strncmp(p, "Version: ", 9) == 0) {
					p += 9;
					break;
				}
				p = lf + 1;
			}

			long val = strtol(p, &dot, 10);

			if(val > VERSION) {
				update_available = TRUE;
			}
			
			long rev = strtol(dot + 1, NULL, 10);

			if(val == VERSION) {
				if(rev > REVISION) {
					update_available = TRUE;
				}
			}

			char message[101];

			if(update_available) {
				snprintf(message, 100, locale_get_string(MSG_NEWVERSIONDL), val, rev);
				if(ask_yesno_req(awin, message)) {
					// download
					BPTR fh = Open("T:avalanche.lha", MODE_NEWFILE);
					if(fh) {
						res = http_get_url("https://aminet.net/util/arc/avalanche.lha", SSL_ctx, buffer, bufsize, fh);
						Close(fh);

						void *new_awin = window_create(get_config(), "T:avalanche.lha", winport, appport);
						if(new_awin != NULL) {
							window_open(new_awin, appwin_mp);
							window_req_open_archive(new_awin, get_config(), TRUE);
						} else {
							open_error_req(locale_get_string(MSG_ERR_UNKNOWN), locale_get_string(MSG_OK), awin);
						}
					} else {
						open_error_req(locale_get_string(MSG_ERR_UNKNOWN), locale_get_string(MSG_OK), awin);
					}
				}
			} else {
				open_info_req(locale_get_string(MSG_NONEWVERSION), locale_get_string(MSG_OK), awin);
			}

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
		http_ssl_free_ctx(SSL_ctx);
		return TRUE;
	}

	return FALSE;
}
