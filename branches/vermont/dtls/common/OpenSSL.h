/* vim: set sts=4 sw=4 cindent nowrap: This modeline was added by Daniel Mentz */

#ifndef OPENSSLINIT_H
#define OPENSSLINIT_H

#ifdef SUPPORT_OPENSSL

#ifdef __cplusplus
extern "C" {
#endif

extern void ensure_openssl_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SUPPORT_OPENSSL */

#endif
