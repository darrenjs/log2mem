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

#include <stdint.h>

#ifndef _MEMLOG_H_
#define _MEMLOG_H_

#define MEMLOGFL __FILE__,__LINE__
#define MEMLOGWRITE( X ) log2mem_write( X, __FILE__, __LINE__)

#ifdef __cplusplus
extern "C" {
#endif


#define MEMLOG_CLOCKSOURCE_MASK 0x0C /* bits 2 and 3 */

enum log2mem_options
{
  MEMLOG_FILELINE     = 1,
  MEMLOG_THREADID     = (1 << 1),

  /* CLOCK SOURCE */
  MEMLOG_GETTIMEOFDAY = (1 << 2),  /* microseconds */
  MEMLOG_CLOCKGETTIME = (1 << 3),  /* nanoseconds */
  MEMLOG_RDTSC        = ((1<<2) + (1<<3))  /* raw cpu */

};

struct log2mem_config
{
    int options;
    unsigned int num_counters;
    unsigned int num_rows;
    unsigned int row_len;
    unsigned int num_vars;
};

/* timestamp format used by log2mem */
typedef  uint64_t log2mem_ts;


/* Initialisation
 */
/*

- Ideally, this should be called before the first log2mem statement is invoked,
  since the first call can take additional time to complete.

 */
void log2mem_init(const char* filename,
                 const struct log2mem_config * config,
                 int config_struct_len);

/* If log2mem has failed, get the last error */
const char* log2mem_strerr();

/* Check if log2mem is valid */
int log2mem_valid();

/* Return the filename of the memmap file, or null if log2mem is not ready */
const char* log2mem_filename();

/* Logging
 *
 */

/* Log a string */
void* log2mem_write(const char* msg, const char* filename, int lineno);

/* Obtain a logging row, to later append values */
void* log2mem_get_row(const char* filename, int lineno);

/* Append data to previously obtained logging row */
void log2mem_append_str2(void* rowptr,  const char* str);
void log2mem_append_str3(void* rowptr,  const char* str, int len);
void log2mem_append_int(void* rowptr,   int);
void log2mem_append_uint(void* rowptr,  unsigned int);
void log2mem_append_long(void* rowptr,  long);
void log2mem_append_ulong(void* rowptr, unsigned long);
void log2mem_append_float(void* rowptr, float);
void log2mem_append_double(void* rowptr,double);
void log2mem_append_char(void* rowptr,  char);

/* Utility functions for putting timing records into a row */
void* log2mem_time_event4(int flowid, int id, const char* msg, int msglen);
void* log2mem_time_event2(int flowid, int id);
void* log2mem_time_event3(int flowid, int id, const char* msg);

log2mem_ts log2mem_time(); // TODO: can we make this inline?

void*   log2mem_set_ts(void* rowptr, log2mem_ts ts); // TOO clumsy ... what this as a first class function

/* Counters */
void log2mem_counter_incr(int counterid);
void log2mem_counter_add(int counterid, int value);
void log2mem_counter_label(int counterid, const char* label);

/* Variables */

void        log2mem_var_double_init(unsigned int id, double, const char*);
int         log2mem_var_double_used(unsigned int id);
void        log2mem_var_double_set(unsigned int id, double);
double      log2mem_var_double_get(unsigned int id);

void        log2mem_var_int_init(unsigned int id, int, const char*);
int         log2mem_var_int_used(unsigned int id);
void        log2mem_var_int_set(unsigned int id, int);
int         log2mem_var_int_get(unsigned int id);

#ifdef __cplusplus
}
#endif

#endif
