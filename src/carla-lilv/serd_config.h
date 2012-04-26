
#ifndef _SERD_CONFIG_H_WAF
#define _SERD_CONFIG_H_WAF

#define HAVE_FMAX 1
#ifndef _WIN32
#define HAVE_POSIX_MEMALIGN 1
#define HAVE_POSIX_FADVISE 1
#endif
#define HAVE_FILENO 1
#define SERD_VERSION "0.14.0"

#endif /* _SERD_CONFIG_H_WAF */
