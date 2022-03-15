/* WARNING! All changes made to this file will be lost! */

#ifndef W_INCLUDE_CSP_CSP_AUTOCONFIG_H_WAF
#define W_INCLUDE_CSP_CSP_AUTOCONFIG_H_WAF

#define GIT_REV "v1.6-0-g8700695"
#define CSP_FREERTOS 1
#undef CSP_POSIX
#undef CSP_WINDOWS
#undef CSP_MACOSX
#define CSP_DEBUG 1
#define CSP_DEBUG_TIMESTAMP 1
#define CSP_USE_RDP 1
#define CSP_USE_RDP_FAST_CLOSE 0
#define CSP_USE_CRC32 1
#define CSP_USE_HMAC 1
#define CSP_USE_XTEA 1
#define CSP_USE_PROMISC 0
#define CSP_USE_QOS 0
#define CSP_USE_DEDUP 0
#define CSP_USE_EXTERNAL_DEBUG 0
#define CSP_LOG_LEVEL_DEBUG 1
#define CSP_LOG_LEVEL_INFO 1
#define CSP_LOG_LEVEL_WARN 1
#define CSP_LOG_LEVEL_ERROR 1
#define CSP_BIG_ENDIAN 1
#define LIBCSP_VERSION "1.6"
#define MY_STRNLEN
#ifdef MY_STRNLEN
   static inline size_t strnlen (const char *string, size_t length)
   {
       size_t i = 0;
       for ( ; i < length && string[i] != '\0'; ++i);
       return i;
   }
#endif
#endif /* W_INCLUDE_CSP_CSP_AUTOCONFIG_H_WAF */
