#ifndef CRYPT_H
#define CRYPT_H

#if defined(__cplusplus)
extern "C" {
#endif

char *crypt(const char *buf,const char *salt);

#if defined(__cplusplus)
}
#endif

#endif
