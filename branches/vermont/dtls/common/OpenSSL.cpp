/* vim: set sts=4 sw=4 cindent nowrap: This modeline was added by Daniel Mentz */
#ifdef SUPPORT_DTLS

#include "OpenSSL.h"
#include "Mutex.h"
#include "msg.h"
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>

namespace { /* unnamed namespace */
    Mutex m;
    bool initialized = false; /* Determines wether OpenSSL is initialized already */
    int check_x509_cert(X509 *peer, int (*cb)(void *context, const char *dnsname), void *context);
}

void ensure_openssl_init(void) {
    m.lock();
    if ( ! initialized) { /* skip this if we already initialized OpenSSL */
	initialized = true;
	SSL_load_error_strings(); /* readable error messages */
	SSL_library_init(); /* initialize library */
#if 0
	if (SSL_COMP_add_compression_method(0, COMP_zlib())) {
	    msg(MSG_ERROR, "OpenSSL: SSL_COMP_add_compression_method() failed.");
	    msg_openssl_errors();
	};
#endif
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


/* returns 1 on success, 0 on error */
int verify_ssl_peer(SSL *ssl, int (*cb)(void *context, const char *dnsname), void *context) {
    long verify_result;

    verify_result = SSL_get_verify_result(ssl);
    DPRINTF("SSL_get_verify_result() returned: %s",X509_verify_cert_error_string(verify_result));
    if(verify_result!=X509_V_OK) {
	msg(MSG_ERROR,"Certificate doesn't verify: %s", X509_verify_cert_error_string(verify_result));
	return 0;
    }

    X509 *peer = SSL_get_peer_certificate(ssl);
    if (! peer) {
	msg(MSG_ERROR,"No peer certificate");
	return 0;
    }
    int ret = check_x509_cert(peer, cb, context);
    X509_free(peer);
    return ret;
}

namespace {

/* returns 1 on success, 0 on error */
int check_x509_cert(X509 *peer, int (*cb)(void *context, const char *dnsname), void *context) {
    char buf[512];
#if DEBUG
    X509_NAME_oneline(X509_get_subject_name(peer),buf,sizeof buf);
    DPRINTF("peer certificate subject: %s",buf);
#endif
    const STACK_OF(GENERAL_NAME) * gens;
    const GENERAL_NAME *gn;
    int num;
    size_t len;
    const char *dnsname;

    gens = (STACK_OF(GENERAL_NAME) *) X509_get_ext_d2i(peer, NID_subject_alt_name, 0, 0);
    num = sk_GENERAL_NAME_num(gens);
    num = sk_num(((_STACK*) (1 ? (gens) : (struct stack_st_GENERAL_NAME*)0)));

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

	while(len>0 && dnsname[len-1] == 0) --len;

	if (len != strlen(dnsname)) {
	    msg(MSG_ERROR, "malformed X509 cert");
	    return 0;
	}
	DPRINTF("Subject Alternative Name: DNS:%s",dnsname);
	if ( (*cb)(context, dnsname) ) {
	    DPRINTF("Subject Alternative Name matched one of the "
		    "permitted FQDNs");
	    return 1;
	}

    }
    if (X509_NAME_get_text_by_NID
	    (X509_get_subject_name(peer),
	     NID_commonName, buf, sizeof buf) <=0 ) {
	DPRINTF("CN not part of certificate");
    } else {
	DPRINTF("most specific (1st) Common Name: %s",buf);
	if ( (*cb)(context, dnsname) ) {
	    DPRINTF("Common Name (CN) matched one of the "
		    "permitted FQDNs");
	    return 1;
	}
    }
    msg(MSG_ERROR,"Neither any of the Subject Alternative Names nor the Common Name "
	    "matched one of the permitted FQDNs");
    return 0;
}

} /* unnamed namespace */

#endif /* SUPPORT_DTLS */

