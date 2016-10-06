
#ifndef __CASTOR_ARCHCONFIG_H__
#define __CASTOR_ARCHCONFIG_H__

#if defined(__amd64__)
#define PAGESIZE	4096
#ifdef USE_L3CACHELINE
#define CACHELINE	128
#else
#define CACHELINE	64
#endif
#else
#error "Unsupported Architecture"
#endif

#endif /* __CASTOR_ARCHCONFIG_H__ */

