#ifndef  __MEMLOG_ATOMIC_H__
#define  __MEMLOG_ATOMIC_H__

#include "config.h"

#undef MEMLOG_HAVE_ATOMICS

#ifdef HAVE___SYNC_ADD_AND_FETCH
#include "log2mem_atomic_gcc.h"
#elif defined (__x86_64__) || defined (__amd64__)
//#include "log2mem_atomic_x86_64.h"
#error "atomic functions for x86_64 not yet implemented"
#elif defined (__i386) || defined (__i386__)
//#include "log2mem_atomic_i386.h"
#error "atomic functions for i386 not yet implemented"
#endif


#ifndef MEMLOG_HAVE_ATOMICS
#error "atomic functions not available for your platform"
#endif



#endif
