
#ifndef _LILV_CONFIG_H_WAF
#define _LILV_CONFIG_H_WAF

#define HAVE_LV2 1
#define HAVE_SERD 1
#define HAVE_SORD 1
#define HAVE_SRATOM 1
#define LILV_NEW_LV2 1
#define HAVE_FILENO 1
#ifndef _WIN32
#define HAVE_FLOCK 1
#endif
#define LILV_VERSION "0.14.2"

#ifdef _WIN32
#define LILV_PATH_SEP ";"
#define LILV_DIR_SEP "\\"
#define LILV_DEFAULT_LV2_PATH "%APPDATA%\\LV2;%COMMONPROGRAMFILES%\\LV2"
#else
#define LILV_PATH_SEP ":"
#define LILV_DIR_SEP "/"
#define LILV_DEFAULT_LV2_PATH "~/.lv2:/usr/lib/lv2:/usr/local/lib/lv2"
#endif

#endif /* _LILV_CONFIG_H_WAF */
