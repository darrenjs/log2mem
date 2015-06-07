#ifndef  __MEMLOG_ATOMIC_GNU_H__
#define __MEMLOG_ATOMIC_GNU_H__

#define MEMLOG_HAVE_ATOMICS 1

/*

Provide implementations of required atomic & synchronization operations uing
GNU builtins.

*/

// TODO: do I really need to have separate int/uint functions here?

/* Increment by value and return the new value of v */
static
inline int log2mem_atomic_incr_fetch_int(int* v, int value)
{
  return __sync_add_and_fetch(v, value);
}

static
inline int log2mem_atomic_incr_fetch_uint(unsigned int* v, int value)
{
  return __sync_add_and_fetch(v, value);
}


// TODO: can this be simplified?  I.e., just base on atomic integers? I.e.,
// implement a basic CAS loop?


static
inline void log2mem_atomic_spinlock(volatile int * v)
{
  /* According to GCC documentation, will perform an acquire barrier (rather
     than a full barrier).

     This means that references after the builtin cannot move to (or be
     speculated to) before the builtin, but previous memory stores may not be
     globally visible yet, and previous memory loads may not yet be satisfied.
  */
  while( __sync_lock_test_and_set(v,1) ) {}
}

static
inline void log2mem_atomic_spinunlock(volatile int * v)
{
  __sync_lock_release(v);
}



#endif
