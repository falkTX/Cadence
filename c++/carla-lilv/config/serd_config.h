
#ifndef _SERD_CONFIG_H_
#define _SERD_CONFIG_H_

#define SERD_VERSION "0.14.0"

#define HAVE_FMAX 1
#define HAVE_FILENO 1

#ifndef _WIN32
#define HAVE_POSIX_MEMALIGN 1
#define HAVE_POSIX_FADVISE 1
#endif

#endif /* _SERD_CONFIG_H_ */
