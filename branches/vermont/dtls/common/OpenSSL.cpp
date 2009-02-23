/* vim: set sts=4 sw=4 cindent nowrap: This modeline was added by Daniel Mentz */
#ifdef SUPPORT_OPENSSL

#include "OpenSSL.h"
#include "Mutex.h"
#include "msg.h"
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>

namespace { /* unnamed namespace */
    Mutex m;
    bool initialized = false; /* Determines wether OpenSSL is initialized already */

}

void ensure_openssl_init(void) {
    m.lock();
    if ( ! initialized) { /* skip this if we already initialized OpenSSL */
	initialized = true;
	SSL_load_error_strings(); /* readable error messages */
	SSL_library_init(); /* initialize library */
	DPRINTF("Initialized OpenSSL");
    }
    m.unlock();
}

/* Get errors from OpenSSL error queue and output them using msg() */
void msg_openssl_errors(void) {
    char errbuf[512];
    char buf[4096];
    unsigned long e;
    const char *file, *data;
    int line, flags;

    while ((e = ERR_get_error_line_data(&file,&line,&data,&flags))) {
	ERR_error_string_n(e,errbuf,sizeof errbuf);
	snprintf(buf, sizeof buf, "%s:%s:%d:%s", errbuf,
                        file, line, (flags & ERR_TXT_STRING) ? data : "");
	msg(MSG_ERROR, "OpenSSL: %s",buf);
    }
}


#if 0
int check_x509_cert(SSL *ssl,const char *host) {
    long verify_result;
    X509 *peer;
    char peer_CN[512];
    char buf[512];

    verify_result = SSL_get_verify_result(conn.ssl);
    DPRINTF("SSL_get_verify_result() returned: %s",X509_verify_cert_error_string(verify_result));
    if(SSL_get_verify_result(ssl)!=X509_V_OK) {
	msg(MSG_ERROR,"Certificate doesn't verify: %s", X509_verify_cert_error_string(verify_result));
	return 0;
    }

    peer = SSL_get_peer_certificate(conn.ssl);
    if (! peer) {
	msg(MSG_ERROR,"No peer certificate");
	return 0;
    }
#if DEBUG
    X509_NAME_oneline(X509_get_subject_name(peer),buf,sizeof buf);
    DPRINTF("peer certificate subject=%s",buf);
#endif
    STACK_OF(GENERAL_NAME) * gens;
    const GENERAL_NAME *gn;
    int num;
    size_t len;
    char *dnsname;

    gens = (STACK_OF(GENERAL_NAME) *) X509_get_ext_d2i(peer, NID_subject_alt_name, 0, 0);
    num = sk_GENERAL_NAME_num(gens);

    for (int i = 0; i < num; ++i) {
	gn = sk_GENERAL_NAME_value(gens, i);
	if (gn->type != GEN_DNS)
	    continue;
	if (ASN1_STRING_type(gn->d.ia5) != V_ASN1_IA5STRING) {
	    msg(MSG_ERROR, "malformed X509 cert: Type of ASN.1 string not IA5");
	    return 0;
	}

	dnsname = (char *) ASN1_STRING_data(gn->d.ia5);
	len = ASN1_STRING_length(gn->d.ia5);

#define TRIM0(s, l) do { while ((l) > 0 && (s)[(l)-1] == 0) --(l); } while (0)
	TRIM0(dnsname, len);

	if (len != strlen(dnsname))
	    DPRINTF("/* malformed cert */");
	DPRINTF("dnsname: %s",dnsname);
    }
    char peer_CN[512];
    if (X509_NAME_get_text_by_NID
	    (X509_get_subject_name(peercert),/* NID_localityName */
	     NID_commonName, peer_CN, sizeof peer_CN) <=0 ) {
	DPRINTF("CN not part of certificate");
    } else {
	DPRINTF("CN: %s",peer_CN);
    }
    // if(strcasecmp(peer_CN,host))
    X509_free(peercert);
    /*Check the cert chain. The chain length
      is automatically checked by OpenSSL when
      we set the verify depth in the ctx */

    /*Check the common name*/
    peer=SSL_get_peer_certificate(ssl);
    X509_NAME_get_text_by_NID
	(X509_get_subject_name(peer),
	 NID_commonName, peer_CN, 256);
    if(strcasecmp(peer_CN,host))
	err_exit
	    ("Common name doesn't match host name");
}
#endif /* 0 */


#endif /* SUPPORT_OPENSSL */

