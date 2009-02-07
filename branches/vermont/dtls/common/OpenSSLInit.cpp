/* vim: set sts=4 sw=4 cindent nowrap: This modeline was added by Daniel Mentz */
#ifdef SUPPORT_OPENSSL

#include "OpenSSLInit.h"
#include "Mutex.h"
#include "msg.h"

namespace { /* unnamed namespace */
    Mutex m;
    bool initialized = false; /* Determines wether OpenSSL is initialized already */

}

void ensure_openssl_init(void) {
    m.lock();
    if (initialized) return; /* skip this if we already initialized OpenSSL */
    initialized = true;
    SSL_load_error_strings(); /* readable error messages */
    SSL_library_init(); /* initialize library */
    DPRINTF("Initialized OpenSSL");
    m.unlock();
}

#endif /* SUPPORT_OPENSSL */

