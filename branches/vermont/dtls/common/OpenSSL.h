/* vim: set sts=4 sw=4 cindent nowrap: This modeline was added by Daniel Mentz */

#ifndef OPENSSLINIT_H
#define OPENSSLINIT_H

#ifdef SUPPORT_OPENSSL

#ifdef __cplusplus
extern "C" {
#endif

#include <openssl/ssl.h>

void ensure_openssl_init(void);
void msg_openssl_errors(void);
int verify_ssl_peer(SSL *ssl, int (*cb)(void *context, const char *dnsname), void *context);

#ifdef __cplusplus
}
#endif

#endif /* SUPPORT_OPENSSL */

#endif