/*
    Copyright 2013, Darren Smith

    This file is part of log2mem, a library for providing high performance
    logging, timing and debugging to applications.

    log2mem is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    log2mem is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with log2mem.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _MEMLOG_DEV_H_
#define _MEMLOG_DEV_H_

#include "log2mem/log2mem.h"

#include <sys/types.h>


#ifdef __cplusplus
extern "C" {
#endif

#define MEMLOG_VERSION 2

#define MEMLOG_LABEL_LEN 64

#define MEMLOG_DESC_LEN 24
#define MEMLOG_TIMESTR_LEN 32

/* Memmap header size -- always fixed to this */
#define MEMLOG_MM_HEADER_SIZE 256

/* Data types stored with rows are represented via datatype ID  */
#define MEMLOG_DATATYPE_FILENAME 1
#define MEMLOG_DATATYPE_STRING   2
#define MEMLOG_DATATYPE_UINT     3
#define MEMLOG_DATATYPE_INT      4
#define MEMLOG_DATATYPE_ULONG    5
#define MEMLOG_DATATYPE_LONG     6
#define MEMLOG_DATATYPE_FLOAT    7
#define MEMLOG_DATATYPE_DOUBLE   8
#define MEMLOG_DATATYPE_CHAR     9
#define MEMLOG_DATATYPE_LINENO   10 /* lineno is an integer */

/*

for time values, the 64 bits are split into:

  | seconds    | nanosec    |
  |<--- 34 --->|<--- 30 --->|

34 bit mask: 0x3FFFFFFFF
30 bit mask:  0x3FFFFFFF

*/

#define MEMLOG_BITS_SECS 34
#define MEMLOG_MASK_SECS 0x3FFFFFFFF

#define MEMLOG_BITS_NANO 30
#define MEMLOG_MASK_NANO 0x3FFFFFFF

/* Memmap header */
struct log2mem_mm_header
{
    char desc[MEMLOG_DESC_LEN];
    int  flags;
    unsigned int nextrow;
    struct log2mem_config config;
    int  version;
    int  arch;
    char datecreated[MEMLOG_TIMESTR_LEN];
};


/* Data item header */
struct log2mem_data_hdr
{
    char type;
    unsigned int  len; // length of payload + length of this header
};


struct log2mem_row
{
    int            tid;
    log2mem_ts      ts;
    unsigned int   id;
    unsigned int   datalen;
};

/*
Place our atomic int within a simple struct wrapper, to prevent variables of
type atomic_int from being accidentally placed within standard numeric
expressions (which would invalidate atomicity). Eg, atomic_int i; i = i + 1;
causes a compiler error.
*/
struct atomic_int
{
    volatile int v;
};

struct log2mem_counter
{
    struct atomic_int value;
    int used;
};

/* calculate the payload address for a row address */
static inline char*  log2mem_row_payload(struct log2mem_row * r)
{
  char* p = (char*) r;
  return p + sizeof(struct log2mem_row);
}

static inline size_t calc_row_total_size(const struct log2mem_config * cfg)
{
  return sizeof(struct log2mem_row) + cfg->row_len;
}

enum log2mem_bitflags
{
  log2mem_eInitialised = 0x01,
  log2mem_eUsed        = 0x02
};

struct log2mem_label
{
    char text[ MEMLOG_LABEL_LEN ];
};

struct log2mem_var_double
{
    double value;
    int    flags;
    char   text[ MEMLOG_LABEL_LEN ];
    // TODO: need to add some kind of lock, spinlock?
};

struct log2mem_var_int
{
    int    value;
    int    flags;
    char   text[ MEMLOG_LABEL_LEN ];
    // TODO: need to add some kind of lock, spinlock?
};

struct log2mem_handle
{
  void *  mem;

  /* pointers to major memmap sections */
  struct log2mem_mm_header * header;
  struct log2mem_counter *   counters;
  struct log2mem_row *       rows;
  struct log2mem_label *     counter_labels;
  struct log2mem_var_double* double_vars;
  struct log2mem_var_int*    int_vars;

  char* filename;
};

/* Attach to an existing memmap file, and return handle.  Returns NULL upon
 * failure. */
struct log2mem_handle *
log2mem_attach (const char* filename);

/* Memmap layout is described by the offsets to the major sections */
struct log2mem_layout
{
    int size;     /* whole region footprint */

    /* int header_off; -- not needed, because always zero */

    int counters_off;
    int rows_off;
    int counter_labels_off;
    int double_vars_off;
    int int_vars_off;
};

void log2mem_calculate_layout(const struct log2mem_config * cfg,
                             struct log2mem_layout * layout);

/*
  struct timeval {
      time_t      tv_sec;
      suseconds_t tv_usec;
  };

  struct timespec {
      time_t   tv_sec;
      long     tv_nsec;
  };
*/

struct timespec;
struct timeval;

void log2mem_ts_to_timespec(log2mem_ts, struct timespec*);
void log2mem_ts_to_timeval(log2mem_ts,struct timeval*);

#ifdef __cplusplus
}
#endif

#endif
