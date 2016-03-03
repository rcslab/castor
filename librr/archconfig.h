
#ifndef __CASTOR_ARCHCONFIG_H__
#define __CASTOR_ARCHCONFIG_H__

#ifdef USE_L3CACHELINE
#define CACHELINE	128
#else
#define CACHELINE	64
#endif

#define PAGESIZE	4096

#endif /* __CASTOR_ARCHCONFIG_H__ */

