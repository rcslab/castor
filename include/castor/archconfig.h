
#ifndef __CASTOR_ARCHCONFIG_H__
#define __CASTOR_ARCHCONFIG_H__

#if defined(__amd64__)
#define PAGESIZE	4096U
#ifdef USE_L3CACHELINE
#define CACHELINE	128U
#else
#define CACHELINE	64U
#endif
#else
#error "Unsupported Architecture"
#endif

#endif /* __CASTOR_ARCHCONFIG_H__ */

